#pragma once

#include <string>
#include <vector>

namespace dnet {

/**
 * 代表一条消息.
 *
 * @author daixian
 * @date 2021/3/9
 *
 * @tparam T Generic type parameter.
 */
template <class T>
class Message
{
  public:
    // 这条消息的类型id.
    int type;

    // 这条消息的数据内容.
    T data;

    std::string to_string()
    {
        return std::string(data.data(), data.size());
    }
};

//二进制的消息
typedef Message<std::vector<char>> BinMessage;

//文本消息
typedef Message<std::string> TextMessage;

} // namespace dnet