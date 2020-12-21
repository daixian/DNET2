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
#include "clock.hpp"
#include <regex>
#include "Accept.h"

namespace dxlib {

/**
 * A TCP accept runnable.
 *
 * @author daixian
 * @date 2020/12/21
 */
class TCPAcceptRunnable : public Poco::Runnable
{
  public:
    TCPAcceptRunnable(Poco::Net::ServerSocket* socket) : socket(socket)
    {
    }

    virtual ~TCPAcceptRunnable()
    {
    }

    virtual void run()
    {
        while (isRun) {
            Poco::Net::StreamSocket streamSocket = socket->acceptConnection(); //这个函数是阻塞的
        }
    }

    // 是否运行
    std::atomic_bool isRun{true};

  private:
    Poco::Net::ServerSocket* socket = nullptr;
};

class TCPServer::Impl
{
  public:
    Impl() {}
    ~Impl()
    {
    }

    Poco::Net::ServerSocket* socket = nullptr;

    void Start()
    {
        socket->close();
        delete socket;
    }
};

TCPServer::TCPServer(const std::string& name,
                     const std::string& host, int port)
{
    _impl = new Impl();
}

TCPServer::~TCPServer()
{
    delete _impl;
}

void TCPServer::Start()
{
}

} // namespace dxlib