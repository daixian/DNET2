#include "KCPX.h"

#include "dlog/dlog.h"

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/MulticastSocket.h"
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/Observer.h"
#include "Poco/Thread.h"
#include "Poco/Runnable.h"
#include "Poco/RunnableAdapter.h"

#include "./kcp/ikcp.h"
#include <thread>
#include <mutex>
#include "clock.hpp"
#include <regex>

namespace dxlib {

//TODO: 可以让上层通过检测 ikcp_waitsnd 函数来判断还有多少包没有发出去，灵活抉择是否向 snd_queue 缓存追加数据包还是其他。
//TODO: 管理大规模连接 https://github.com/skywind3000/kcp/wiki/KCP-Best-Practice
//如果需要同时管理大规模的 KCP连接（比如大于3000个），比如你正在实现一套类 epoll的机制，那么为了避免每秒钟对每个连接调用大量的调用 ikcp_update，我们可以使用 ikcp_check 来大大减少 ikcp_update调用的次数。
//ikcp_check返回值会告诉你需要在什么时间点再次调用 ikcp_update（如果中途没有 ikcp_send, ikcp_input的话，否则中途调用了 ikcp_send, ikcp_input的话，需要在下一次interval时调用 update）
//标准顺序是每次调用了 ikcp_update后，使用 ikcp_check决定下次什么时间点再次调用 ikcp_update，而如果中途发生了 ikcp_send, ikcp_input 的话，在下一轮 interval 立马调用 ikcp_update和 ikcp_check。
//使用该方法，原来在处理2000个 kcp连接且每 个连接每10ms调用一次update，改为 check机制后，cpu从 60% 降低到 15%。

/**
 * 一个远程记录对象,用于绑定kcp对象.因为kcp需要设置一个回调函数output.
 *
 * @author daixian
 * @date 2020/12/19
 */
class KCPXUser
{
  public:
    // 远程对象名字
    const char* name = "unnamed";

    // 自己的socket,用于发送函数
    Poco::Net::DatagramSocket* socket = nullptr;

    // 要发送过去的地址
    Poco::Net::SocketAddress* remote = nullptr;
};

class KCPX::Impl
{
  public:
    Impl() {}
    ~Impl() {}

    // 用于监听握手用的kcp对象,id号为0
    ikcpcb* kcpAccept = nullptr;

    // 远程对象列表
    std::vector<ikcpcb*> remotes;
}

} // namespace dxlib