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
    ~TCPClient();

    static void CreateWithServer(int tcpID, void* socket, TCPClient& obj);

    /**
     * Gets the tcpid
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @returns The tcpid.
     */
    int GetTcpID();

    /**
     * 连接主机.
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @param  host The host.
     * @param  port The port.
     *
     * @returns 连接成功返回0.
     */
    int Connect(const std::string& host, int port);

    /**
     * 非阻塞的发送一段数据.正常发送成功返回0.
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param  data  要发送的数据.
     * @param  len   数据长度.
     *
     * @returns 返回发送成功的长度.
     */
    int Send(const char* data, int len);

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

  private:
    class Impl;
    Impl* _impl;
};

} // namespace dxlib