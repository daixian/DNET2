#include "gtest/gtest.h"
#include "DNET/KCP.h"
#include <thread>
#include "dlog/dlog.h"

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

//这个实验跑是跑起来了,但是发了100次用了6秒,一次就需要60ms
TEST(UDP, send)
{
    //dlog_set_console_thr(dlog_level::warn);

    KCP kcp_s("server", 1024, "localhost", 9991, "localhost", 8881);
    kcp_s.Init();
    KCP kcp_c("client", 1024, "localhost", 8881, "localhost", 9991);
    kcp_c.Init();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    char* sendData = new char[1024 * 1024];
    for (size_t i = 0; i < 1024 * 1024; i++) {
        sendData[i] = (char)i;
    }
    char* receBuff = new char[1024 * 1024];

    for (size_t i = 0; i < 100; i++) {

        int sendLen = 1204;
        kcp_c.Send(sendData, sendLen);

        int len = 0;
        while (len <= 0) {
            len = kcp_s.Receive(receBuff, sendLen);
        }
        LogI("UDP.test():接收到了一条");
        ASSERT_TRUE(len == sendLen);
    }
}