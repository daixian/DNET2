#pragma once

#include <string>
#include <vector>
#include <map>

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/DatagramSocket.h"

#include "Accept.h"

#include "./kcp/ikcp.h"

namespace dxlib {

/**
 * KCP的数据收发,这个实现使用的是非阻塞套接字.
 * 一个对象带有一个socket,然后需要支持和多个对象之间的通信.
 * 两个对象之间的握手:始终使用0号conv进行握手.
 * 
 *
 * @author daixian
 * @date 2020/5/12
 */
class KCPClient
{
  public:
    /**
     * 构造.注意localhost可能被解析成一个ipv6,然后poco库就连不上ipv4的127.0.0.1了
     *
     * @author daixian
     * @date 2020/5/12
     */
    KCPClient();

    ~KCPClient();

    // 是否是TCP Server端所属的TcpClient
    bool isServer = false;

    // 自己的UDPSocket,只用于发送函数(对象的生命周期在外面管理).
    Poco::Net::DatagramSocket* socket = nullptr;

    // 接收数据Socket Buffer.
    //std::vector<char> socketBuffer;

    // 要发送过去的地址(这里还是用对象算了).
    Poco::Net::SocketAddress remote;

    // 这个远端之间的认证信息(每个kcp连接之间都有自己的认证随机Key).
    //Accept accept;

    // kcp对象.
    ikcpcb* kcp = nullptr;

    // kcp接收数据buffer.
    std::vector<char> kcpReceBuf;

    // 接收到的待处理的数据.
    std::vector<std::string> receData;

    /**
     * kcp的id.
     *
     * @author daixian
     * @date 2021/1/11
     *
     * @returns id.
     */
    int Conv()
    {
        if (kcp == nullptr) {
            return -1;
        }
        return kcp->conv;
    }

    /**
     * 创建一个kcp协议.
     *
     * @author daixian
     * @date 2021/1/11
     */
    void Create(int conv);

    /**
     * 绑定一个TCP的Poco::Net::StreamSocket的端口，当tcp断线重连之后需要重新绑定这个.
     *   Poco::Net::DatagramSocket
     * @author daixian
     * @date 2021/1/9
     *
     * @param [in] streamSocket Poco::Net::StreamSocket对象指针.
     */
    void Bind(Poco::Net::DatagramSocket* datagramSocket, const Poco::Net::SocketAddress& remote);

    /**
     * 非阻塞的接收.返回-1表示没有接收到完整的消息或者接收失败，由于kcp的协议，只有接收成功了这里才会返回>0的实际接收消息条数.
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param [out] msgs 以conv为key的所有客户端的消息.
     *
     * @returns 返回大于0的实际接收到的消息条数.
     */
    int Receive(const char* buff, size_t len, std::vector<std::string>& msgs);

    /**
     * 非阻塞的发送一段数据.正常发送成功返回0.
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param  data 要发送的数据.
     * @param  len  数据长度.
     *
     * @returns 正常发送成功返回0.
     */
    int Send(const char* data, size_t len);

    /**
     * 当前等待发送的消息计数.如果这个数量太多,那么已经拥塞.
     *
     * @author daixian
     * @date 2020/12/20
     *
     * @param  conv The convert.
     *
     * @returns An int.
     */
    int WaitSendCount();

  private:
};

} // namespace dxlib