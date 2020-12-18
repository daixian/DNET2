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

    // 这个认证是否加密
    bool isEncrypt = false;

    /**
     * 得到一个随机的认证字符串.它是一个json文本.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns The accept string.
     */
    std::string CreateAcceptString();

    /**
     * 对一个随机的认证字符串进行回应.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @param  acceptString 收到的认证字符串.
     * @param  name         自己的名字.
     * @param  conv         一个用于表示会话编号的整数.
     *
     * @returns A std::string.
     */
    std::string ReplyAcceptString(const std::string& acceptString, const std::string& name, int conv);

    /**
     * 校验服务端的返回,之后使用协商的conv进行连接.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @param          replyAcceptString The reply accept string.
     * @param [in,out] serName           服务器端的名字.
     * @param [in,out] conv              协商的conv.
     *
     * @returns True if it succeeds, false if it fails.
     */
    bool VerifyReplyAccept(const std::string& replyAcceptString, std::string& serName, int& conv);

  private:
};

} // namespace dxlib