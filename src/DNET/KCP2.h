#pragma once

#include "./kcp/ikcp.h"
#include <string>

namespace dxlib {

/**
 * KCP的数据收发,这个实现使用的是非阻塞套接字,并且带的简单的认证逻辑实现.
 *
 * @author daixian
 * @date 2020/5/12
 */
class KCP2
{
  public:
    /**
     * 构造.注意localhost可能被解析成一个ipv6,然后poco库就连不上ipv4的127.0.0.1了
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param  name       名称.
     * @param  conv       一个表示会话编号的整数.
     * @param  host       主机名,可以是ip,但是自己的和remote的不能一个是IPv4一个是IPv6.
     * @param  port       The port.
     * @param  remoteHost The remote host.
     * @param  remotePort The remote port.
     */
    KCP2(const std::string& name,
         int conv,
         const std::string& host, int port,
         const std::string& remoteHost, int remotePort);

    /**
     * 只知道自己的ip和端口,此时启动之后在等待对方连入,如果服务端是这样启动的话, 那么客户端必须要先SendAccept.
     *
     * @author daixian
     * @date 2020/5/14
     *
     * @param  name 名称.
     * @param  conv 一个表示会话编号的整数.
     * @param  host The host.
     * @param  port The port.
     */
    KCP2(const std::string& name,
         int conv,
         const std::string& host, int port);

    /**
     * Destructor.
     *
     * @author daixian
     * @date 2020/5/12
     */
    ~KCP2();

    std::string name;

    // 一个表示会话编号的整数.
    int conv;

    // 主机.
    std::string host;

    // 端口.
    int port = -1;

    //  远端主机.
    std::string remoteHost;

    // 远端端口.
    int remotePort = -1;

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
     * 向服务端发送一条认证字符串,然后等待服务器回复.这个函数是会阻塞一段时间的.
     *
     * @author daixian
     * @date 2020/5/14
     *
     * @returns An int.
     */
    int SendAccept();

    /**
     * 非阻塞的接收.返回-1表示没有接收到完整的消息或者接收失败，由于kcp的协议，只有接收成功了这里才会返回>0的实际接收长度.
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param [in,out] buffer bufferr.
     * @param          len    数据长度.
     *
     * @returns 返回大于0的实际接收到的长度
     */
    int Receive(char* buffer, int len);

    /**
     * 非阻塞的发送一段数据.
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param  data 要发送的数据.
     * @param  len  数据长度.
     *
     * @returns An int.
     */
    int Send(const char* data, int len);

    /**
     * 需要定时执行的update.
     *
     * @author daixian
     * @date 2020/5/13
     */
    void Update();

  private:
    class Impl;
    Impl* _impl = nullptr;
};

} // namespace dxlib