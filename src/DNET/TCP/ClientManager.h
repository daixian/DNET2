#pragma once

#include <string>
#include <vector>
#include <map>
#include "TCPClient.h"

#include "Poco/Net/StreamSocket.h"
#include "dlog/dlog.h"

namespace dxlib {

class ClientManager
{
  public:
    ClientManager() {}
    ~ClientManager() {}

    // 所有连接了的客户端.
    std::map<int, TCPClient*> mClients;

    // 有了uuid返回的及客户端记录
    std::map<std::string, TCPClient*> mAcceptClients;

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
        TCPClient* tcobj = new TCPClient();
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
     * Registers the client with uuid described by uuid
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
            delete oldclient;
            mAcceptClients[uuid] = client;

            mClients[tcpID] = client;
            mClients.erase(client->TcpID()); //移除新的tcpID
            client->SetTcpID(tcpID);
            LogI("ClientManager.RegisterClientWithUUID():找到了之前的记录,断线重连~");
        }
        else {
            //如果找不到以前的记录
            mAcceptClients[uuid] = client;
        }

        poco_assert(mClients.size() == mAcceptClients.size());
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

  private:
    // 用于分配tcpID,id从1开始,0保留
    int _clientCount = 1;
};

} // namespace dxlib