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

    int portC = 0;

    int portS = 0;

    int conv = 0;

    XUEXUE_JSON_OBJECT_M7(nameC, nameS, keyC, keyS, portC, portS, conv)
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

std::string Accept::CreateAcceptString(const std::string& name, int portC)
{

    Poco::Random rnd;
    rnd.seed();

    _impl->dto.nameC = name;
    _impl->dto.portC = portC;
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

std::string Accept::ReplyAcceptString(const std::string& acceptString, const std::string& name, int conv, int portS)
{
    _impl->dto = xuexue::json::JsonMapper::toObject<AcceptDto>(acceptString);
    if (_impl->dto.keyC.empty() || _impl->dto.nameC.empty()) {
        //如果无法解析json
        return "";
    }

    _impl->dto.nameS = name;
    _impl->dto.portS = portS;
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

bool Accept::VerifyReplyAccept(const std::string& replyAcceptString, std::string& serName, int& conv)
{
    if (replyAcceptString.empty()) {
        _impl->isVerified = false;
        return false;
    }

    AcceptDto serverDto = xuexue::json::JsonMapper::toObject<AcceptDto>(replyAcceptString);

    bool isSuccess = false;
    if (serverDto.nameC == _impl->dto.nameC &&
        serverDto.portC == _impl->dto.portC &&
        serverDto.keyC == _impl->dto.keyC) {
        isSuccess = true;
        _impl->dto.keyS = serverDto.keyS;
        _impl->dto.nameS = serverDto.nameS;
        _impl->dto.portS = serverDto.portS;
        _impl->dto.conv = serverDto.conv;

        serName = serverDto.nameS;
        conv = serverDto.conv;
    }

    _impl->isVerified = isSuccess;
    return isSuccess;
}

bool Accept::isVerified()
{
    return _impl->isVerified;
}

std::string Accept::nameC()
{
    return _impl->dto.nameC;
}

std::string Accept::nameS()
{
    return _impl->dto.nameS;
}

int Accept::portC()
{
    return _impl->dto.portC;
}

int Accept::portS()
{
    return _impl->dto.portS;
}

} // namespace dxlib