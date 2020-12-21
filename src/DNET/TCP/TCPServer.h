#pragma once

#include <string>
#include <vector>
#include <map>

#include "Poco/BasicEvent.h"
#include "Poco/Delegate.h"

#include "TCPEvent.h"

namespace dxlib {

class TCPServer
{
  public:
    TCPServer(const std::string& name,
              const std::string& host, int port);
    ~TCPServer();

    // 自己的服务器名字
    std::string name;

    // 主机.
    std::string host;

    // 端口.
    int port = -1;

    /**
     * 启动监听.
     *
     * @author daixian
     * @date 2020/12/21
     */
    void Start();

    /**
     * 得到Accept的事件.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @returns A reference to a Poco::BasicEvent<TCPEvent>
     */
    Poco::BasicEvent<TCPEventAccept>& EventAccept();

  private:
    class Impl;
    Impl* _impl;
};

} // namespace dxlib