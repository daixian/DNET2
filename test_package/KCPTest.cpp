#include "gtest/gtest.h"
// #include "DNET/KCP.h"
// #include "DNET/KCP2.h"
#include <thread>
#include "dlog/dlog.h"
#include <atomic>
#include "DNET/TCP/KCPServer.h"

#include "Poco/Format.h"

using namespace dnet;
using namespace std;

// 一种Task的工作模式
// TEST(UDP, create)
//{
//     KCP kcp(KCP::Type::Server, "localhost", 9991, 1024);
//     kcp.Init();
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
// }

// TEST(UDP, createMany_S)
//{
//     for (size_t i = 0; i < 2; i++) {
//         KCP kcp(KCP::Type::Server, "localhost", 9991, 1024);
//         kcp.Init();
//     }
// }
//
// TEST(UDP, createMany_C)
//{
//     for (size_t i = 0; i < 1000; i++) {
//         KCP kcp(KCP::Type::Client, "localhost", 9991, 1024);
//         kcp.Init();
//     }
// }

// TEST(UDP, send)
//{
//     KCP kcp_c(KCP::Type::Client, "localhost", 9991, 1024);
//     kcp_c.Init();
//     const char* msg = "12345678";
//     kcp_c.Send(msg, sizeof(msg));
//     std::this_thread::sleep_for(std::chrono::milliseconds(10500));
// }

TEST(KCPClient, send_rece_1)
{
    KCPServer server;
    server.Start(8810);
    server.AddClient(123);

    KCPServer client;
    client.Start(8811);
    client.AddClient(123);
    client.ClientSetRemote(123, "127.0.0.1", 8810);

    for (size_t i = 0; i < 40; i++) {
        std::string msg = std::to_string(i);
        client.Send(123, msg.c_str(), msg.size());
    }

    int serverReceCount = 0;
    int clientReceCount = 0;
    int waitCount = 0;
    while (true) {
        server.GetClient(123)->Update();
        client.GetClient(123)->Update();
        // client.GetClient(123)->Flush();
        // 接收驱动
        serverReceCount += server.ReceMessage();
        waitCount = server.GetClient(123)->WaitSendCount();
        server.GetClient(123)->Flush();

        clientReceCount += client.ReceMessage(); // 客户端也要不停的调用这个
        // std::vector<TextMessage> cmsg;
        // client.ReceMessage(cmsg);
        client.GetClient(123)->Flush();
        waitCount = client.GetClient(123)->WaitSendCount();
        if (serverReceCount == 40) {
            break;
        }
    }
}

TEST(KCPClient, send_rece_256)
{
    KCPServer server;
    server.Start(8810);
    KCPServer client;
    client.Start(8811);

    // 分配256个信道
    for (size_t i = 0; i < 256; i++) {
        server.AddClient(i);

        client.AddClient(i);
        client.ClientSetRemote(i, "127.0.0.1", 8810);
    }

    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 100; j++) {
            std::string msg = Poco::format("conv%d send message id=%d", i, j);
            client.Send(i, msg.c_str(), msg.size());
        }
    }

    int serverReceCount = 0;
    int clientReceCount = 0;
    int waitCount = 0;
    while (true) {
        // server.GetClient(123)->Update();
        // client.GetClient(123)->Update();
        //  client.GetClient(123)->Flush();
        //  接收驱动
        serverReceCount += server.ReceMessage();
        waitCount = server.GetClient(123)->WaitSendCount();
        server.Update(); // 这里似乎不能调用Flush,但是可以调用Update没事

        clientReceCount += client.ReceMessage(); // 客户端也要不停的调用这个
        // std::vector<TextMessage> cmsg;
        // client.ReceMessage(cmsg);
        client.Update();
        waitCount = client.GetClient(123)->WaitSendCount();

        int successCount = 0;
        if (server.mReceMessage.size() == 256) {
            for (auto& kvp : server.mReceMessage) {
                if (kvp.second.size() == 100)
                    successCount += 1;
            }
        }

        if (successCount == 256) {
            break;
        }
    }
}

// 多线程的测试
TEST(KCPClient, send_rece_MT)
{
    int msgTestNum = 1000;

    KCPServer server;
    server.Start(8810);
    for (size_t i = 0; i < 8; i++) {
        server.AddClient(i); // 8个信道
    }

    KCPServer clients[8];
    for (size_t i = 0; i < 8; i++) {
        clients[i].Start(8811 + i);
        clients[i].AddClient(i);
        clients[i].ClientSetRemote(i, "127.0.0.1", 8810);
    }

    bool isRun = true;
    std::vector<std::thread> threads;
    // 八线程，八个客户端
    for (int tID = 0; tID < 8; tID++) {
        KCPServer* client = &clients[tID];
        threads.emplace_back(std::thread([client, tID, &isRun]() {
            // for (int j = 0; j < 100; j++) {
            //     std::string msg = Poco::format("conv%d send message id=%d", tID, j);
            //     client->Send(tID, msg.c_str(), msg.size());

            //    client->ReceMessage();
            //}
            int msgCount = 0;
            while (isRun) {
                client->ReceMessage();
                int waitCount = client->GetClient(tID)->WaitSendCount();
                if (waitCount > 50) {
                    // 如果比较拥挤就先不发送
                    //LogI("conv%d waitCount=%d", tID, waitCount);
                }
                else {
                    std::string msg = Poco::format("conv%d send message id=%d", tID, msgCount);
                    client->Send(tID, msg.c_str(), msg.size());
                    msgCount++;
                }
            }
        }));
    }
    int serverReceCount = 0;
    int waitCount = 0;
    while (true) {
        //  接收驱动
        serverReceCount += server.ReceMessage();
        server.Update(); // 这里似乎不能调用Flush,但是可以调用Update没事

        int successCount = 0;
        if (server.mReceMessage.size() == 8) {
            for (auto& kvp : server.mReceMessage) {
                if (kvp.second.size() >= msgTestNum)
                    successCount += 1;
            }
        }

        if (successCount == 8) {
            isRun = false;
            break;
        }
    }

    for (auto& t : threads) {
        t.join();
    }
}

#if aaa0123

// 目前100条需要0.5秒
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

// 这个是没有blocking的KCP实现
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
            kcp_c.Receive(receBuff2, sendLen * 2); // 它只是客户端接收回复用来驱动的
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

// 这个是没有blocking的KCP实现
TEST(KCP2, NoBlocking_SendRece_Two_Thread)
{
    dlog_set_console_thr(dlog_level::warn);

    // 这里不能一个localhost一个127.0.0.1,必须全部是ipv4或者v6
    KCP2 kcp_s("server", 1025, "127.0.0.1", 9991, "127.0.0.1", 8881);
    kcp_s.Init();
    KCP2 kcp_c("client", 1025, "127.0.0.1", 8881, "127.0.0.1", 9991);
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
                kcp_c.Receive(receBuff2, sendLen * 2); // 它只是客户端接收回复用来驱动的
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

// 这个是没有blocking的KCP实现
TEST(KCP2, NoBlocking_SendRece_Two_Thread_Accept)
{
    dlog_set_console_thr(dlog_level::info);

    char* sendData = new char[1024 * 1024];
    for (size_t i = 0; i < 1024 * 1024; i++) {
        sendData[i] = (char)i;
    }
    char* receBuff = new char[1024 * 1024];
    char* receBuff2 = new char[1024 * 1024];

    int sendLen = 1024;

    std::atomic_bool isInit_c{false};
    std::atomic_bool isInit_s{false};

    std::atomic_int count{0};

    // 模拟客户端启动了
    std::thread thrSend = std::thread([&]() {
        KCP2 kcp_c("client", 1025, "127.0.0.1", 11024, "127.0.0.1", 9991);
        kcp_c.Init();
        isInit_c = true;
        while (!isInit_s.load()) {
        }

        kcp_c.SendAccept();
        kcp_c.Update();
        for (int i = 0; i < 100; i++) {
            kcp_c.Send(sendData, sendLen);
            while (count.load() <= i) {
                kcp_c.Receive(receBuff2, sendLen); // 它只是客户端接收回复用来驱动的
                kcp_c.Update();
            }
        }
    });

    // 模拟服务端启动了
    std::thread thrRece = std::thread([&]() {
        KCP2 kcp_s("server", 1025, "0.0.0.0", 9991);
        kcp_s.Init();
        isInit_s = true;
        // while (!isInit_c.load()) {
        // }
        kcp_s.Update();

        for (int i = 0; i < 100; i++) {
            int len = 0;
            while (len <= 0) {
                len = kcp_s.Receive(receBuff, sendLen);
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

#endif