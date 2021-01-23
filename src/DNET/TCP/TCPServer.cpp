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

    // 只能用于认证的Socket
    Poco::Net::ServerSocket* serverSocket = nullptr;

    // kcp使用的UPD的Socket
    Poco::Net::DatagramSocket* acceptUDPSocket = nullptr;

    // 接收用的buffer
    std::vector<char> receBuffUDP;

    // 用户连接进来了并且完成了握手协议的事件.
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
        Close();

        this->name = name;
        Poco::Net::SocketAddress sAddr(Poco::Net::AddressFamily::IPv4, host, port);
        try {
            serverSocket = new Poco::Net::ServerSocket(sAddr);
            serverSocket->setBlocking(false); //这里不能设置为false,因为Accept的时候只能Block,执行Accept函数的时候会直接异常.

            acceptUDPSocket = new Poco::Net::DatagramSocket(sAddr);
            acceptUDPSocket->setBlocking(false);
            receBuffUDP.resize(8 * 1024);

            clientManager.acceptUDPSocket = acceptUDPSocket;
        }
        catch (const Poco::Exception& e) {
            LogE("TCPServer.Start():创建Socket异常e=%s,%s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("TCPServer.Start():创建Socket异常e=%s", e.what());
        }

        LogI("TCPServer.Start():启动...");
    }

    void Close()
    {
        if (!IsStarted()) //如果还没启动那么直接返回
            return;

        //关闭所有客户端
        clientManager.Clear();

        if (acceptUDPSocket != nullptr) {
            try {
                acceptUDPSocket->close();
                delete acceptUDPSocket;

                serverSocket->close();
                delete serverSocket;
            }
            catch (const Poco::Exception& e) {
                LogE("TCPServer.Close():关闭Socket异常e=%s,%s", e.what(), e.message().c_str());
            }
            catch (const std::exception& e) {
                LogE("TCPServer.Close():关闭Socket异常e=%s", e.what());
            }
            serverSocket = nullptr;
            acceptUDPSocket = nullptr;
        }
        TCPEventClose evArgs = TCPEventClose();
        eventClose.notify(this, evArgs);
    }

    bool IsStarted()
    {
        if (serverSocket != nullptr) {
            return true;
        }
        else {
            return false;
        }
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

    void SocketAccept()
    {
        if (serverSocket == nullptr) {
            return;
        }

        try {
            // 注意这里是异步poll调用了
            if (serverSocket->poll(Poco::Timespan(0), Poco::Net::Socket::SELECT_READ)) {
                Poco::Net::StreamSocket streamSocket = serverSocket->acceptConnection();
                streamSocket.setBlocking(false);
                TCPClient* client = clientManager.AddClient(streamSocket); //添加这个用户
                LogI("TCPServer.SocketAccept():新连接来了一个客户端,临时tcpid=%d", client->TcpID());

                //eventAccept->notify(this, TCPEventAccept(tcpID)); //发出这个事件消息
            }
            else {
            }
        }
        catch (const Poco::Exception& e) {
            LogE("TCPServer.SocketAccept():异常e=%s,%s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("TCPServer.SocketAccept():异常e=%s", e.what());
        }
    }

    template <typename T>
    int Receive(std::map<int, std::vector<T>>& msgs)
    {
        // 执行tcp socket的accept
        SocketAccept();

        msgs.clear();

        for (auto itr = clientManager.mClients.begin(); itr != clientManager.mClients.end();) {
            std::vector<T> clientMsgs;
            if (itr->second->Receive(clientMsgs) > 0) {
                msgs[itr->first] = clientMsgs;
            }

            //清理出错的客户端
            if (itr->second->isError() && itr->second->TimeFormErrorToNow() > 100) {
                LogI("TCPServer.Receive():一个客户端 id=%d 已经网络错误,删除它!", itr->second->TcpID());
                TCPEventRemoteClose evArgs = TCPEventRemoteClose(itr->second->TcpID());
                eventRemoteClose.notify(this, evArgs); //发出事件

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
        if (acceptUDPSocket == nullptr) {
            return -1;
        }
        msgs.clear();

        // 当前接收长度
        int receLen = 0;

        try { //socket尝试接收
            Poco::Net::SocketAddress remote(Poco::Net::AddressFamily::IPv4);
            receLen = acceptUDPSocket->receiveFrom(receBuffUDP.data(), (int)receBuffUDP.size(), remote);
        }
        catch (const Poco::Exception& e) {
            LogE("TCPServer.KCPReceive():异常e=%s,%s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("TCPServer.KCPReceive():异常e=%s", e.what());
        }

        for (auto itr = clientManager.mAcceptClients.begin(); itr != clientManager.mAcceptClients.end();) {
            std::vector<std::string> clientMsgs;
            int res = itr->second->KCPReceive(receBuffUDP.data(), receLen, clientMsgs);
            if (res < 0) {
                //-1或者未初始化等其他值是不匹配的信道
                itr++;
                continue;
            }
            else {
                //==0或者大于0是匹配的信道,那么不需要再往下遍历了
                if (res > 0) {
                    msgs[itr->second->TcpID()] = clientMsgs;
                }
                break;
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
    return (int)_impl->clientManager.mAcceptClients.size();
}

std::map<int, TCPClient*> TCPServer::GetRemotes()
{
    std::map<int, TCPClient*> map;
    for (auto& kvp : _impl->clientManager.mAcceptClients) {
        map[kvp.second->TcpID()] = kvp.second;
    }
    return map;
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
