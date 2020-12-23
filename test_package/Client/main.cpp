#include "DNET/KCPX.h"

#include "dlog/dlog.h"
#include <thread>

#include "xuexuejson/Serialize.hpp"
#include "xuexue/csharp/csharp.h"

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/MulticastSocket.h"

using namespace dxlib;
using namespace std;

int main(int argc, char* argv[])
{
    using xuexue::csharp::File;
    using xuexue::csharp::Path;

    dlog_init("log", "DNETServer", dlog_init_relative::MODULE);
    dlog_memory_log_enable(false);
    dlog_set_console_thr(dlog_level::info);

    KCPX kcp_c("client", "0.0.0.0", 8882);
    kcp_c.Init();
    //kcp_c.SendAccept("127.0.0.1", 9991);
    kcp_c.SendAccept("home.xuexuesoft.com", 9991);

    int conv = 0;
    std::map<int, std::vector<std::string>> msgs;
    while (true) {
        std::this_thread::yield();
        kcp_c.Receive(msgs);
        conv = kcp_c.GetConvIDWithName("server");
        if (conv > 0) {
            break;
        }
    }

    int count = 0;
    while (true) {
        kcp_c.Receive(msgs);
        poco_assert(msgs.empty());
        if (kcp_c.WaitSendCount(conv) < 2) {
            std::string str = "1234567890 - " + std::to_string(count);
            int rece = kcp_c.Send(conv, str.c_str(), str.size());
            LogI("发送条数=%d,result=%d", count, rece);
            count++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}