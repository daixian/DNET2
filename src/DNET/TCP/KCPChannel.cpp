#include "KCPChannel.h"

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/MulticastSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/NetException.h"

#include <thread>
#include <mutex>

#include <regex>

#include "TCPClient.h"

namespace dnet {

// TODO: 可以让上层通过检测 ikcp_waitsnd 函数来判断还有多少包没有发出去，灵活抉择是否向 snd_queue 缓存追加数据包还是其他。
// TODO: 管理大规模连接 https://github.com/skywind3000/kcp/wiki/KCP-Best-Practice
// 如果需要同时管理大规模的 KCP连接（比如大于3000个），比如你正在实现一套类 epoll的机制，那么为了避免每秒钟对每个连接调用大量的调用 ikcp_update，我们可以使用 ikcp_check 来大大减少 ikcp_update调用的次数。
// ikcp_check返回值会告诉你需要在什么时间点再次调用 ikcp_update（如果中途没有 ikcp_send, ikcp_input的话，否则中途调用了 ikcp_send, ikcp_input的话，需要在下一次interval时调用 update）
// 标准顺序是每次调用了 ikcp_update后，使用 ikcp_check决定下次什么时间点再次调用 ikcp_update，而如果中途发生了 ikcp_send, ikcp_input 的话，在下一轮 interval 立马调用 ikcp_update和 ikcp_check。
// 使用该方法，原来在处理2000个 kcp连接且每 个连接每10ms调用一次update，改为 check机制后，cpu从 60% 降低到 15%。

// KCP的下层协议输出函数，KCP需要发送数据时会调用它
// buf/len 表示缓存和长度
// user指针为 kcp对象创建时传入的值，用于区别多个 KCP对象
int kcpc_udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    try {
        // kcp里有几个位置调用udp_output的时候没有传user参数进来,所以不能使用这个参数
        KCPChannel* u = (KCPChannel*)kcp->user;
        if (u->remote == nullptr) {
            LogE("KCPChannel.kcpc_udp_output():conv%d还没有remote记录,不能发送!", u->kcp->conv);
            return -1;
        }
        if (u->udpSocket == nullptr) {
            LogE("KCPChannel.kcpc_udp_output():conv%d还没有socket,不能发送!", u->kcp->conv);
            return -1;
        }
        // LogI("KCPChannel.kcpc_udp_output():向{%s}发送! len=%d", u->remote->toString().c_str(), len);
        //
        //  返回发送了的byte
        int sentbytes = u->udpSocket->sendTo(buf, len, *u->remote);

        // dx:注意,windows下这里永远是会发送成功的。但是如果此时这个远程已经关闭了,那么会在接收udpSocket->receiveFrom()的时候
        // 出现异常,十分无解. https://www.cnblogs.com/leading/archive/2012/06/24/udp-connection-reset-10054-under-windows.html
        //
        // if (sentbytes == 0) {
        //    LogW("KCPChannel.kcpc_udp_output():发送长度为0？");
        //}
        return sentbytes;
    }
    catch (const Poco::Exception& e) {
        LogE("KCPChannel.kcpc_udp_output():异常%s %s", e.what(), e.message().c_str());
        return -1;
    }
    catch (const std::exception& e) {
        LogE("KCPChannel.kcpc_udp_output():异常:%s", e.what());
        return -1;
    }
}

KCPChannel::KCPChannel()
{
    lastReceMsgTime = clock();
    kcpReceBuf.resize(4 * 1024, 0);
}

KCPChannel::KCPChannel(Poco::Net::DatagramSocket* udpSocket, int conv) : udpSocket(udpSocket)
{
    lastReceMsgTime = clock();
    kcpReceBuf.resize(4 * 1024, 0);
    Create(conv);
}

KCPChannel::~KCPChannel()
{
    if (kcp != nullptr) {
        ikcp_release(kcp);
    }
}

void KCPChannel::Create(int conv)
{
    if (kcp != nullptr) {
        ikcp_release(kcp);
    }

    kcp = ikcp_create(conv, this);
    // 设置回调函数
    kcp->output = kcpc_udp_output;

    // 配置窗口大小：平均延迟200ms，每20ms发送一个包，
    // 调整这个窗口大小好像没什么用。
    ikcp_wndsize(kcp, 32, 128);
    // 普通模式/正常模式：
    // ikcp_nodelay(kcp, 0, 40, 0, 0);

    // 这是一个标准的快速模式的配置
    ikcp_nodelay(kcp, 1, 10, 2, 1);
    kcp->rx_minrto = 10; //  TCP还是KCP计算RTO时都有最小RTO的限制，即便计算出来RTO为40ms，由于默认的RTO是100ms，协议只有在100ms后才能检测到丢包，快速模式下为30ms
    kcp->fastresend = 1;
}

int KCPChannel::Send(const char* data, size_t len, int type)
{
    if (udpSocket == nullptr || kcp == nullptr) {
        LogE("KCPChannel.Send():还没有初始化,不能发送!");
        return -1;
    }

    // 数据打包
    std::vector<char> package;
    packet.Pack(data, (int)len, package, type);
    sendMsgCount++;

    int res = ikcp_send(kcp, package.data(), (int)package.size());
    if (res < 0) {
        LogE("KCPChannel.Send():发送异常返回 res=%d", res);
    }
    // ikcp_flush(kcp); // 尝试暴力flush

    ikcp_update(kcp, iclock());
    return res;
}

// 这是KCP的协议接收
int KCPChannel::IKCPRecv(const char* buff, size_t len, std::vector<TextMessage>& msgs)
{
    if (kcp == nullptr) {
        LogE("KCPChannel.IKCPRecv():还没有初始化,不能接收!");
        return -2;
    }
    // 注意这里clear了
    msgs.clear();

    if (len != -1) {
        // LogD("KCPChannel.Receive(): Socket接收到了数据,长度%d", len);

        // 尝试给kcp看看是否是它的信道的数据
        int rece = ikcp_input(kcp, buff, (long)len);
        if (rece != 0) {
            // conv不对应或者其它错误
            return -1;
        }
        else {
            // ikcp_flush(kcp); //尝试暴力flush

            while (rece >= 0) {
                rece = ikcp_recv(kcp, kcpReceBuf.data(), (int)kcpReceBuf.size());
                if (rece == -3) {
                    LogI("KCPChannel.IKCPRecv():ikcp_recv返回了-3");
                }
                if (rece > 0) {
                    // 这里实际上应该只能找到1条消息
                    std::vector<TextMessage> msg1;
                    receMsgCount += packet.Unpack(kcpReceBuf.data(), rece, msg1);
                    lastReceMsgTime = clock(); // 记录这个时间
                    for (size_t i = 0; i < msg1.size(); i++) {
                        msgs.push_back(msg1[i]);
                    }
                    // receData.push_back(std::string(kcpReceBuf.data(), rece)); //拷贝记录这一条收到的信息
                }
            }
        }
    }
    return msgs.size();
}
} // namespace dnet