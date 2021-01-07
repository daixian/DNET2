#pragma once

#include "DNET/KCPServer.h"
#include "DNET/KCPClient.h"

// --------------------- windows ---------------------
#if defined(_WIN32) || defined(_WIN64)
// 如果是库自身构建时
#    if defined(DNET_DLL_EXPORTS) //导出库使用dll模式
#        define DNET_EXPORT extern "C" __declspec(dllexport)
#        define DNET__LOCAL
#    else
#        pragma comment(lib, "DNET.lib")
#        define DNET_EXPORT extern "C" __declspec(dllimport)
#    endif

// ---------- 非VC的编译器那么先不区分dllimport ---------------
#else
#    define DLOG_EXPORT __attribute__((visibility("default")))
#    define DLOG__LOCAL __attribute__((visibility("hidden")))
#
#    define __cdecl //默认是，加上了反而有warning __attribute__((__cdecl__))
#endif

enum class DNetError
{
    Unknown = -1,
    Ok = 0,
    NotImplemented = 1,
    NotInitialized = 2,
    AlreadyInitialized = 3,
    InvalidParameter = 4,
    InvalidContext = 5,
    InvalidHandle = 6,
    RuntimeIncompatible = 7,
    RuntimeNotFound = 8,
    SymbolNotFound = 9,
    BufferTooSmall = 10,
    SyncFailed = 11,
    OperationFailed = 12,
    InvalidAttribute = 13,
};

// 字符串消息的处理回调
typedef void (*MessageProcCallback)(void* sender, int id, const char* message);

/**
 * u3d设置一个字符串消息的回调函数进来.
 *
 * @author daixian
 * @date 2020/8/22
 *
 * @param [in] server 绑定的服务器.
 * @param      proc   u3d传过来的回调函数指针.
 */
DNET_EXPORT DNetError __cdecl dnServerSetMessageProc(dxlib::KCPServer* server, MessageProcCallback proc);

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
DNET_EXPORT DNetError __cdecl dnServerCreate(const char* name, const char* host, int port, dxlib::KCPServer*& server);

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
DNET_EXPORT DNetError __cdecl dnServerStart(dxlib::KCPServer* server);

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
DNET_EXPORT DNetError __cdecl dnServerClose(dxlib::KCPServer* server);

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
DNET_EXPORT DNetError __cdecl dnServerUpdate(dxlib::KCPServer* server);

/**
 * 向某个客户端发送数据.
 *
 * @author daixian
 * @date 2021/1/7
 *
 * @param [in,out] server If non-null, the server.
 * @param          id     The identifier.
 * @param          msg    The message.
 * @param          len    The length.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __cdecl dnServerSend(dxlib::KCPServer* server, int id, const char* msg, int len);

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
DNET_EXPORT DNetError __cdecl dnClientSetMessageProc(dxlib::KCPClient* client, MessageProcCallback proc);

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
DNET_EXPORT DNetError __cdecl dnClientCreate(char* name, dxlib::KCPClient*& client);

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
DNET_EXPORT DNetError __cdecl dnClientConnect(dxlib::KCPClient* client, char* host, int port);

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
DNET_EXPORT DNetError __cdecl dnClientClose(dxlib::KCPClient* client);

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
DNET_EXPORT DNetError __cdecl dnClientUpdate(dxlib::KCPClient* client);