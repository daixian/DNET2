#include "gtest/gtest.h"
#include "DNET/KCP.h"
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

using namespace dxlib;
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

TEST(TCPServer, sendBytes)
{
    TCPServer server("server", "0.0.0.0", 8341);
    server.Start();
    while (!server.IsStarted()) {
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    TCPClient client;
    client.Connect("127.0.0.1", 8341);

    //this_thread::sleep_for(std::chrono::seconds(1));

    std::string msg = "1234567890";
    int res = client.Send(msg.c_str(), msg.size());

    {
        std::map<int, std::vector<std::vector<char>>> msgs;
        server.WaitAvailable(0); //只有一个客户端,那么id应该为0
        server.Receive(msgs);
        if (!msg.empty())
            for (auto& kvp : msgs) {
                ASSERT_EQ(kvp.second.size(), 1);
                ASSERT_EQ(kvp.second[0].size(), msg.size());
                LogI("服务器收到了客户端数据!");
                //回发这条数据
                server.Send(kvp.first, kvp.second[0].data(), kvp.second[0].size());
            }
    }

    {
        std::vector<std::vector<char>> msgs;
        client.WaitAvailable();
        client.Receive(msgs);
        if (!msgs.empty()) {
            ASSERT_EQ(msgs.size(), 1);
            ASSERT_EQ(msgs[0].size(), msg.size());
            LogI("客户端收到了服务器数据!");
        }
    }

    server.Close();
}

TEST(TCPServer, sendBytes_localhost)
{
    TCPServer server("server", "localhost", 8341);
    server.Start();
    while (!server.IsStarted()) {
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    TCPClient client;
    client.Connect("localhost", 8341);

    //this_thread::sleep_for(std::chrono::seconds(1));

    std::string msg = "1234567890";
    int res = client.Send(msg.c_str(), msg.size());

    {
        std::map<int, std::vector<std::vector<char>>> msgs;
        server.WaitAvailable(0); //只有一个客户端,那么id应该为0
        server.Receive(msgs);
        if (!msg.empty())
            for (auto& kvp : msgs) {
                ASSERT_EQ(kvp.second.size(), 1);
                ASSERT_EQ(kvp.second[0].size(), msg.size());
                LogI("服务器收到了客户端数据!");
                //回发这条数据
                server.Send(kvp.first, kvp.second[0].data(), kvp.second[0].size());
            }
    }

    {
        std::vector<std::vector<char>> msgs;
        client.WaitAvailable();
        client.Receive(msgs);
        if (!msgs.empty()) {
            ASSERT_EQ(msgs.size(), 1);
            ASSERT_EQ(msgs[0].size(), msg.size());
            LogI("客户端收到了服务器数据!");
        }
    }

    server.Close();
}

TEST(TCPServer, sendText)
{
    TCPServer server("server", "127.0.0.1", 8341);
    server.Start();
    while (!server.IsStarted()) {
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    TCPClient client;
    client.Connect("127.0.0.1", 8341);

    std::string msg = "1234567890";
    int res = client.Send(msg.c_str(), msg.size());

    std::map<int, std::vector<std::string>> msgs;
    server.WaitAvailable(0); //只有一个客户端,那么id应该为0
    server.Receive(msgs);
    if (!msg.empty())
        for (auto& kvp : msgs) {
            ASSERT_EQ(kvp.second.size(), 1);
            ASSERT_EQ(kvp.second[0], msg);
            LogI("服务器收到了客户端数据!");
            //回发这条数据
            server.Send(kvp.first, kvp.second[0].data(), kvp.second[0].size());
        }

    std::vector<std::string> clienMsgs;
    client.WaitAvailable();
    client.Receive(clienMsgs);
    if (!clienMsgs.empty()) {
        ASSERT_EQ(clienMsgs.size(), 1);
        ASSERT_EQ(clienMsgs[0], msg);
        LogI("客户端收到了服务器数据!");
    }

    server.Close();
}

TEST(TCPServer, sendText_128Client)
{
    TCPServer server("server", "127.0.0.1", 8341);
    server.Start();
    while (!server.IsStarted()) {
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::vector<TCPClient> clients;
    clients.resize(128);
    for (size_t i = 0; i < clients.size(); i++) {
        clients[i].Connect("127.0.0.1", 8341);
    }

    std::string msg = "1234567890";
    for (size_t i = 0; i < clients.size(); i++) {
        int res = clients[i].Send(msg.c_str(), msg.size());
    }

    //服务器接收
    {
        for (size_t i = 0; i < clients.size(); i++) {
            server.WaitAvailable(i); //等待所有的客户端...
        }
        std::map<int, std::vector<std::string>> msgs;
        server.Receive(msgs);
        if (!msg.empty()) {
            for (auto& kvp : msgs) {
                ASSERT_EQ(kvp.second.size(), 1);
                ASSERT_EQ(kvp.second[0], msg);
                LogI("服务器收到了客户端%d数据!", kvp.first);
                //回发这条数据
                server.Send(kvp.first, kvp.second[0].data(), kvp.second[0].size());
            }
        }
    }

    //客户端接收
    {
        for (size_t i = 0; i < clients.size(); i++) {
            std::vector<std::string> msgs;
            clients[i].WaitAvailable();
            clients[i].Receive(msgs);
            if (!msgs.empty()) {
                ASSERT_EQ(msgs.size(), 1);
                ASSERT_EQ(msgs[0], msg);
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
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    TCPClient client;
    client.Connect("127.0.0.1", 8341);

    std::string msg = "1234567890";
    int res = client.Send(msg.c_str(), msg.size());
    std::string msg2 = "abcdefghijklmn";
    res = client.Send(msg2.c_str(), msg2.size());

    {
        int receCount = 0;
        while (receCount < 2) {
            std::map<int, std::vector<std::string>> msgs;
            server.WaitAvailable(0); //只有一个客户端,那么id应该为0
            server.Receive(msgs);
            if (!msg.empty())
                for (auto& kvp : msgs) {
                    receCount += kvp.second.size();

                    LogI("服务器收到了客户端%d数据!", kvp.second.size());

                    ASSERT_EQ(kvp.second.size(), 2); //一次应该收到了两条数据
                    ASSERT_EQ(kvp.second[0], msg);
                    ASSERT_EQ(kvp.second[1], msg2);

                    //回发这条数据
                    server.Send(kvp.first, kvp.second[0].data(), kvp.second[0].size());
                    server.Send(kvp.first, kvp.second[1].data(), kvp.second[1].size());
                }
        }
    }

    std::vector<std::string> clienMsgs;
    client.WaitAvailable();
    client.Receive(clienMsgs);
    if (!clienMsgs.empty()) {
        LogI("客户端收到了服务器数据%d条!", clienMsgs.size());

        ASSERT_EQ(clienMsgs.size(), 2);
        ASSERT_EQ(clienMsgs[0], msg);
        ASSERT_EQ(clienMsgs[1], msg2);
    }

    server.Close();
}