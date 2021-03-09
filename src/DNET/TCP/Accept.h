#pragma once

#include <string>
#include <memory>
#include "xuexuejson/JsonMapper.hpp"

namespace dnet {

/**
 * 一种握手方式.
 *
 * @author daixian
 * @date 2020/12/19
 */
class Accept : XUEXUE_JSON_OBJECT
{
  public:
    Accept();
    ~Accept();

    // 客户端的Name.
    std::string nameC;

    // 服务器端的Name.
    std::string nameS;

    // 客户端生成的随机key.
    std::string keyC;

    // 服务器端生成的随机key.
    std::string keyS;

    // 客户端的uuid.
    std::string uuidC;

    // 服务器端的uuid.
    std::string uuidS;

    // 通信ID
    int conv = -1;

    XUEXUE_JSON_OBJECT_M7(nameC, nameS, keyC, keyS, uuidC, uuidS, conv)

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
     * 对一个随机的认证字符串进行校验.(服务器端调用)
     *
     * @author daixian
     * @date 2021/1/9
     *
     * @param  AcceptString The accept string.
     *
     * @returns True if it succeeds, false if it fails.
     */
    bool VerifyAccept(const std::string& AcceptString);

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
     * @param  replyAcceptString 收到的认证响应字符串.
     *
     * @returns 校验成功返回true.
     */
    bool VerifyReplyAccept(const std::string& replyAcceptString);

    /**
     * 是否已经校验通过了.
     *
     * @author daixian
     * @date 2020/12/19
     *
     * @returns True if verified, false if not.
     */
    bool isVerified()
    {
        return _isVerified;
    }

  private:
    // 是否已经校验通过了.
    bool _isVerified = false;
};

} // namespace dnet