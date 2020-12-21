#pragma once

#include <string>
#include <vector>
#include <map>
#include "Poco/Net/StreamSocket.h"

namespace dxlib {

class ClientManager
{
  public:
    ClientManager() {}
    ~ClientManager() {}

    // 所有连接了的客户端.
    std::map<int, Poco::Net::StreamSocket> mClients;

    /**
     * 添加一个客户端进来记录.
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
        mClients[_clientCount] = client;
        _clientCount++; //永远递增
        return _clientCount - 1;
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
    Poco::Net::StreamSocket* GetClient(int tcpID)
    {
        if (mClients.find(tcpID) != mClients.end()) {
            return &mClients[tcpID];
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
            mClients.erase(tcpID);
        }
    }

  private:
    int _clientCount = 0;
};

} // namespace dxlib