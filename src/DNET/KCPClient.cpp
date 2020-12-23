#include "KCPClient.h"

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

#include "TCP/TCPClient.h"

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
int kcpc_udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    try {
        //kcp里有几个位置调用udp_output的时候没有传user参数进来,所以不能使用这个参数
        KCPUser* u = (KCPUser*)kcp->user;
        //if (u->remote.a nullptr) {
        //    LogE("KCP2.udp_output():%s还没有remote记录,不能发送!", u->name);
        //    return -1;
        //}
        LogD("KCPClient.udp_output():向%s执行sendBytes()! len=%d", u->uuid_remote.c_str(), len);
        return u->socket->sendTo(buf, len, u->remote);
    }
    catch (const Poco::Exception& e) {
        LogE("KCPClient.udp_output():异常%s %s", e.what(), e.message().c_str());
        return -1;
    }
    catch (const std::exception& e) {
        LogE("KCPClient.udp_output():异常:%s", e.what());
        return -1;
    }
}

class KCPClient::Impl
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
        delete clientTempAccept;
    }

    // 自己的名字
    std::string name;

    // 唯一标识符
    std::string uuid;

    // kcp使用的UDP端口
    int port = 0;

    // 自己的本地socket
    Poco::Net::DatagramSocket* socket = nullptr;

    // TCP的连接用于客户端连接tcp服务器
    TCPClient* tcpClient = nullptr;

    //socket使用的buff ,最好大于1400吧
    std::vector<char> receBuf;

    //// 用于监听握手用的kcp对象,id号为0
    //ikcpcb* kcpAccept = nullptr;

    //是否是等待连接状态
    Accept* clientTempAccept = nullptr;

    // 远程对象列表
    ikcpcb* kcpRemote = nullptr;

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
        Close();

        if (clientTempAccept != nullptr) {
            LogW("KCPClient.SendAccept():当前存在Accept未完成的状态,这是一次强制重发.");
            delete clientTempAccept;
        }

        tcpClient = new TCPClient();
        tcpClient->SetUUID(uuid);
        int res = tcpClient->Connect(host, port);
        if (res == 0) {
            //tcp连接成功了
            Poco::Net::DatagramSocket* tcs = (Poco::Net::DatagramSocket*)tcpClient->Socket();
            Poco::Net::SocketAddress sAddr = tcs->address();

            socket = new Poco::Net::DatagramSocket(sAddr); //使用和TCP端口相同的端口开始一个UDP接收
            socket->setBlocking(false);                    //它可以设置是否是阻塞
            clientTempAccept = new Accept();
            std::string acceptStr = clientTempAccept->CreateAcceptString(uuid, 0); //自己的端口是无意义的
            return tcpClient->Send(acceptStr.c_str(), acceptStr.size());
        }
        return -1;
    }

    void Close()
    {
        if (tcpClient != nullptr) {
            try {
                tcpClient->Close();
            }
            catch (const std::exception&) {
            }
            delete tcpClient;
            tcpClient = nullptr;
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

        delete kcpRemote;
        kcpRemote = nullptr;
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

            KCPUser* kcpUser = new KCPUser(socket, conv);
            kcpUser->accept = *clientTempAccept;
            kcpUser->uuid_remote = remoteName;
            kcpUser->remote = tcs->peerAddress(); //使用实际的远程端口就行了应该

            LogI("KCPClient.SendAccept():%s(%s:%d)远程通知来的conv=%d,认证成功!", remoteName.c_str(),
                 kcpUser->remote.host().toString().c_str(),
                 kcpUser->remote.port(), conv);

            ikcpcb* kcp = ikcp_create(conv, kcpUser);
            // 设置回调函数
            kcp->output = kcpc_udp_output;

            ikcp_nodelay(kcp, 1, 10, 2, 1);
            //kcp->rx_minrto = 10;
            //kcp->fastresend = 1;
            kcpRemote = kcp; //添加这个新客户端

            //认证完成,关闭认证相关的tcp客户端(先不要关闭了)
            delete clientTempAccept;
            clientTempAccept = nullptr;
            //tcpClient->Close();
            //delete tcpClient;
            //tcpClient = nullptr;
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
    int Receive()
    {
        //执行一下TCP的
        TCPClientAcceptReceive();

        if (socket == nullptr) {
            LogE("KCPX.Receive():%s 还没有初始化,不能接收!", name);
            return -1;
        }

        if (kcpRemote == nullptr) {
            return -1;
        }
        int rece;
        ikcpcb* kcp = kcpRemote;
        KCPUser* user = (KCPUser*)kcp->user;
        try {
            //socket尝试接收
            Poco::Net::SocketAddress remote(Poco::Net::AddressFamily::IPv4);
            int n = socket->receiveFrom(receBuf.data(), (int)receBuf.size(), remote);
            if (n != -1) {
                LogD("KCPX.Receive():%s Socket接收到了数据,长度%d", name, n);

                //尝试给kcp看看是否是它的信道的数据
                rece = ikcp_input(kcp, receBuf.data(), n);
                if (rece == -1) {
                    //conv不对应
                }
                else {
                    ikcp_flush(kcp); //尝试暴力flush

                    while (rece >= 0) {
                        rece = ikcp_recv(kcp, user->buffer.data(), user->buffer.size());
                        if (rece > 0) {
                            user->receData.push_back(std::string(user->buffer.data(), rece)); //拷贝记录这一条收到的信息
                        }
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

        return (int)user->receData.size();
    }

    // 查询等待发送的消息条数
    int WaitSendCount()
    {
        if (socket == nullptr || kcpRemote == nullptr) {
            LogE("KCPX.WaitSendCount():%s 还没有初始化,不能发送!", name);
            return 0;
        }

        return ikcp_waitsnd(kcpRemote);
    }

    int Send(const char* data, int len)
    {
        if (socket == nullptr || kcpRemote == nullptr) {
            LogE("KCPX.Send():%s 还没有初始化,不能发送!", name);
            return -1;
        }

        int res = ikcp_send(kcpRemote, data, len);
        if (res < 0) {
            LogE("KCPX.Send():发送异常返回%d", res);
        }
        ikcp_flush(kcpRemote); //尝试暴力flush
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
        if (kcpRemote != nullptr)
            ikcp_update(kcpRemote, iclock());
    }
};

KCPClient::KCPClient(const std::string& name) : name(name)
{
    _impl = new Impl();
}

KCPClient::~KCPClient()
{
    delete _impl;
}

void KCPClient::Close()
{
    _impl->Close();
}

int KCPClient::Receive(std::vector<std::string>& msgs)
{
    _impl->Receive();

    msgs.clear();
    if (_impl->kcpRemote == nullptr) {
        return -1;
    }
    KCPUser* kcpUser = (KCPUser*)_impl->kcpRemote->user;
    msgs = kcpUser->receData;
    kcpUser->receData.clear();

    _impl->Update();
    return msgs.size();
}

int KCPClient::Send(const char* data, int len)
{
    int res = _impl->Send(data, len);
    _impl->Update();
    return res;
}

int KCPClient::Connect(const std::string& host, int port)
{
    int res = _impl->SendAccept(host, port);
    _impl->Update();
    return res;
}

int KCPClient::WaitSendCount()
{
    return _impl->WaitSendCount();
}

bool KCPClient::isAccepting()
{
    if (_impl->clientTempAccept != nullptr) {
        return true;
    }
    else {
        return false;
    }
}

} // namespace dxlib