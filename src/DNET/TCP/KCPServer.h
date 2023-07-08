#pragma once

#include "KCPChannel.h"

namespace dnet {

/**
 * @brief 任何udp端实际都应该是一个server.
 *        由于KCP有信道,一个remote又可以对应多个信道.同时一个本地socket数据可以对应多个信道.
 */
class KCPServer
{
  public:
    KCPServer(const std::string& name = "KCPServer");
    ~KCPServer();

    // 这个对象的名字，只是方便调试
    std::string name;

    // key是kcp的信道.
    std::map<int, KCPChannel*> mClient;

    // key是kcp的信道.value是信道中的所有消息.
    std::map<int, std::vector<TextMessage>> mReceMessage;

    /**
     * @brief 开始监听一个UDP端口.
     * @param port
     */
    void Start(int port)
    {
        Poco::Net::SocketAddress sa(Poco::Net::IPAddress("0.0.0.0"), port);
        udpSocket = new Poco::Net::DatagramSocket(sa);
        udpSocket->setBlocking(false);
    }

    /**
     * @brief 关闭这个UDP端口。
     */
    void Close()
    {
        if (udpSocket != nullptr) {
            udpSocket->close();
            delete udpSocket;
        }
    }

    /**
     * @brief 创建一个客户端,必须要有一个信道.
     * @param conv 信道id.
     */
    void AddChannel(int conv)
    {
        delete GetChannel(conv); // 关闭原来存在的
        mClient[conv] = new KCPChannel(udpSocket, conv);
    }

    /**
     * @brief 移除一个客户端。
     * @param conv
     */
    void RemoveChannel(int conv)
    {
        if (mClient.find(conv) != mClient.end()) {
            delete mClient[conv];
            mClient.erase(conv);
        }
    }

    /**
     * @brief 按信道得到一个客户端.
     * @param conv
     * @return
     */
    KCPChannel* GetChannel(int conv)
    {
        if (mClient.find(conv) != mClient.end()) {
            return mClient[conv];
        }
        return nullptr;
    }

    /**
     * @brief 接收消息，这个需要不停的调用，因为update也要不停的调用，所以可以放到一起update。
     * @param update 是否顺便update。
     * @return
     */
    int ReceMessage(bool update = true)
    {
        int receCount = 0;
        try {
            if (update) {
                Update();
            }

            Poco::Net::SocketAddress remote(Poco::Net::AddressFamily::IPv4);
            int n = udpSocket->receiveFrom(socketRecebuff.data(), (int)socketRecebuff.size(), remote);
            if (n <= 0) {
                return receCount;
            }
            // LogI("KCPServer.ReceMessage():%s的UDPSocket接收到了消息长度%d", name.c_str(), n);

            // 要把所有客户端遍历一遍
            for (auto& kvp : mClient) {
                std::vector<TextMessage> msgs;
                int res = kvp.second->IKCPRecv(socketRecebuff.data(), n, msgs);
                if (res == -1) {
                    // 如果是-1那么表示不是这个信道的数据
                    continue;
                }
                kvp.second->remote = remote; // 记录这个remote
                if (res > 0) {
                    std::vector<TextMessage>& vReceMessage = mReceMessage[kvp.first]; // 这个信道的所有消息
                    vReceMessage.insert(vReceMessage.end(), msgs.begin(), msgs.end());
                    receCount += msgs.size();
                    // lastKcpReceTime = clock();
                }

                // 这里好像可以Flush一下
                // kvp.second->Flush();
            }
        }
        catch (const Poco::Exception& e) {
            LogE("KCPServer.ReceMessage():异常e=%s,%s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("KCPServer.ReceMessage():异常e=%s", e.what());
        }
        return receCount;
    }

    /**
     * @brief 给某个信道设置一个远端IP地址.
     * @param conv 信道.
     * @param ip 远端ip地址.
     * @param port 远端端口.
     */
    void ClientSetRemote(int conv, std::string ip, int port)
    {
        Poco::Net::SocketAddress addr(Poco::Net::IPAddress(ip), port);
        mClient[conv]->Bind(udpSocket, addr);
    }

    /**
     * @brief 向某个客户端发送一个消息.
     * @param conv 客户端的信道.
     * @param data 数据.
     * @param len 数据长度.
     * @param type 这个消息协议的类型.
     */
    void Send(int conv, const char* data, size_t len, int type = -1)
    {
        mClient[conv]->Send(data, len, type);
    }

    /**
     * @brief
     */
    void Update()
    {
        for (auto& kvp : mClient) {
            kvp.second->Update();
        }
    }

    /**
     * @brief
     */
    void Flush()
    {
        for (auto& kvp : mClient) {
            kvp.second->Flush();
        }
    }

  private:
    // 自己的UDPSocket,所有的client都用的是这个
    Poco::Net::DatagramSocket* udpSocket = nullptr;

    // Socket接收用的buffer
    std::vector<char> socketRecebuff;
};

} // namespace dnet