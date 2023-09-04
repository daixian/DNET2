#pragma once

#include "ErrorType.h"

#include "DNET/TCP/TCPServer.h"
#include "DNET/TCP/TCPClient.h"

// --------------------- windows ---------------------
#if defined(_WIN32) || defined(_WIN64)
// 如果是库自身构建时
#    if defined(DNET_DLL_EXPORTS) // 导出库使用dll模式
#        define DNET_EXPORT extern "C" __declspec(dllexport)
#        define DNET__LOCAL
#    else
#        pragma comment(lib, "DNET.lib")
#        define DNET_EXPORT extern "C" __declspec(dllimport)
#    endif

// ---------- 非VC的编译器那么先不区分dllimport ---------------
#else
#    define DNET_EXPORT __attribute__((visibility("default")))
#    define DNET__LOCAL __attribute__((visibility("hidden")))
#
#    define __stdcall // 默认是，加上了反而有warning __attribute__((__stdcall__))
#endif

// 字符串消息的处理回调函数指针类型
typedef void (*MessageProcCallback)(void* sender, int id, int msgType, const char* message);

/**
 * u3d设置一个字符串消息的回调函数进来.
 *
 * @author daixian
 * @date 2020/8/22
 *
 * @param [in] server  绑定的服务器.
 * @param      tcpProc u3d传过来的tcp回调函数指针.
 * @param      kcpProc u3d传过来的kcp回调函数指针.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __stdcall dnServerSetMessageProc(dnet::TCPServer* server, MessageProcCallback tcpProc, MessageProcCallback kcpProc);

/**
 * 创建服务器端.
 *
 * @author daixian
 * @date 2018/4/22
 *
 * @param       name   服务器端的友好名.
 * @param       host   The host.
 * @param       port   The port.
 * @param [out] server 创建的结果指针.
 *
 * @returns 成功返回0.
 */
DNET_EXPORT DNetError __stdcall dnServerCreate(const char* name, const char* host, int port, dnet::TCPServer*& server);

/**
 * 服务器启动.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] server If non-null, the server.
 * @param [in] host   If non-null, the host.
 * @param      port   The port.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __stdcall dnServerStart(dnet::TCPServer* server);

/**
 * 服务器关闭.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] server If non-null, the server.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __stdcall dnServerClose(dnet::TCPServer* server);

/**
 * 服务器Update,实际上就是接收.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] server If non-null, the server.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __stdcall dnServerUpdate(dnet::TCPServer* server);

/**
 * 向某个客户端发送数据.
 *
 * @author daixian
 * @date 2021/1/7
 *
 * @param [in] server server对象.
 * @param      id     The identifier.
 * @param      msg    The message.
 * @param      len    The length.
 * @param      type   消息类型.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __stdcall dnServerSend(dnet::TCPServer* server, int id, const char* msg, int len, int type);

/**
 * 向某个客户端发送数据.
 *
 * @author daixian
 * @date 2021/1/7
 *
 * @param [in] server If non-null, the server.
 * @param      id     The identifier.
 * @param      msg    The message.
 * @param      len    The length.
 * @param      type   The type.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __stdcall dnServerKCPSend(dnet::TCPServer* server, int id, const char* msg, int len, int type);

//----------------------------------------------------------- 客户端 -----------------------------------------------------------

/**
 * u3d设置一个字符串消息的回调函数进来.
 *
 * @author daixian
 * @date 2020/8/22
 *
 * @param [in] client  客户端对象指针.
 * @param      tcpProc u3d传过来的回调函数指针.
 * @param      kcpProc The kcp procedure.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __stdcall dnClientSetMessageProc(dnet::TCPClient* client, MessageProcCallback tcpProc, MessageProcCallback kcpProc);

/**
 * 创建客户端.
 *
 * @author daixian
 * @date 2018/4/22
 *
 * @param [in]  name   客户端名字.
 * @param [out] client 客户端对象指针.
 *
 * @returns 成功返回0.
 */
DNET_EXPORT DNetError __stdcall dnClientCreate(char* name, dnet::TCPClient*& client);

/**
 * 客户端连接服务器.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] client 客户端对象指针.
 * @param [in] host   If non-null, the host.
 * @param      port   The port.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __stdcall dnClientConnect(dnet::TCPClient* client, char* host, int port);

/**
 * 客户端是否已经连接服务器
 *
 * @author daixian
 * @date 2021/1/25
 *
 * @param [in]  client     客户端对象指针.
 * @param [out] isAccepted True if is accepted, false if not.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __stdcall dnClientIsAccepted(dnet::TCPClient* client, bool& isAccepted);

/**
 * 客户端关闭.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] client 客户端对象指针.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __stdcall dnClientClose(dnet::TCPClient* client);

/**
 * 客户端Update,实际上就是接收.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] client 客户端对象指针.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __stdcall dnClientUpdate(dnet::TCPClient* client);

/**
 * 向服务器端发送数据.
 *
 * @author daixian
 * @date 2021/1/7
 *
 * @param [in] client 客户端对象指针.
 * @param      msg    The message.
 * @param      len    The length.
 * @param      type   The type.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __stdcall dnClientSend(dnet::TCPClient* client, const char* msg, int len, int type);

/**
 * 向服务器端发送KCP数据.
 *
 * @author daixian
 * @date 2021/1/7
 *
 * @param [in] client 客户端对象指针.
 * @param      msg    The message.
 * @param      len    The length.
 * @param      type   The type.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __stdcall dnClientKCPSend(dnet::TCPClient* client, const char* msg, int len, int type);
