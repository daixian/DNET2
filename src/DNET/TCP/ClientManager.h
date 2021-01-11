﻿#pragma once

#include <string>
#include <vector>
#include <map>
#include "TCPClient.h"

#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/DatagramSocket.h"
#include "dlog/dlog.h"

namespace dxlib {

class ClientManager
{
  public:
    ClientManager()
    {
        receBuffUDP.resize(8 * 1024);
    }
    ~ClientManager() {}

    // 所有连接了的客户端.
    std::map<int, TCPClient*> mClients;

    // 有了uuid返回的及客户端记录
    std::map<std::string, TCPClient*> mAcceptClients;

    // 用户连接成功的事件.
    Poco::BasicEvent<TCPEventAccept>* eventAccept = nullptr;

    /**
     * 服务器添加一个客户端进来记录.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param [in] client 要记录的客户端.
     *
     * @returns 给这个client分配的id号.
     */
    TCPClient* AddClient(Poco::Net::StreamSocket& client)
    {
        TCPClient* tcobj = new TCPClient("TCPServer");
        TCPClient::CreateWithServer(_clientCount, &client, this, *tcobj); //这个函数传入一个tcpID
        mClients[_clientCount] = tcobj;
        _clientCount++; //永远递增
        return tcobj;
    }

    /**
     * 得到一个客户端.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param  tcpID Identifier for the TCP.
     *
     * @returns Null if it fails, else the client.
     */
    TCPClient* GetClient(int tcpID)
    {
        if (mClients.find(tcpID) != mClients.end()) {
            return mClients[tcpID];
        }
        return nullptr;
    }

    /**
     * 删除一个客户端.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param  tcpID Identifier for the TCP.
     *
     * @returns Null if it fails, else the client.
     */
    void RemoveClient(int tcpID)
    {
        if (mClients.find(tcpID) != mClients.end()) {
            delete mClients[tcpID];
            mClients.erase(tcpID);
        }
    }

    /**
     * 服务器端的客户端在accept的时候会调用这个函数.
     *
     * @author daixian
     * @date 2021/1/9
     *
     * @param  uuid The uuid.
     */
    int RegisterClientWithUUID(const std::string& uuid, int tempTcpID)
    {
        TCPClient* client = mClients[tempTcpID];
        if (mAcceptClients.find(uuid) != mAcceptClients.end()) {

            TCPClient* oldclient = mAcceptClients[uuid];
            int tcpID = oldclient->TcpID();

            client->MoveKCPClient(oldclient); //移动也就是继承kcp部分

            mAcceptClients[uuid] = client;

            mClients[tcpID] = client;        //写到原先的位置
            mClients.erase(client->TcpID()); //移除新的tcpID记录
            client->SetTcpID(tcpID);

            delete oldclient;
            LogI("ClientManager.RegisterClientWithUUID():找到了之前的记录,断线重连~");
        }
        else {
            //如果找不到以前的记录
            mAcceptClients[uuid] = client;
        }

        LogI("ClientManager.RegisterClientWithUUID():当前的连接的客户端个数%d,其中通过Accept的个数%d", mClients.size(), mAcceptClients.size());

        //发出事件
        if (eventAccept != nullptr) {
            eventAccept->notify(client, TCPEventAccept(client->TcpID(), client->AcceptData()));
        }

        return client->TcpID();
    }

    /**
     * Searches for the first client with uuid
     *
     * @author daixian
     * @date 2021/1/9
     *
     * @param  uuid The uuid.
     *
     * @returns Null if it fails, else the found client with uuid.
     */
    TCPClient* FindClientWithUUID(const std::string& uuid)
    {
        if (mAcceptClients.find(uuid) != mAcceptClients.end()) {
            return mAcceptClients[uuid];
        }
        return nullptr;
    }

    /**
     * 根据tcpID删除两个map中的记录.
     *
     * @author daixian
     * @date 2021/1/11
     *
     * @param  tcpID Identifier for the TCP.
     *
     * @returns Null if it fails, else a pointer to a TCPClient.
     */
    TCPClient* Erase(int tcpID)
    {
        if (mClients.find(tcpID) != mClients.end()) {
            TCPClient* client = mClients[tcpID];
            if (client->AcceptData() != nullptr) {
                std::string uuid = client->AcceptData()->uuidC;
                if (mAcceptClients.find(uuid) != mAcceptClients.end()) {
                    mAcceptClients.erase(uuid);
                }
            }
            mClients.erase(tcpID);
        }
    }

    /**
     * 关闭所有客户端并且重置.
     *
     * @author daixian
     * @date 2020/12/23
     */
    void Clear()
    {
        for (auto& kvp : mClients) {
            delete kvp.second;
        }
        mClients.clear();
        _clientCount = 1;

        mAcceptClients.clear();
    }

    /************************** kcp ***********************************/

    // tcp监听端口开的同端口UDP
    Poco::Net::DatagramSocket* acceptUDPSocket = nullptr;

    // 接收用的buffer
    std::vector<char> receBuffUDP;

    // 当前接收长度
    int receLen = 0;

    // KCP的接收
    void KCPSocketReceive()
    {
        try { //socket尝试接收
            Poco::Net::SocketAddress remote(Poco::Net::AddressFamily::IPv4);
            receLen = acceptUDPSocket->receiveFrom(receBuffUDP.data(), (int)receBuffUDP.size(), remote);
        }
        catch (const Poco::Exception& e) {
            LogE("TCPClient.InitUDPSocket():异常e=%s,%s", e.what(), e.message().c_str());
        }
        catch (const std::exception& e) {
            LogE("TCPClient.InitUDPSocket():异常e=%s", e.what());
        }
    }

  private:
    // 用于分配tcpID,id从1开始,0保留
    int _clientCount = 1;
};

} // namespace dxlib