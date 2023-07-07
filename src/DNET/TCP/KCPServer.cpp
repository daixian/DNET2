#include "KCPServer.h"

namespace dnet {
KCPServer::KCPServer()
{
    socketRecebuff.resize(4 * 1024);
}

KCPServer::~KCPServer()
{
    Close();
}

} // namespace dnet