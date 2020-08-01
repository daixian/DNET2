#include "KCP.h"
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
};

// KCP的下层协议输出函数，KCP需要发送数据时会调用它
// buf/len 表示缓存和长度
// user指针为 kcp对象创建时传入的值，用于区别多个 KCP对象
int kcp_udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    try {
        //kcp里有几个位置调用udp_output的时候没有传user参数进来,所以不能使用这个参数
        KCPUser* user = (KCPUser*)kcp->user;
        LogD("KCP.udp_output():%s 执行sendBytes()! len=%d", user->name, len);
        return user->socket->sendBytes(buf, len);
    }
    catch (const Poco::Exception& e) {
        LogE("KCP.udp_output():异常%s %s", e.what(), e.message().c_str());
        return -1;
    }
}

class ReceRunnable : public Poco::Runnable
{
  public:
    ReceRunnable(const char* name, ikcpcb* kcp, Poco::Net::DatagramSocket* socket, std::mutex* mut_kcp)
        : name(name), kcp(kcp), socket(socket), mut_kcp(mut_kcp)
    {
    }

    virtual void run()
    {
        while (kcp->user == nullptr) {
            LogD("ReceRunnable.run():%s kcp->user为空,等待赋值...", name);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        LogD("ReceRunnable.run():%s kcp->user赋值完毕...", name);

        while (isRun.load()) {
            try {
                //LogD("ReceRunnable.run():%s 开始接收!", name);
                //Poco::Net::SocketAddress sender;
                int n = socket->receiveFrom(buffer, sizeof(buffer), sender); //这一句是阻塞等待接收

                LogD("ReceRunnable.run():%s 接收到了数据,长度%d", name, n);
#ifndef _WIN32
                if (n == 0) {
                    //给对面也发一个关闭?
                    socket->sendBytes("\0", 0, SHUT_RDWR);
                }
#endif // !_WIN32
                mut_kcp->lock();
                ikcp_input(kcp, buffer, n);
                ikcp_flush(kcp);
                mut_kcp->unlock();
            }
            catch (const Poco::TimeoutException te) {
                //超时就忽略它
            }
            catch (const Poco::Exception& e) {
                LogE("ReceRunnable.run():%s 异常%s %s", name, e.what(), e.message().c_str());
            }
        }
    }

    const char* name;
    std::atomic_bool isRun{true};

    /// <summary> UDP默认分片是1400,这个值要大于1400. </summary>
    char buffer[1024 * 4];

    Poco::Net::DatagramSocket* socket;
    ikcpcb* kcp;
    std::mutex* mut_kcp;
    Poco::Net::SocketAddress sender;
};

class UpdateRunnable : public Poco::Runnable
{
  public:
    UpdateRunnable(const char* name, ikcpcb* kcp, std::mutex* mut_kcp) : name(name), kcp(kcp), mut_kcp(mut_kcp)
    {
    }

    virtual void run()
    {
        while (kcp->user == nullptr) {
            LogD("UpdateRunnable.run():%s kcp->user为空,等待赋值...", name);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        LogD("UpdateRunnable.run():%s kcp->user赋值完毕...", name);

        while (isRun.load()) {
            try {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));

                mut_kcp->lock();
                //LogI("UpdateRunnable.run():调用ikcp_update()");
                // 以一定频率调用 ikcp_update来更新 kcp状态，并且传入当前时钟（毫秒单位）
                // 如 10ms调用一次，或用 ikcp_check确定下次调用 update的时间不必每次调用
                ikcp_update(kcp, iclock());
                mut_kcp->unlock();
            }
            catch (const Poco::Exception& e) {
                LogE("UpdateRunnable.run():%s异常%s %s", name, e.what(), e.message().c_str());
            }
        }
    }
    const char* name;
    std::atomic_bool isRun{true};
    ikcpcb* kcp;
    std::mutex* mut_kcp;
};

class KCP::Impl
{
  public:
    Impl() {}
    ~Impl()
    {
        LogI("KCP.~Impl():%s释放KCP对象", kcpUser.name);
        if (kcpUser.socket != nullptr) {
            try {
                LogI("KCP.~Impl():%s关闭socket", kcpUser.name);
                //windows下好像不支持这种附加SHUT_RDWR的调用,会提示Net Exception Operation not supported.
                //windows下可以直接close()就会中断阻塞的接收,所以也不需要发送一下
#ifndef _WIN32
                //给对面发一个关闭?
                kcpUser.socket->sendBytes("\0", 0, SHUT_RDWR);
#endif // !_WIN32
                kcpUser.socket->close();
            }
            catch (const Poco::Exception& e) {
                LogE("KCP.~Impl():%s异常%s %s", kcpUser.name, e.what(), e.message().c_str());
            }
        }

        if (thrRece != nullptr) {
            runRece->isRun = false;
        }
        if (runUpdate != nullptr) {
            runUpdate->isRun = false;
        }

        if (thrRece != nullptr) {
            LogI("KCP.~Impl():%s等待接收线程关闭", kcpUser.name);
            //好像poco的这个线程可以直接delete,这里直接delete不会报错,但是会到底端口没有释放
            thrRece->join();
        }

        if (thrUpdate != nullptr) {
            LogI("KCP.~Impl():%s等待update线程关闭", kcpUser.name);
            thrUpdate->join();
        }

        //再次尝试关闭
        if (kcpUser.socket != nullptr) {
            try {
                kcpUser.socket->close();
            }
            catch (const Poco::Exception& e) {
                LogE("KCP.~Impl():%s异常%s %s", kcpUser.name, e.what(), e.message().c_str());
            }
        }

        delete thrRece;
        delete thrUpdate;
        delete runRece;
        delete runUpdate;

        mut_kcp.lock();
        ikcp_release(kcp);
        //delete kcp;//使用上面那个就不能delete了
        mut_kcp.unlock();

        delete kcpUser.socket;
        LogI("KCP.~Impl():%s 释放完毕!", kcpUser.name);
    }

    std::mutex mut_kcp;

    // 初始化 kcp对象，conv为一个表示会话编号的整数，和tcp的 conv一样，通信双
    // 方需保证 conv相同，相互的数据包才能够被认可，user是一个给回调函数的指针
    ikcpcb* kcp = nullptr;

    ///// <summary> UDP socket. </summary>
    //Poco::Net::DatagramSocket* socket = nullptr;

    KCPUser kcpUser;

    ReceRunnable* runRece = nullptr;
    UpdateRunnable* runUpdate = nullptr;

    Poco::Thread* thrRece = nullptr;
    Poco::Thread* thrUpdate = nullptr;

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
        try {
            kcpUser.name = name;

            Poco::Net::SocketAddress sa(Poco::Net::AddressFamily::IPv4, host, port);
            kcpUser.socket = new Poco::Net::DatagramSocket(sa); //使用一个端口开始一个接收
            kcpUser.socket->connect(Poco::Net::SocketAddress(Poco::Net::AddressFamily::IPv4, remoteHost, remotePort));

            //它可以设置是否是阻塞
            kcpUser.socket->setBlocking(true);
            //linux下close这个socket并没有引起接收的异常返回,所以只能加一个timeout了
            kcpUser.socket->setReceiveTimeout(Poco::Timespan::MILLISECONDS * 5000);
            kcp = ikcp_create(conv, &kcpUser);
            // 设置回调函数
            kcp->output = kcp_udp_output;

            ikcp_nodelay(kcp, 1, 10, 2, 1);
            kcp->rx_minrto = 10;
            kcp->fastresend = 1;

            runRece = new ReceRunnable(name, kcp, kcpUser.socket, &mut_kcp);
            runUpdate = new UpdateRunnable(name, kcp, &mut_kcp);

            thrRece = new Poco::Thread();
            thrUpdate = new Poco::Thread();

            thrRece->start(*runRece);
            thrUpdate->start(*runUpdate);
        }
        catch (const Poco::Exception& e) {
            LogE("KCP.Init():异常%s %s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("KCP.Init():异常:%s", e.what());
        }
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

KCP::KCP(const std::string& name,
         int conv,
         const std::string& host, int port,
         const std::string& remoteHost, int remotePort)
    : name(name), conv(conv), host(host), port(port), remoteHost(remoteHost), remotePort(remotePort)
{
    _impl = new Impl();
}

KCP::~KCP()
{
    delete _impl;
}

void KCP::Init()
{
    _impl->Init(name.c_str(), conv, host, port, remoteHost, remotePort);
}

int KCP::Receive(char* buffer, int len)
{
    if (_impl->kcp == nullptr) {
        LogE("KCP.Receive():%s 还没有启动,不能接收!", name.c_str());
        return 0;
    }
    //LogI("KCP.Receive():%s 执行接收!", name.c_str());
    _impl->mut_kcp.lock();
    int rece = ikcp_recv(_impl->kcp, buffer, len);
    if (rece == -3) {
        LogE("KCP.Receive():提供的buffer过小len=%d,peeksize=%d,KCP表示不能接收!", len, ikcp_peeksize(_impl->kcp));
    }
    _impl->mut_kcp.unlock();
    return rece;
}

int KCP::Send(const char* data, int len)
{
    if (_impl->kcp == nullptr) {
        LogE("KCP.Send()::%s 还没有连接,不能发送!", name.c_str());
        return 0;
    }
    LogI("KCP.Send()::%s 开始发送!原始数据长度为%d", name.c_str(), len);
    _impl->mut_kcp.lock();
    ikcp_send(_impl->kcp, data, len);
    //ikcp_update(_impl->kcp, clock() / CLOCKS_PER_SEC * 1000);
    ikcp_flush(_impl->kcp);
    _impl->mut_kcp.unlock();
    return 0;
}

} // namespace dxlib