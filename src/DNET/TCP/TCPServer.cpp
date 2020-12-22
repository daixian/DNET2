﻿#include "TCPServer.h"

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
#include "./Protocol/FastPacket.h"

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
    TCPAcceptRunnable(Poco::Net::SocketAddress& address,
                      Poco::BasicEvent<TCPEventAccept>* eventAccept,
                      ClientManager* clientManager)
        : address(address), eventAccept(eventAccept), clientManager(clientManager)
    {
    }

    virtual ~TCPAcceptRunnable()
    {
    }

    virtual void run()
    {
        int port = address.port();
        socket = new Poco::Net::ServerSocket(port);

        socket->setBlocking(false);
        LogI("TCPServer.Start():TCPServer开始监听...");
        Poco::Timespan span(250000);
        while (isRun) {
            try {
                if (socket->poll(span, Poco::Net::Socket::SELECT_READ)) {
                    Poco::Net::StreamSocket streamSocket = socket->acceptConnection(); //这个函数是阻塞的
                    streamSocket.setBlocking(false);
                    int tcpID = clientManager->AddClient(streamSocket); //添加这个用户
                    TCPEventAccept msg(tcpID);
                    LogI("TCPAcceptRunnable.run():新连接来了一个客户端%d", tcpID);
                    eventAccept->notify(this, msg); //发出这个消息
                }
                else {
                    isStarted = true; //标记已经启动了
                }
            }
            catch (const Poco::Exception& e) {
                LogE("TCPAcceptRunnable.run():异常e=%s,%s", e.what(), e.message().c_str());
            }
            catch (const std::exception& e) {
                LogE("TCPAcceptRunnable.run():异常e=%s", e.what());
            }
        }

        try {
            socket->close();
        }
        catch (const std::exception&) {
        }
        delete socket;
    }

    // 是否运行
    std::atomic_bool isRun{true};

    bool isStarted = false;

    Poco::Net::SocketAddress address;

    Poco::Net::ServerSocket* socket = nullptr;

  private:
    Poco::BasicEvent<TCPEventAccept>* eventAccept;

    ClientManager* clientManager;
};

class TCPServer::Impl
{
  public:
    Impl() {}
    ~Impl()
    {
        Close();
    }

    // 服务器的名字.
    std::string name;

    Poco::Thread* acceptThread = nullptr;

    TCPAcceptRunnable* acceptRunnable = nullptr;

    // 用户连接进来了的事件.
    Poco::BasicEvent<TCPEventAccept> eventAccept;

    // 客户端记录.
    ClientManager clientManager;

    // TCP协议
    FastPacket packet;

    void Start(const std::string& name, const std::string& host, int port)
    {
        this->name = name;
        Poco::Net::SocketAddress sAddr(Poco::Net::AddressFamily::IPv4, host, port);

        //socket = new Poco::Net::ServerSocket(sAddr);
        // socket->setBlocking(true); //这里不能设置为false,因为Accept的时候只能Block,执行Accept函数的时候会直接异常.

        acceptRunnable = new TCPAcceptRunnable(sAddr, &eventAccept, &clientManager);
        acceptThread = new Poco::Thread("AcceptRunnableThread");
        acceptThread->start(*acceptRunnable);
        LogI("TCPServer.Start():Accept线程启动.");
    }

    void Close()
    {
        if (acceptRunnable != nullptr) {
            acceptRunnable->isRun = false;
            //acceptRunnable->socket->close();
        }

        if (acceptThread != nullptr) {
            acceptThread->join();
            delete acceptThread;
            acceptThread = nullptr;
        }
        if (acceptRunnable != nullptr) {
            delete acceptRunnable;
            acceptRunnable = nullptr;
        }
    }

    bool IsStarted()
    {
        if (acceptRunnable == nullptr)
            return false;
        else
            return acceptRunnable->isStarted; //这个是监听已经启动
    }

    int Send(int tcpID, const char* data, int len)
    {
        TCPClient* client = clientManager.GetClient(tcpID);
        if (client == nullptr) {
            return -1;
        }

        return client->Send(data, len); //发送打包后的数据
    }

    //客户端接收查询
    int Available(int tcpID)
    {
        TCPClient* client = clientManager.GetClient(tcpID);
        if (client == nullptr) { //客户端不存在
            return -1;
        }

        return client->Available();
    }

    int WaitAvailable(int tcpID, int waitCount)
    {
        TCPClient* client = clientManager.GetClient(tcpID);
        if (client == nullptr) { //客户端不存在
            return -1;
        }

        return client->WaitAvailable(waitCount);
    }

    int Receive(std::map<int, std::vector<std::vector<char>>>& msgs)
    {
        msgs.clear();

        for (auto& kvp : clientManager.mClients) {
            std::vector<std::vector<char>> clientMsgs;
            if (kvp.second.Receive(clientMsgs) > 0) {
                msgs[kvp.first] = clientMsgs;
            }
        }
        return (int)msgs.size();
    }

    int Receive(std::map<int, std::vector<std::string>>& msgs)
    {
        msgs.clear();

        for (auto& kvp : clientManager.mClients) {
            std::vector<std::string> clientMsgs;
            if (kvp.second.Receive(clientMsgs) > 0) {
                msgs[kvp.first] = clientMsgs;
            }
        }
        return (int)msgs.size();
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

void TCPServer::Close()
{
    _impl->Close();
}

bool TCPServer::IsStarted()
{
    return _impl->IsStarted();
}

int TCPServer::Send(int tcpID, const char* data, int len)
{
    return _impl->Send(tcpID, data, len);
}

Poco::BasicEvent<TCPEventAccept>& TCPServer::EventAccept()
{
    return _impl->eventAccept;
}

int TCPServer::Available(int tcpID)
{
    return _impl->Available(tcpID);
}

int TCPServer::Receive(std::map<int, std::vector<std::vector<char>>>& msgs)
{
    return _impl->Receive(msgs);
}

int TCPServer::Receive(std::map<int, std::vector<std::string>>& msgs)
{
    return _impl->Receive(msgs);
}

int TCPServer::WaitAvailable(int tcpID, int waitCount)
{
    return _impl->WaitAvailable(tcpID, waitCount);
}
} // namespace dxlib