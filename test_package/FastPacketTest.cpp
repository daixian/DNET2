#include "gtest/gtest.h"

#include "DNET/TCP/Protocol/FastPacket.h"
#include <thread>
#include "dlog/dlog.h"
#include <atomic>

using namespace dnet;
using namespace std;

TEST(FastPacket, pack)
{
    FastPacket pack;
    string data = "12fkldsangkjdfngkldfsngjsdkfnjgfsdhn";

    vector<char> packetedData;
    pack.Pack(data.c_str(), data.size(), packetedData, 123);

    ASSERT_EQ(packetedData.size(), data.size() + 1 + 4 + 4);
    ASSERT_EQ(packetedData[0], 'x');
    int* ptr = (int*)(&packetedData[1]);
    ASSERT_EQ(*ptr, data.size());
    ptr++;
    ASSERT_EQ(*ptr, 123);
}

TEST(FastPacket, unpackLen1)
{
    FastPacket pack;
    string data = "x12fklxxdsangkjxdfngkldfxsngjsdkfnjgfsdhnx";

    vector<char> packetedData;
    pack.Pack(data.c_str(), data.size(), packetedData, 123);

    ASSERT_EQ(packetedData.size(), data.size() + 1 + 4 + 4);

    // 一个一个的往里面添加数据去检察是否能够正常解包
    int unPackLen = 1;
    for (size_t i = 0; i < packetedData.size(); i++) {
        std::vector<BinMessage> result;
        int res = pack.Unpack(&packetedData[i], unPackLen, result);
        if (res > 0) {
            ASSERT_EQ(result.size(), 1);
            ASSERT_EQ(result[0].type, 123);
            ASSERT_EQ(result[0].data.size(), data.size());
            for (size_t j = 0; j < result[0].data.size(); j++) {
                ASSERT_EQ(result[0].data[j], data[j]);
            }
        }
    }

    ASSERT_FALSE(pack.isUnpackCached()); //这标记着已经解析到了一次数据了
}

TEST(FastPacket, unpackLenMany)
{
    FastPacket pack;
    string data = "x12fxxxsangkjdfngkxxgjsdkfnjgxxfsdhn534354x";

    vector<char> packetedData;
    pack.Pack(data.c_str(), data.size(), packetedData, 123);

    ASSERT_EQ(packetedData.size(), data.size() + 1 + 4 + 4);

    //每次传入的unPackLen不同,进行n次试验
    for (size_t unPackLen = 1; unPackLen <= packetedData.size(); unPackLen++) {
        std::vector<BinMessage> result;
        int i = 0;
        while (i < packetedData.size()) {
            if (i + unPackLen <= packetedData.size()) {

                int res = pack.Unpack(&packetedData[i], unPackLen, result);
                i += unPackLen;
            }
            else {
                int res = pack.Unpack(&packetedData[i], packetedData.size() - i, result);
                break;
            }
        }

        ASSERT_FALSE(pack.isUnpackCached()); //这标记着已经解析到了一次数据了
        ASSERT_EQ(result.size(), 1);
        ASSERT_EQ(result[0].data.size(), data.size());
        for (size_t j = 0; j < result[0].data.size(); j++) {
            ASSERT_EQ(result[0].data[j], data[j]);
        }
    }
}

TEST(FastPacket, unpackLenManyString)
{
    FastPacket pack;
    string data = "x12fxxxsangkjdfngkxxgjsdkfnjgxxfsdhn534354x";

    vector<char> packetedData;
    pack.Pack(data.c_str(), data.size(), packetedData, 123);

    ASSERT_EQ(packetedData.size(), data.size() + 1 + 4 + 4);

    //每次传入的unPackLen不同,进行n次试验
    for (size_t unPackLen = 1; unPackLen <= packetedData.size(); unPackLen++) {
        std::vector<TextMessage> result;
        int i = 0;
        while (i < packetedData.size()) {
            if (i + unPackLen <= packetedData.size()) {
                int res = pack.Unpack(&packetedData[i], unPackLen, result);
                i += unPackLen;
            }
            else {
                int res = pack.Unpack(&packetedData[i], packetedData.size() - i, result);
                break;
            }
        }

        ASSERT_FALSE(pack.isUnpackCached()); //这标记着已经解析到了一次数据了
        ASSERT_EQ(result.size(), 1);
        ASSERT_EQ(result[0].data.size(), data.size());
        ASSERT_EQ(result[0].data, data);
    }
}