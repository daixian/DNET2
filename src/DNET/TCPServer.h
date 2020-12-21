#pragma once

#include <string>
#include <vector>
#include <map>

namespace dxlib {

class TCPServer
{
  public:
    TCPServer(const std::string& name,
              const std::string& host, int port);
    ~TCPServer();

    /**
     * 启动监听.
     *
     * @author daixian
     * @date 2020/12/21
     */
    void Start();

  private:
    class Impl;
    Impl* _impl;
};

} // namespace dxlib