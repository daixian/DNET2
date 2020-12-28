#include "KCPServer.h"

#include "dlog/dlog.h"

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/MulticastSocket.h"
#include "Poco/Net/ServerSocket.h"

#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/Observer.h"
#include "Poco/Thread.h"
#include "Poco/Runnable.h"
#include "Poco/RunnableAdapter.h"
#include "Poco/UUIDGenerator.h"

#include "./kcp/ikcp.h"
#include <thread>
#include <mutex>
#include "clock.hpp"
#include <regex>
#include "Accept.h"

#include "TCP/TCPServer.h"

#include "KCPUser.h"

namespace dxlib {

//TODO: 可以让上层通过检测 ikcp_waitsnd 函数来判断还有多少包没有发出去，灵活抉择是否向 snd_queue 缓存追加数据包还是其他。
//TODO: 管理大规模连接 https://github.com/skywind3000/kcp/wiki/KCP-Best-Practice
//如果需要同时管理大规模的 KCP连接（比如大于3000个），比如你正在实现一套类 epoll的机制，那么为了避免每秒钟对每个连接调用大量的调用 ikcp_update，我们可以使用 ikcp_check 来大大减少 ikcp_update调用的次数。
//ikcp_check返回值会告诉你需要在什么时间点再次调用 ikcp_update（如果中途没有 ikcp_send, ikcp_input的话，否则中途调用了 ikcp_send, ikcp_input的话，需要在下一次interval时调用 update）
//标准顺序是每次调用了 ikcp_update后，使用 ikcp_check决定下次什么时间点再次调用 ikcp_update，而如果中途发生了 ikcp_send, ikcp_input 的话，在下一轮 interval 立马调用 ikcp_update和 ikcp_check。
//使用该方法，原来在处理2000个 kcp连接且每 个连接每10ms调用一次update，改为 check机制后，cpu从 60% 降低到 15%。

// KCP的下层协议输出函数，KCP需要发送数据时会调用它
// buf/len 表示缓存和长度
// user指针为 kcp对象创建时传入的值，用于区别多个 KCP对象
int kcps_udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    try {
        //kcp里有几个位置调用udp_output的时候没有传user参数进来,所以不能使用这个参数
        KCPUser* u = (KCPUser*)kcp->user;
        //if (u->remote.a nullptr) {
        //    LogE("KCP2.udp_output():%s还没有remote记录,不能发送!", u->name);
        //    return -1;
        //}
        LogD("KCPServer.udp_output():向{%s}发送! len=%d", u->uuid_remote.c_str(), len);
        return u->socket->sendTo(buf, len, u->remote);
    }
    catch (const Poco::Exception& e) {
        LogE("KCPServer.udp_output():异常%s %s", e.what(), e.message().c_str());
        return -1;
    }
    catch (const std::exception& e) {
        LogE("KCPServer.udp_output():异常:%s", e.what());
        return -1;
    }
}

class KCPServer::Impl
{
  public:
    Impl()
    {
        receBuf.resize(8 * 1024);

        //随机生成一个uuid
        Poco::UUIDGenerator uuidGen;
        uuid = uuidGen.createRandom().toString();
    }
    ~Impl()
    {
        Close();
    }

    // 自己的名字
    std::string name;

    // 唯一标识符
    std::string uuid;

    // kcp使用的UDP端口
    int port = 0;

    // 自己的本地socket
    Poco::Net::DatagramSocket* socket = nullptr;

    // TCP的socket用于服务器监听握手信息端口和UDP一致
    TCPServer* tcpServer = nullptr;

    //socket使用的buff ,最好大于1400吧
    std::vector<char> receBuf;

    // 远程对象列表
    std::map<int, ikcpcb*> remotes;

    /**
     * Initializes this object
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @param  name The name.
     * @param  host The host.
     * @param  port The port.
     */
    void Start(const std::string& name, const std::string& host, int port)
    {
        try {
            this->name = name;
            this->port = port;
            Poco::Net::SocketAddress sAddr(Poco::Net::AddressFamily::IPv4, host, port);

            socket = new Poco::Net::DatagramSocket(sAddr); //使用一个端口开始一个UDP接收
            socket->setBlocking(false);                    //它可以设置是否是阻塞

            //启动TCP监听,等待客户端握手连入
            tcpServer = new TCPServer(name, host, port);
            tcpServer->SetUUID(uuid); //设置自己的uuid给它.
            tcpServer->Start();
            tcpServer->WaitStarted();
        }
        catch (const Poco::Exception& e) {
            LogE("KCPServer.Start():异常%s %s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("KCPServer.Start():异常:%s", e.what());
        }
    }

    void Close()
    {
        if (tcpServer != nullptr) {
            try {
                tcpServer->Close();
            }
            catch (const std::exception&) {
            }
            delete tcpServer;
            tcpServer = nullptr;
        }

        if (socket != nullptr) {
            try {
                socket->close();
            }
            catch (const std::exception&) {
            }
            delete socket;
            socket = nullptr;
        }

        for (auto& kvp : remotes) {
            delete kvp.second;
        }
        remotes.clear();
    }

    /**
     * 得到一个当前可用的UPD的conv号
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns The convert.
     */
    int GetConv()
    {
        int conv = 1;
        while (true) {
            if (remotes.find(conv) != remotes.end()) {
                conv++;
            }
            else {
                return conv;
            }
        }
    }

    // TCP的服务器端接收认证的处理
    void TCPServerAcceptReceive()
    {
        if (tcpServer == nullptr) {
            return;
        }
        std::map<int, std::vector<std::string>> msgs;
        tcpServer->Receive(msgs);
        for (auto& kvp : msgs) {
            int tcpID = kvp.first;

            std::string& acceptStr = kvp.second[0]; //只处理第一条

            //int conv = GetConv(); 这里改为使用tcpid算了
            int conv = tcpID;
            KCPUser* kcpUser = new KCPUser(socket, conv);                                                //准备创建新的客户端用户
            std::string replyStr = kcpUser->accept.ReplyAcceptString(acceptStr, uuid, conv, this->port); //使用这个新的客户端用户做accept
            if (replyStr.empty()) {
                // replyStr为空,非法的认证信息
                LogE("KCPServer.TCPServerAcceptReceive():非法的认证信息!");
                delete kcpUser;
            }
            else {
                // replyStr有内容,有效的认证信息,自己是服务器端
                Poco::Net::StreamSocket* tcs = (Poco::Net::StreamSocket*)tcpServer->GetClientSocket(tcpID);
                kcpUser->remote = tcs->peerAddress(); //绑定客户端实际发来的端口
                kcpUser->uuid_remote = kcpUser->accept.nameC();
                tcpServer->Send(tcpID, replyStr.c_str(), replyStr.size());

                ikcpcb* kcp = ikcp_create(conv, kcpUser);
                // 设置回调函数
                kcp->output = kcps_udp_output;

                ikcp_nodelay(kcp, 1, 10, 2, 1);
                //kcp->rx_minrto = 10;
                //kcp->fastresend = 1;
                remotes[conv] = kcp; //添加这个新客户端
                LogI("KCPServer.TCPServerAcceptReceive():添加了一个新客户端%s,Addr=%s:%d,分配conv=%d", kcpUser->uuid_remote.c_str(),
                     kcpUser->remote.host().toString().c_str(),
                     kcpUser->remote.port(), conv);
            }
        }
    }

    /**
     * 接收.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns 接收到的消息的条数.
     *
     */
    int Receive(std::vector<KCPUser*>& vRece)
    {
        if (socket == nullptr) {
            LogE("KCPServer.Receive():%s 还没有初始化,不能接收!", name);
            return -1;
        }

        //执行一下TCP的
        TCPServerAcceptReceive();

        vRece.clear();
        int rece;
        try {
            //socket尝试接收
            Poco::Net::SocketAddress remote(Poco::Net::AddressFamily::IPv4);
            int n = socket->receiveFrom(receBuf.data(), (int)receBuf.size(), remote);
            if (n != -1) {
                LogD("KCPServer.Receive():%s Socket接收到了数据,长度%d", name, n);

                //进行Accept的判断
                for (auto& kvp : remotes) {
                    //尝试给kcp看看是否是它的信道的数据
                    ikcpcb* kcp = kvp.second;
                    rece = ikcp_input(kcp, receBuf.data(), n);
                    if (rece == -1) {
                        //conv不对应
                        continue;
                    }
                    ikcp_flush(kcp); //尝试暴力flush

                    KCPUser* user = (KCPUser*)kcp->user;
                    while (rece >= 0) {
                        rece = ikcp_recv(kcp, user->buffer.data(), (int)user->buffer.size());
                        if (rece > 0) {
                            user->receData.push_back(std::string(user->buffer.data(), rece)); //拷贝记录这一条收到的信息
                        }
                    }

                    //如果这个user里面还有未处理的消息那么就添加进来吧
                    if (user->receData.size() > 0) {
                        vRece.push_back(user);
                    }
                }
            }
        }
        catch (const Poco::Exception& e) {
            LogE("KCPServer.Receive():异常%s,%s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("KCPServer.Receive():异常:%s", e.what());
        }

        return vRece.size();
    }

    // 查询等待发送的消息条数
    int WaitSendCount(int conv)
    {
        if (socket == nullptr) {
            LogE("KCPServer.WaitSendCount():%s 还没有初始化,不能发送!", name);
            return 0;
        }
        if (remotes.find(conv) == remotes.end()) {
            LogE("KCPServer.WaitSendCount():remotes中不包含conv=%d的项!", conv);
            return 0;
        }
        ikcpcb* kcp = remotes[conv];
        return ikcp_waitsnd(kcp);
    }

    /**
     * 向某个远端发送一条消息.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @param  conv The convert.
     * @param  data The data.
     * @param  len  The length.
     *
     * @returns An int.
     */
    int Send(int conv, const char* data, int len)
    {
        if (socket == nullptr) {
            LogE("KCPServer.Send():%s 还没有初始化,不能发送!", name);
            return -1;
        }
        if (remotes.find(conv) == remotes.end()) {
            LogE("KCPServer.Send():remotes中不包含conv=%d的项!", conv);
            return -1;
        }

        ikcpcb* kcp = remotes[conv];
        int res = ikcp_send(kcp, data, len);
        if (res < 0) {
            LogE("KCPServer.Send():发送异常返回%d", res);
        }
        ikcp_flush(kcp); //尝试暴力flush
        return res;
    }

    /**
     * 需要定时调用.在调用接收或者发送之后调用好了
     *
     * @author daixian
     * @date 2020/12/19
     */
    void Update()
    {
        for (auto& kvp : remotes) {
            ikcp_update(kvp.second, iclock());
        }
    }
};

KCPServer::KCPServer(const std::string& name,
                     const std::string& host, int port) : name(name), host(host), port(port)
{
    _impl = new Impl();
}

KCPServer::~KCPServer()
{
    delete _impl;
}

std::string KCPServer::UUID()
{
    return _impl->uuid;
}

std::string KCPServer::SetUUID(const std::string& uuid)
{
    _impl->uuid = uuid;
    _impl->tcpServer->SetUUID(uuid);
    return _impl->uuid;
}

void KCPServer::Start()
{
    _impl->Start(name, host, port);
}

void KCPServer::Close()
{
    _impl->Close();
}

int KCPServer::Receive(std::map<int, std::vector<std::string>>& msgs)
{
    std::vector<KCPUser*> vRece;
    _impl->Receive(vRece);

    msgs.clear();

    for (size_t i = 0; i < vRece.size(); i++) {
        msgs[vRece[i]->conv] = vRece[i]->receData;
        vRece[i]->receData.clear();
    }

    _impl->Update();
    return msgs.size();
}

int KCPServer::Send(int conv, const char* data, int len)
{
    int res = _impl->Send(conv, data, len);
    _impl->Update();
    return res;
}

int KCPServer::WaitSendCount(int conv)
{
    return _impl->WaitSendCount(conv);
}

int KCPServer::RemoteCount()
{
    return static_cast<int>(_impl->remotes.size());
}

std::map<int, std::string> KCPServer::GetRemotes()
{
    std::map<int, std::string> result;
    for (auto& kvp : _impl->remotes) {
        KCPUser* user = (KCPUser*)(kvp.second->user);
        result[kvp.first] = user->uuid_remote;
    }
    return result;
}

int KCPServer::GetConvIDWithUUID(const std::string& uuid)
{
    for (auto& kvp : _impl->remotes) {
        KCPUser* user = (KCPUser*)(kvp.second->user);
        if (user->uuid_remote == uuid)
            return kvp.first;
    }
    return -1;
}

TCPServer* KCPServer::GetTCPServer()
{
    return _impl->tcpServer;
}
} // namespace dxlib