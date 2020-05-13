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
#include <regex>

namespace dxlib {

struct KCPUser
{
    const char* name = "unnamed";
    Poco::Net::DatagramSocket* socket = nullptr;
    Poco::Net::SocketAddress* remote = nullptr;
};

// KCP的下层协议输出函数，KCP需要发送数据时会调用它
// buf/len 表示缓存和长度
// user指针为 kcp对象创建时传入的值，用于区别多个 KCP对象
int kcp2_udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    try {
        //kcp里有几个位置调用udp_output的时候没有传user参数进来,所以不能使用这个参数
        KCPUser* u = (KCPUser*)kcp->user;
        if (u->remote == nullptr) {
            LogE("KCP2.udp_output():%s还没有remote记录,不能发送!", u->name);
            return -1;
        }
        LogD("KCP2.udp_output():%s 执行sendBytes()! len=%d", u->name, len);
        return u->socket->sendTo(buf, len, *u->remote);
    }
    catch (const Poco::Exception& e) {
        LogE("KCP2.udp_output():异常%s %s", e.what(), e.message().c_str());
        return -1;
    }
    catch (const std::exception& e) {
        LogE("KCP2.udp_output():异常:%s", e.what());
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
        try {
            if (kcpUser.socket != nullptr) {
                kcpUser.socket->close();
            }
        }
        catch (const Poco::Exception& e) {
            LogE("KCP2.Impl():异常%s %s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("KCP2.Impl():异常:%s", e.what());
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

    /// <summary> 同意连接的认证信息. </summary>
    std::string acceptStr = "xuexue_kcp";

    ///-------------------------------------------------------------------------------------------------
    /// <summary> 启动工作. </summary>
    ///
    /// <remarks> Dx, 2020/5/12. </remarks>
    ///
    /// <param name="host"> The host. </param>
    /// <param name="port"> The port. </param>
    /// <param name="conv"> The convert. </param>
    ///-------------------------------------------------------------------------------------------------
    inline void Init(const char* name, int conv, const std::string& host, int port,
                     const std::string& remoteHost, int remotePort)
    {
        try {
            kcpUser.name = name;

            Poco::Net::SocketAddress sa(Poco::Net::AddressFamily::IPv4, host, port);
            kcpUser.socket = new Poco::Net::DatagramSocket(sa);              //使用一个端口开始一个接收
            if (!remoteHost.empty() && remotePort > 0 && remotePort < 65536) //如果记录已知远端那么就连接远端
            {
                kcpUser.remote = new Poco::Net::SocketAddress(Poco::Net::AddressFamily::IPv4, remoteHost, remotePort);
            }
            else {
                kcpUser.remote = nullptr;
            }

            //它可以设置是否是阻塞
            kcpUser.socket->setBlocking(false);

            kcp = ikcp_create(conv, &kcpUser);
            // 设置回调函数
            kcp->output = kcp2_udp_output;

            ikcp_nodelay(kcp, 1, 10, 2, 1);
            kcp->rx_minrto = 10;
            kcp->fastresend = 1;
        }
        catch (const Poco::Exception& e) {
            LogE("KCP2.Init():异常%s %s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("KCP2.Init():异常:%s", e.what());
        }
    }

    inline int SendAccept()
    {
        if (kcp == nullptr || kcpUser.remote == nullptr) {
            LogE("KCP2.Receive():%s 还没有初始化,不能发送!", kcpUser.name);
            return -1;
        }
        LogI("KCP2.SendAccept():%s 尝试向服务器发送认证...", kcpUser.name);
        kcpUser.socket->sendTo(acceptStr.c_str(), acceptStr.size(), *kcpUser.remote);

        Poco::Net::SocketAddress remote(Poco::Net::AddressFamily::IPv4);
        //这里应该是要使用临时的buff ,最好大于1400吧
        char* receBuf = new char[1024 * 2];
        int n = 0;
        //最多重试500次,也就是大约5秒
        for (int i = 0; i < 5000; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            n = kcpUser.socket->receiveFrom(receBuf, 1024 * 4, remote);
            if (n > 0) {
                receBuf[n] = 0; //截断一下,好比较字符串
                if (n == acceptStr.size() && std::strcmp(acceptStr.c_str(), receBuf) == 0) {
                    LogI("KCP2.SendAccept():%s 认证成功!", kcpUser.name);
                    return 0;
                }
            }
        }
        delete[] receBuf;
        LogE("KCP2.SendAccept():%s 认证失败!", kcpUser.name);
        return -1;
    }

    inline int Receive(char* buffer, int len)
    {
        if (kcp == nullptr) {
            LogE("KCP2.Receive():%s 还没有启动,不能接收!", kcpUser.name);
            return -1;
        }
        //这里可能是要使用临时的buff ,最好大于1400吧
        char* receBuf = new char[1024 * 4];

        try {
            //先尝试接收,如果能收到那么就直接返回
            int rece = ikcp_recv(kcp, buffer, len);
            if (rece != -1)
                return rece;

            //尝试接收
            Poco::Net::SocketAddress remote(Poco::Net::AddressFamily::IPv4);
            int n = kcpUser.socket->receiveFrom(receBuf, 1024 * 4, remote);
            if (n != -1) {
                LogD("KCP2.Receive():%s Socket接收到了数据,长度%d", kcpUser.name, n);

                //如果还没有一个remote,那就启动认证逻辑
                if (kcpUser.remote == nullptr) {
                    LogD("KCP2.Receive():%s 尝试判断认证", kcpUser.name);
                    receBuf[n] = 0; //截断一下,好比较字符串
                    if (std::strcmp(acceptStr.c_str(), receBuf) == 0) {
                        kcpUser.remote = new Poco::Net::SocketAddress();
                        *kcpUser.remote = remote; //拷贝一下
                        LogI("KCP2.Receive():%s 认证通过,客户端->%s", kcpUser.name, remote.toString().c_str());
                        kcpUser.socket->sendTo(acceptStr.c_str(), acceptStr.size(), *kcpUser.remote); //再回发一下这个认证口令
                    }
                }
                else {
                    //这是已经有了remote了可以正常工作了
                    ikcp_input(kcp, receBuf, n);
                    ikcp_flush(kcp); //尝试暴力flush
                }
            }

            delete[] receBuf; //这里正常接收了,释放

            rece = ikcp_recv(kcp, buffer, len);
            return rece;
        }
        catch (const Poco::Exception& e) {
            LogE("KCP2.Receive():异常%s %s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("KCP2.Receive():异常:%s", e.what());
        }
        delete[] receBuf; //这里不正常接工作,也释放
        return -1;
    }

    ///-------------------------------------------------------------------------------------------------
    /// <summary> 根据一个字符串创建一个地址(这个函数不需要,他能自己判断) </summary>
    ///
    /// <remarks> Dx, 2020/5/13. </remarks>
    ///
    /// <param name="str">  The string. </param>
    /// <param name="port"> The port. </param>
    ///
    /// <returns> The Poco::Net::SocketAddress. </returns>
    ///-------------------------------------------------------------------------------------------------
    static Poco::Net::SocketAddress CreatSocketAddress(const std::string& str, int port)
    {
        std::smatch sm;
        const std::regex pattern("((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})(\\.((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})){3}");

        //正则匹配IPv4地址,使用match的含义是整个字符串都能匹配上
        if (std::regex_match(str, sm, pattern)) {
            return Poco::Net::SocketAddress(Poco::Net::IPAddress(str), port);
        }
        else {
            return Poco::Net::SocketAddress(str, port);
        }
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

KCP2::KCP2(const std::string& name,
           int conv,
           const std::string& host, int port)
    : name(name), conv(conv), host(host), port(port)
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

int KCP2::SendAccept()
{
    return _impl->SendAccept();
}

int KCP2::Receive(char* buffer, int len)
{
    return _impl->Receive(buffer, len);
}

int KCP2::Send(const char* data, int len)
{
    if (_impl->kcp == nullptr) {
        LogE("KCP2.Send()::%s 还没有初始化,不能发送!", name.c_str());
        return 0;
    }
    LogI("KCP2.Send()::%s 开始发送!原始数据长度为%d", name.c_str(), len);
    try {
        ikcp_send(_impl->kcp, data, len);
        ikcp_flush(_impl->kcp); //尝试暴力flush
        return 0;
    }
    catch (const Poco::Exception& e) {
        LogE("KCP2.Send():异常%s %s", e.what(), e.message().c_str());
        return -1;
    }
    catch (const std::exception& e) {
        LogE("KCP2.Send():异常:%s", e.what());
        return -1;
    }
}

void KCP2::Update()
{
    if (_impl->kcp != nullptr)
        ikcp_update(_impl->kcp, iclock());
}
} // namespace dxlib