#include "TCPServer.h"

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

#include <thread>
#include <mutex>
#include <regex>

#include "dlog/dlog.h"

#include "ClientManager.h"

namespace dxlib {

/**
 * 始终阻塞的接收认证.
 *
 * @author daixian
 * @date 2020/12/21
 */
class TCPAcceptRunnable : public Poco::Runnable
{
  public:
    TCPAcceptRunnable(Poco::Net::ServerSocket* socket,
                      Poco::BasicEvent<TCPEventAccept>* eventAccept,
                      ClientManager* clientManager)
        : socket(socket), eventAccept(eventAccept), clientManager(clientManager)
    {
    }

    virtual ~TCPAcceptRunnable()
    {
    }

    virtual void run()
    {
        while (isRun) {
            Poco::Net::StreamSocket streamSocket = socket->acceptConnection(); //这个函数是阻塞的
            int tcpID = clientManager->AddClient(streamSocket);
            TCPEventAccept msg(tcpID);
            eventAccept->notify(this, msg); //发出这个消息
        }
    }

    // 是否运行
    std::atomic_bool isRun{true};

  private:
    Poco::Net::ServerSocket* socket = nullptr;

    Poco::BasicEvent<TCPEventAccept>* eventAccept;

    ClientManager* clientManager;
};

class TCPServer::Impl
{
  public:
    Impl() {}
    ~Impl()
    {
    }

    // 服务器的名字.
    std::string name;

    // 服务器socket.
    Poco::Net::ServerSocket* socket = nullptr;

    Poco::Thread* acceptThread = nullptr;
    TCPAcceptRunnable* acceptRunnable = nullptr;

    // 用户连接进来了的事件.
    Poco::BasicEvent<TCPEventAccept> eventAccept;

    // 客户端记录.
    ClientManager clientManager;

    void Start(const std::string& name, const std::string& host, int port)
    {
        if (socket != nullptr) {
            socket->close();
            delete socket;
        }

        this->name = name;
        Poco::Net::SocketAddress sAddr(Poco::Net::AddressFamily::IPv4, host, port);

        socket = new Poco::Net::ServerSocket(sAddr);
        socket->setBlocking(false);
        LogI("TCPServer.Start():TCPServer开始监听...  %s:%d", host.c_str(), port);

        acceptRunnable = new TCPAcceptRunnable(socket, &eventAccept, &clientManager);
        acceptThread = new Poco::Thread("AcceptRunnableThread");
        acceptThread->start(*acceptRunnable);
        LogI("TCPServer.Start():Accept线程启动.");
    }
};

TCPServer::TCPServer(const std::string& name, const std::string& host, int port)
    : name(name), host(host), port(port)
{
    _impl = new Impl();
}

TCPServer::~TCPServer()
{
    delete _impl;
}

void TCPServer::Start()
{
    _impl->Start(name, host, port);
}

} // namespace dxlib