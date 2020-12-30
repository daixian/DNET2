#pragma once

#include <string>
#include <vector>
#include <map>

#include "Poco/BasicEvent.h"
#include "Poco/Delegate.h"

#include "TCPEvent.h"

namespace dxlib {

/**
 * 一个TCP客户端.它同时指单纯的客户端和服务器端的客户端.
 *
 * @author daixian
 * @date 2020/12/22
 */
class TCPClient
{
  public:
    TCPClient();

    TCPClient(int port);

    ~TCPClient();

    /**
     * 服务器创建一个客户端.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @param          tcpID  Identifier for the TCP.
     * @param [in,out] socket If non-null, the socket.
     * @param [in,out] obj    The object.
     */
    static void CreateWithServer(int tcpID, void* socket, TCPClient& obj);

    /**
     * 如果有就是它的tcpID,没有则是-1
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @returns The tcpid.
     */
    int TcpID();

    /**
     * 返回这个客户端的UUID.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns A std::string.
     */
    std::string UUID();

    /**
     * 设置它的UUID.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns A std::string.
     */
    std::string SetUUID(const std::string& uuid);

    /**
     * 关闭TCP客户端
     *
     * @author daixian
     * @date 2020/12/23
     */
    void Close();

    /**
     * 是否已经网络异常了.
     *
     * @author daixian
     * @date 2020/12/30
     *
     * @returns True if error, false if not.
     */
    bool isError();

    /**
     * 连接主机.
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @param  host 主机地址,目前在内部使用的是IPv4.
     * @param  port The port.
     *
     * @returns 连接成功返回0.
     */
    int Connect(const std::string& host, int port);

    /**
     * 非阻塞的发送一段数据.返回发送的字节数(打包后的)，可能是少于指定的字节数。某些套接字实现也可能返回负数,表示特定条件的值。
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param  data 要发送的数据.
     * @param  len  数据长度.
     *
     * @returns 返回发送成功的长度(打包后的).
     */
    int Send(const char* data, size_t len);

    /**
     * 可读取(接收)的数据数.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns An int.
     */
    int Available();

    /**
     * 等待,一直等到可接收有数据.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @param  waitCount (Optional) 等待的次数,一次100ms吧.
     *
     * @returns 等于Available()函数的返回值.
     */
    int WaitAvailable(int waitCount = 50);

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
    int Receive(std::vector<std::vector<char>>& msgs);

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
    int Receive(std::vector<std::string>& msgs);

    /**
     * 得到这个客户端的Poco的Socket指针(Poco::Net::StreamSocket).
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns Poco::Net::StreamSocket类型的指针.
     */
    void* Socket();

  private:
    class Impl;
    Impl* _impl;
};

} // namespace dxlib