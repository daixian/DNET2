#include "DNET.h"

//所有创建出来的服务器
std::vector<dxlib::KCPServer*> vServer;
std::vector<dxlib::KCPClient*> vClient;

dxlib::KCPServer* findServer(dxlib::KCPServer* ptr)
{
    for (size_t i = 0; i < vServer.size(); i++) {
        if (vServer[i] == ptr) {
            return vServer[i];
        }
    }
    return nullptr;
}

void deleteServer(dxlib::KCPServer* ptr)
{
    for (auto itr = vServer.begin(); itr != vServer.end();) {
        if (*itr == ptr) {
            itr = vServer.erase(itr);
        }
        else {
            itr++;
        }
    }
}

dxlib::KCPClient* findClient(dxlib::KCPClient* ptr)
{
    for (size_t i = 0; i < vClient.size(); i++) {
        if (vClient[i] == ptr) {
            return vClient[i];
        }
    }
    return nullptr;
}

void deleteClient(dxlib::KCPClient* ptr)
{
    for (auto itr = vClient.begin(); itr != vClient.end();) {
        if (*itr == ptr) {
            itr = vClient.erase(itr);
        }
        else {
            itr++;
        }
    }
}

/**
 * u3d设置一个字符串消息的回调函数进来.
 *
 * @author daixian
 * @date 2020/8/22
 *
 * @param [in] server 绑定的服务器.
 * @param      proc   u3d传过来的回调函数指针.
 */
DNET_EXPORT DNetError __cdecl dnServerSetMessageProc(dxlib::KCPServer* server, MessageProcCallback proc)
{
    server->user = proc;
    return DNetError::Ok;
}

/**
 * 创建服务器端.
 *
 * @author daixian
 * @date 2018/4/22
 *
 * @param       name   服务器端的友好名.
 * @param       host   The host.
 * @param       port   The port.
 * @param [out] server 创建的结果指针.
 *
 * @returns 成功返回0.
 */
DNET_EXPORT DNetError __cdecl dnServerCreate(const char* name, const char* host, int port, dxlib::KCPServer*& server)
{
    try {
        server = new dxlib::KCPServer(name, host, port);

        vServer.push_back(server); //记录这个服务器
        return DNetError::Ok;
    }
    catch (const std::exception&) {
        return DNetError::Unknown;
    }
}

/**
 * 服务器启动.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] server If non-null, the server.
 * @param [in] host   If non-null, the host.
 * @param      port   The port.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __cdecl dnServerStart(dxlib::KCPServer* server)
{
    dxlib::KCPServer* ptr = findServer(server);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }

    try {
        ptr->Start();
        return DNetError::Ok;
    }
    catch (const std::exception&) {
        return DNetError::Unknown;
    }
}

/**
 * 服务器关闭.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] server If non-null, the server.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __cdecl dnServerClose(dxlib::KCPServer* server)
{
    dxlib::KCPServer* ptr = findServer(server);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }
    try {
        ptr->Close();
        deleteServer(ptr);
        delete ptr;
        return DNetError::Ok;
    }
    catch (const std::exception&) {
        return DNetError::Unknown;
    }
}

/**
 * 服务器Update,实际上就是接收.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] server If non-null, the server.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __cdecl dnServerUpdate(dxlib::KCPServer* server)
{
    dxlib::KCPServer* ptr = findServer(server);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }

    //处理回调就绑定在对象上
    MessageProcCallback serverMsgProc = (MessageProcCallback)(server->user);

    std::map<int, std::vector<std::string>> msgs;
    int count = ptr->Receive(msgs);
    for (auto& kvp : msgs) {
        int id = kvp.first;
        std::vector<std::string>& userMsgs = kvp.second;
        for (size_t i = 0; i < userMsgs.size(); i++) {

            try {
                // 执行消息处理
                serverMsgProc(server, id, userMsgs[i].c_str());
            }
            catch (const std::exception&) {
            }
        }
    }

    return DNetError::Ok;
}

/**
 * 向某个客户端发送数据.
 *
 * @author daixian
 * @date 2021/1/7
 *
 * @param [in,out] server If non-null, the server.
 * @param          id     The identifier.
 * @param          msg    The message.
 * @param          len    The length.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __cdecl dnServerSend(dxlib::KCPServer* server, int id, const char* msg, int len)
{
    int res = server->Send(id, msg, len);
    if (res > 0) {
        return DNetError::Ok;
    }
    else {
        return DNetError::OperationFailed;
    }
}

/**
 * u3d设置一个字符串消息的回调函数进来.
 *
 * @author daixian
 * @date 2020/8/22
 *
 * @param [in] client 绑定的客户端.
 * @param       proc   u3d传过来的回调函数指针.
 */
DNET_EXPORT DNetError __cdecl dnClientSetMessageProc(dxlib::KCPClient* client, MessageProcCallback proc)
{
    client->user = proc;
    return DNetError::Ok;
}

/**
 * 创建客户端.
 *
 * @author daixian
 * @date 2018/4/22
 *
 * @param [in]  name   If non-null, the name.
 * @param [out] client [in,out] If non-null, the client.
 *
 * @returns 成功返回0.
 */
DNET_EXPORT DNetError __cdecl dnClientCreate(char* name, dxlib::KCPClient*& client)
{
    try {
        client = new dxlib::KCPClient(name);

        vClient.push_back(client); //记录这个服务器
        return DNetError::Ok;
    }
    catch (const std::exception&) {
        return DNetError::Unknown;
    }
}

/**
 * 客户端连接服务器.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] client If non-null, the server.
 * @param [in] host   If non-null, the host.
 * @param      port   The port.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __cdecl dnClientConnect(dxlib::KCPClient* client, char* host, int port)
{
    dxlib::KCPClient* ptr = findClient(client);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }

    try {
        ptr->Connect(host, port);
        return DNetError::Ok;
    }
    catch (const std::exception&) {
        return DNetError::Unknown;
    }
}

/**
 * 客户端关闭.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] client If non-null, the server.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __cdecl dnClientClose(dxlib::KCPClient* client)
{
    dxlib::KCPClient* ptr = findClient(client);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }

    try {
        ptr->Close();
        deleteClient(ptr);
        delete ptr;
        return DNetError::Ok;
    }
    catch (const std::exception&) {
        return DNetError::Unknown;
    }
}

/**
 * 客户端Update,实际上就是接收.
 *
 * @author daixian
 * @date 2020/12/31
 *
 * @param [in] client If non-null, the server.
 *
 * @returns An int.
 */
DNET_EXPORT DNetError __cdecl dnClientUpdate(dxlib::KCPClient* client)
{
    dxlib::KCPClient* ptr = findClient(client);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }

    //处理回调就绑定在对象上
    MessageProcCallback clientMsgProc = (MessageProcCallback)(client->user);

    std::vector<std::string> msgs;
    int count = ptr->Receive(msgs);

    int id = 0; //客户端就一直处理id为0吧
    for (size_t i = 0; i < msgs.size(); i++) {

        try {
            // 执行消息处理
            clientMsgProc(client, id, msgs[i].c_str());
        }
        catch (const std::exception&) {
        }
    }

    return DNetError::Ok;
}

/**
 * 向服务器端发送数据.
 *
 * @author daixian
 * @date 2021/1/7
 *
 * @param [in,out] client If non-null, the server.
 * @param          msg    The message.
 * @param          len    The length.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __cdecl dnClientSend(dxlib::KCPClient* client, const char* msg, int len)
{
    int res = client->Send(msg, len);
    if (res > 0) {
        return DNetError::Ok;
    }
    else {
        return DNetError::OperationFailed;
    }
}