#include "TCPClient.h"
#include "xuexuejson/Serialize.hpp"

#include "Poco/Net/Socket.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/NetException.h"
#include "Poco/Timespan.h"
#include "Poco/Net/SocketStream.h"
#include "Poco/UUIDGenerator.h"

#include "ClientManager.h"
#include "Protocol/FastPacket.h"
#include "dlog/dlog.h"

#include <thread>

#include "KCPClient.h"

#define XUEXUE_TCP_CLIENT_BUFFER_SIZE 8 * 1024

namespace dxlib {

class TCPClient::Impl
{
  public:
    Impl()
    {
        receBuff.resize(XUEXUE_TCP_CLIENT_BUFFER_SIZE);

        //随机生成一个uuid
        Poco::UUIDGenerator uuidGen;
        uuid = uuidGen.createRandom().toString();

        kcpClient = std::shared_ptr<KCPClient>(new KCPClient());
    }

    ~Impl()
    {
        //析构的时候尝试关闭
        Close();

        delete acceptData;
        acceptData = nullptr;
    }

    // 名字.
    std::string name = "TCPClient";

    // 设置这个是服务器，当这个client是服务器端的时候会有使用它
    ClientManager* clientManager = nullptr;

    // 一个tcp的ID.
    int tcpID = -1;

    // 这个客户端的唯一标识符
    std::string uuid;

    // 客户端的socket
    Poco::Net::StreamSocket socket;

    // 客户端的udp的socket,如果自己是客户端的时候使用本地的成员来管理.如果是server里的那么使用clientManager里的.
    Poco::Net::DatagramSocket* udpSocket = nullptr;

    // TCP通信协议
    FastPacket packet;

    // 是否已经连接了
    std::atomic_bool isConnected{false};

    // 接收用的buffer
    std::vector<char> receBuff;

    // 接收用的buffer
    std::vector<char> receBuffUDP;

    // 客户端和服务器端认证的数据
    Accept* acceptData = nullptr;

    // 是否已经网络错误了
    bool isError = false;

    // 网络错误的计时
    clock_t errorTime;

    // 所有接受到的消息的总条数
    int receMsgCount = 0;

    // 所有接受到的消息的总条数
    int sendMsgCount = 0;

    // 用户连接成功的事件.
    Poco::BasicEvent<TCPEventAccept> eventAccept;

    // 客户端关闭的事件
    Poco::BasicEvent<TCPEventClose> eventClose;

    // 远程端关闭的事件
    Poco::BasicEvent<TCPEventRemoteClose> eventRemoteClose;

    // 这个TCP可以附加绑定一个kcp
    std::shared_ptr<KCPClient> kcpClient{nullptr};

    // 是否是服务端的client
    bool IsInServer()
    {
        if (clientManager == nullptr)
            return false;
        else
            return true;
    }

    /**
     * Connects
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @param  host The host.
     * @param  port The port.
     *
     * @returns An int.
     */
    int Connect(const std::string& host, int port)
    {
        if (IsInServer()) {
            LogE("TCPClient.Connect():服务器端的Client不应该调用这个函数!");
            return -1;
        }

        using Poco::Timespan;

        Close(); //先试试无脑关闭

        try {
            LogI("TCPClient.Connect():{%s}尝试连接远程%s:%d...", uuid.c_str(), host.c_str(), port);
            Poco::Net::SocketAddress sa(Poco::Net::SocketAddress::Family::IPv4, host, port);

            socket.connect(sa); //这个是阻塞的连接
        }
        catch (Poco::Net::ConnectionRefusedException& e) {
            LogE("TCPClient.Connect():连接拒绝!");
            return -1;
        }
        catch (Poco::Net::NetException& e) {
            LogE("TCPClient.Connect():NetException:%s,%s", e.what(), e.message().c_str());
            return -1;
        }
        catch (Poco::TimeoutException& e) {
            LogE("TCPClient.Connect():TimeoutException:%s,%s", e.what(), e.message().c_str());
            return -1;
        }
        catch (std::exception e) {
            LogE("TCPClient.Connect():异常:%s", e.what());
            return -1;
        }
        LogI("TCPClient.Connect():连接成功!");
        isConnected = true;

        //setopt timeout
        Timespan timeout3(5000000);
        socket.setReceiveTimeout(timeout3); //retn void
        Timespan timeout4(5000000);
        socket.setSendTimeout(timeout4); //retn void

        //setopt bufsize
        socket.setReceiveBufferSize(XUEXUE_TCP_CLIENT_BUFFER_SIZE); //buff大小
        socket.setSendBufferSize(XUEXUE_TCP_CLIENT_BUFFER_SIZE);    //buff大小

        socket.setNoDelay(true);
        socket.setBlocking(false);

        SendAccept();

        return 0;
    }

    /**
     * 关闭.
     *
     * @author daixian
     * @date 2020/12/22
     */
    void Close()
    {
        if (isConnected) {
            try {
                socket.close();
            }
            catch (const std::exception&) {
            }

            if (udpSocket != nullptr) {
                try {
                    udpSocket->close();
                }
                catch (const std::exception&) {
                }
                delete udpSocket;
                udpSocket = nullptr;
            }

            isConnected = false;

            eventClose.notify(this, TCPEventClose());
        }
    }

    /**
     * Send this message
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @param  data The data.
     * @param  len  The length.
     *
     * @returns An int.
     */
    int Send(const char* data, size_t len, int type)
    {
        if (!isConnected) {
            return -1;
        }
        //数据打包
        std::vector<char> package;
        packet.Pack(data, (int)len, package, type);
        sendMsgCount++; //计数

        int sendCount = 0;
        for (size_t i = 0; i < 10; i++) {
            int res = socket.sendBytes(package.data() + sendCount, (int)package.size() - sendCount); //发送打包后的数据
            sendCount += res;
            if (sendCount == (int)package.size()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); //如果不能完整发送那么就休息100ms
        }

        return sendCount;
    }

    //发送认证
    int SendAccept()
    {
        if (acceptData != nullptr) {
            delete acceptData;
        }

        acceptData = new Accept();
        std::string acceptStr = acceptData->CreateAcceptString(uuid, name); //创建一个认证的自字符串发给服务器
        return Send(acceptStr.c_str(), acceptStr.size(), 0);
    }

    int ProcCMD(std::vector<std::string>& msgs, std::vector<int>& types)
    {
        //遍历所有收到的消息
        for (size_t msgIndex = 0; msgIndex < msgs.size();) {
            if (types[msgIndex] == 0) {
                //命令消息:0号命令
                std::string& acceptStr = msgs[msgIndex];
                ProcAccept(acceptStr);

                // 移除这个命令消息
                msgs.erase(msgs.begin() + msgIndex);
                types.erase(types.begin() + msgIndex);
            }
            else {
                msgIndex++;
            }
        }

        return (int)msgs.size();
    }

    // 使用所有接受到的消息然后过滤其中的CMD消息
    int ProcCMD(std::vector<std::vector<char>>& msgs, std::vector<int>& types)
    {
        //遍历所有收到的消息
        for (size_t msgIndex = 0; msgIndex < msgs.size();) {
            if (types[msgIndex] == 0) {
                //命令消息:0号命令
                std::string& acceptStr = std::string(msgs[msgIndex].data(), msgs[msgIndex].size());
                ProcAccept(acceptStr);

                // 移除这个命令消息
                msgs.erase(msgs.begin() + msgIndex);
                types.erase(types.begin() + msgIndex);
            }
            else {
                msgIndex++;
            }
        }

        return (int)msgs.size();
    }

    // 处理accept字符串
    void ProcAccept(std::string& acceptStr)
    {
        if (IsInServer()) {
            //自己是服务器端
            acceptData = new Accept();

            if (!acceptData->VerifyAccept(acceptStr)) {
                // replyStr为空,非法的认证信息
                LogE("TCPClient.ProcAccept():非法的认证信息!");
            }
            else {
                // 重新指向分配过的tcpID,这里clientManager会发出事件
                clientManager->RegisterClientWithUUID(acceptData->uuidC, tcpID); //这个函数会重新分配tcpID
                std::string replyStr = acceptData->ReplyAcceptString(acceptStr, uuid, name, tcpID);
                poco_assert(!replyStr.empty());
                // replyStr有内容,有效的认证信息,自己是服务器端
                Send(replyStr.c_str(), replyStr.size(), 0);

                LogI("TCPClient.ProcAccept():添加了一个新客户端%s,Addr=%s:%d,分配conv=%d",
                     acceptData->uuidC.c_str(),
                     socket.peerAddress().host().toString().c_str(),
                     socket.peerAddress().port(),
                     tcpID);

                //如果id不等那么说明没有继承原来的kcp
                if (kcpClient->Conv() != tcpID) {
                    kcpClient->Create(tcpID);
                }

                poco_assert(clientManager != nullptr);
                kcpClient->isServer = true;
                kcpClient->Bind(clientManager->acceptUDPSocket, socket.peerAddress());
            }
        }
        else {
            //自己是客户端
            if (acceptData->VerifyReplyAccept(acceptStr.data())) {
                tcpID = acceptData->conv;

                //服务器返回的Accept验证成功
                poco_assert(acceptData->conv >= 0);

                //如果id不等那么说明没有继承原来的kcp
                if (kcpClient->Conv() != tcpID) {
                    kcpClient->Create(tcpID);
                }
                InitUDPSocket();
                kcpClient->isServer = false;
                kcpClient->Bind(udpSocket, socket.peerAddress());
                eventAccept.notify(this, TCPEventAccept(tcpID, acceptData));
            }
        }
    }

    //根据当前TCPSocket的本地端口来绑定UDP.
    void InitUDPSocket()
    {
        if (udpSocket != nullptr) {
            LogE("TCPClient.InitUDPSocket():不太应该不为null...");
            udpSocket->close();
            delete udpSocket;
        }

        receBuffUDP.resize(XUEXUE_TCP_CLIENT_BUFFER_SIZE);
        try {
            udpSocket = new Poco::Net::DatagramSocket(socket.address());
            udpSocket->setBlocking(false);
        }
        catch (const Poco::Exception& e) {
            LogE("TCPClient.InitUDPSocket():异常e=%s,%s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("TCPClient.InitUDPSocket():异常e=%s", e.what());
        }
    }

    int Available()
    {
        if (!isConnected) {
            return -1;
        }

        return socket.available();
    }

    /**
     * Receives the given msgs
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @param [out] msgs  The msgs.
     * @param [out] types 消息的类型.
     *
     * @returns 接收到的数据条数.
     */
    int Receive(std::vector<std::vector<char>>& msgs, std::vector<int>& types)
    {
        msgs.clear();
        types.clear();

        if (!isConnected) {
            return -1;
        }

        if (socket.poll(Poco::Timespan(0), Poco::Net::Socket::SelectMode::SELECT_ERROR)) {
            LogE("TCPClient.Receive():poll到了异常!");
            isError = true;
            errorTime = clock();
            eventRemoteClose.notify(this, TCPEventRemoteClose(tcpID));
            //Close();
            //return -1; //网络出错那么就不接收算了
        }

        while (true) {
            if (socket.available() > 0) {
                int res = socket.receiveBytes(receBuff.data(), (int)receBuff.size());
                if (res <= 0) {
                    break;
                }
                receMsgCount += packet.Unpack(receBuff.data(), res, msgs, types);
            }
            else {
                break;
            }
        }

        return (int)msgs.size();
    }

    /**
     * Receives the given msgs
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @param [out] msgs  The msgs.
     * @param [out] types 消息的类型.
     *
     * @returns 接收到的数据条数.
     */
    int Receive(std::vector<std::string>& msgs, std::vector<int>& types)
    {
        msgs.clear();
        types.clear();

        if (!isConnected) {
            return -1;
        }

        if (socket.poll(Poco::Timespan(0), Poco::Net::Socket::SelectMode::SELECT_ERROR)) {
            LogE("TCPClient.Receive():poll到了异常!");
            isError = true;
            errorTime = clock();
            eventRemoteClose.notify(this, TCPEventRemoteClose(tcpID));
            //Close();
            //return -1; //网络出错那么就不接收算了
        }

        while (true) {
            if (socket.available() > 0) {
                int res = socket.receiveBytes(receBuff.data(), (int)receBuff.size());
                packet.Unpack(receBuff.data(), res, msgs, types);
                if (res <= 0) {
                    break;
                }
            }
            else {
                break;
            }
        }

        return (int)msgs.size();
    }

    int KCPReceive(std::vector<std::string>& msgs)
    {
        if (udpSocket != nullptr) {
            //这是本地的client
            try {
                //socket尝试接收
                Poco::Net::SocketAddress remote(Poco::Net::AddressFamily::IPv4);
                int n = udpSocket->receiveFrom(receBuffUDP.data(), (int)receBuffUDP.size(), remote);
                return kcpClient->Receive(receBuffUDP.data(), n, msgs);
            }
            catch (const Poco::Exception& e) {
                LogE("TCPClient.KCPReceive():异常e=%s,%s", e.what(), e.message().c_str());
            }
            catch (const std::exception& e) {
                LogE("TCPClient.KCPReceive():异常e=%s", e.what());
            }
        }
        else {
            //这是server中的client
            if (clientManager == nullptr) {
                LogE("TCPClient.KCPReceive():clientManager==null");
                return -1;
            }
            //实际上此时如果是TCPServer那么已经由TCPServer的函数中调用了一次Socket接收,所以这里直接送数据.
            return kcpClient->Receive(clientManager->receBuffUDP.data(), clientManager->receLen, msgs);
        }
    }
};

TCPClient::TCPClient(const std::string& name)
{
    _impl = std::shared_ptr<Impl>(new Impl());
    _impl->name = name;
}

TCPClient::~TCPClient()
{
}

void TCPClient::CreateWithServer(int tcpID, void* socket, void* clientManager,
                                 TCPClient& obj)
{
    using Poco::Timespan;

    //TCPClient obj;
    obj._impl->tcpID = tcpID; //这里有访问权限
    obj._impl->clientManager = (ClientManager*)clientManager;
    obj._impl->socket = *(Poco::Net::StreamSocket*)socket; //拷贝一次

    obj._impl->isConnected = true;

    //setopt timeout
    Timespan timeout3(5000000);
    obj._impl->socket.setReceiveTimeout(timeout3); //retn void
    Timespan timeout4(5000000);
    obj._impl->socket.setSendTimeout(timeout4); //retn void

    //setopt bufsize
    obj._impl->socket.setReceiveBufferSize(XUEXUE_TCP_CLIENT_BUFFER_SIZE); //buff大小
    obj._impl->socket.setSendBufferSize(XUEXUE_TCP_CLIENT_BUFFER_SIZE);    //buff大小

    obj._impl->socket.setNoDelay(true);
    obj._impl->socket.setBlocking(false);
    return;
}

int TCPClient::TcpID()
{
    return _impl->tcpID;
}

void TCPClient::SetTcpID(int tcpID)
{
    _impl->tcpID = tcpID;
}

std::string TCPClient::UUID()
{
    return _impl->uuid;
}

std::string TCPClient::SetUUID(const std::string& uuid)
{
    _impl->uuid = uuid;
    return _impl->uuid;
}

bool TCPClient::IsInServer()
{
    if (_impl->clientManager == nullptr)
        return false;
    else
        return true;
}

Accept* TCPClient::AcceptData()
{
    return _impl->acceptData;
}

bool TCPClient::IsAccepted()
{
    if (_impl->acceptData != nullptr) {
        return _impl->acceptData->isVerified();
    }
    return false;
}

void TCPClient::Close()
{
    return _impl->Close();
}

bool TCPClient::isError()
{
    return _impl->isError;
}

float TCPClient::ErrorTimeUptoNow()
{
    return (float)(clock() - _impl->errorTime) / CLOCKS_PER_SEC;
}

int TCPClient::Connect(const std::string& host, int port)
{
    return _impl->Connect(host, port);
}

int TCPClient::Send(const char* data, size_t len)
{
    return _impl->Send(data, len, 1); //规定用户数据类型为1
}

int TCPClient::Receive(std::vector<std::vector<char>>& msgs)
{
    std::vector<int> types;
    _impl->Receive(msgs, types);
    return _impl->ProcCMD(msgs, types);
}

int TCPClient::Receive(std::vector<std::string>& msgs)
{
    std::vector<int> types;
    _impl->Receive(msgs, types);
    return _impl->ProcCMD(msgs, types);
}

int TCPClient::Available()
{
    return _impl->Available();
}

int TCPClient::WaitAvailable(int waitCount)
{
    for (size_t i = 0; i < waitCount; i++) {
        int res = _impl->Available();
        if (res < 0) { //这里应该是错误,就不用等了
            return res;
        }
        if (_impl->Available() > 0) {
            return res; //如果等到了已经有了接收数据那么就直接返回.
        }
        //等待100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}

int TCPClient::WaitAccepted(int waitCount)
{
    for (size_t i = 0; i < waitCount; i++) {
        std::vector<std::string> msgs;
        Receive(msgs);

        if (IsAccepted()) { //这里应该是错误,就不用等了
            return 1;
        }
        //等待100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}

void* TCPClient::Socket()
{
    return &_impl->socket;
}

void* TCPClient::GetKCPClient()
{
    return _impl->kcpClient.get();
}

void TCPClient::CopyKCPClient(TCPClient* src)
{
    _impl->kcpClient = src->_impl->kcpClient;
}

int TCPClient::KCPSend(const char* data, size_t len)
{
    if (_impl->kcpClient == nullptr) {
        return -1;
    }
    return _impl->kcpClient->Send(data, len);
}

int TCPClient::KCPReceive(std::vector<std::string>& msgs)
{
    return _impl->KCPReceive(msgs);
}

int TCPClient::KCPWaitSendCount()
{
    if (_impl->kcpClient == nullptr) {
        return 0;
    }
    return _impl->kcpClient->WaitSendCount();
}

Poco::BasicEvent<TCPEventAccept>& TCPClient::EventAccept()
{
    return _impl->eventAccept;
}

Poco::BasicEvent<TCPEventClose>& TCPClient::EventClose()
{
    return _impl->eventClose;
}

Poco::BasicEvent<TCPEventRemoteClose>& TCPClient::EventRemoteClose()
{
    return _impl->eventRemoteClose;
}

} // namespace dxlib