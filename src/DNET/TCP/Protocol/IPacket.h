#pragma once

#include <string>
#include <vector>
#include <map>
#include "Message.hpp"

namespace dnet {

/**
 * 代表一个协议的打包方法.
 *
 * @author daixian
 * @date 2020/12/21
 */
class IPacket
{
  public:
    IPacket() {}
    virtual ~IPacket() {}

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
    virtual int Pack(const char* data, int len, std::vector<char>& result, int type) = 0;

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
    virtual int Pack(const char* data, int len, std::string& result, int type) = 0;

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
    virtual int Pack(const char* data, int len, char* buffer, int bufferLen, int type) = 0;

    /**
     * Unpacks
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param       receBuff Buffer for rece data.
     * @param       len      Number of.
     * @param [out] result   解包数据.
     *
     * @returns 如果解析到了完整数据包,返回解析到的结果个数.
     */
    virtual int Unpack(const char* receBuff, int len, std::vector<BinMessage>& result) = 0;

    /**
     * Unpacks
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @param       receBuff Buffer for rece data.
     * @param       len      Number of.
     * @param [out] result   解包数据.
     *
     * @returns 如果解析到了完整数据包,返回解析到的结果个数.
     */
    virtual int Unpack(const char* receBuff, int len, std::vector<TextMessage>& result) = 0;

    /**
     * 当前是否有不完整的解析的数据还在缓存里面.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @returns 当前所有数据都刚好处理完了则返回true.
     */
    virtual bool isUnpackCached() = 0;

  private:
};

} // namespace dnet