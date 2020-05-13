#include "KCP2.h"
#include "dlog/dlog.h"

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/MulticastSocket.h"
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

namespace dxlib {

struct KCPUser
{
    const char* name;
    Poco::Net::DatagramSocket* socket = nullptr;
};

// KCP的下层协议输出函数，KCP需要发送数据时会调用它
// buf/len 表示缓存和长度
// user指针为 kcp对象创建时传入的值，用于区别多个 KCP对象
int kcp2_udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    try {
        //kcp里有几个位置调用udp_output的时候没有传user参数进来,所以不能使用这个参数
        KCPUser* user = (KCPUser*)kcp->user;
        LogD("KCP2.udp_output():%s 执行sendBytes()! len=%d", user->name, len);
        return user->socket->sendBytes(buf, len);
    }
    catch (const Poco::Exception& e) {
        LogE("KCP2.udp_output():异常%s %s", e.what(), e.message().c_str());
        return -1;
    }
}

class KCP2::Impl
{
  public:
    Impl() {}
    ~Impl()
    {
        LogI("KCP2.~Impl():%s 释放KCP对象", kcpUser.name);
        if (kcpUser.socket != nullptr) {
            kcpUser.socket->close();
        }

        if (kcp != nullptr)
            ikcp_release(kcp);
        //delete kcp;//使用上面那个就不能delete了

        delete kcpUser.socket;
    }

    // 初始化 kcp对象，conv为一个表示会话编号的整数，和tcp的 conv一样，通信双
    // 方需保证 conv相同，相互的数据包才能够被认可，user是一个给回调函数的指针
    ikcpcb* kcp = nullptr;

    //KCPUser数据
    KCPUser kcpUser;

    ///-------------------------------------------------------------------------------------------------
    /// <summary> 启动工作. </summary>
    ///
    /// <remarks> Dx, 2020/5/12. </remarks>
    ///
    /// <param name="host"> The host. </param>
    /// <param name="port"> The port. </param>
    /// <param name="conv"> The convert. </param>
    ///-------------------------------------------------------------------------------------------------
    void Init(const char* name, int conv, const std::string& host, int port,
              const std::string& remoteHost, int remotePort)
    {
        kcpUser.name = name;

        //Poco::Net::SocketAddress sa(Poco::Net::IPAddress("127.0.0.1"), 24005);
        Poco::Net::SocketAddress sa(host, port);
        kcpUser.socket = new Poco::Net::DatagramSocket(sa); //使用一个端口开始一个接收
        kcpUser.socket->connect(Poco::Net::SocketAddress(remoteHost, remotePort));

        //它可以设置是否是阻塞
        kcpUser.socket->setBlocking(false);

        kcp = ikcp_create(conv, &kcpUser);
        // 设置回调函数
        kcp->output = kcp2_udp_output;

        ikcp_nodelay(kcp, 1, 10, 2, 1);
        kcp->rx_minrto = 10;
        kcp->fastresend = 1;
    }
};

KCP2::KCP2(const std::string& name,
           int conv,
           const std::string& host, int port,
           const std::string& remoteHost, int remotePort)
    : name(name), conv(conv), host(host), port(port), remoteHost(remoteHost), remotePort(remotePort)
{
    _impl = new Impl();
}

KCP2::~KCP2()
{
    delete _impl;
}

void KCP2::Init()
{
    _impl->Init(name.c_str(), conv, host, port, remoteHost, remotePort);
}

int KCP2::Receive(char* buffer, int len)
{
    if (_impl->kcp == nullptr) {
        LogE("KCP2.Receive():%s 还没有启动,不能接收!", name.c_str());
        return 0;
    }

    //先尝试接收,如果能收到那么就直接返回
    int rece = ikcp_recv(_impl->kcp, buffer, len);
    if (rece != -1)
        return rece;

    //尝试接收
    Poco::Net::SocketAddress sender;
    //这里应该是要使用临时的buff ,最好大于1400吧
    char* receBuf = new char[1024 * 4];
    int n = _impl->kcpUser.socket->receiveFrom(receBuf, 1024 * 4, sender); //这一句是阻塞等待接收
    if (n != -1) {
        LogD("KCP2.Receive():%s 接收到了数据,长度%d", name.c_str(), n);
        ikcp_input(_impl->kcp, receBuf, n);
    }
    delete[] receBuf;

    //LogI("KCP.Receive():%s 执行接收!", name.c_str());
    rece = ikcp_recv(_impl->kcp, buffer, len);
    return rece;
}

int KCP2::Send(const char* data, int len)
{
    if (_impl->kcp == nullptr) {
        LogE("KCP2.Send()::%s 还没有连接,不能发送!", name.c_str());
        return 0;
    }
    LogI("KCP2.Send()::%s 开始发送!原始数据长度为%d", name.c_str(), len);
    ikcp_send(_impl->kcp, data, len);
    return 0;
}

void KCP2::Update()
{
    if (_impl->kcp != nullptr)
        ikcp_update(_impl->kcp, iclock());
}
} // namespace dxlib