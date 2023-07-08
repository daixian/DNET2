#include "KCP.h"

using namespace dnet;

/**
 * @brief 打开一个KCP服务器.
 * @param server
 * @param name
 * @param port
 * @return
 */
DNET_EXPORT DNetError __cdecl xxKcpCreate(KCPServer*& server, const char* name, int port)
{
    server = new KCPServer(name);
    server->Start(port);
    return DNetError::Ok;
}

/**
 * @brief 添加一个信道.
 * @param server
 * @param conv
 * @return
 */
DNET_EXPORT DNetError __cdecl xxKcpAddChannel(dnet::KCPServer* server, int conv)
{
    if (server == nullptr) {
        return DNetError::InvalidContext;
    }
    server->AddChannel(conv);
    return DNetError::Ok;
}

/**
 * @brief 这个必须要不停的调用.
 * @param server
 * @return
 */
DNET_EXPORT DNetError __cdecl xxKcpReceMessage(dnet::KCPServer* server)
{
    if (server == nullptr) {
        return DNetError::InvalidContext;
    }
    server->ReceMessage();
    // server->mReceMessage
    return DNetError::Ok;
}

DNET_EXPORT DNetError __cdecl xxKcpUpdate(dnet::KCPServer* server)
{
    if (server == nullptr) {
        return DNetError::InvalidContext;
    }
    server->Update();
    return DNetError::Ok;
}