#include "Accept.h"
#include "Poco/Random.h"
#include "xuexuejson/Serialize.hpp"

namespace dxlib {

class AcceptDto : XUEXUE_JSON_OBJECT
{
  public:
    std::string nameC;

    std::string nameS;

    std::string keyC;

    std::string keyS;

    std::string uuidC;

    std::string uuidS;

    int conv = -1;

    XUEXUE_JSON_OBJECT_M7(nameC, nameS, keyC, keyS, uuidC, uuidS, conv)
};

class Accept::Impl
{
  public:
    Impl() {}
    ~Impl() {}

    // 记录发送出去的dto对象.
    AcceptDto dto;

    // 是否已经校验过了
    bool isVerified = false;
};

Accept::Accept()
{
    _impl = new Impl();
}

Accept::~Accept()
{
    delete _impl;
}

Accept::Accept(const Accept& obj)
{
    _impl = new Impl();
    _impl->dto = obj._impl->dto;
    _impl->isVerified = obj._impl->isVerified;
}

Accept& Accept::operator=(const Accept& obj)
{
    _impl->dto = obj._impl->dto;
    _impl->isVerified = obj._impl->isVerified;
    return *this;
}

std::string Accept::CreateAcceptString(const std::string& uuidC, const std::string& nameC)
{

    Poco::Random rnd;
    rnd.seed();

    _impl->dto.nameC = nameC;
    _impl->dto.uuidC = uuidC;
    _impl->dto.keyC.clear();

    //生成一段16个字节的随机字符串
    for (size_t i = 0; i < 16; i++) {
        char c = '\0';
        while (c < '!' || c > '~') {
            c = rnd.nextChar();
        }
        _impl->dto.keyC.push_back(c);
    }
    _impl->isVerified = false; //这里直接标记自己是正在等待校验
    return xuexue::json::JsonMapper::toJson(_impl->dto);
}

std::string Accept::ReplyAcceptString(const std::string& acceptString, const std::string& uuidS, const std::string& nameS, int conv)
{
    _impl->dto = xuexue::json::JsonMapper::toObject<AcceptDto>(acceptString);
    if (_impl->dto.keyC.empty() || _impl->dto.uuidC.empty()) {
        //如果无法解析json
        return "";
    }

    _impl->dto.nameS = nameS;
    _impl->dto.uuidS = uuidS;
    _impl->dto.conv = conv;
    _impl->dto.keyS.clear();

    Poco::Random rnd;
    rnd.seed();

    //生成一段16个字节的随机字符串
    for (size_t i = 0; i < 16; i++) {
        char c = '\0';
        while (c < '!' || c > '~') {
            c = rnd.nextChar();
        }
        _impl->dto.keyS.push_back(c);
    }

    _impl->isVerified = true; //这里直接标记自己是已经校验过的
    return xuexue::json::JsonMapper::toJson(_impl->dto);
}

bool Accept::VerifyReplyAccept(const std::string& replyAcceptString, std::string& uuidS, std::string& nameS, int& conv)
{
    if (replyAcceptString.empty()) {
        _impl->isVerified = false;
        return false;
    }

    AcceptDto serverDto = xuexue::json::JsonMapper::toObject<AcceptDto>(replyAcceptString);

    bool isSuccess = false;
    if (serverDto.nameC == _impl->dto.nameC &&
        serverDto.uuidC == _impl->dto.uuidC &&
        serverDto.keyC == _impl->dto.keyC) {
        isSuccess = true;
        _impl->dto.keyS = serverDto.keyS;
        _impl->dto.nameS = serverDto.nameS;
        _impl->dto.uuidS = serverDto.uuidS;
        _impl->dto.conv = serverDto.conv;

        uuidS = serverDto.uuidS;
        nameS = serverDto.nameS;
        conv = serverDto.conv;
    }

    _impl->isVerified = isSuccess;
    return isSuccess;
}

bool Accept::isVerified()
{
    return _impl->isVerified;
}
int Accept::conv()
{
    return _impl->dto.conv;
}

std::string Accept::nameC()
{
    return _impl->dto.nameC;
}

std::string Accept::nameS()
{
    return _impl->dto.nameS;
}

std::string Accept::uuidC()
{
    return _impl->dto.uuidC;
}

std::string Accept::uuidS()
{
    return _impl->dto.uuidS;
}

std::string Accept::keyC()
{
    return _impl->dto.keyC;
}

std::string Accept::keyS()
{
    return _impl->dto.keyS;
}

} // namespace dxlib