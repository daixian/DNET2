#pragma once

#include <string>
#include <vector>
#include "Accept.h"

namespace dxlib {

/**
 * KCP服务器端接收到的用户认证的消息.
 *
 * @author daixian
 * @date 2020/12/21
 */
class KCPEventAccept
{
  public:
    KCPEventAccept(int convID, Accept& accept) : convID(convID), accept(accept) {}
    ~KCPEventAccept() {}

    // tcp连接里的id
    int convID = -1;

    // 认证信息
    Accept accept;
};

/**
 * 当前对象关闭.
 *
 * @author daixian
 * @date 2020/12/21
 */
class KCPEventClose
{
  public:
    KCPEventClose() {}
    ~KCPEventClose() {}
};

/**
 * 远程对象关闭.
 *
 * @author daixian
 * @date 2020/12/21
 */
class KCPEventRemoteClose
{
  public:
    KCPEventRemoteClose(int tcpID) : tcpID(tcpID) {}
    ~KCPEventRemoteClose() {}

    // tcp连接里的id
    int tcpID = 0;
};

} // namespace dxlib