#include "DNET.h"

#define DLOG_DLL_EXPORTS
#include "dlog/dlog.h"

//所有创建出来的服务器
std::vector<dxlib::TCPServer*> vServer;
std::vector<dxlib::TCPClient*> vClient;

class User
{
  public:
    User() {}
    ~User() {}

    MessageProcCallback tcpMessageProc = nullptr;

    MessageProcCallback kcpMessageProc = nullptr;
};

dxlib::TCPServer* findServer(dxlib::TCPServer* ptr)
{
    for (size_t i = 0; i < vServer.size(); i++) {
        if (vServer[i] == ptr) {
            return vServer[i];
        }
    }
    return nullptr;
}

void deleteServer(dxlib::TCPServer* ptr)
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

dxlib::TCPClient* findClient(dxlib::TCPClient* ptr)
{
    for (size_t i = 0; i < vClient.size(); i++) {
        if (vClient[i] == ptr) {
            return vClient[i];
        }
    }
    return nullptr;
}

void deleteClient(dxlib::TCPClient* ptr)
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

DNET_EXPORT DNetError __cdecl dnLogInit(const char* appName)
{
    dlog_init("log", appName, dlog_init_relative::APPDATA);
    return DNetError::Ok;
}

/**
 * u3d设置一个字符串消息的回调函数进来.
 *
 * @author daixian
 * @date 2020/8/22
 *
 * @param [in] server  绑定的服务器.
 * @param      tcpProc u3d传过来的tcp回调函数指针.
 * @param      kcpProc u3d传过来的kcp回调函数指针.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __cdecl dnServerSetMessageProc(dxlib::TCPServer* server, MessageProcCallback tcpProc, MessageProcCallback kcpProc)
{
    User* user = (User*)server->user;
    user->tcpMessageProc = tcpProc;
    user->kcpMessageProc = kcpProc;
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
DNET_EXPORT DNetError __cdecl dnServerCreate(const char* name, const char* host, int port, dxlib::TCPServer*& server)
{
    try {
        server = new dxlib::TCPServer(name, host, port);
        server->user = new User();
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
DNET_EXPORT DNetError __cdecl dnServerStart(dxlib::TCPServer* server)
{
    dxlib::TCPServer* ptr = findServer(server);
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
DNET_EXPORT DNetError __cdecl dnServerClose(dxlib::TCPServer* server)
{
    dxlib::TCPServer* ptr = findServer(server);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }
    try {
        ptr->Close();
        delete ptr->user;
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
DNET_EXPORT DNetError __cdecl dnServerUpdate(dxlib::TCPServer* server)
{
    dxlib::TCPServer* ptr = findServer(server);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }

    //处理回调就绑定在对象上
    User* user = (User*)server->user;

    std::map<int, std::vector<std::string>> msgs;
    int count = ptr->Receive(msgs);
    for (auto& kvp : msgs) {
        int id = kvp.first;
        std::vector<std::string>& userMsgs = kvp.second;
        for (size_t i = 0; i < userMsgs.size(); i++) {

            try {
                //执行tcp消息处理
                if (user->tcpMessageProc != nullptr)
                    user->tcpMessageProc(server, id, userMsgs[i].c_str());
            }
            catch (const std::exception&) {
            }
        }
    }

    msgs.clear();
    count = ptr->KCPReceive(msgs);
    for (auto& kvp : msgs) {
        int id = kvp.first;
        std::vector<std::string>& userMsgs = kvp.second;
        for (size_t i = 0; i < userMsgs.size(); i++) {

            try {
                //执行kcp消息处理
                if (user->kcpMessageProc != nullptr)
                    user->kcpMessageProc(server, id, userMsgs[i].c_str());
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
DNET_EXPORT DNetError __cdecl dnServerSend(dxlib::TCPServer* server, int id, const char* msg, int len)
{
    dxlib::TCPServer* ptr = findServer(server);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }
    int res = ptr->Send(id, msg, len);
    if (res > 0) {
        return DNetError::Ok;
    }
    else {
        return DNetError::OperationFailed;
    }
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
DNET_EXPORT DNetError __cdecl dnServerKCPSend(dxlib::TCPServer* server, int id, const char* msg, int len)
{
    dxlib::TCPServer* ptr = findServer(server);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }
    int res = ptr->KCPSend(id, msg, len);
    if (res > 0) {
        return DNetError::Ok;
    }
    else {
        return DNetError::OperationFailed;
    }
}

//----------------------------------------------------------- 客户端 -----------------------------------------------------------

/**
 * u3d设置一个字符串消息的回调函数进来.
 *
 * @author daixian
 * @date 2020/8/22
 *
 * @param [in] client  绑定的客户端.
 * @param      tcpProc u3d传过来的回调函数指针.
 * @param      kcpProc The kcp procedure.
 *
 * @returns A DNetError.
 */
DNET_EXPORT DNetError __cdecl dnClientSetMessageProc(dxlib::TCPClient* client, MessageProcCallback tcpProc, MessageProcCallback kcpProc)
{
    User* user = (User*)client->user;
    user->tcpMessageProc = tcpProc;
    user->kcpMessageProc = kcpProc;
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
DNET_EXPORT DNetError __cdecl dnClientCreate(char* name, dxlib::TCPClient*& client)
{
    try {
        client = new dxlib::TCPClient(name);
        client->user = new User();
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
DNET_EXPORT DNetError __cdecl dnClientConnect(dxlib::TCPClient* client, char* host, int port)
{
    dxlib::TCPClient* ptr = findClient(client);
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
DNET_EXPORT DNetError __cdecl dnClientClose(dxlib::TCPClient* client)
{
    dxlib::TCPClient* ptr = findClient(client);
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
DNET_EXPORT DNetError __cdecl dnClientUpdate(dxlib::TCPClient* client)
{
    dxlib::TCPClient* ptr = findClient(client);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }

    //处理回调就绑定在对象上
    User* user = (User*)client->user;

    std::vector<std::string> msgs;
    int count = ptr->Receive(msgs);

    int id = 0; //客户端就一直处理id为0吧
    for (size_t i = 0; i < msgs.size(); i++) {

        try {
            // 执行消息处理
            if (user->tcpMessageProc != nullptr)
                user->tcpMessageProc(client, id, msgs[i].c_str());
        }
        catch (const std::exception&) {
        }
    }

    msgs.clear();
    count = ptr->KCPReceive(msgs);
    for (size_t i = 0; i < msgs.size(); i++) {

        try {
            // 执行消息处理
            if (user->kcpMessageProc != nullptr)
                user->kcpMessageProc(client, id, msgs[i].c_str());
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
DNET_EXPORT DNetError __cdecl dnClientSend(dxlib::TCPClient* client, const char* msg, int len)
{
    dxlib::TCPClient* ptr = findClient(client);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }
    int res = ptr->Send(msg, len);
    if (res > 0) {
        return DNetError::Ok;
    }
    else {
        return DNetError::OperationFailed;
    }
}

/**
 * 向服务器端发送KCP数据.
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
DNET_EXPORT DNetError __cdecl dnClientKCPSend(dxlib::TCPClient* client, const char* msg, int len)
{
    dxlib::TCPClient* ptr = findClient(client);
    if (ptr == nullptr) {
        return DNetError::InvalidContext;
    }
    int res = ptr->KCPSend(msg, len);
    if (res > 0) {
        return DNetError::Ok;
    }
    else {
        return DNetError::OperationFailed;
    }
}