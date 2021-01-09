#pragma once

#include <string>

namespace dxlib {

/**
 * 一种握手方式.
 *
 * @author daixian
 * @date 2020/12/19
 */
class Accept
{
  public:
    Accept();
    ~Accept();
    Accept(const Accept& obj);
    Accept& operator=(const Accept& obj);

    // 这个认证是否加密
    bool isEncrypt = false;

    /**
     * 得到一个随机的认证字符串.它是一个json文本.(客户端调用)
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @param  uuidC 客户端的uuid.
     * @param  nameC 客户端自己的名字.
     *
     * @returns The accept string.
     */
    std::string CreateAcceptString(const std::string& uuidC, const std::string& nameC);

    /**
     * 对一个随机的认证字符串进行回应.(服务器端调用)
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @param  acceptString 收到的认证字符串.
     * @param  uuidS        uuid(服务器端).
     * @param  nameS        自己的名字(服务器端).
     * @param  conv         一个用于表示会话编号的整数.
     *
     * @returns 如果认证失败返回空字符串.
     */
    std::string ReplyAcceptString(const std::string& acceptString, const std::string& uuidS, const std::string& nameS, int conv);

    /**
     * 校验服务端的返回,之后使用协商的conv进行连接.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @param          replyAcceptString The reply accept string.
     * @param [in,out] uuidS             服务器端的uuid.
     * @param [in,out] nameS             服务器端的名字.
     * @param [in,out] conv              协商的conv.
     *
     * @returns 校验成功返回true.
     */
    bool VerifyReplyAccept(const std::string& replyAcceptString, std::string& uuidS, std::string& nameS, int& conv);

    /**
     * 是否已经校验过了.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns True if verified, false if not.
     */
    bool isVerified();

    /**
     * 通信ID
     *
     * @author daixian
     * @date 2020/12/28
     *
     * @returns An int.
     */
    int conv();

    /**
     * 客户端的Name.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns A std::string.
     */
    std::string nameC();

    /**
     * 服务器端的Name.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns A std::string.
     */
    std::string nameS();

    /**
     * 客户端的uuid.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns An int.
     */
    std::string uuidC();

    /**
     * 服务器端的uuid.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns An int.
     */
    std::string uuidS();

    /**
     * 客户端生成的随机key.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns An int.
     */
    std::string keyC();

    /**
     * 服务器端生成的随机key.
     *
     * @author daixian
     * @date 2020/12/23
     *
     * @returns An int.
     */
    std::string keyS();

  private:
    class Impl;
    Impl* _impl;
};

} // namespace dxlib