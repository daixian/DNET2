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

namespace dxlib {

struct KCPUser
{
    const char* name;
    Poco::Net::DatagramSocket* socket = nullptr;
};

// KCP的下层协议输出函数，KCP需要发送数据时会调用它
// buf/len 表示缓存和长度
// user指针为 kcp对象创建时传入的值，用于区别多个 KCP对象
int udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    try {
        //kcp里有几个位置调用udp_output的时候没有传user参数进来,所以不能使用这个参数
        KCPUser* user = (KCPUser*)kcp->user;
        LogD("udp_output():%s 执行sendBytes()! len=%d", user->name, len);
        return user->socket->sendBytes(buf, len);
    }
    catch (const Poco::Exception& e) {
        LogE("udp_output():异常%s %s", e.what(), e.message().c_str());
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
                mut_kcp->lock();
                ikcp_input(kcp, buffer, n);
                ikcp_flush(kcp);
                mut_kcp->unlock();
            }
            catch (const Poco::Exception& e) {
                LogE("ReceRunnable.run():%s异常%s %s", name, e.what(), e.message().c_str());
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
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                mut_kcp->lock();
                //LogI("UpdateRunnable.run():调用ikcp_update()");
                // 以一定频率调用 ikcp_update来更新 kcp状态，并且传入当前时钟（毫秒单位）
                // 如 10ms调用一次，或用 ikcp_check确定下次调用 update的时间不必每次调用
                ikcp_update(kcp, clock() / CLOCKS_PER_SEC * 1000);
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
        LogI("KCP.~Impl():释放KCP对象%s", kcpUser.name);
        if (kcpUser.socket != nullptr) {
            kcpUser.socket->close();
        }

        if (thrServer != nullptr) {
            runServer->isRun = false;
            thrServer->join();
        }

        if (thrUpdate != nullptr) {
            runUpdate->isRun = false;
            thrUpdate->join();
        }

        delete runServer;
        delete runUpdate;
        delete thrServer;
        delete thrUpdate;

        mut_kcp.lock();

        ikcp_release(kcp);
        //delete kcp;//使用上面那个就不能delete了

        mut_kcp.unlock();
    }

    std::mutex mut_kcp;

    // 初始化 kcp对象，conv为一个表示会话编号的整数，和tcp的 conv一样，通信双
    // 方需保证 conv相同，相互的数据包才能够被认可，user是一个给回调函数的指针
    ikcpcb* kcp = nullptr;

    ///// <summary> UDP socket. </summary>
    //Poco::Net::DatagramSocket* socket = nullptr;

    KCPUser kcpUser;

    ReceRunnable* runServer = nullptr;
    UpdateRunnable* runUpdate = nullptr;

    Poco::Thread* thrServer = nullptr;
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
        kcpUser.name = name;

        //Poco::Net::SocketAddress sa(Poco::Net::IPAddress("127.0.0.1"), 24005);
        Poco::Net::SocketAddress sa(host, port);
        kcpUser.socket = new Poco::Net::DatagramSocket(sa); //使用一个端口开始一个接收
        kcpUser.socket->connect(Poco::Net::SocketAddress(remoteHost, remotePort));

        kcp = ikcp_create(conv, &kcpUser);
        // 设置回调函数
        kcp->output = udp_output;

        ikcp_nodelay(kcp, 1, 10, 2, 1);

        runServer = new ReceRunnable(name, kcp, kcpUser.socket, &mut_kcp);
        runUpdate = new UpdateRunnable(name, kcp, &mut_kcp);

        thrServer = new Poco::Thread();
        thrUpdate = new Poco::Thread();

        thrServer->start(*runServer);
        thrUpdate->start(*runUpdate);
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