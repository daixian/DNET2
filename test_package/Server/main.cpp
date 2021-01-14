#include "xuexuejson/Serialize.hpp"
#include "DNET/TCP/TCPServer.h"

#include "dlog/dlog.h"
#include <thread>

#include "xuexue/csharp/csharp.h"

using namespace dxlib;
using namespace std;

int main(int argc, char* argv[])
{
    using xuexue::csharp::File;
    using xuexue::csharp::Path;

    dlog_init("log", "DNETServer", dlog_init_relative::MODULE);
    dlog_memory_log_enable(false);
    dlog_set_console_thr(dlog_level::info);

    TCPServer kcp_s("server", "0.0.0.0", 9991);
    kcp_s.Start();

    while (true) {
        std::map<int, std::vector<std::string>> msgs;
        kcp_s.Receive(msgs);
        if (!msgs.empty()) {
            for (auto& kvp : msgs) {
                for (size_t i = 0; i < kvp.second.size(); i++) {
                    LogI("接收到了conv=%d:%s", kvp.first, kvp.second[i].c_str());
                }
            }
        }

        std::map<int, std::vector<std::string>> kcpmsgs;
        kcp_s.KCPReceive(kcpmsgs);
        if (!kcpmsgs.empty()) {
            for (auto& kvp : kcpmsgs) {
                for (size_t i = 0; i < kvp.second.size(); i++) {
                    LogI("KCP接收到了conv=%d:%s", kvp.first, kvp.second[i].c_str());
                }
            }
        }

        //std::this_thread::yield(); //快速响应
        //std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return 0;
}