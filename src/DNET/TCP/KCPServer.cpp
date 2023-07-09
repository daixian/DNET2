#include "KCPServer.h"

namespace dnet {

KCPServer::KCPServer(const std::string& name) : name(name)
{
    socketRecebuff.resize(4 * 1024, 0);
}

KCPServer::~KCPServer()
{
    Close();

    for (auto& kvp : mChannel) {
        delete kvp.second;
    }
}

} // namespace dnet