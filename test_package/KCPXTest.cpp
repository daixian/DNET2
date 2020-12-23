#include "gtest/gtest.h"
#include "DNET/KCPX.h"
#include <thread>
#include "dlog/dlog.h"
#include <atomic>

using namespace dxlib;
using namespace std;

TEST(KCPX, Accept)
{
    dlog_set_console_thr(dlog_level::debug);

    KCPX kcp_s("server", "127.0.0.1", 9991); // "localhost",localhost连接不上在cmd中ping localhost解析出来的是IPV6的::1
    kcp_s.Init();
    KCPX kcp_c("client", "127.0.0.1", 8881);
    kcp_c.Init();

    kcp_c.SendAccept( "127.0.0.1", 9991);

    while (true) {
        std::map<int, std::vector<std::string>> msgs;
        kcp_s.Receive(msgs);
        kcp_c.Receive(msgs);

        std::this_thread::yield();
        if (kcp_s.RemoteCount() == 1 &&
            kcp_c.RemoteCount() == 1) {
            auto remotes = kcp_s.GetRemotes();
            ASSERT_EQ(remotes.size(), 1);
            ASSERT_EQ(remotes[1], "client"); //server端得到的远程名字为client

            remotes = kcp_c.GetRemotes();
            ASSERT_EQ(remotes.size(), 1);
            ASSERT_EQ(remotes[1], "server"); //client端得到的远程名字为server
            break;
        }
    }
}

TEST(KCPX, SendRece)
{
    dlog_set_console_thr(dlog_level::debug);

    KCPX kcp_s("server", "127.0.0.1", 9991);
    kcp_s.Init();
    KCPX kcp_c("client", "127.0.0.1", 8881);
    kcp_c.Init();

    kcp_c.SendAccept("127.0.0.1", 9991);

    int successCount = 0;
    while (true) {
        std::map<int, std::vector<std::string>> msgs;
        kcp_s.Receive(msgs);
        if (!msgs.empty()) {
            ASSERT_EQ(msgs.size(), 1);
            ASSERT_EQ(msgs[1].size(), 1);
            ASSERT_EQ(msgs[1][0], "123456");
            successCount++;
            LogI("successCount=%d", successCount);
            if (successCount > 200) {
                break;
            }
        }

        kcp_c.Receive(msgs);
        if (!msgs.empty()) {
            ASSERT_EQ(msgs.size(), 1);
            ASSERT_EQ(msgs[1].size(), 1);
            ASSERT_EQ(msgs[1][0], "abcdefg");
            successCount++;
            LogI("successCount=%d", successCount);
            if (successCount > 200) {
                break;
            }
        }

        std::this_thread::yield();
        if (kcp_s.RemoteCount() == 1 &&
            kcp_c.RemoteCount() == 1) {
            kcp_c.Send(1, "123456", 6);  //c->s
            kcp_s.Send(1, "abcdefg", 7); //s->c
        }
    }
}