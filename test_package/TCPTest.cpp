#include "gtest/gtest.h"
#include "DNET/TCP/TCPClient.h"
#include "DNET/TCP/TCPServer.h"
#include <thread>
#include "dlog/dlog.h"
#include <atomic>
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/Socket.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/NetException.h"
#include "Poco/Timespan.h"
#include "Poco/Net/SocketStream.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Delegate.h"

using namespace dnet;
using namespace std;

using Poco::IOException;
using Poco::TimeoutException;
using Poco::Timespan;
using Poco::Net::ConnectionRefusedException;
using Poco::Net::InvalidSocketException;
using Poco::Net::NetException;
using Poco::Net::Socket;
using Poco::Net::SocketAddress;
using Poco::Net::StreamSocket;

// 这个确实可以连接上的
//TEST(StreamSocket, connect)
//{
//
//    Poco::Net::SocketAddress sa("127.0.0.1", 23333);
//    Poco::Net::StreamSocket socket;
//    try {
//        socket.connect(sa); //这个是阻塞的连接
//    }
//    catch (ConnectionRefusedException&) {
//        std::cout << "connect refuse" << std::endl;
//    }
//    catch (NetException&) {
//        std::cout << "net exception" << std::endl;
//    }
//    catch (TimeoutException&) {
//        std::cout << "connect time out" << std::endl;
//    }
//    //实验了这个连接是可以连接上的
//}

// 官方的例子
//TEST(ServerSocket, httpserver)
//{
//    Poco::Net::ServerSocket srv(8380); // does bind + listen
//    for (;;) {
//        Poco::Net::StreamSocket ss = srv.acceptConnection();
//        LogI("连接进来了一个客户端...");
//        Poco::Net::SocketStream str(ss);
//        str << "HTTP/1.0 200 OK\r\n"
//               "Content-Type: text/html\r\n"
//               "\r\n"
//               "<html><head><title>My 1st Web Server</title></head>"
//               "<body><h1>Hello, world!</h1></body></html>"
//            << std::flush;
//    }
//}

class Target
{
  public:
    std::atomic_int onEventAcceptCount = 0;
    std::atomic_int OnEventRemoteCloseCount = 0;
    void OnEventAccept(const void* pSender, TCPEventAccept& arg)
    {
        onEventAcceptCount++;
        LogI("Target.OnEventAccept(): onEvent %d !", arg.tcpID);
    }

    void OnEventRemoteClose(const void* pSender, TCPEventRemoteClose& arg)
    {
        OnEventRemoteCloseCount++;
        LogI("Target.OnEventRemoteCloseCount(): onEvent %d !", arg.tcpID);
    }
};

TEST(TCPServer, OpenClose)
{
    Target target;

    TCPServer server("server", "0.0.0.0", 8341);
    server.Start();
    server.EventAccept() += Poco::delegate(&target, &Target::OnEventAccept);
    server.EventRemoteClose() += Poco::delegate(&target, &Target::OnEventRemoteClose);
    server.WaitStarted();

    TCPClient client;
    client.Connect("127.0.0.1", 8341);

    while (server.RemoteCount() == 0 || !server.GetRemotes().begin()->second->IsAccepted()) {
        this_thread::sleep_for(std::chrono::milliseconds(10));
        std::map<int, std::vector<BinMessage>> msgs; //无脑调用接收
        server.Receive(msgs);

        std::vector<TextMessage> cmsgs;
        client.Receive(cmsgs); //无脑调用接收
    }
    ASSERT_TRUE(target.onEventAcceptCount == 1);

    client.Close();

    int tcpId = server.GetRemotes().begin()->first;
    {
        std::map<int, std::vector<BinMessage>> msgs;
        server.Receive(msgs);
        if (!msgs.empty())
            for (auto& kvp : msgs) {
                LogI("服务器收到了客户端数据!");
            }
    }

    //这个客户端应该会出错
    ASSERT_TRUE(server.GetRemotes().begin()->second->isError());

    //这里要100秒之后才实际关闭.已经产生了远程关闭事件
    ASSERT_TRUE(target.OnEventRemoteCloseCount == 0);

    server.Close();
}

TEST(TCPServer, sendBytes)
{
    TCPServer server("server", "0.0.0.0", 8341);
    server.Start();
    server.WaitStarted();

    TCPClient client;
    client.Connect("127.0.0.1", 8341);

    std::string msg = "1234567890";
    int res = client.Send(msg.c_str(), msg.size());

    int tcpId = server.GetRemotes().begin()->first;
    {
        std::map<int, std::vector<BinMessage>> msgs;
        server.WaitAvailable(tcpId); //只有一个客户端,那么id应该为0
        server.Receive(msgs);
        if (!msg.empty())
            for (auto& kvp : msgs) {
                ASSERT_EQ(kvp.second.size(), 1);
                ASSERT_EQ(kvp.second[0].data.size(), msg.size());
                LogI("服务器收到了客户端数据!");
                //回发这条数据
                server.Send(kvp.first, kvp.second[0].data.data(), kvp.second[0].data.size());
            }
    }

    {
        std::vector<BinMessage> msgs;
        client.WaitAvailable();
        client.Receive(msgs);
        if (!msgs.empty()) {
            ASSERT_TRUE(client.IsAccepted());
            ASSERT_EQ(msgs.size(), 1);
            ASSERT_EQ(msgs[0].data.size(), msg.size());
            LogI("客户端收到了服务器数据!");
        }
    }

    server.Close();
}

TEST(TCPServer, sendBytes_localhost)
{
    TCPServer server("server", "localhost", 8341);
    server.Start();
    server.WaitStarted();

    TCPClient client;
    client.Connect("localhost", 8341);

    std::string msg = "1234567890";
    int res = client.Send(msg.c_str(), msg.size());
    int tcpId = server.GetRemotes().begin()->first;

    {
        std::map<int, std::vector<BinMessage>> msgs;
        server.WaitAvailable(tcpId); //只有一个客户端,那么id应该为0
        server.Receive(msgs);
        if (!msg.empty())
            for (auto& kvp : msgs) {
                ASSERT_EQ(kvp.second.size(), 1);
                ASSERT_EQ(kvp.second[0].data.size(), msg.size());
                LogI("服务器收到了客户端数据!");
                //回发这条数据
                server.Send(kvp.first, kvp.second[0].data.data(), kvp.second[0].data.size());
            }
    }

    {
        std::vector<BinMessage> msgs;
        client.WaitAvailable();
        client.Receive(msgs);
        if (!msgs.empty()) {
            ASSERT_EQ(msgs.size(), 1);
            ASSERT_EQ(msgs[0].data.size(), msg.size());
            LogI("客户端收到了服务器数据!");
        }
    }

    server.Close();
}

TEST(TCPServer, sendText)
{
    TCPServer server("server", "127.0.0.1", 8341);
    server.Start();
    server.WaitStarted();

    TCPClient client;
    client.Connect("127.0.0.1", 8341);

    std::string msg = "1234567890";
    int res = client.Send(msg.c_str(), msg.size());
    int tcpId = server.GetRemotes().begin()->first;

    std::map<int, std::vector<TextMessage>> msgs;
    server.WaitAvailable(tcpId); //只有一个客户端,那么id应该为0
    server.Receive(msgs);
    if (!msg.empty())
        for (auto& kvp : msgs) {
            ASSERT_EQ(kvp.second.size(), 1);
            ASSERT_EQ(kvp.second[0].data, msg);
            LogI("服务器收到了客户端数据!");
            //回发这条数据
            server.Send(kvp.first, kvp.second[0].data.data(), kvp.second[0].data.size());
        }

    std::vector<TextMessage> clienMsgs;
    client.WaitAvailable();
    client.Receive(clienMsgs);
    if (!clienMsgs.empty()) {
        ASSERT_EQ(clienMsgs.size(), 1);
        ASSERT_EQ(clienMsgs[0].data, msg);
        LogI("客户端收到了服务器数据!");
    }

    server.Close();
}

TEST(TCPServer, sendText_128Client)
{
    TCPServer server("server", "127.0.0.1", 8341);
    server.Start();
    server.WaitStarted();

    std::vector<TCPClient> clients;
    clients.resize(128);
    for (size_t i = 0; i < clients.size(); i++) {
        clients[i].Connect("127.0.0.1", 8341);

        std::map<int, std::vector<TextMessage>> msgs;
        server.Receive(msgs); //无脑调用接收
    }

    //等待所有客户端连接完成
    while (true) {
        std::map<int, std::vector<TextMessage>> msgs;
        server.Receive(msgs); //无脑调用接收

        int acceptCount = 0;
        for (size_t i = 0; i < clients.size(); i++) {
            std::vector<TextMessage> cmsgs;
            clients[i].Receive(cmsgs); //无脑调用接收
            if (clients[i].IsAccepted()) {
                acceptCount++;
            }
        }
        if (acceptCount == clients.size()) {
            break;
        }
    }

    //服务端的应该也已经认证通过了
    for (auto& kvp : server.GetRemotes()) {
        ASSERT_TRUE(kvp.second->IsAccepted());
    }

    std::string msg = "1234567890";
    for (size_t i = 0; i < clients.size(); i++) {
        int res = clients[i].Send(msg.c_str(), msg.size());
    }

    //服务器接收
    {
        std::map<int, std::vector<TextMessage>> msgs;
        server.Receive(msgs);
        if (!msg.empty()) {
            for (auto& kvp : msgs) {
                ASSERT_EQ(kvp.second.size(), 1);
                ASSERT_EQ(kvp.second[0].data, msg);
                LogI("服务器收到了客户端%d数据!", kvp.first);
                //回发这条数据
                server.Send(kvp.first, kvp.second[0].data.data(), kvp.second[0].data.size());
            }
        }
    }

    //客户端接收
    {
        for (size_t i = 0; i < clients.size(); i++) {
            std::vector<TextMessage> msgs;
            clients[i].WaitAvailable();
            clients[i].Receive(msgs);
            if (!msgs.empty()) {
                ASSERT_EQ(msgs.size(), 1);
                ASSERT_EQ(msgs[0].data, msg);
                LogI("客户端%d收到了服务器数据!", i);
            }
        }
    }
    server.Close();
}

TEST(TCPServer, sendText2)
{
    TCPServer server("server", "127.0.0.1", 8341);
    server.Start();
    while (!server.IsStarted()) {
        this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    TCPClient client;
    client.Connect("127.0.0.1", 8341);

    //等待所有客户端连接完成
    while (true) {
        std::map<int, std::vector<TextMessage>> msgs;
        server.Receive(msgs); //无脑调用接收

        int acceptCount = 0;

        std::vector<TextMessage> cmsgs;
        client.Receive(cmsgs); //无脑调用接收
        if (client.IsAccepted()) {
            break;
        }
    }
    //服务端的应该也已经认证通过了
    for (auto& kvp : server.GetRemotes()) {
        ASSERT_TRUE(kvp.second->IsAccepted());
    }

    std::string msg = "1234567890";
    int res = client.Send(msg.c_str(), msg.size());
    std::string msg2 = "abcdefghijklmn";
    res = client.Send(msg2.c_str(), msg2.size());

    int tcpId = server.GetRemotes().begin()->first;
    {
        int receCount = 0;
        while (receCount < 2) {
            std::map<int, std::vector<TextMessage>> msgs;
            server.WaitAvailable(tcpId); //只有一个客户端,那么id应该为0
            server.Receive(msgs);
            if (!msg.empty())
                for (auto& kvp : msgs) {
                    receCount += kvp.second.size();

                    LogI("服务器收到了客户端%d数据!", kvp.second.size());

                    ASSERT_EQ(kvp.second.size(), 2); //一次应该收到了两条数据
                    ASSERT_EQ(kvp.second[0].data, msg);
                    ASSERT_EQ(kvp.second[1].data, msg2);

                    //回发这条数据
                    server.Send(kvp.first, kvp.second[0].data.data(), kvp.second[0].data.size());
                    server.Send(kvp.first, kvp.second[1].data.data(), kvp.second[1].data.size());
                }
        }
    }

    std::vector<TextMessage> clienMsgs;
    client.WaitAvailable();
    client.Receive(clienMsgs);
    if (!clienMsgs.empty()) {
        LogI("客户端收到了服务器数据%d条!", clienMsgs.size());

        ASSERT_EQ(clienMsgs.size(), 2);
        ASSERT_EQ(clienMsgs[0].data, msg);
        ASSERT_EQ(clienMsgs[1].data, msg2);
    }

    server.Close();
}