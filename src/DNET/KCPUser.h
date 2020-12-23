#pragma once

#include <string>
#include <vector>
#include <map>

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/DatagramSocket.h"

#include "Accept.h"

namespace dxlib {
/**
 * 一个远程记录对象,用于绑定kcp对象.因为kcp需要设置一个回调函数output.
 *
 * @author daixian
 * @date 2020/12/19
 */
class KCPUser
{
  public:
    KCPUser(Poco::Net::DatagramSocket* socket, int conv) : socket(socket), conv(conv)
    {
        buffer.resize(4 * 4096);
    }

    // 这里也记录一份conv
    int conv;

    // 远程对象uuid.
    std::string uuid_remote;

    // 自己的UDPSocket,用于发送函数
    Poco::Net::DatagramSocket* socket = nullptr;

    // 要发送过去的地址(这里还是用对象算了)
    Poco::Net::SocketAddress remote;

    // 这个远端之间的认证信息(每个kcp连接之间都有自己的认证随机Key)
    Accept accept;

    // 接收数据buffer
    std::vector<char> buffer;

    // 接收到的待处理的数据
    std::vector<std::string> receData;
};
} // namespace dxlib