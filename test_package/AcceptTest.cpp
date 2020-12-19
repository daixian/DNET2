#include "gtest/gtest.h"
#include "DNET/KCP.h"
#include "DNET/Accept.h"
#include <thread>
#include "dlog/dlog.h"
#include <atomic>

using namespace dxlib;
using namespace std;

TEST(Accept, CreateAcceptString)
{
    Accept accept;

    for (size_t i = 0; i < 1000; i++) {
        std::string str = accept.CreateAcceptString("clinet");

        Accept acceptS;
        std::string str2 = acceptS.ReplyAcceptString(str, "service", i);

        std::string serName;
        int conv = 0;
        bool success = accept.VerifyReplyAccept(str2, serName, conv);
        ASSERT_TRUE(success);
        ASSERT_EQ(serName, "service");
        ASSERT_EQ(conv, i);
    }
}

TEST(Accept, fail)
{
    Accept accept;

    for (size_t i = 0; i < 1000; i++) {
        std::string str = accept.CreateAcceptString("clinet");

        Accept acceptS;
        std::string str2 = "32131";

        std::string serName;
        int conv = 0;
        bool success = accept.VerifyReplyAccept(str2, serName, conv);
        ASSERT_FALSE(success);
    }
}