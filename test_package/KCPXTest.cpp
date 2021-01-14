#include "gtest/gtest.h"

#include "DNET/TCP/TCPServer.h"
#include "DNET/TCP/TCPClient.h"
#include "DNET/TCP/KCPClient.h"

#include <thread>
#include "dlog/dlog.h"
#include <atomic>
#include <Poco/Net/ServerSocket.h>

using namespace dxlib;
using namespace std;

TEST(KCPServer, Accept)
{
    dlog_set_console_thr(dlog_level::debug);

    TCPServer server("server", "0.0.0.0", 8341);
    server.Start();
    server.WaitStarted();

    TCPClient client;
    client.Connect("127.0.0.1", 8341);

    //等待所有客户端连接完成
    while (true) {
        std::map<int, std::vector<std::string>> msgs;
        server.Receive(msgs); //无脑调用接收

        int acceptCount = 0;

        std::vector<std::string> cmsgs;
        client.Receive(cmsgs); //无脑调用接收
        if (client.IsAccepted()) {
            break;
        }
    }
    //服务端的应该也已经认证通过了
    for (auto& kvp : server.GetRemotes()) {
        ASSERT_TRUE(kvp.second->IsAccepted());
    }

    //KCPClient* kcp_s = (KCPClient*)server.ge;
    //KCPClient* kcp_c = (KCPClient*)client.GetKCPClient();

    while (true) {
        //接收驱动
        std::map<int, std::vector<std::string>> smsgs;
        server.KCPReceive(smsgs);

        std::vector<std::string> cmsg;
        client.KCPReceive(cmsg);

        std::this_thread::yield();

        break;
    }
}

TEST(KCPServer, SendRece)
{
    dlog_set_console_thr(dlog_level::debug);

    TCPServer server("server", "0.0.0.0", 8341);
    server.Start();
    server.WaitStarted();

    TCPClient client;
    client.Connect("127.0.0.1", 8341);

    //等待所有客户端连接完成
    while (true) {
        std::map<int, std::vector<std::string>> msgs;
        server.Receive(msgs); //无脑调用接收

        int acceptCount = 0;

        std::vector<std::string> cmsgs;
        client.Receive(cmsgs); //无脑调用接收
        if (client.IsAccepted()) {
            break;
        }
    }
    //服务端的应该也已经认证通过了
    for (auto& kvp : server.GetRemotes()) {
        ASSERT_TRUE(kvp.second->IsAccepted());
    }

    int successCount = 0;
    while (true) {
        //接收驱动
        std::map<int, std::vector<std::string>> smsgs;
        server.KCPReceive(smsgs);

        std::vector<std::string> cmsg;
        client.KCPReceive(cmsg);

        if (!smsgs.empty()) {
            ASSERT_EQ(smsgs.size(), 1);
            ASSERT_TRUE(smsgs[1].size() > 0); //1号客户端的消息一条(现在是32条)
            ASSERT_EQ(smsgs[1][0], "123456");
            successCount++;
            LogI("successCount=%d", successCount);
            if (successCount > 200) {
                break;
            }
        }

        if (!cmsg.empty()) {
            ASSERT_TRUE(cmsg.size() > 0);
            ASSERT_EQ(cmsg[0], "abcdefg");
            successCount++;
            LogI("successCount=%d", successCount);
            if (successCount > 200) {
                break;
            }
        }

        std::this_thread::yield();

        client.KCPSend("123456", 6);     //c->s
        server.KCPSend(1, "abcdefg", 7); //s->c
    }
}

//TEST(KCPServer, SendRece)
//{
//    dlog_set_console_thr(dlog_level::debug);
//
//    KCPServer kcp_s("server", "127.0.0.1", 9991);
//    kcp_s.Start();
//
//    KCPClient kcp_c("client");
//    kcp_c.Connect("127.0.0.1", 9991);
//
//    int successCount = 0;
//    while (true) {
//        //接收驱动
//        std::map<int, std::vector<std::string>> msgs;
//        kcp_s.Receive(msgs);
//
//        std::vector<std::string> cmsg;
//        kcp_c.Receive(cmsg);
//
//        if (!msgs.empty()) {
//            ASSERT_EQ(msgs.size(), 1);
//            ASSERT_EQ(msgs[1].size(), 1); //1号客户端的消息一条
//            ASSERT_EQ(msgs[1][0], "123456");
//            successCount++;
//            LogI("successCount=%d", successCount);
//            if (successCount > 200) {
//                break;
//            }
//        }
//
//        if (!cmsg.empty()) {
//            ASSERT_EQ(cmsg.size(), 1);
//            ASSERT_EQ(cmsg[0], "abcdefg");
//            successCount++;
//            LogI("successCount=%d", successCount);
//            if (successCount > 200) {
//                break;
//            }
//        }
//
//        std::this_thread::yield();
//        if (kcp_s.RemoteCount() == 1 &&
//            kcp_c.isAccepted()) {
//            kcp_c.Send("123456", 6);     //c->s
//            kcp_s.Send(1, "abcdefg", 7); //s->c
//        }
//    }
//}