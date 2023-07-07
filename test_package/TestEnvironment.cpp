#include "gtest/gtest.h"
#include <chrono>

#include "dlog/dlog.h"

// 来自: https://stackoverflow.com/questions/30593762/how-to-initialize-constant-string-for-multiple-tests-in-google-test

std::string currentDateTime()
{
    return std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
}

class TestEnvironment : public ::testing::Environment
{
  public:
    //做一个静态函数让测试方能够调用
    static std::string getStartTime()
    {
        static const std::string timestamp = currentDateTime();
        return timestamp;
    }

    // Initialise the timestamp.
    virtual void SetUp()
    {
    }
};

int main(int argc, char* argv[])
{
    dlog_set_is_chcp65001(true);
    dlog_init("DNetTest");

    ::testing::InitGoogleTest(&argc, argv);
    // gtest takes ownership of the TestEnvironment ptr - we don't delete it.
    ::testing::AddGlobalTestEnvironment(new TestEnvironment);
    return RUN_ALL_TESTS();
}
