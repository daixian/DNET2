#pragma once

#include <string>
#include <vector>
#include <map>
#include "TCP/TCPClient.h"
#include "KCPEvent.h"

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
     *
     * @param  name 名称.
     */
    KCPClient(const std::string& name);

    /**
     * Destructor.
     *
     * @author daixian
     * @date 2020/5/12
     */
    ~KCPClient();

    // 自己的服务器名字
    std::string name;

    // 用户扩展
    void* user = nullptr;

    /**
     * 得到它的ConvID号.
     *
     * @author daixian
     * @date 2020/12/28
     *
     * @returns An int.
     */
    int ConvID();

    /**
     * 返回这个UUID.
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
     * 远端的uuid.
     *
     * @author daixian
     * @date 2020/12/28
     *
     * @returns A std::string.
     */
    std::string RemoteUUID();

    /**
     * 远端的Name.
     *
     * @author daixian
     * @date 2020/12/28
     *
     * @returns A std::string.
     */
    std::string RemoteName();

    /**
     * 向服务端发送一条认证字符串,然后等待服务器回复.
     *
     * @author daixian
     * @date 2020/5/14
     *
     * @returns An int.
     */
    int Connect(const std::string& host, int port);

    /**
     * 客户端是否正在向服务器端Accept
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns True if accepting, false if not.
     */
    bool isAccepting();

    /**
     * 是否已经通过了认证
     *
     * @author daixian
     * @date 2020/12/24
     *
     * @returns True if accepted, false if not.
     */
    bool isAccepted();

    /**
     * 等待认证成功
     *
     * @author daixian
     * @date 2020/12/24
     *
     * @param  waitCount (Optional) 等待的次数,一次100ms吧.
     */
    void WaitAccepted(int waitCount = 50);

    /**
     * 关闭.
     *
     * @author daixian
     * @date 2020/12/18
     */
    void Close();

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
    int Receive(std::vector<std::string>& msgs);

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

    /**
     * 得到它的TCP成员对象.在握手阶段会使用这个TCP.
     *
     * @author daixian
     * @date 2020/12/25
     *
     * @returns TCPClient指针.
     */
    TCPClient* GetTCPClient();

    /**
     * 得到连接成功的事件.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @returns A reference to a Poco::BasicEvent<KCPEvent>
     */
    Poco::BasicEvent<KCPEventAccept>& EventConnect();

    /**
     * 自身关闭的事件.
     *
     * @author daixian
     * @date 2021/1/7
     *
     * @returns A reference to a Poco::BasicEvent<KCPEventClose>
     */
    Poco::BasicEvent<KCPEventClose>& EventClose();

  private:
    class Impl;
    Impl* _impl = nullptr;
};

} // namespace dxlib