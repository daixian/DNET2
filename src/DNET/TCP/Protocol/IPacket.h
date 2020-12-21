#pragma once

#include <string>
#include <vector>
#include <map>

namespace dxlib {

/**
 * ����һ��Э��Ĵ������.
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
     * @returns ����ɹ�,���ش��������ݳ���.
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
     * @returns ������������������ݰ�,���ؽ������Ľ������.
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
     * @returns ������������������ݰ�,���ؽ������Ľ������.
     */
    virtual int Unpack(const char* receBuff, int count, std::vector<std::string>& result) = 0;

    /**
     * ��ǰ�Ƿ��в������Ľ��������ݻ��ڻ�������.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @returns ��ǰ�������ݶ��պô��������򷵻�true.
     */
    virtual bool isUnpackCached() = 0;

  private:
};

} // namespace dxlib