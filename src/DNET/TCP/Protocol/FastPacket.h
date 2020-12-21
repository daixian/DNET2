#pragma once

#include "IPacket.h"

namespace dxlib {

/**
 * ����Ϣ��ͷʹ��һ��int�������Ϣ����.
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
        //Э��ͷ
        result[0] = 'x';
        //Э�鳤
        int* ptr = (int*)(&result[1]);
        *ptr = len;
        //��������
        memcpy(&result[5], data, len);
        return result.size();
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
        //Э��ͷ
        result[0] = 'x';
        //Э�鳤
        int* ptr = (int*)(&result[1]);
        *ptr = len;
        //��������
        memcpy(&result[5], data, len);
        return result.size();
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
     * @returns ����ɹ�,���ش��������ݳ���,���������򷵻ظ�ֵ.
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
        //��������
        memcpy(&buffer[5], data, len);
        return packLen;
    }

    /**
     * Unpacks.������ʽ��,�����յ����������εĴ������Ϳ�����.
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
    virtual int Unpack(const char* receBuff, int count, std::vector<std::vector<char>>& result) override
    {
        result.clear();

        int curIndex = 0;
        while (curIndex < count) {
            if (!isHasHead) {
                for (int i = curIndex; i < count; i++) {
                    if (receBuff[i] == 'x') {
                        isHasHead = true;
                        _unpackLenBuff.clear();
                        _unpackDataBuff.clear();
                        curIndex = i + 1; //����һ��λ�ÿ�ʼ������
                        break;
                    }
                    else {
                        curIndex = i + 1; //����һ��λ�ÿ�ʼ
                    }
                }
            }

            //��������������Ҳ���һ��Э��ͷ
            if (!isHasHead) {
                return 0;
            }

            //�����û�ж�ȡ�곤��
            if (_unpackLenBuff.size() < 4) {
                for (int i = curIndex; i < count; i++) {
                    _unpackLenBuff.push_back(receBuff[i]);
                    if (_unpackLenBuff.size() == 4) {
                        //��ǰ��Ҫȥ�������
                        int* ptr = (int*)(&_unpackLenBuff[0]);
                        curMsgLen = *ptr;
                        curIndex = i + 1; //����һ��λ�ÿ�ʼ��������
                        break;
                    }
                    else {
                        curIndex = i + 1; //����һ��λ�ÿ�ʼ
                    }
                }
            }

            //��������Ѿ�����
            if (_unpackLenBuff.size() == 4) {
                for (int i = curIndex; i < count; i++) {
                    _unpackDataBuff.push_back(receBuff[i]);
                    if (_unpackDataBuff.size() == curMsgLen) {
                        //��ǰ��������һ��������Ϣ
                        result.push_back(_unpackDataBuff);

                        //��ռ�¼״̬
                        isHasHead = false;
                        _unpackLenBuff.clear();
                        curMsgLen = 0;
                        _unpackDataBuff.clear();

                        curIndex = i + 1; //����һ��λ�ÿ�ʼ
                        break;
                    }
                    else {
                        curIndex = i + 1; //����һ��λ�ÿ�ʼ
                    }
                }
            }
        }
        return result.size();
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
     * @returns ������������������ݰ�,���ؽ������Ľ������.
     */
    virtual int Unpack(const char* receBuff, int count, std::vector<std::string>& result) override
    {
        result.clear();

        int curIndex = 0;
        while (curIndex < count) {
            if (!isHasHead) {
                for (int i = curIndex; i < count; i++) {
                    if (receBuff[i] == 'x') {
                        isHasHead = true;
                        _unpackLenBuff.clear();
                        _unpackDataBuff.clear();
                        curIndex = i + 1; //����һ��λ�ÿ�ʼ������
                        break;
                    }
                    else {
                        curIndex = i + 1; //����һ��λ�ÿ�ʼ
                    }
                }
            }

            //��������������Ҳ���һ��Э��ͷ
            if (!isHasHead) {
                return 0;
            }

            //�����û�ж�ȡ�곤��
            if (_unpackLenBuff.size() < 4) {
                for (int i = curIndex; i < count; i++) {
                    _unpackLenBuff.push_back(receBuff[i]);
                    if (_unpackLenBuff.size() == 4) {
                        //��ǰ��Ҫȥ�������
                        int* ptr = (int*)(&_unpackLenBuff[0]);
                        curMsgLen = *ptr;
                        curIndex = i + 1; //����һ��λ�ÿ�ʼ��������
                        break;
                    }
                    else {
                        curIndex = i + 1; //����һ��λ�ÿ�ʼ
                    }
                }
            }

            //��������Ѿ�����
            if (_unpackLenBuff.size() == 4) {
                for (int i = curIndex; i < count; i++) {
                    _unpackDataBuff.push_back(receBuff[i]);
                    if (_unpackDataBuff.size() == curMsgLen) {
                        //��ǰ��������һ��������Ϣ
                        result.push_back(std::string(_unpackDataBuff.data(), _unpackDataBuff.size()));

                        //��ռ�¼״̬
                        isHasHead = false;
                        _unpackLenBuff.clear();
                        curMsgLen = 0;
                        _unpackDataBuff.clear();

                        curIndex = i + 1; //����һ��λ�ÿ�ʼ
                        break;
                    }
                    else {
                        curIndex = i + 1; //����һ��λ�ÿ�ʼ
                    }
                }
            }
        }
        return result.size();
    }

    /**
     * ��ǰ�Ƿ��в������Ľ��������ݻ��ڻ�������.
     *
     * @author daixian
     * @date 2020/12/21
     *
     * @returns ��ǰ�������ݶ��պô��������򷵻�true.
     */
    virtual bool isUnpackCached() override
    {
        return isHasHead;
    }

  private:
    bool isHasHead = false;

    // ������unpackδ��ɵ�'���ݳ���'�ֶεĻ���,�����Ӧ��ֻ��4������
    std::vector<char> _unpackLenBuff;

    // ���ܵó������ݳ���
    int curMsgLen = 0;

    // ��������unpackδ��ɵ����ݵ�buff
    std::vector<char> _unpackDataBuff;
};

} // namespace dxlib