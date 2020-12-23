#include "TCPClient.h"

#include "Poco/Net/Socket.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/NetException.h"
#include "Poco/Timespan.h"
#include "Poco/Net/SocketStream.h"

#include "ClientManager.h"
#include "Protocol/FastPacket.h"
#include "dlog/dlog.h"

#include <thread>

#define XUEXUE_TCP_CLIENT_BUFFER_SIZE 8 * 1024

namespace dxlib {

class TCPClient::Impl
{
  public:
    Impl()
    {
        receBuff.resize(XUEXUE_TCP_CLIENT_BUFFER_SIZE);
    }
    ~Impl()
    {
        //析构的时候尝试关闭
        Close();
    }

    // 名字.
    std::string name;

    // 一个tcp的ID.
    int tcpID = -1;

    // 客户端的socket
    Poco::Net::StreamSocket socket;

    // TCP通信协议
    FastPacket packet;

    // 是否已经连接了
    std::atomic_bool isConnected{false};

    //接收用的buffer
    std::vector<char> receBuff;

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
        using Poco::Timespan;

        Close(); //先试试无脑关闭

        try {
            LogI("TCPClient.Connect():尝试连接%s:%d...", host.c_str(), port);
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

            isConnected = false;
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
    int Send(const char* data, int len)
    {
        if (!isConnected) {
            return -1;
        }
        //数据打包
        std::vector<char> package;
        packet.Pack(data, len, package);

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

    /**
     * Receives the given msgs
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @param [out] msgs The msgs.
     *
     * @returns 接收到的数据条数.
     */
    int Receive(std::vector<std::vector<char>>& msgs)
    {
        msgs.clear();
        if (!isConnected) {
            return -1;
        }

        while (true) {
            if (socket.available() > 0) {
                int res = socket.receiveBytes(receBuff.data(), (int)receBuff.size());
                packet.Unpack(receBuff.data(), res, msgs);
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
     * @param [out] msgs The msgs.
     *
     * @returns 接收到的数据条数.
     */
    int Receive(std::vector<std::string>& msgs)
    {
        msgs.clear();
        if (!isConnected) {
            return -1;
        }

        while (true) {
            if (socket.available() > 0) {
                int res = socket.receiveBytes(receBuff.data(), (int)receBuff.size());
                packet.Unpack(receBuff.data(), res, msgs);
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
};

TCPClient::TCPClient()
{
    _impl = new Impl();
}

TCPClient::TCPClient(int port)
{
    _impl = new Impl();
}

TCPClient::~TCPClient()
{
    delete _impl;
}

void TCPClient::CreateWithServer(int tcpID, void* socket, TCPClient& obj)
{
    using Poco::Timespan;

    //TCPClient obj;
    obj._impl->tcpID = tcpID;                              //这里有访问权限
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

int TCPClient::GetTcpID()
{
    return _impl->tcpID;
}

void TCPClient::Close()
{
    return _impl->Close();
}

int TCPClient::Connect(const std::string& host, int port)
{
    return _impl->Connect(host, port);
}

int TCPClient::Send(const char* data, int len)
{
    return _impl->Send(data, len);
}
int TCPClient::Receive(std::vector<std::vector<char>>& msgs)
{
    return _impl->Receive(msgs);
}

int TCPClient::Receive(std::vector<std::string>& msgs)
{
    return _impl->Receive(msgs);
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

void* TCPClient::Socket()
{
    return &_impl->socket;
}
} // namespace dxlib