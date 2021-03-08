#include "Accept.h"
#include "Poco/Random.h"

namespace dnet {

Accept::Accept()
{
}

Accept::~Accept()
{
}

std::string Accept::CreateAcceptString(const std::string& uuidC, const std::string& nameC)
{

    Poco::Random rnd;
    rnd.seed();

    this->nameC = nameC;
    this->uuidC = uuidC;
    this->keyC.clear();

    //生成一段16个字节的随机字符串
    for (size_t i = 0; i < 16; i++) {
        char c = '\0';
        while (c < '!' || c > '~') {
            c = rnd.nextChar();
        }
        this->keyC.push_back(c);
    }
    this->_isVerified = false; //这里直接标记自己是正在等待校验
    return xuexue::json::JsonMapper::toJson(*this);
}

bool Accept::VerifyAccept(const std::string& AcceptString)
{
    *this = xuexue::json::JsonMapper::toObject<Accept>(AcceptString);
    if (this->keyC.empty() || this->uuidC.empty()) {
        //如果无法解析json
        return false;
    }
    return true;
}

std::string Accept::ReplyAcceptString(const std::string& acceptString, const std::string& uuidS, const std::string& nameS, int conv)
{
    *this = xuexue::json::JsonMapper::toObject<Accept>(acceptString);
    if (this->keyC.empty() || this->uuidC.empty()) {
        //如果无法解析json
        return "";
    }

    this->nameS = nameS;
    this->uuidS = uuidS;
    this->conv = conv;
    this->keyS.clear();

    Poco::Random rnd;
    rnd.seed();

    //生成一段16个字节的随机字符串
    for (size_t i = 0; i < 16; i++) {
        char c = '\0';
        while (c < '!' || c > '~') {
            c = rnd.nextChar();
        }
        this->keyS.push_back(c);
    }

    this->_isVerified = true; //这里直接标记自己是已经校验过的
    return xuexue::json::JsonMapper::toJson(*this);
}

bool Accept::VerifyReplyAccept(const std::string& replyAcceptString)
{
    if (replyAcceptString.empty()) {
        this->_isVerified = false;
        return false;
    }

    Accept serverDto = xuexue::json::JsonMapper::toObject<Accept>(replyAcceptString);

    bool isSuccess = false;
    if (serverDto.nameC == this->nameC &&
        serverDto.uuidC == this->uuidC &&
        serverDto.keyC == this->keyC) {
        isSuccess = true;
        //赋值
        this->keyS = serverDto.keyS;
        this->nameS = serverDto.nameS;
        this->uuidS = serverDto.uuidS;
        this->conv = serverDto.conv;
    }

    this->_isVerified = isSuccess;
    return isSuccess;
}

} // namespace dxlib