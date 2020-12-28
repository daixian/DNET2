﻿#pragma once

#include <string>
#include <vector>
#include <map>
#include "TCPClient.h"

#include "Poco/Net/StreamSocket.h"

namespace dxlib {

class ClientManager
{
  public:
    ClientManager() {}
    ~ClientManager() {}

    // 所有连接了的客户端.
    std::map<int, TCPClient*> mClients;

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
    int AddClient(Poco::Net::StreamSocket& client)
    {
        TCPClient* tcobj = new TCPClient();
        TCPClient::CreateWithServer(_clientCount, &client, *tcobj); //这个函数传入一个tcpID
        mClients[_clientCount] = tcobj;
        _clientCount++; //永远递增
        return tcobj->TcpID();
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
    }

  private:
    // 用于分配tcpID,id从1开始,0保留
    int _clientCount = 1;
};

} // namespace dxlib