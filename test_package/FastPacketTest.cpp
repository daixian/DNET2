#include "gtest/gtest.h"
#include "DNET/KCP.h"
#include "DNET/TCP/Protocol/FastPacket.h"
#include <thread>
#include "dlog/dlog.h"
#include <atomic>

using namespace dxlib;
using namespace std;

TEST(FastPacket, pack)
{
    FastPacket pack;
    string data = "12fkldsangkjdfngkldfsngjsdkfnjgfsdhn";

    vector<char> packetedData;
    pack.Pack(data.c_str(), data.size(), packetedData);

    ASSERT_EQ(packetedData.size(), data.size() + 5);
    ASSERT_EQ(packetedData[0], 'x');
    int* ptr = (int*)(&packetedData[1]);
    ASSERT_EQ(*ptr, data.size());
}

TEST(FastPacket, unpackLen1)
{
    FastPacket pack;
    string data = "x12fklxxdsangkjxdfngkldfxsngjsdkfnjgfsdhnx";

    vector<char> packetedData;
    pack.Pack(data.c_str(), data.size(), packetedData);

    ASSERT_EQ(packetedData.size(), data.size() + 5);

    // һ��һ������������������ȥ����Ƿ��ܹ��������
    int unPackLen = 1;
    for (size_t i = 0; i < packetedData.size(); i++) {
        std::vector<std::vector<char>> result;
        int res = pack.Unpack(&packetedData[i], unPackLen, result);
        if (res > 0) {
            ASSERT_EQ(result.size(), 1);
            ASSERT_EQ(result[0].size(), data.size());
            for (size_t j = 0; j < result[0].size(); j++) {
                ASSERT_EQ(result[0][j], data[j]);
            }
        }
    }

    ASSERT_FALSE(pack.isUnpackCached()); //�������Ѿ���������һ��������
}

TEST(FastPacket, unpackLenMany)
{
    FastPacket pack;
    string data = "x12fxxxsangkjdfngkxxgjsdkfnjgxxfsdhn534354x";

    vector<char> packetedData;
    pack.Pack(data.c_str(), data.size(), packetedData);

    ASSERT_EQ(packetedData.size(), data.size() + 5);

    //ÿ�δ����unPackLen��ͬ,����n������
    for (size_t unPackLen = 1; unPackLen <= packetedData.size(); unPackLen++) {
        std::vector<std::vector<char>> result;
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

        ASSERT_FALSE(pack.isUnpackCached()); //�������Ѿ���������һ��������
        ASSERT_EQ(result.size(), 1);
        ASSERT_EQ(result[0].size(), data.size());
        for (size_t j = 0; j < result[0].size(); j++) {
            ASSERT_EQ(result[0][j], data[j]);
        }
    }
}

TEST(FastPacket, unpackLenManyString)
{
    FastPacket pack;
    string data = "x12fxxxsangkjdfngkxxgjsdkfnjgxxfsdhn534354x";

    vector<char> packetedData;
    pack.Pack(data.c_str(), data.size(), packetedData);

    ASSERT_EQ(packetedData.size(), data.size() + 5);

    //ÿ�δ����unPackLen��ͬ,����n������
    for (size_t unPackLen = 1; unPackLen <= packetedData.size(); unPackLen++) {
        std::vector<std::string> result;
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

        ASSERT_FALSE(pack.isUnpackCached()); //�������Ѿ���������һ��������
        ASSERT_EQ(result.size(), 1);
        ASSERT_EQ(result[0].size(), data.size());
        ASSERT_EQ(result[0], data);
    }
}