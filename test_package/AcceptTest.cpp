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
        std::string str = accept.CreateAcceptString("uuid_clinet", "clinet");

        Accept acceptS;
        std::string str2 = acceptS.ReplyAcceptString(str, "uuid_service", "service", i);

        bool success = accept.VerifyReplyAccept(str2);
        ASSERT_TRUE(success);
        ASSERT_EQ(accept.uuidS, "uuid_service");
        ASSERT_EQ(accept.nameS, "service");
        ASSERT_EQ(accept.nameC, "clinet");
        ASSERT_EQ(accept.nameS, "service");
        ASSERT_EQ(accept.uuidC, "uuid_clinet");
        ASSERT_EQ(accept.uuidS, "uuid_service");
        ASSERT_EQ(accept.conv, i);
    }
}

TEST(Accept, fail)
{
    Accept accept;

    std::string str = accept.CreateAcceptString("uuid_clinet", "clinet");

    Accept acceptS;
    std::string str2 = "32131"; //这是一段不是json的字符串.

    std::string serUUID;
    std::string serName;
    int conv = 0;
    bool success = accept.VerifyReplyAccept(str2);
    ASSERT_FALSE(success);
}