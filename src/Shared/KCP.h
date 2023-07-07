#pragma once

#include "ErrorType.h"

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
#    define DLOG_EXPORT __attribute__((visibility("default")))
#    define DLOG__LOCAL __attribute__((visibility("hidden")))
#
#    define __cdecl // 默认是，加上了反而有warning __attribute__((__cdecl__))
#endif

#include "DNET/TCP/KCPClient.h"

/**
 * @brief 创建一个客户端。
 * @param conv
 * @param client
 * @return
 */
DNET_EXPORT DNetError __cdecl DNetKCPClientCreate(int conv, dnet::KCPClient*& client);

/**
 * @brief 绑定。
 * @param client
 * @param datagramSocket
 * @param remote
 * @return
 */
DNET_EXPORT DNetError __cdecl DNetKCPClientBind(dnet::KCPClient*& client,
                                                Poco::Net::DatagramSocket* datagramSocket,
                                                const Poco::Net::SocketAddress& remote);
