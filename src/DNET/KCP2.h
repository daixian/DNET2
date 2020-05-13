#pragma once

#include "./kcp/ikcp.h"
#include <string>

namespace dxlib {

///-------------------------------------------------------------------------------------------------
/// <summary> KCP的数据收发,这个实现使用的是非阻塞套接字. </summary>
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
    int port;

    /// <summary> 远端主机. </summary>
    std::string remoteHost;

    /// <summary> 远端端口. </summary>
    int remotePort;

    ///-------------------------------------------------------------------------------------------------
    /// <summary> 初始化. </summary>
    ///
    /// <remarks> Dx, 2020/5/12. </remarks>
    ///-------------------------------------------------------------------------------------------------
    void Init();

    ///-------------------------------------------------------------------------------------------------
    /// <summary> 非阻塞的接收. </summary>
    ///
    /// <remarks> Dx, 2020/5/12. </remarks>
    ///
    /// <param name="buffer"> [in,out] If non-null, the
    ///                       buffer. </param>
    /// <param name="len">    The length. </param>
    ///
    /// <returns> An int. </returns>
    ///-------------------------------------------------------------------------------------------------
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