#include "gtest/gtest.h"

#include "DNET/TCP/TCPServer.h"
#include "DNET/TCP/TCPClient.h"
#include "DNET/TCP/KCPClient.h"

#include <thread>
#include "dlog/dlog.h"
#include <atomic>
#include <Poco/Net/ServerSocket.h>

using namespace dnet;
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

    //KCPClient* kcp_s = (KCPClient*)server.ge;
    //KCPClient* kcp_c = (KCPClient*)client.GetKCPClient();

    while (true) {
        //接收驱动
        std::map<int, std::vector<TextMessage>> smsgs;
        server.KCPReceive(smsgs);

        std::vector<TextMessage> cmsg;
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

    int successCount = 0;
    while (true) {
        //接收驱动
        std::map<int, std::vector<TextMessage>> smsgs;
        server.KCPReceive(smsgs);

        std::vector<TextMessage> cmsg;
        client.KCPReceive(cmsg);

        if (!smsgs.empty()) {
            ASSERT_EQ(smsgs.size(), 1);
            ASSERT_TRUE(smsgs[1].size() > 0); //1号客户端的消息一条(现在是32条)
            ASSERT_EQ(smsgs[1][0].data, "123456");
            successCount++;
            LogI("successCount=%d", successCount);
            if (successCount > 200) {
                break;
            }
        }

        if (!cmsg.empty()) {
            ASSERT_TRUE(cmsg.size() > 0);
            ASSERT_EQ(cmsg[0].data, "abcdefg");
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
