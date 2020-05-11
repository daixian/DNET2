#include "gtest/gtest.h"
#include "Poco/Format.h"
#include <thread>
#include <Poco/Net/TCPServer.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Timestamp.h>
#include <Poco/DateTimeFormatter.h>
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/MulticastSocket.h"

#pragma execution_character_set("utf-8")

#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/Observer.h"

//using namespace dxlib;
using namespace std;
using namespace Poco::Net;
using namespace Poco;

class SampleTask : public Poco::Task
{
  public:
    SampleTask(const std::string& name) : Task(name)
    {
    }
    void runTask()
    {
        for (int i = 0; i < 100; ++i) {
            setProgress(float(i) / 100); // report progress
            if (sleep(1000))
                break;
        }
    }
};

class ProgressHandler
{
  public:
    void onProgress(Poco::TaskProgressNotification* pNf)
    {
        std::cout << pNf->task()->name()
                  << " progress: " << pNf->progress() << std::endl;
        pNf->release();
    }
    void onFinished(Poco::TaskFinishedNotification* pNf)
    {
        std::cout << pNf->task()->name() << " finished." << std::endl;
        pNf->release();
    }
};

namespace dxtest {

//UDP服务端的Task
class UDPServerTask : public Poco::Task
{
  public:
    UDPServerTask(const std::string& name) : Task(name)
    {
    }
    void runTask()
    {
        Poco::Net::SocketAddress sa(Poco::Net::IPAddress("127.0.0.1"), 24005);
        socket = Poco::Net::DatagramSocket(sa);
        for (;;) {
            try {
                Poco::Net::SocketAddress sender;
                int n = socket.receiveFrom(buffer, sizeof(buffer), sender); //这一句是阻塞等待接收
                socket.sendTo(buffer, n, sender);                           //直接回发
                //buffer[n] = '\0';
                //std::cout << sender.toString() << ": " << buffer << std::endl;
            }
            catch (const Poco::Exception& e) {
                std::cout << e.what() << e.message() << std::endl;
            }
        }
    }

    char buffer[1024 * 8];
    //char sendBuffer[1024 * 8];
    Poco::Net::DatagramSocket socket;
};

class UDPClientTask : public Poco::Task
{
  public:
    UDPClientTask(const std::string& name) : Task(name)
    {
        for (size_t i = 0; i < sizeof(data); i++) {
            data[i] = static_cast<char>(i);
        }
    }
    void runTask()
    {
        Poco::Net::SocketAddress sa(Poco::Net::IPAddress("127.0.0.1"), 24005);
        Thread::sleep(1000);
        socket.connect(sa);
        for (;;) {
            try {
                Poco::Timestamp now;
                //std::string msg = Poco::DateTimeFormatter::format(now, "<14>%w %f %H:%M:%S Hello, world!");
                //socket.sendBytes(msg.data(), msg.size());
                socket.sendBytes(data, sizeof(data));
            }
            catch (const Poco::Exception& e) {
                std::cout << e.what() << e.message() << std::endl;
            }
        }
    }

    char data[1024 * 8];
    // is used to send and receive UDP packets.
    Poco::Net::DatagramSocket socket;
};

class UDPTest : public testing::Test
{
  public:
    UDPTest()
    {
    }
    ~UDPTest()
    {
    }

  private:
};

//一种Task的工作模式
TEST(UDP, TestSendRece)
{
    Poco::TaskManager tm;
    ProgressHandler pm;
    tm.addObserver(Poco::Observer<ProgressHandler, Poco::TaskProgressNotification>(pm, &ProgressHandler::onProgress));
    tm.addObserver(Poco::Observer<ProgressHandler, Poco::TaskFinishedNotification>(pm, &ProgressHandler::onFinished));
    tm.start(new UDPServerTask("UDPServerTask1")); // tm takes ownership
    tm.start(new UDPClientTask("UDPClientTask1"));
    //这里可以查看网络通信数据了
    tm.joinAll();
}

//TEST(UDP, send)
//{
//    Poco::Net::SocketAddress sa("localhost", 514);
//    Poco::Net::DatagramSocket dgs;
//    dgs.connect(sa);
//    Poco::Timestamp now;
//    std::string msg = Poco::DateTimeFormatter::format(now, "<14>%w %f %H:%M:%S Hello, world!");
//    dgs.sendBytes(msg.data(), msg.size());
//}

//TEST(UDP, receive)
//{
//    Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), 514);
//    Poco::Net::DatagramSocket dgs(sa);
//
//    char buffer[1024];
//
//    for (;;) {
//        Poco::Net::SocketAddress sender;
//        int n = dgs.receiveFrom(buffer, sizeof(buffer) - 1, sender); //这一句是阻塞等待接收
//        buffer[n] = '\0';
//        std::cout << sender.toString() << ": " << buffer << std::endl;
//    }
//}
//
////组播的实验
//TEST(UDP, MultiCastSocketReceive)
//{
//    Poco::Net::SocketAddress address("239.255.255.250", 1900);
//    Poco::Net::MulticastSocket socket(Poco::Net::SocketAddress(Poco::Net::IPAddress(), address.port()));
//    // to receive any data you must join
//    socket.joinGroup(address.host());
//    Poco::Net::SocketAddress sender;
//    char buffer[512];
//    int n = socket.receiveFrom(buffer, sizeof(buffer), sender);
//    socket.sendTo(buffer, n, sender);
//}

} // namespace dxtest