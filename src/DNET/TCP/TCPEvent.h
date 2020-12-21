﻿#pragma once

#include <string>
#include <vector>

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
    TCPEventAccept(int tcpID) : tcpID(tcpID) {}
    ~TCPEventAccept() {}

    // tcp连接里的id
    int tcpID = 0;
};

} // namespace dxlib