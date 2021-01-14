#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "Poco/BasicEvent.h"
#include "Poco/Delegate.h"

#include "TCPEvent.h"
#include "Accept.h"

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
    TCPClient(const std::string& name = "TCPClient");

    ~TCPClient();

    // 用户扩展
    void* user = nullptr;

    /**
     * 服务器创建一个客户端(由TCPServer来调用创建).
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @param       tcpID         tcpID.
     * @param [in]  socket        Accept线程产生的tcp socket.
     * @param [in]  clientManager 传递给这个TCPClient对象的一个clientManager数据指针.
     * @param [out] obj           返回的这个obj.
     */
    static void CreateWithServer(int tcpID, void* socket, void* clientManager,
                                 TCPClient& obj);

    /**
     * 如果有就是它的tcpID,没有则是-1.
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @returns The tcpid.
     */
    int TcpID();

    /**
     * 设置TCP id.
     *
     * @author daixian
     * @date 2021/1/9
     *
     * @param  tcpID tcpID.
     */
    void SetTcpID(int tcpID);

    /**
     * 返回这个客户端的UUID.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns UUID的字符串.
     */
    std::string UUID();

    /**
     * 设置它的UUID(通常不应该随便改变它).
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns A std::string.
     */
    std::string SetUUID(const std::string& uuid);

    /**
     * 是否是Server端的Client.
     *
     * @author daixian
     * @date 2021/1/11
     *
     * @returns True if in server, false if not.
     */
    bool IsInServer();

    /**
     * 客户端和服务器之间的认证数据.
     *
     * @author daixian
     * @date 2021/1/9
     *
     * @returns Null if it fails, else a pointer to an Accept.
     */
    Accept* AcceptData();

    /**
     * 是否已经通过了认证
     *
     * @author daixian
     * @date 2020/12/24
     *
     * @returns True if accepted, false if not.
     */
    bool IsAccepted();

    /**
     * 等待Accepte完成.会调用Receive()函数.
     *
     * @author daixian
     * @date 2021/1/11
     *
     * @param  waitCount (Optional) Number of waits.
     *
     * @returns An int.
     */
    int WaitAccepted(int waitCount = 50);

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
     * 错误时间距离现在多久了,单位秒.
     *
     * @author daixian
     * @date 2021/1/13
     *
     * @returns A float.
     */
    float ErrorTimeUptoNow();

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

    /**
     * 得到KCPClient类型指针.
     *
     * @author daixian
     * @date 2021/1/11
     *
     * @returns 一个KCPClient类型指针.
     */
    void* GetKCPClient();

    /**
     * 移动KCPClient的指针.
     *
     * @author daixian
     * @date 2021/1/11
     *
     * @param [in] src 移动源,老的对象,移动给自己.
     */
    void CopyKCPClient(TCPClient* src);

    /**
     * 走kcp通道进行发送.
     *
     * @author daixian
     * @date 2021/1/11
     *
     * @param  data The data.
     * @param  len  The length.
     *
     * @returns An int.
     */
    int KCPSend(const char* data, size_t len);

    /**
     * KCP的接收.
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @param [out] msgs The msgs.
     *
     * @returns 接收到的数据条数.
     */
    int KCPReceive(std::vector<std::string>& msgs);

    /**
     * 服务器端的调用ClientManger里的socket的接收,然后调用这个KCP的接收.
     *
     * @author daixian
     * @date 2021/1/14
     *
     * @param       data socket接收到的数据.
     * @param       len  socket接收到的数据长度.
     * @param [out] msgs The msgs.
     *
     * @returns 接收到的数据条数.
     */
    int KCPReceive(const char* data, size_t len, std::vector<std::string>& msgs);

    /**
     * 当前等待发送的消息计数.如果这个数量太多,那么已经拥塞.
     *
     * @author daixian
     * @date 2020/12/20
     *
     * @returns An int.
     */
    int KCPWaitSendCount();

    /**
     * 得到Accept的事件.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @returns A reference to a Poco::BasicEvent<TCPEvent>
     */
    Poco::BasicEvent<TCPEventAccept>& EventAccept();

    /**
     * 得到关闭的事件.
     * server.EventAccept() += Poco::delegate(&target, &Target::OnEventAccept);
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @returns A reference to a Poco::BasicEvent<TCPEvent>
     */
    Poco::BasicEvent<TCPEventClose>& EventClose();

    /**
     * 远端关闭的事件.
     *
     * @author daixian
     * @date 2021/1/7
     *
     * @returns A reference to a Poco::BasicEvent<TCPEventRemoteClose>
     */
    Poco::BasicEvent<TCPEventRemoteClose>& EventRemoteClose();

  private:
    class Impl;
    // 使用智能指针来拷贝.
    std::shared_ptr<Impl> _impl;
};

} // namespace dxlib