#pragma once

#include "./kcp/ikcp.h"
#include <string>

namespace dxlib {

/**
 * KCP的数据收发,这个实现使用的是阻塞套接字.
 *
 * @author daixian
 * @date 2020/5/12
 */
class KCP
{
  public:
    /**
     * 构造.注意localhost可能被解析成一个ipv6,然后poco库就连不上ipv4的127.0.0.1了.
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
    KCP(const std::string& name,
        int conv,
        const std::string& host, int port,
        const std::string& remoteHost, int remotePort);

    /**
     * Destructor.
     *
     * @author daixian
     * @date 2020/5/12
     */
    ~KCP();

    std::string name;

    // 一个表示会话编号的整数.
    int conv;

    // 主机.
    std::string host;

    // 端口.
    int port;

    // 远端主机.
    std::string remoteHost;

    // 远端端口.
    int remotePort;

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
     * 接收.
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param [in,out] buffer If non-null, the buffer.
     * @param          len    The length.
     *
     * @returns An int.
     */
    int Receive(char* buffer, int len);

    /**
     * 发送一段数据.
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

  private:
    class Impl;
    Impl* _impl = nullptr;
};

} // namespace dxlib