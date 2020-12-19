#pragma once

#include <string>
#include <vector>
#include <map>

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
class KCPX
{
  public:
    /**
     * 构造.注意localhost可能被解析成一个ipv6,然后poco库就连不上ipv4的127.0.0.1了
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param  name 名称.
     * @param  host 主机名,可以是ip,但是自己的和remote的不能一个是IPv4一个是IPv6.
     * @param  port The port.
     */
    KCPX(const std::string& name,
         const std::string& host, int port);

    /**
     * Destructor.
     *
     * @author daixian
     * @date 2020/5/12
     */
    ~KCPX();

    // 自己的服务器名字
    std::string name;

    // 主机.
    std::string host;

    // 端口.
    int port = -1;

    ////  远端主机.
    //std::string remoteHost;

    //// 远端端口.
    //int remotePort = -1;

    /**
     * 初始化.
     *
     * @author daixian
     * @date 2020/5/12
     */
    void Init();

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
    int Receive(std::map<int, std::vector<std::string>>& msgs);

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
    int Send(int conv, const char* data, int len);

    /**
     * 向服务端发送一条认证字符串,然后等待服务器回复.
     *
     * @author daixian
     * @date 2020/5/14
     *
     * @returns An int.
     */
    int SendAccept(const std::string& host, int port);

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
     * 得到当前所有记录的Remote名字.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns The remotes.
     */
    std::map<int, std::string> GetRemotes();

    /**
     * 通过一个名字查询一个ConvID
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @param  name The name.
     *
     * @returns The convert with name.
     */
    int GetConvIDWithName(const std::string& name);

  private:
    class Impl;
    Impl* _impl = nullptr;
};

} // namespace dxlib