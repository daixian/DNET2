#include "TCPClient.h"

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/Socket.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/NetException.h"
#include "Poco/Timespan.h"
#include "Poco/Net/SocketStream.h"
#include "Poco/Net/SocketAddress.h"

#include "ClientManager.h"
#include "Protocol/FastPacket.h"
#include "dlog/dlog.h"

namespace dxlib {

class TCPClient::Impl
{
  public:
    Impl()
    {
        receBuff.resize(16 * 1024);
    }
    ~Impl()
    {
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
            Poco::Net::SocketAddress sa(host, port);
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
        socket.setReceiveBufferSize(16 * 1024); //buff大小
        socket.setSendBufferSize(16 * 1024);    //buff大小

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

        int res = socket.sendBytes(package.data(), (int)package.size()); //发送打包后的数据
        return res;
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
    obj._impl->socket.setReceiveBufferSize(16 * 1024); //buff大小
    obj._impl->socket.setSendBufferSize(16 * 1024);    //buff大小

    obj._impl->socket.setNoDelay(true);
    obj._impl->socket.setBlocking(false);
    return;
}

int TCPClient::GetTcpID()
{
    return _impl->tcpID;
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

} // namespace dxlib