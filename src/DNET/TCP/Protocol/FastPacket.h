#pragma once

#include "IPacket.h"

namespace dxlib {

/**
 * 在消息的头使用一个int来标记消息长度.
 *
 * @author daixian
 * @date 2020/12/21
 */
class FastPacket : public IPacket
{
  public:
    FastPacket() {}
    virtual ~FastPacket() {}

    /**
     * Packs
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param  data If non-null, the data.
     * @param  len  The length.
     *
     * @returns A std::vector<char>
     */
    virtual int Pack(const char* data, int len, std::vector<char>& result) override
    {
        result.resize(1 + 4 + (size_t)len);
        //协议头
        result[0] = 'x';
        //协议长
        int* ptr = (int*)(&result[1]);
        *ptr = len;
        //数据内容
        memcpy(&result[5], data, len);
        return (int)result.size();
    }

    /**
     * Packs
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param  data If non-null, the data.
     * @param  len  The length.
     *
     * @returns A std::vector<char>
     */
    virtual int Pack(const char* data, int len, std::string& result) override
    {
        result.resize(1 + 4 + (size_t)len);
        //协议头
        result[0] = 'x';
        //协议长
        int* ptr = (int*)(&result[1]);
        *ptr = len;
        //数据内容
        memcpy(&result[5], data, len);
        return (int)result.size();
    }

    /**
     * Packs
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param  data      The data.
     * @param  len       The length.
     * @param  buffer    The buffer.
     * @param  bufferLen Length of the buffer.
     *
     * @returns 如果成功,返回打包后的数据长度,产生错误则返回负值.
     */
    virtual int Pack(const char* data, int len, char* buffer, int bufferLen) override
    {
        int packLen = 1 + 4 + len;

        if (bufferLen < packLen) {
            return -1;
        }
        buffer[0] = 'x';
        int* ptr = (int*)(&buffer[1]);
        *ptr = len;
        //数据内容
        memcpy(&buffer[5], data, len);
        return packLen;
    }

    /**
     * Unpacks.它是流式的,将接收到的数据依次的传进来就可以了.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param       receBuff Buffer for rece data.
     * @param       count    Number of.
     * @param [out] result   The result.
     *
     * @returns 如果解析到了完整数据包,返回解析到的结果个数.
     */
    virtual int Unpack(const char* receBuff, int count, std::vector<std::vector<char>>& result) override
    {
        //result.clear();

        int curIndex = 0;
        while (curIndex < count) {
            if (!isHasHead) {
                for (int i = curIndex; i < count; i++) {
                    if (receBuff[i] == 'x') {
                        isHasHead = true;
                        _unpackLenBuff.clear();
                        _unpackDataBuff.clear();
                        curIndex = i + 1; //从下一个位置开始看长度
                        break;
                    }
                    else {
                        curIndex = i + 1; //从下一个位置开始
                    }
                }
            }

            //如果整个遍历都找不到一个协议头
            if (!isHasHead) {
                return 0;
            }

            //如果还没有读取完长度
            if (_unpackLenBuff.size() < 4) {
                for (int i = curIndex; i < count; i++) {
                    _unpackLenBuff.push_back(receBuff[i]);
                    if (_unpackLenBuff.size() == 4) {
                        //当前需要去检察结果了
                        int* ptr = (int*)(&_unpackLenBuff[0]);
                        curMsgLen = *ptr;
                        curIndex = i + 1; //从下一个位置开始拷贝数据
                        break;
                    }
                    else {
                        curIndex = i + 1; //从下一个位置开始
                    }
                }
            }

            //如果长度已经有了
            if (_unpackLenBuff.size() == 4) {
                for (int i = curIndex; i < count; i++) {
                    _unpackDataBuff.push_back(receBuff[i]);
                    if (_unpackDataBuff.size() == curMsgLen) {
                        //当前解析到了一条完整消息
                        result.push_back(_unpackDataBuff);

                        //清空记录状态
                        isHasHead = false;
                        _unpackLenBuff.clear();
                        curMsgLen = 0;
                        _unpackDataBuff.clear();

                        curIndex = i + 1; //从下一个位置开始
                        break;
                    }
                    else {
                        curIndex = i + 1; //从下一个位置开始
                    }
                }
            }
        }
        return (int)result.size();
    }

    /**
     * Unpacks
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param       receBuff Buffer for rece data.
     * @param       offset   The offset.
     * @param       count    Number of.
     * @param [out] result   The result.
     *
     * @returns 如果解析到了完整数据包,返回解析到的结果个数.
     */
    virtual int Unpack(const char* receBuff, int count, std::vector<std::string>& result) override
    {
        //result.clear();

        int curIndex = 0;
        while (curIndex < count) {
            if (!isHasHead) {
                for (int i = curIndex; i < count; i++) {
                    if (receBuff[i] == 'x') {
                        isHasHead = true;
                        _unpackLenBuff.clear();
                        _unpackDataBuff.clear();
                        curIndex = i + 1; //从下一个位置开始看长度
                        break;
                    }
                    else {
                        curIndex = i + 1; //从下一个位置开始
                    }
                }
            }

            //如果整个遍历都找不到一个协议头
            if (!isHasHead) {
                return 0;
            }

            //如果还没有读取完长度
            if (_unpackLenBuff.size() < 4) {
                for (int i = curIndex; i < count; i++) {
                    _unpackLenBuff.push_back(receBuff[i]);
                    if (_unpackLenBuff.size() == 4) {
                        //当前需要去检察结果了
                        int* ptr = (int*)(&_unpackLenBuff[0]);
                        curMsgLen = *ptr;
                        curIndex = i + 1; //从下一个位置开始拷贝数据
                        break;
                    }
                    else {
                        curIndex = i + 1; //从下一个位置开始
                    }
                }
            }

            //如果长度已经有了
            if (_unpackLenBuff.size() == 4) {
                for (int i = curIndex; i < count; i++) {
                    _unpackDataBuff.push_back(receBuff[i]);
                    if (_unpackDataBuff.size() == curMsgLen) {
                        //当前解析到了一条完整消息
                        result.push_back(std::string(_unpackDataBuff.data(), _unpackDataBuff.size()));

                        //清空记录状态
                        isHasHead = false;
                        _unpackLenBuff.clear();
                        curMsgLen = 0;
                        _unpackDataBuff.clear();

                        curIndex = i + 1; //从下一个位置开始
                        break;
                    }
                    else {
                        curIndex = i + 1; //从下一个位置开始
                    }
                }
            }
        }
        return (int)result.size();
    }

    /**
     * 当前是否有不完整的解析的数据还在缓存里面.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @returns 当前所有数据都刚好处理完了则返回true.
     */
    virtual bool isUnpackCached() override
    {
        return isHasHead;
    }

  private:
    bool isHasHead = false;

    // 来缓存unpack未完成的'数据长度'字段的缓存,它最多应该只有4个长度
    std::vector<char> _unpackLenBuff;

    // 它能得出的数据长度
    int curMsgLen = 0;

    // 用来缓存unpack未完成的数据的buff
    std::vector<char> _unpackDataBuff;
};

} // namespace dxlib