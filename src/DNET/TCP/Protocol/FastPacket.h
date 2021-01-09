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
     * @param       data   要打包的原始数据.
     * @param       len    The length.
     * @param [out] result The result.
     * @param       type   (Optional) 数据类型(如可以分成命令和用户数据两种).
     *
     * @returns 打包结果长度.
     */
    virtual int Pack(const char* data, int len, std::vector<char>& result, int type) override
    {
        result.resize(sizeof(int) + sizeof(int) + 1 + (size_t)len);
        //协议头
        result[0] = 'x';
        //协议数据长
        int* ptr = (int*)(&result[1]);
        *ptr = len; //写数据长度
        ptr++;
        *ptr = type; //写数据类型
        ptr++;
        //数据内容
        memcpy(ptr, data, len);
        return (int)result.size();
    }

    /**
     * Packs
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param       data   要打包的原始数据.
     * @param       len    原始数据长度.
     * @param [out] result The result.
     * @param       type   (Optional) The type.
     *
     * @returns 打包结果长度.
     */
    virtual int Pack(const char* data, int len, std::string& result, int type) override
    {
        result.resize(sizeof(int) + sizeof(int) + 1 + (size_t)len);
        //协议头
        result[0] = 'x';
        //协议数据长
        int* ptr = (int*)(&result[1]);
        *ptr = len; //写数据长度
        ptr++;
        *ptr = type; //写数据类型
        ptr++;
        //数据内容
        memcpy(ptr, data, len);
        return (int)result.size();
    }

    /**
     * Packs
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param       data      要打包的原始数据.
     * @param       len       原始数据长度.
     * @param [out] buffer    The buffer.
     * @param       bufferLen Length of the buffer.
     * @param       type      (Optional) The type.
     *
     * @returns 如果成功,返回打包后的数据长度.
     */
    virtual int Pack(const char* data, int len, char* buffer, int bufferLen, int type) override
    {
        int packLen = sizeof(int) + sizeof(int) + 1 + len;

        if (bufferLen < packLen) {
            return -1;
        }
        buffer[0] = 'x';
        //协议数据长
        int* ptr = (int*)(&buffer[1]);
        *ptr = len; //写数据长度
        ptr++;
        *ptr = type; //写数据类型
        ptr++;
        //数据内容
        memcpy(ptr, data, len);
        return packLen;
    }

    /**
     * Unpacks
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param       receBuff Buffer for rece data.
     * @param       len      Number of.
     * @param [out] result   解包数据.
     * @param [out] type     解包数据类型.
     *
     * @returns 如果解析到了完整数据包,返回解析到的结果个数.
     */
    virtual int Unpack(const char* receBuff, int count, std::vector<std::vector<char>>& result, std::vector<int>& types) override
    {
        //result.clear();

        int curIndex = 0;
        while (curIndex < count) {
            if (!isHasHead) {
                for (int i = curIndex; i < count; i++) {
                    if (receBuff[i] == 'x') {
                        isHasHead = true;
                        _unpackLenBuff.clear();
                        _unpackTypeBuff.clear();
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
                        curIndex = i + 1; //从下一个位置开始看数据类型数据
                        break;
                    }
                    else {
                        curIndex = i + 1; //从下一个位置开始
                    }
                }
            }
            else {
                //如果长度已经有了
                if (_unpackTypeBuff.size() < 4) {
                    //如果还没有Type
                    for (int i = curIndex; i < count; i++) {
                        _unpackTypeBuff.push_back(receBuff[i]);
                        if (_unpackTypeBuff.size() == 4) {
                            //当前需要去检察结果了
                            int* ptr = (int*)(&_unpackTypeBuff[0]);
                            curMsgType = *ptr;
                            curIndex = i + 1; //从下一个位置开始拷贝数据
                            break;
                        }
                        else {
                            curIndex = i + 1; //从下一个位置开始
                        }
                    }
                }
                else {
                    //如果Type也有了
                    for (int i = curIndex; i < count; i++) {
                        _unpackDataBuff.push_back(receBuff[i]);
                        if (_unpackDataBuff.size() == curMsgLen) {
                            //当前解析到了一条完整消息
                            result.push_back(_unpackDataBuff);
                            types.push_back(curMsgType);

                            //清空记录状态
                            isHasHead = false;
                            _unpackLenBuff.clear();
                            curMsgLen = 0;
                            _unpackTypeBuff.clear();
                            curMsgType = 0;
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
     * @param       count    Number of.
     * @param [out] result   解包数据.
     * @param [out] types    解包数据类型.
     *
     * @returns 如果解析到了完整数据包,返回解析到的结果个数.
     */
    virtual int Unpack(const char* receBuff, int count, std::vector<std::string>& result, std::vector<int>& types) override
    {
        //result.clear();

        int curIndex = 0;
        while (curIndex < count) {
            if (!isHasHead) {
                for (int i = curIndex; i < count; i++) {
                    if (receBuff[i] == 'x') {
                        isHasHead = true;
                        _unpackLenBuff.clear();
                        _unpackTypeBuff.clear();
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
                        curIndex = i + 1; //从下一个位置开始看数据类型数据
                        break;
                    }
                    else {
                        curIndex = i + 1; //从下一个位置开始
                    }
                }
            }
            else {
                //如果长度已经有了
                if (_unpackTypeBuff.size() < 4) {
                    //如果还没有Type
                    for (int i = curIndex; i < count; i++) {
                        _unpackTypeBuff.push_back(receBuff[i]);
                        if (_unpackTypeBuff.size() == 4) {
                            //当前需要去检察结果了
                            int* ptr = (int*)(&_unpackTypeBuff[0]);
                            curMsgType = *ptr;
                            curIndex = i + 1; //从下一个位置开始拷贝数据
                            break;
                        }
                        else {
                            curIndex = i + 1; //从下一个位置开始
                        }
                    }
                }
                else {
                    //如果Type也有了
                    for (int i = curIndex; i < count; i++) {
                        _unpackDataBuff.push_back(receBuff[i]);
                        if (_unpackDataBuff.size() == curMsgLen) {
                            //当前解析到了一条完整消息
                            result.push_back(std::string(_unpackDataBuff.data(), _unpackDataBuff.size()));
                            types.push_back(curMsgType);

                            //清空记录状态
                            isHasHead = false;
                            _unpackLenBuff.clear();
                            curMsgLen = 0;
                            _unpackTypeBuff.clear();
                            curMsgType = 0;
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

    // 来缓存unpack未完成的'数据类型'字段的缓存,它最多应该只有4个长度
    std::vector<char> _unpackTypeBuff;

    // 当前这一条消息的数据类型
    int curMsgType = 0;

    // 用来缓存unpack未完成的数据的buff
    std::vector<char> _unpackDataBuff;
};

} // namespace dxlib