#pragma once

#include <string>
#include <vector>
#include <map>

#include "Poco/BasicEvent.h"
#include "Poco/Delegate.h"

#include "TCPEvent.h"
#include "TCPClient.h"

namespace dxlib {

class TCPServer
{
  public:
    TCPServer(const std::string& name,
              const std::string& host, int port);
    ~TCPServer();

    // 自己的服务器名字
    std::string name;

    // 主机.
    std::string host;

    // 端口.
    int port = -1;

    // 用户扩展
    void* user = nullptr;

    /**
     * 返回这个服务端的UUID.
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
     * 启动监听.
     *
     * @author daixian
     * @date 2020/12/21
     */
    void Start();

    /**
     * 关闭服务器.
     *
     * @author daixian
     * @date 2020/12/22
     */
    void Close();

    /**
     * 是否监听已经启动.
     *
     * @author daixian
     * @date 2020/12/22
     *
     * @returns True if started, false if not.
     */
    bool IsStarted();

    /**
     * 等待启动完成.
     *
     * @author daixian
     * @date 2020/12/23
     */
    void WaitStarted();

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
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @returns A reference to a Poco::BasicEvent<TCPEvent>
     */
    Poco::BasicEvent<TCPEventClose>& EventClose();

    /**
     * 非阻塞的发送一段数据.正常发送成功返回0.
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param  tcpID tcp连接的ID.
     * @param  data  要发送的数据.
     * @param  len   数据长度.
     *
     * @returns 发送成功的数据长度.
     */
    int Send(int tcpID, const char* data, size_t len);

    /**
     * 可读取(接收)的数据数.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns An int.
     */
    int Available(int tcpID);

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
    int WaitAvailable(int tcpID, int waitCount = 50);

    /**
     * 尝试非阻塞的接收.返回-1表示没有接收到完整的消息或者接收失败，由于kcp的协议，只有接收成功了这里才会返回>0的实际接收消息条数.
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param [out] msgs 以conv为key的所有客户端的消息.
     *
     * @returns 返回大于0的实际接收到的消息条数.
     */
    int Receive(std::map<int, std::vector<std::vector<char>>>& msgs);

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
    int Receive(std::map<int, std::vector<std::string>>& msgs);

    /**
     * 得到这个客户端的Poco的Socket指针(Poco::Net::StreamSocket).
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @param  tcpID Identifier for the TCP.
     *
     * @returns Poco::Net::StreamSocket类型的指针.
     */
    void* GetClientSocket(int tcpID);

    /**
     * Remote count
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns An int.
     */
    int RemoteCount();

    /**
     * 得到当前所有记录的客户端Remote
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns The remotes.
     */
    std::map<int, TCPClient*> GetRemotes();

  private:
    class Impl;
    Impl* _impl;
};

} // namespace dxlib