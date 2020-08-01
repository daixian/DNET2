#pragma once

#include "./kcp/ikcp.h"
#include <string>

namespace dxlib {

///-------------------------------------------------------------------------------------------------
/// <summary>
/// KCP的数据收发,这个实现使用的是非阻塞套接字,并且带的简单的认证逻辑实现.
/// </summary>
///
/// <remarks> Dx, 2020/5/12. </remarks>
///-------------------------------------------------------------------------------------------------
class KCP2
{
  public:
    ///-------------------------------------------------------------------------------------------------
    /// <summary> Constructor. </summary>
    ///
    /// <remarks>
    /// 注意localhost可能被解析成一个ipv6,然后poco库就连不上ipv4的127.0.0.1了.
    ///
    /// Dx, 2020/5/12. </remarks>
    ///
    /// <param name="name">       名称. </param>
    /// <param name="conv">       一个表示会话编号的整数. </param>
    /// <param name="host">       主机名,可以是ip,但是自己的和remote的不能一个是IPv4一个是IPv6. </param>
    /// <param name="port">       The port. </param>
    /// <param name="remoteHost"> The remote host. </param>
    /// <param name="remotePort"> The remote port. </param>
    ///-------------------------------------------------------------------------------------------------
    KCP2(const std::string& name,
         int conv,
         const std::string& host, int port,
         const std::string& remoteHost, int remotePort);

    ///-------------------------------------------------------------------------------------------------
    /// <summary>
    /// 只知道自己的ip和端口,此时启动之后在等待对方连入,如果服务端是这样启动的话,
    /// 那么客户端必须要先SendAccept.
    /// </summary>
    ///
    /// <remarks> Dx, 2020/5/14. </remarks>
    ///
    /// <param name="name"> 名称. </param>
    /// <param name="conv"> 一个表示会话编号的整数. </param>
    /// <param name="host"> The host. </param>
    /// <param name="port"> The port. </param>
    ///-------------------------------------------------------------------------------------------------
    KCP2(const std::string& name,
         int conv,
         const std::string& host, int port);

    ///-------------------------------------------------------------------------------------------------
    /// <summary> Destructor. </summary>
    ///
    /// <remarks> Dx, 2020/5/12. </remarks>
    ///-------------------------------------------------------------------------------------------------
    ~KCP2();

    std::string name;

    /// <summary> 一个表示会话编号的整数. </summary>
    int conv;

    /// <summary> 主机. </summary>
    std::string host;

    /// <summary> 端口. </summary>
    int port = -1;

    /// <summary> 远端主机. </summary>
    std::string remoteHost;

    /// <summary> 远端端口. </summary>
    int remotePort = -1;

    ///-------------------------------------------------------------------------------------------------
    /// <summary> 初始化. </summary>
    ///
    /// <remarks> Dx, 2020/5/12. </remarks>
    ///-------------------------------------------------------------------------------------------------
    void Init();

    ///-------------------------------------------------------------------------------------------------
    /// <summary>
    /// 向服务端发送一条认证字符串,然后等待服务器回复.这个函数是会阻塞一段时间的.
    /// </summary>
    ///
    /// <remarks> Dx, 2020/5/14. </remarks>
    ///
    /// <returns> An int. </returns>
    ///-------------------------------------------------------------------------------------------------
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

    ///-------------------------------------------------------------------------------------------------
    /// <summary> 非阻塞的发送一段数据. </summary>
    ///
    /// <remarks> Dx, 2020/5/12. </remarks>
    ///
    /// <param name="data"> 要发送的数据. </param>
    /// <param name="len">  数据长度. </param>
    ///-------------------------------------------------------------------------------------------------
    int Send(const char* data, int len);

    ///-------------------------------------------------------------------------------------------------
    /// <summary> 需要定时执行的update. </summary>
    ///
    /// <remarks> Dx, 2020/5/13. </remarks>
    ///-------------------------------------------------------------------------------------------------
    void Update();

  private:
    class Impl;
    Impl* _impl = nullptr;
};

} // namespace dxlib