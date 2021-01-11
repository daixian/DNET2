#pragma once

#include <string>
#include <vector>
#include "Accept.h"

namespace dxlib {

/**
 * 服务器端接收到的用户认证的消息.
 *
 * @author daixian
 * @date 2020/12/21
 */
class TCPEventAccept
{
  public:
    TCPEventAccept(int tcpID, Accept* accept) : tcpID(tcpID), accept(accept) {}
    ~TCPEventAccept() {}

    // tcp连接里的id
    int tcpID = 0;

    Accept* accept;
};

/**
 * 当前对象关闭.
 *
 * @author daixian
 * @date 2020/12/21
 */
class TCPEventClose
{
  public:
    TCPEventClose() {}
    ~TCPEventClose() {}
};

/**
 * 远程对象关闭.
 *
 * @author daixian
 * @date 2020/12/21
 */
class TCPEventRemoteClose
{
  public:
    TCPEventRemoteClose(int tcpID) : tcpID(tcpID) {}
    ~TCPEventRemoteClose() {}

    // tcp连接里的id
    int tcpID = 0;
};

} // namespace dxlib