#include "gtest/gtest.h"

#include "DNET/KCPClient.h"
#include <thread>
#include "dlog/dlog.h"
#include <atomic>
#include <Poco/Net/ServerSocket.h>

using namespace dxlib;
using namespace std;

//TEST(KCPServer, Accept)
//{
//    dlog_set_console_thr(dlog_level::debug);
//
//    KCPServer kcp_s("server", "127.0.0.1", 9991); // "localhost",localhost连接不上在cmd中ping localhost解析出来的是IPV6的::1
//    kcp_s.Start();
//
//    KCPClient kcp_c("client");
//    kcp_c.Connect("127.0.0.1", 9991);
//
//    while (true) {
//        //接收驱动
//        std::map<int, std::vector<std::string>> msgs;
//        kcp_s.Receive(msgs);
//
//        std::vector<std::string> cmsg;
//        kcp_c.Receive(cmsg);
//
//        std::this_thread::yield();
//        if (kcp_s.RemoteCount() == 1) {
//            auto remotes = kcp_s.GetRemotes();
//            ASSERT_EQ(remotes.size(), 1);
//            ASSERT_EQ(remotes[1], kcp_c.UUID()); //server端得到的远程名字为它的uuid
//
//            break;
//        }
//    }
//}

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