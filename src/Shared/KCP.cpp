#include "KCP.h"

using namespace dnet;

/**
 * @brief 打开一个KCP服务器.
 * @param server
 * @param name
 * @param port
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpCreate(KCPServer*& server, const char* name, int port)
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
DNET_EXPORT DNetError __stdcall xxKcpAddChannel(dnet::KCPServer* server, int conv)
{
    if (server == nullptr) {
        return DNetError::InvalidContext;
    }
    server->AddChannel(conv);
    return DNetError::Ok;
}

/**
 * @brief 设置一个远端地址.
 * @param server
 * @param conv
 * @param ip
 * @param port
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpChannelSetRemote(dnet::KCPServer* server, int conv, const char* ip, int port)
{
    if (server == nullptr) {
        return DNetError::InvalidContext;
    }
    server->ChannelSetRemote(conv, ip, port);
    return DNetError::Ok;
}

/**
 * @brief 这个必须要不停的调用.
 * @param server
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpReceMessage(dnet::KCPServer* server)
{
    if (server == nullptr) {
        return DNetError::InvalidContext;
    }
    server->ReceMessage();
    // server->mReceMessage
    return DNetError::Ok;
}

/**
 * @brief 有时候也可以Update一下.
 * @param server
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpUpdate(dnet::KCPServer* server)
{
    if (server == nullptr) {
        return DNetError::InvalidContext;
    }
    server->Update();
    return DNetError::Ok;
}

/**
 * @brief 发送数据.
 * @param server
 * @param conv
 * @param data
 * @param len
 * @param type
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpSend(dnet::KCPServer* server, int conv, char* data, int len, int type)
{
    if (server == nullptr) {
        return DNetError::InvalidContext;
    }
    KCPChannel* channel = server->GetChannel(conv);
    if (channel != nullptr) {
        channel->Send(data, len, type);
    }
    else {
        return DNetError::InvalidParameter;
    }

    return DNetError::Ok;
}

/**
 * @brief 尝试提取一条消息.
 * @param server
 * @param buffer
 * @param bufferSize
 * @param success [out] 是否成功.
 * @param conv [out] 信道.
 * @param type [out] 消息类型.
 * @param len [out] 消息长度.
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpGetMessage(dnet::KCPServer* server,
                                                char* buffer, int bufferSize,
                                                bool& success, int& conv, int& type, int& len)
{
    if (server == nullptr) {
        return DNetError::InvalidContext;
    }
    for (auto& kvp : server->mReceMessage) {
        if (kvp.second.size() > 0) {
            success = true; // 标记成功
            conv = kvp.first;
            auto& msg = kvp.second.front();
            type = msg.type;
            len = msg.data.size();
            memcpy(buffer, msg.data.data(), msg.data.size());
            kvp.second.pop_front(); // 移除这条
            return DNetError::Ok;
        }
    }
    // 这里是没有提取到消息.
    success = false;
    conv = -1;
    type = -1;
    len = 0;
    return DNetError::Ok;
}