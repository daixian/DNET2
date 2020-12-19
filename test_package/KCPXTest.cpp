#include "gtest/gtest.h"
#include "DNET/KCPX.h"
#include <thread>
#include "dlog/dlog.h"
#include <atomic>

using namespace dxlib;
using namespace std;

//目前100条需要0.5秒
TEST(KCPX, Accept)
{
    dlog_set_console_thr(dlog_level::debug);

    KCPX kcp_s("server", "localhost", 9991);
    kcp_s.Init();
    KCPX kcp_c("client", "localhost", 8881);
    kcp_c.Init();

    kcp_c.SendAccept("localhost", 9991);

    while (true) {
        std::map<int, std::vector<std::string>> msgs;
        kcp_s.Receive(msgs);
        kcp_c.Receive(msgs);

        std::this_thread::yield();
        if (kcp_s.RemoteCount() == 1 &&
            kcp_c.RemoteCount() == 1) {
            break;
        }
    }
}
