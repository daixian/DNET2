#include "KCP.h"
#include <sstream>
#include "Poco/DeflatingStream.h"
#include "Poco/InflatingStream.h"

using namespace dnet;

/**
 * @brief 打开一个KCP服务器.
 * @param kcp
 * @param name
 * @param port
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpCreate(KCPServer*& kcp, const char* name, int port)
{
    kcp = new KCPServer(name);
    bool success = kcp->Start(port);
    if (success) {
        return DNetError::Ok;
    }
    else {
        delete kcp;
        kcp = nullptr;
        return DNetError::OperationFailed;
    }
}

/**
 * @brief 添加一个信道.
 * @param kcp
 * @param conv
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpAddChannel(dnet::KCPServer* kcp, int conv)
{
    if (kcp == nullptr) {
        return DNetError::InvalidContext;
    }
    kcp->AddChannel(conv);
    return DNetError::Ok;
}

/**
 * @brief 设置一个远端地址.
 * @param kcp
 * @param conv
 * @param ip
 * @param port
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpChannelSetRemote(dnet::KCPServer* kcp, int conv, const char* ip, int port)
{
    if (kcp == nullptr) {
        return DNetError::InvalidContext;
    }
    kcp->ChannelSetRemote(conv, ip, port);
    return DNetError::Ok;
}

/**
 * @brief 这个必须要不停的调用.
 * @param kcp
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpReceMessage(dnet::KCPServer* kcp)
{
    if (kcp == nullptr) {
        return DNetError::InvalidContext;
    }
    int res = kcp->ReceMessage();
    if (res < 0) {
        DNetError::NetException;
    }
    else {
        return DNetError::Ok;
    }
}

/**
 * @brief 有时候也可以Update一下.
 * @param kcp
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpUpdate(dnet::KCPServer* kcp)
{
    if (kcp == nullptr) {
        return DNetError::InvalidContext;
    }
    kcp->Update();
    return DNetError::Ok;
}

/**
 * @brief 发送数据.
 * @param kcp
 * @param conv
 * @param data
 * @param len
 * @param type
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpSend(dnet::KCPServer* kcp, int conv, char* data, int len, int type)
{
    if (kcp == nullptr) {
        return DNetError::InvalidContext;
    }
    KCPChannel* channel = kcp->GetChannel(conv);
    if (channel != nullptr) {
        int result = channel->Send(data, len, type);
        if (result < 0) {
            return DNetError::OperationFailed;
        }
    }
    else {
        return DNetError::InvalidParameter;
    }

    return DNetError::Ok;
}

/**
 * @brief 尝试提取一条消息.
 * @param kcp
 * @param buffer
 * @param bufferSize
 * @param success [out] 是否成功.
 * @param conv [out] 信道.
 * @param type [out] 消息类型.
 * @param len [out] 消息长度.
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpGetMessage(dnet::KCPServer* kcp,
                                                char* buffer, int bufferSize,
                                                bool& success, int& conv, int& type, int& len)
{
    if (kcp == nullptr) {
        return DNetError::InvalidContext;
    }
    for (auto& kvp : kcp->mReceMessage) {
        if (kvp.second.size() > 0) {
            success = true; // 标记成功
            conv = kvp.first;
            auto& msg = kvp.second.front();
            type = msg.type;
            if (type == 100) {
                // 这是GZIP压缩流,解压缩一下
                std::stringstream iss(msg.data);
                Poco::InflatingInputStream ideflater(iss, Poco::InflatingStreamBuf::STREAM_GZIP);
                std::string str; // 解压缩的流结果
                ideflater >> str;
                len = str.size();
                memcpy(buffer, str.data(), str.size());
            }
            else {
                len = msg.data.size();
                memcpy(buffer, msg.data.data(), msg.data.size());
            }

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

/**
 * @brief 上次接收到消息到现在的时间。
 * @param kcp
 * @param conv
 * @param timeToNow
 * @return
 */
DNET_EXPORT DNetError __stdcall xxKcpLastReceMessageTimeToNow(dnet::KCPServer* kcp, int conv, float& timeToNow)
{
    if (kcp == nullptr) {
        return DNetError::InvalidContext;
    }
    dnet::KCPChannel* channel = kcp->GetChannel(conv);
    if (channel == nullptr) {
        return DNetError::InvalidParameter;
    }
    else {
        timeToNow = channel->LastReceMessageTimeToNow();
    }
}