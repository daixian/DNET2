#pragma once

#include <string>
#include <vector>
#include <map>

namespace dxlib {

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
     * @param  data If non-null, the data.
     * @param  len  The length.
     *
     * @returns A std::vector<char>
     */
    virtual int Pack(const char* data, int len, std::vector<char>& result) = 0;

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
    virtual int Pack(const char* data, int len, std::string& result) = 0;

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
     * @returns 如果成功,返回打包后的数据长度.
     */
    virtual int Pack(const char* data, int len, char* buffer, int bufferLen) = 0;

    /**
     * Unpacks
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
    virtual int Unpack(const char* receBuff, int count, std::vector<std::vector<char>>& result) = 0;

    /**
     * Unpacks
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
    virtual int Unpack(const char* receBuff, int count, std::vector<std::string>& result) = 0;

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

} // namespace dxlib