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
#include "Poco/UUIDGenerator.h"

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
        try {
            socket = new Poco::Net::ServerSocket(address);
            socket->setBlocking(false);

            LogI("TCPServer.Start():TCPServer开始监听(%s:%d)...", address.host().toString().c_str(), address.port());
        }
        catch (const Poco::Exception& e) {
            LogE("TCPAcceptRunnable.构造():异常e=%s,%s", e.what(), e.message().c_str());
            isRun = false;
        }
        catch (const std::exception& e) {
            LogE("TCPAcceptRunnable.构造():异常e=%s", e.what());
            isRun = false;
        }
    }

    virtual ~TCPAcceptRunnable()
    {
    }

    virtual void run()
    {

        Poco::Timespan span(250000);
        while (isRun) {
            try {
                // 注意这里是异步poll调用了
                if (socket->poll(span, Poco::Net::Socket::SELECT_READ)) {
                    Poco::Net::StreamSocket streamSocket = socket->acceptConnection(); //这个函数是阻塞的
                    streamSocket.setBlocking(false);
                    TCPClient* client = clientManager->AddClient(streamSocket); //添加这个用户
                    LogI("TCPAcceptRunnable.run():新连接来了一个客户端,临时tcpid=%d", client->TcpID());

                    //eventAccept->notify(this, TCPEventAccept(tcpID)); //发出这个事件消息
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

        if (socket != nullptr) {
            try {
                socket->close();
            }
            catch (const Poco::Exception& e) {
                LogE("TCPAcceptRunnable.run():关闭TCPServer Socket异常e=%s,%s", e.what(), e.message().c_str());
            }
            catch (const std::exception& e) {
                LogE("TCPAcceptRunnable.run():关闭TCPServer Socket异常e=%s", e.what());
            }
            delete socket;
        }
    }

    // 是否运行
    std::atomic_bool isRun{true};

    bool isStarted = false;

    Poco::Net::SocketAddress address;

    Poco::Net::ServerSocket* socket = nullptr;

  private:
    Poco::BasicEvent<TCPEventAccept>* eventAccept = nullptr;

    ClientManager* clientManager = nullptr;
};

class TCPServer::Impl
{
  public:
    Impl()
    {
        //随机生成一个uuid
        Poco::UUIDGenerator uuidGen;
        uuid = uuidGen.createRandom().toString();

        clientManager.eventAccept = &eventAccept;
    }
    ~Impl()
    {
        Close();
    }

    // 服务器的名字.
    std::string name;

    // 这个客户端的唯一标识符
    std::string uuid;

    // 认证线程
    Poco::Thread* acceptThread = nullptr;

    // 认证线程执行逻辑
    TCPAcceptRunnable* acceptRunnable = nullptr;

    // 用户连接进来了的事件.
    Poco::BasicEvent<TCPEventAccept> eventAccept;

    // 对象自身关闭的事件
    Poco::BasicEvent<TCPEventClose> eventClose;

    // 远程端关闭的事件
    Poco::BasicEvent<TCPEventRemoteClose> eventRemoteClose;

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
        clientManager.acceptUDPSocket = new Poco::Net::DatagramSocket(sAddr);
        clientManager.acceptUDPSocket->setBlocking(false);

        acceptRunnable = new TCPAcceptRunnable(sAddr, &eventAccept, &clientManager);
        acceptThread = new Poco::Thread("AcceptRunnableThread");
        acceptThread->start(*acceptRunnable);
        LogI("TCPServer.Start():Accept线程启动.");
    }

    void Close()
    {
        if (!IsStarted()) //如果还没启动那么直接返回
            return;

        //关闭所有客户端
        clientManager.Clear();

        if (clientManager.acceptUDPSocket != nullptr) {
            try {
                clientManager.acceptUDPSocket->close();
                delete clientManager.acceptUDPSocket;
            }
            catch (const Poco::Exception& e) {
                LogE("TCPAcceptRunnable.run():关闭UDPSocket异常e=%s,%s", e.what(), e.message().c_str());
            }
            catch (const std::exception& e) {
                LogE("TCPAcceptRunnable.run():关闭UDPSocket异常e=%s", e.what());
            }
            clientManager.acceptUDPSocket = nullptr;
        }

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

        eventClose.notify(this, TCPEventClose());
    }

    bool IsStarted()
    {
        if (acceptRunnable == nullptr)
            return false;
        else
            return acceptRunnable->isStarted; //这个是监听已经启动
    }

    int Send(int tcpID, const char* data, size_t len)
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

    int WaitAccepted(int tcpID, int waitCount)
    {
        TCPClient* client = clientManager.GetClient(tcpID);
        if (client == nullptr) { //客户端不存在
            return -1;
        }

        return client->WaitAccepted(waitCount);
    }

    int Receive(std::map<int, std::vector<std::vector<char>>>& msgs)
    {
        msgs.clear();

        for (auto itr = clientManager.mClients.begin(); itr != clientManager.mClients.end();) {
            std::vector<std::vector<char>> clientMsgs;
            if (itr->second->Receive(clientMsgs) > 0) {
                msgs[itr->first] = clientMsgs;
            }

            //清理出错的客户端
            if (itr->second->isError()) {
                LogI("TCPServer.Receive():一个客户端 id=%d 已经网络错误,删除它!", itr->second->TcpID());
                eventRemoteClose.notify(this, TCPEventRemoteClose(itr->second->TcpID())); //发出事件

                clientManager.mAcceptClients.erase(itr->second->AcceptData()->uuidC);
                delete itr->second;
                itr = clientManager.mClients.erase(itr);
            }
            else {
                itr++;
            }
        }

        return (int)msgs.size();
    }

    int Receive(std::map<int, std::vector<std::string>>& msgs)
    {
        msgs.clear();

        for (auto itr = clientManager.mClients.begin(); itr != clientManager.mClients.end();) {
            std::vector<std::string> clientMsgs;
            if (itr->second->Receive(clientMsgs) > 0) {
                msgs[itr->first] = clientMsgs;
            }

            //清理出错的客户端
            if (itr->second->isError()) {
                LogI("TCPServer.Receive():一个客户端 id=%d 已经网络错误,删除它!", itr->second->TcpID());
                eventRemoteClose.notify(this, TCPEventRemoteClose(itr->second->TcpID())); //发出事件

                clientManager.mAcceptClients.erase(itr->second->AcceptData()->uuidC);
                delete itr->second;
                itr = clientManager.mClients.erase(itr);
            }
            else {
                itr++;
            }
        }
        return (int)msgs.size();
    }

    int KCPSend(int tcpID, const char* data, size_t len)
    {
        TCPClient* client = clientManager.GetClient(tcpID);
        if (client == nullptr) {
            return -1;
        }

        return client->KCPSend(data, len); //发送打包后的数据
    }

    int KCPReceive(std::map<int, std::vector<std::string>>& msgs)
    {
        msgs.clear();

        clientManager.KCPSocketReceive();

        for (auto itr = clientManager.mClients.begin(); itr != clientManager.mClients.end();) {
            std::vector<std::string> clientMsgs;
            int res = itr->second->KCPReceive(clientMsgs);
            //if (res == -1) {
            //    continue;
            //}
            if (res > 0) {
                msgs[itr->first] = clientMsgs;
            }

            itr++;
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

std::string TCPServer::UUID()
{
    return _impl->uuid;
}

std::string TCPServer::SetUUID(const std::string& uuid)
{
    _impl->uuid = uuid;
    return _impl->uuid;
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

void TCPServer::WaitStarted()
{
    while (!IsStarted()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int TCPServer::Send(int tcpID, const char* data, size_t len)
{
    return _impl->Send(tcpID, data, len);
}

Poco::BasicEvent<TCPEventAccept>& TCPServer::EventAccept()
{
    return _impl->eventAccept;
}

Poco::BasicEvent<TCPEventClose>& TCPServer::EventClose()
{
    return _impl->eventClose;
}

Poco::BasicEvent<TCPEventRemoteClose>& TCPServer::EventRemoteClose()
{
    return _impl->eventRemoteClose;
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

int TCPServer::WaitAccepted(int tcpID, int waitCount)
{
    return _impl->WaitAccepted(tcpID, waitCount);
}

void* TCPServer::GetClientSocket(int tcpID)
{
    TCPClient* client = _impl->clientManager.GetClient(tcpID);
    if (client != nullptr) {
        return client->Socket();
    }
    return nullptr;
}
int TCPServer::RemoteCount()
{
    return _impl->clientManager.mClients.size();
}

std::map<int, TCPClient*> TCPServer::GetRemotes()
{
    return _impl->clientManager.mClients;
}

int TCPServer::KCPSend(int tcpID, const char* data, size_t len)
{
    return _impl->KCPSend(tcpID, data, len);
}

int TCPServer::KCPReceive(std::map<int, std::vector<std::string>>& msgs)
{
    return _impl->KCPReceive(msgs);
}
} // namespace dxlib