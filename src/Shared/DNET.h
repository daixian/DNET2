#pragma once

#include "DNET/KCPServer.h"
#include "DNET/KCPClient.h"

// --------------------- windows ---------------------
#if defined(_WIN32) || defined(_WIN64)
// 如果是库自身构建时
#    if defined(DNET_DLL_EXPORTS) //导出库使用dll模式
#        define DNET_EXPORT __declspec(dllexport)
#        define DNET__LOCAL
#    else
#        pragma comment(lib, "DNET.lib")
#        define DNET_EXPORT __declspec(dllimport)
#    endif

// ---------- 非VC的编译器那么先不区分dllimport ---------------
#else
#    define DLOG_EXPORT __attribute__((visibility("default")))
#    define DLOG__LOCAL __attribute__((visibility("hidden")))
#
#    define __cdecl //默认是，加上了反而有warning __attribute__((__cdecl__))
#endif

// 字符串消息的处理回调
typedef void (*MessageProcCallback)(int id, char* message);

/**
 * u3d设置一个字符串消息的回调函数进来.
 *
 * @author daixian
 * @date 2020/8/22
 *
 * @param [in] server 绑定的服务器.
 * @param      proc   u3d传过来的回调函数指针.
 */
extern "C" DNET_EXPORT void __cdecl dnServerSetMessageProc(dxlib::KCPServer* server, MessageProcCallback proc);

/**
 * 创建服务器端.
 *
 * @author daixian
 * @date 2018/4/22
 *
 * @returns 成功返回0.
 */
extern "C" DNET_EXPORT int __cdecl dnServerCreate(char* name, dxlib::KCPServer*& server);

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
extern "C" DNET_EXPORT int __cdecl dnServerStart(dxlib::KCPServer* server, char* host, int port);

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
extern "C" DNET_EXPORT int __cdecl dnServerClose(dxlib::KCPServer* server);

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
extern "C" DNET_EXPORT int __cdecl dnServerUpdate(dxlib::KCPServer* server);

//----------------------------- 客户端 -----------------------------

/**
 * u3d设置一个字符串消息的回调函数进来.
 *
 * @author daixian
 * @date 2020/8/22
 *
 * @param [in] client 绑定的客户端.
 * @param       proc   u3d传过来的回调函数指针.
 */
extern "C" DNET_EXPORT void __cdecl dnClientSetMessageProc(dxlib::KCPClient* client, MessageProcCallback proc);

/**
 * 创建客户端.
 *
 * @author daixian
 * @date 2018/4/22
 *
 * @param [in]  name   If non-null, the name.
 * @param [out] client [in,out] If non-null, the client.
 *
 * @returns 成功返回0.
 */
extern "C" DNET_EXPORT int __cdecl dnClientCreate(char* name, dxlib::KCPClient*& client);

/**
 * 客户端连接服务器.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] client If non-null, the server.
 * @param [in] host   If non-null, the host.
 * @param      port   The port.
 *
 * @returns An int.
 */
extern "C" DNET_EXPORT int __cdecl dnClientConnect(dxlib::KCPClient* client, char* host, int port);

/**
 * 客户端关闭.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] client If non-null, the server.
 *
 * @returns An int.
 */
extern "C" DNET_EXPORT int __cdecl dnClientClose(dxlib::KCPClient* client);

/**
 * 客户端Update,实际上就是接收.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] client If non-null, the server.
 *
 * @returns An int.
 */
extern "C" DNET_EXPORT int __cdecl dnClientUpdate(dxlib::KCPClient* client);