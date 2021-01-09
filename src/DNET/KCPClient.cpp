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
        KCPClient* u = (KCPClient*)kcp->user;
        //if (u->remote.a nullptr) {
        //    LogE("KCP2.udp_output():%s还没有remote记录,不能发送!", u->name);
        //    return -1;
        //}
        //LogD("KCPClient.udp_output():向{%s}发送! len=%d", u->uuid_remote.c_str(), len);
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

KCPClient::KCPClient()
{
    socketBuffer.resize(8 * 1024);
    kcpReceBuf.resize(8 * 1024);
}

KCPClient::~KCPClient()
{
}

void KCPClient::Bind(void* tcp)
{
    TCPClient* tcpClient = (TCPClient*)tcp;
    if (tcpClient->isAccepted()) {
        std::string uuidS = tcpClient->AcceptData()->uuidS;
        std::string nameS = tcpClient->AcceptData()->nameS;
        int conv = tcpClient->AcceptData()->conv;

        Poco::Net::StreamSocket* tcs = (Poco::Net::StreamSocket*)tcpClient->Socket();
        Poco::Net::SocketAddress sAddr = tcs->address();

        socket = new Poco::Net::DatagramSocket(sAddr); //使用和TCP端口相同的端口开始一个UDP接收
        socket->setBlocking(false);                    //它可以设置是否是阻塞

        remote = tcs->peerAddress(); //使用实际的远程端口就行了应该

        kcp = ikcp_create(conv, this);
        // 设置回调函数
        kcp->output = kcpc_udp_output;

        ikcp_nodelay(kcp, 1, 10, 2, 1);
        //kcp->rx_minrto = 10;
        //kcp->fastresend = 1;
    }
}

int KCPClient::Receive(std::vector<std::string>& msgs)
{
    if (socket == nullptr) {
        LogE("KCPClient.Receive():还没有初始化,不能接收!");
        return -1;
    }

    if (kcp == nullptr) {
        return -1;
    }
    int rece;

    try {
        //socket尝试接收
        Poco::Net::SocketAddress remote(Poco::Net::AddressFamily::IPv4);
        int n = socket->receiveFrom(socketBuffer.data(), (int)socketBuffer.size(), remote);
        if (n != -1) {
            LogD("KCPClient.Receive(): Socket接收到了数据,长度%d", n);

            //尝试给kcp看看是否是它的信道的数据
            rece = ikcp_input(kcp, socketBuffer.data(), n);
            if (rece == -1) {
                //conv不对应
            }
            else {
                ikcp_flush(kcp); //尝试暴力flush

                while (rece >= 0) {
                    rece = ikcp_recv(kcp, kcpReceBuf.data(), kcpReceBuf.size());
                    if (rece > 0) {
                        receData.push_back(std::string(kcpReceBuf.data(), rece)); //拷贝记录这一条收到的信息
                    }
                }
            }
        }
    }
    catch (const Poco::Exception& e) {
        LogE("KCPClient.Receive():异常%s,%s", e.what(), e.message().c_str());
    }
    catch (const std::exception& e) {
        LogE("KCPClient.Receive():异常:%s", e.what());
    }

    return (int)receData.size();

    msgs.clear();

    msgs = receData;
    receData.clear();

    ikcp_update(kcp, iclock()); //需要定时调用.在调用接收或者发送之后调用好了
    return msgs.size();
}

int KCPClient::Send(const char* data, size_t len)
{
    if (socket == nullptr || kcp == nullptr) {
        LogE("KCPClient.Send():还没有初始化,不能发送!");
        return -1;
    }

    int res = ikcp_send(kcp, data, (int)len);
    if (res < 0) {
        LogE("KCPClient.Send():发送异常返回 res=%d", res);
    }
    ikcp_flush(kcp); //尝试暴力flush
    ikcp_update(kcp, iclock());
    return res;
}

} // namespace dxlib