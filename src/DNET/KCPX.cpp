#include "KCPX.h"

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

#include "./kcp/ikcp.h"
#include <thread>
#include <mutex>
#include "clock.hpp"
#include <regex>
#include "Accept.h"

#include "TCP/TCPServer.h"
#include "TCP/TCPClient.h"

namespace dxlib {

//TODO: 可以让上层通过检测 ikcp_waitsnd 函数来判断还有多少包没有发出去，灵活抉择是否向 snd_queue 缓存追加数据包还是其他。
//TODO: 管理大规模连接 https://github.com/skywind3000/kcp/wiki/KCP-Best-Practice
//如果需要同时管理大规模的 KCP连接（比如大于3000个），比如你正在实现一套类 epoll的机制，那么为了避免每秒钟对每个连接调用大量的调用 ikcp_update，我们可以使用 ikcp_check 来大大减少 ikcp_update调用的次数。
//ikcp_check返回值会告诉你需要在什么时间点再次调用 ikcp_update（如果中途没有 ikcp_send, ikcp_input的话，否则中途调用了 ikcp_send, ikcp_input的话，需要在下一次interval时调用 update）
//标准顺序是每次调用了 ikcp_update后，使用 ikcp_check决定下次什么时间点再次调用 ikcp_update，而如果中途发生了 ikcp_send, ikcp_input 的话，在下一轮 interval 立马调用 ikcp_update和 ikcp_check。
//使用该方法，原来在处理2000个 kcp连接且每 个连接每10ms调用一次update，改为 check机制后，cpu从 60% 降低到 15%。

/**
 * 一个远程记录对象,用于绑定kcp对象.因为kcp需要设置一个回调函数output.
 *
 * @author daixian
 * @date 2020/12/19
 */
class KCPXUser
{
  public:
    KCPXUser(Poco::Net::DatagramSocket* socket, int conv) : socket(socket), conv(conv)
    {
        buffer.resize(4096);
    }

    // 这里也记录一份conv
    int conv;

    // 远程对象名字
    std::string name = "unnamed";

    // 自己的UDPSocket,用于发送函数
    Poco::Net::DatagramSocket* socket = nullptr;

    // 要发送过去的地址(这里还是用对象算了)
    Poco::Net::SocketAddress remote;

    // 这个远端之间的认证信息(每个kcp连接之间都有自己的认证随机Key)
    Accept accept;

    // 接收数据buffer
    std::vector<char> buffer;

    // 接收到的待处理的数据
    std::vector<std::string> receData;
};

// KCP的下层协议输出函数，KCP需要发送数据时会调用它
// buf/len 表示缓存和长度
// user指针为 kcp对象创建时传入的值，用于区别多个 KCP对象
int kcpx_udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    try {
        //kcp里有几个位置调用udp_output的时候没有传user参数进来,所以不能使用这个参数
        KCPXUser* u = (KCPXUser*)kcp->user;
        //if (u->remote.a nullptr) {
        //    LogE("KCP2.udp_output():%s还没有remote记录,不能发送!", u->name);
        //    return -1;
        //}
        LogD("KCPX.udp_output():%s 执行sendBytes()! len=%d", u->name.c_str(), len);
        return u->socket->sendTo(buf, len, u->remote);
    }
    catch (const Poco::Exception& e) {
        LogE("KCPX.udp_output():异常%s %s", e.what(), e.message().c_str());
        return -1;
    }
    catch (const std::exception& e) {
        LogE("KCPX.udp_output():异常:%s", e.what());
        return -1;
    }
}

class KCPX::Impl
{
  public:
    Impl() {}
    ~Impl()
    {
        if (tcpServer != nullptr) {
            try {
                tcpServer->Close();
            }
            catch (const std::exception&) {
            }
            delete tcpServer;
        }

        if (tcpClient != nullptr) {
            try {
                tcpClient->Close();
            }
            catch (const std::exception&) {
            }
            delete tcpClient;
        }

        if (socket != nullptr) {
            try {
                socket->close();
            }
            catch (const std::exception&) {
            }
            delete socket;
        }

        delete receBuf;

        for (auto& kvp : remotes) {
            delete kvp.second;
        }
    }

    // 自己的名字
    std::string name;

    // kcp使用的UDP端口
    int port = 0;

    // 自己的本地socket
    Poco::Net::DatagramSocket* socket = nullptr;

    // TCP的socket用于服务器监听握手信息端口和UDP一致
    TCPServer* tcpServer = nullptr;

    // TCP的连接用于客户端连接tcp服务器
    TCPClient* tcpClient = nullptr;

    //socket使用的buff ,最好大于1400吧
    char* receBuf = new char[1024 * 4];

    //// 用于监听握手用的kcp对象,id号为0
    //ikcpcb* kcpAccept = nullptr;

    //是否是等待连接状态
    Accept* clientTempAccept = nullptr;

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
    void Init(const std::string& name, const std::string& host, int port)
    {
        try {
            this->name = name;
            this->port = port;
            Poco::Net::SocketAddress sAddr(Poco::Net::AddressFamily::IPv4, host, port);

            socket = new Poco::Net::DatagramSocket(sAddr); //使用一个端口开始一个UDP接收
            socket->setBlocking(false);                    //它可以设置是否是阻塞

            //启动TCP监听,等待客户端握手连入
            tcpServer = new TCPServer(name, host, port);
            tcpServer->Start();
            tcpServer->WaitStarted();

            //KCPXUser* kcpUser = new KCPXUser(socket, 0);
            //kcpAccept = ikcp_create(0, kcpUser);
            //// 设置回调函数
            //kcpAccept->output = kcpx_udp_output;

            //ikcp_nodelay(kcpAccept, 1, 10, 2, 1);
            //kcpAccept->rx_minrto = 10;
            //kcpAccept->fastresend = 1;
        }
        catch (const Poco::Exception& e) {
            LogE("KCPX.Init():异常%s %s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("KCPX.Init():异常:%s", e.what());
        }
    }

    /**
     * 得到一个当前可用的ID号
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns The convert.
     */
    int GetConv()
    {
        int conv = 1;
        bool isFind = false;
        while (!isFind) {
            for (auto& kvp : remotes) {
                if (kvp.first == conv) {
                    conv++;
                    break;
                }
            }
            isFind = true;
        }
        return conv;
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
            int conv = GetConv();
            KCPXUser* kcpUser = new KCPXUser(socket, conv);                                              //准备创建新的客户端用户
            std::string replyStr = kcpUser->accept.ReplyAcceptString(acceptStr, name, conv, this->port); //使用这个新的客户端用户做accept
            if (replyStr.empty()) {
                // replyStr为空,非法的认证信息
                LogE("KCPX.kcpAcceptReceive():非法的认证信息!");
                delete kcpUser;
            }
            else {
                // replyStr有内容,有效的认证信息,自己是服务器端
                Poco::Net::StreamSocket* tcs = (Poco::Net::StreamSocket*)tcpServer->GetClientSocket(tcpID);
                kcpUser->remote = Poco::Net::SocketAddress(tcs->peerAddress().host(), kcpUser->accept.portC()); //绑定客户端的端口
                kcpUser->name = kcpUser->accept.nameC();
                tcpServer->Send(tcpID, replyStr.c_str(), replyStr.size());

                ikcpcb* kcp = ikcp_create(conv, kcpUser);
                // 设置回调函数
                kcp->output = kcpx_udp_output;

                ikcp_nodelay(kcp, 1, 10, 2, 1);
                //kcp->rx_minrto = 10;
                //kcp->fastresend = 1;
                remotes[conv] = kcp; //添加这个新客户端
                LogI("KCPX.kcpAcceptReceive():添加了一个新客户端%s,Addr=%s:%d,分配conv=%d", kcpUser->name.c_str(),
                     kcpUser->remote.host().toString().c_str(),
                     kcpUser->remote.port(), conv);
            }
        }
    }

    void TCPClientAcceptReceive()
    {
        if (tcpClient == nullptr) {
            return;
        }
        if (clientTempAccept == nullptr) {
            return;
        }

        std::vector<std::string> msgs;
        tcpClient->Receive(msgs);
        if (msgs.size() > 0) {
            std::string& acceptStr = msgs[0]; //只处理第一条(设计上应该只有一条)
            std::string remoteName;
            int conv;

            clientTempAccept->VerifyReplyAccept(acceptStr.data(), remoteName, conv);
            poco_assert(conv > 0);

            Poco::Net::StreamSocket* tcs = (Poco::Net::StreamSocket*)tcpClient->Socket();

            KCPXUser* kcpUser = new KCPXUser(socket, conv);
            kcpUser->accept = *clientTempAccept;
            kcpUser->name = remoteName;
            kcpUser->remote = Poco::Net::SocketAddress(tcs->peerAddress().host(), clientTempAccept->portS()); //kcp的端口是固定的,和自己一样;

            LogI("KCPX.SendAccept():%s(%s:%d)远程通知来的conv=%d,认证成功!", remoteName.c_str(),
                 kcpUser->remote.host().toString().c_str(),
                 kcpUser->remote.port(), conv);

            ikcpcb* kcp = ikcp_create(conv, kcpUser);
            // 设置回调函数
            kcp->output = kcpx_udp_output;

            ikcp_nodelay(kcp, 1, 10, 2, 1);
            //kcp->rx_minrto = 10;
            //kcp->fastresend = 1;
            remotes[conv] = kcp; //添加这个新客户端

            //认证完成,关闭认证相关的tcp客户端
            delete clientTempAccept;
            clientTempAccept = nullptr;
            tcpClient->Close();
            delete tcpClient;
            tcpClient = nullptr;
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
    int Receive(std::vector<KCPXUser*>& vRece)
    {
        if (socket == nullptr) {
            LogE("KCPX.Receive():%s 还没有初始化,不能接收!", name);
            return -1;
        }

        //执行一下TCP的
        TCPServerAcceptReceive();
        TCPClientAcceptReceive();

        vRece.clear();
        int rece;
        try {
            //socket尝试接收
            Poco::Net::SocketAddress remote(Poco::Net::AddressFamily::IPv4);
            int n = socket->receiveFrom(receBuf, 1024 * 4, remote);
            if (n != -1) {
                LogD("KCPX.Receive():%s Socket接收到了数据,长度%d", name, n);

                //进行Accept的判断
                for (auto& kvp : remotes) {
                    //尝试给kcp看看是否是它的信道的数据
                    ikcpcb* kcp = kvp.second;
                    rece = ikcp_input(kcp, receBuf, n);
                    if (rece == -1) {
                        //conv不对应
                        continue;
                    }
                    ikcp_flush(kcp); //尝试暴力flush

                    KCPXUser* user = (KCPXUser*)kcp->user;
                    while (rece >= 0) {
                        rece = ikcp_recv(kcp, user->buffer.data(), user->buffer.size());
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
            LogE("KCPX.Receive():异常%s,%s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("KCPX.Receive():异常:%s", e.what());
        }

        return vRece.size();
    }

    // 查询等待发送的消息条数
    int WaitSendCount(int conv)
    {
        if (socket == nullptr) {
            LogE("KCPX.WaitSendCount():%s 还没有初始化,不能发送!", name);
            return 0;
        }
        if (remotes.find(conv) == remotes.end()) {
            LogE("KCPX.WaitSendCount():remotes中不包含conv=%d的项!", conv);
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
            LogE("KCPX.Send():%s 还没有初始化,不能发送!", name);
            return -1;
        }
        if (remotes.find(conv) == remotes.end()) {
            LogE("KCPX.Send():remotes中不包含conv=%d的项!", conv);
            return -1;
        }

        ikcpcb* kcp = remotes[conv];
        int res = ikcp_send(kcp, data, len);
        if (res < 0) {
            LogE("KCPX.Send():发送异常返回%d", res);
        }
        ikcp_flush(kcp); //尝试暴力flush
        return res;
    }

    /**
     * Sends an accept
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @param  host The host.
     * @param  port The port.
     *
     * @returns An int.
     */
    int SendAccept(const std::string& host, int port)
    {
        if (socket == nullptr) {
            LogE("KCPX.SendAccept():%s UDP还没有初始化,不能发送!", name);
            return -1;
        }

        if (tcpClient != nullptr) {
            tcpClient->Close();
            delete tcpClient;
        }

        if (clientTempAccept != nullptr) {
            LogW("KCPX.SendAccept():当前存在Accept未完成的状态,这是一次强制重发.");
            delete clientTempAccept;
        }

        tcpClient = new TCPClient();
        tcpClient->Connect(host, port);
        clientTempAccept = new Accept();

        std::string acceptStr = clientTempAccept->CreateAcceptString(name, this->port);
        return tcpClient->Send(acceptStr.c_str(), acceptStr.size());
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

KCPX::KCPX(const std::string& name,
           const std::string& host, int port) : name(name), host(host), port(port)
{
    _impl = new Impl();
}

KCPX::~KCPX()
{
    delete _impl;
}

void KCPX::Init()
{
    _impl->Init(name, host, port);
}

void KCPX::Close()
{
    delete _impl;
    _impl = new Impl();
}

int KCPX::Receive(std::map<int, std::vector<std::string>>& msgs)
{
    std::vector<KCPXUser*> vRece;
    _impl->Receive(vRece);

    msgs.clear();

    for (size_t i = 0; i < vRece.size(); i++) {
        msgs[vRece[i]->conv] = vRece[i]->receData;
        vRece[i]->receData.clear();
    }

    _impl->Update();
    return msgs.size();
}

int KCPX::Send(int conv, const char* data, int len)
{
    int res = _impl->Send(conv, data, len);
    _impl->Update();
    return res;
}

int KCPX::SendAccept(const std::string& host, int port)
{
    int res = _impl->SendAccept(host, port);
    _impl->Update();
    return res;
}

int KCPX::WaitSendCount(int conv)
{
    return _impl->WaitSendCount(conv);
}

int KCPX::RemoteCount()
{
    return static_cast<int>(_impl->remotes.size());
}

std::map<int, std::string> KCPX::GetRemotes()
{
    std::map<int, std::string> result;
    for (auto& kvp : _impl->remotes) {
        KCPXUser* user = (KCPXUser*)(kvp.second->user);
        result[kvp.first] = user->name;
    }
    return result;
}

int KCPX::GetConvIDWithName(const std::string& name)
{
    for (auto& kvp : _impl->remotes) {
        KCPXUser* user = (KCPXUser*)(kvp.second->user);
        if (user->name == name)
            return kvp.first;
    }
    return -1;
}

bool KCPX::isAccepting()
{
    if (_impl->clientTempAccept != nullptr) {
        return true;
    }
    else {
        return false;
    }
}

} // namespace dxlib