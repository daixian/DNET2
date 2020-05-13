#include "gtest/gtest.h"
#include "DNET/KCP.h"
#include "DNET/KCP2.h"
#include <thread>
#include "dlog/dlog.h"
#include <atomic>

using namespace dxlib;
using namespace std;

//一种Task的工作模式
//TEST(UDP, create)
//{
//    KCP kcp(KCP::Type::Server, "localhost", 9991, 1024);
//    kcp.Init();
//    std::this_thread::sleep_for(std::chrono::milliseconds(100));
//}

//TEST(UDP, createMany_S)
//{
//    for (size_t i = 0; i < 2; i++) {
//        KCP kcp(KCP::Type::Server, "localhost", 9991, 1024);
//        kcp.Init();
//    }
//}
//
//TEST(UDP, createMany_C)
//{
//    for (size_t i = 0; i < 1000; i++) {
//        KCP kcp(KCP::Type::Client, "localhost", 9991, 1024);
//        kcp.Init();
//    }
//}

//TEST(UDP, send)
//{
//    KCP kcp_c(KCP::Type::Client, "localhost", 9991, 1024);
//    kcp_c.Init();
//    const char* msg = "12345678";
//    kcp_c.Send(msg, sizeof(msg));
//    std::this_thread::sleep_for(std::chrono::milliseconds(10500));
//}

//目前100条需要0.5秒
TEST(KCP, Blocking_SendRece)
{
    dlog_set_console_thr(dlog_level::warn);

    KCP kcp_s("server", 1024, "localhost", 9991, "localhost", 8881);
    kcp_s.Init();
    KCP kcp_c("client", 1024, "localhost", 8881, "localhost", 9991);
    kcp_c.Init();

    char* sendData = new char[1024 * 1024];
    for (size_t i = 0; i < 1024 * 1024; i++) {
        sendData[i] = (char)i;
    }
    char* receBuff = new char[1024 * 1024];

    for (int i = 0; i < 100; i++) {

        int sendLen = 1204;
        kcp_c.Send(sendData, sendLen);

        int len = 0;
        while (len <= 0) {
            len = kcp_s.Receive(receBuff, sendLen);
        }
        LogI("KCP.test():接收到了第%d条", i);
        ASSERT_TRUE(len == sendLen);
        for (size_t i = 0; i < sendLen; i++) {
            ASSERT_TRUE(receBuff[i] == sendData[i]);
        }
    }
}

//这个是没有blocking的KCP实现
TEST(KCP2, NoBlocking_SendRece)
{
    dlog_set_console_thr(dlog_level::warn);

    KCP2 kcp_s("server", 1025, "localhost", 9991, "localhost", 8881);
    kcp_s.Init();
    KCP2 kcp_c("client", 1025, "localhost", 8881, "localhost", 9991);
    kcp_c.Init();

    char* sendData = new char[1024 * 1024];
    for (size_t i = 0; i < 1024 * 1024; i++) {
        sendData[i] = (char)i;
    }
    char* receBuff = new char[1024 * 1024];
    char* receBuff2 = new char[1024 * 1024];

    kcp_c.Update();
    kcp_s.Update();
    for (int i = 0; i < 100; i++) {

        int sendLen = 1024;
        kcp_c.Send(sendData, sendLen);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        int len = 0;
        while (len <= 0) {
            len = kcp_s.Receive(receBuff, sendLen * 2);
            kcp_c.Receive(receBuff2, sendLen * 2); //它只是客户端接收回复用来驱动的
            kcp_c.Update();
            kcp_s.Update();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        LogI("KCP2.test():接收到了第%d条", i);
        ASSERT_TRUE(len == sendLen);
        for (size_t i = 0; i < sendLen; i++) {
            ASSERT_TRUE(receBuff[i] == sendData[i]);
        }
    }
}

//这个是没有blocking的KCP实现
TEST(KCP2, NoBlocking_SendRece_Two_Thread)
{
    dlog_set_console_thr(dlog_level::warn);

    KCP2 kcp_s("server", 1025, "localhost", 9991, "localhost", 8881);
    kcp_s.Init();
    KCP2 kcp_c("client", 1025, "localhost", 8881, "localhost", 9991);
    kcp_c.Init();

    char* sendData = new char[1024 * 1024];
    for (size_t i = 0; i < 1024 * 1024; i++) {
        sendData[i] = (char)i;
    }
    char* receBuff = new char[1024 * 1024];
    char* receBuff2 = new char[1024 * 1024];

    int sendLen = 1024;

    std::atomic_int count{0};

    std::thread thrSend = std::thread([&]() {
        kcp_c.Update();
        for (int i = 0; i < 100; i++) {
            kcp_c.Send(sendData, sendLen);
            while (count.load() <= i) {
                kcp_c.Receive(receBuff2, sendLen * 2); //它只是客户端接收回复用来驱动的
                kcp_c.Update();
            }
        }
    });

    std::thread thrRece = std::thread([&]() {
        kcp_s.Update();

        for (int i = 0; i < 100; i++) {
            int len = 0;
            while (len <= 0) {
                len = kcp_s.Receive(receBuff, sendLen * 2);
                kcp_s.Update();
            }
            count = i + 1;
            LogI("KCP2.test():接收到了第%d条", i);
            ASSERT_TRUE(len == sendLen);
            for (size_t i = 0; i < sendLen; i++) {
                ASSERT_TRUE(receBuff[i] == sendData[i]);
            }
        }
    });

    thrSend.join();
    thrRece.join();
    ASSERT_TRUE(count == 100);
}