#include "dlog/dlog.h"
#include <thread>

#include "xuexuejson/Serialize.hpp"
#include "xuexue/csharp/csharp.h"

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/MulticastSocket.h"

#include "DNET/TCP/TCPClient.h"

using namespace dxlib;
using namespace std;

class Config : XUEXUE_JSON_OBJECT
{
  public:
    string host = "127.0.0.1";
    int port = 9991;

    XUEXUE_JSON_OBJECT_M2(host, port)
};

int main(int argc, char* argv[])
{
    using xuexue::csharp::File;
    using xuexue::csharp::Path;
    using namespace xuexue::json;

    dlog_init("log", "DNETClient", dlog_init_relative::MODULE);
    dlog_memory_log_enable(false);
    dlog_set_console_thr(dlog_level::info);

    Config config;
    string configPath = Path::Combine(Path::ModuleDir(), "config.json");
    if (File::Exists(configPath)) {
        config = JsonMapper::toObject<Config>(File::ReadAllText(configPath));
    }

    TCPClient kcp_c("client");
    //kcp_c.SendAccept("127.0.0.1", 9991);
    kcp_c.Connect(config.host, config.port);

    //等待连接成功
    kcp_c.WaitAccepted();

    int count = 0;
    std::vector<std::string> msgs;
    while (true) {
        kcp_c.KCPReceive(msgs);
        poco_assert(msgs.empty());
        if (kcp_c.KCPWaitSendCount() < 3) {
            std::string str = "1234567890 - " + std::to_string(count);
            int rece = kcp_c.KCPSend(str.c_str(), str.size());
            LogI("发送条数=%d,result=%d", count, rece);
            count++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}