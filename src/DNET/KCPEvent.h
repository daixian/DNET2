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

} // namespace dxlib