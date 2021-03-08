#pragma once

#include <string>
#include <vector>
#include <map>

#include "Accept.h"

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/DatagramSocket.h"

#include "../kcp/ikcp.h"
#include "../kcp/clock.hpp"

#include "Protocol/FastPacket.h"
#include "dlog/dlog.h"

namespace dnet {

/**
 * KCP的数据收发,这个实现使用的是非阻塞套接字.
 * 一个对象带有一个socket,然后需要支持和多个对象之间的通信.
 * 这个类附加在TCPClient里面,TCP连接上之后先进行一次握手,主要是交换UUID和信道.
 *
 * @author daixian
 * @date 2020/5/12
 */
class KCPClient
{
  public:
    KCPClient();
    ~KCPClient();

    // 是否是TCPServer端所属的Client
    bool isServer = false;

    // 自己的UDPSocket,只用于发送函数(对象的生命周期在外面管理).
    Poco::Net::DatagramSocket* socket = nullptr;

    // 要用UPD发送过去的地址.
    Poco::Net::SocketAddress remote;

    // kcp对象.
    ikcpcb* kcp = nullptr;

    // kcp接收数据buffer.
    std::vector<char> kcpReceBuf;

    //// 接收到的待处理的数据.
    //std::vector<std::string> receData;

    // 所有接受到的消息的总条数
    int receMsgCount = 0;

    // 所有发送的消息的总条数
    int sendMsgCount = 0;

    // TCP通信协议(这里先临时也使用这个,主要是要打进去一个msg type,好和tcp端一致)
    FastPacket packet;

    /**
     * kcp的id.
     *
     * @author daixian
     * @date 2021/1/11
     *
     * @returns id.
     */
    int Conv()
    {
        if (kcp == nullptr) {
            //这里经常作为有没有初始化的一并判断
            //LogE("KCPClient.Conv():kcp == nullptr,没有conv..");
            return -1;
        }
        return kcp->conv;
    }

    /**
     * 创建一个kcp协议.
     *
     * @author daixian
     * @date 2021/1/11
     *
     * @param  conv 通信id.
     */
    void Create(int conv);

    /**
     * 绑定一个和TCP一致的UDP端口，当tcp断线重连之后需要重新绑定这个UDP端口.因此这个对象不做UDP端口生命周期的管理.
     *
     * @author daixian
     * @date 2021/1/9
     *
     * @param [in] datagramSocket udp端口对象.
     * @param      remote         远端地址.
     */
    void Bind(Poco::Net::DatagramSocket* datagramSocket, const Poco::Net::SocketAddress& remote)
    {
        this->socket = datagramSocket;
        this->remote = remote;
    }

    /**
     * (内部调用)传入UPD的Socket的API接收到的数据,如果返回-1表示当前接收到的不是信道的消息. 由于kcp的协议，只有接收成功了这里才会返回>0的实际接收消息条数.
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param       buff Socket接收的结果.
     * @param       len  Socket接收到的数据长度.
     * @param [out] msgs 以conv为key的所有客户端的消息.
     *
     * @returns 返回大于0的实际接收到的消息条数.
     */
    int Receive(const char* buff, size_t len, std::vector<TextMessage>& msgs)
    {
        if (socket == nullptr || kcp == nullptr) {
            LogE("KCPClient.Receive():还没有初始化,不能接收!");
            return -2;
        }

        msgs.clear();

        if (len != -1) {
            //LogD("KCPClient.Receive(): Socket接收到了数据,长度%d", len);

            //尝试给kcp看看是否是它的信道的数据
            int rece = ikcp_input(kcp, buff, (long)len);
            if (rece != 0) {
                //conv不对应或者其它错误
                return -1;
            }
            else {
                //ikcp_flush(kcp); //尝试暴力flush

                while (rece >= 0) {
                    rece = ikcp_recv(kcp, kcpReceBuf.data(), (int)kcpReceBuf.size());
                    if (rece > 0) {
                        //这里实际上应该只能找到1条消息
                        std::vector<TextMessage> msg1;
                        receMsgCount += packet.Unpack(kcpReceBuf.data(), rece, msg1);
                        for (size_t i = 0; i < msg1.size(); i++) {
                            msgs.push_back(msg1[i]);
                        }
                        //receData.push_back(std::string(kcpReceBuf.data(), rece)); //拷贝记录这一条收到的信息
                    }
                }
            }
        }

        //msgs = receData;
        //receData.clear();

        ikcp_update(kcp, iclock()); //需要定时调用.在调用接收或者发送之后调用好了
        return (int)msgs.size();
    }

    /**
     * 使用UDPSocket非阻塞的发送一段数据.正常发送成功返回0.
     *
     * @author daixian
     * @date 2020/5/12
     *
     * @param  data 要发送的数据.
     * @param  len  数据长度.
     *
     * @returns 正常发送成功返回0.
     */
    int Send(const char* data, size_t len, int type = -1)
    {
        if (socket == nullptr || kcp == nullptr) {
            LogE("KCPClient.Send():还没有初始化,不能发送!");
            return -1;
        }

        //数据打包
        std::vector<char> package;
        packet.Pack(data, (int)len, package, type);
        sendMsgCount++;

        int res = ikcp_send(kcp, package.data(), (int)package.size());
        if (res < 0) {
            LogE("KCPClient.Send():发送异常返回 res=%d", res);
        }
        //ikcp_flush(kcp); //尝试暴力flush

        ikcp_update(kcp, iclock());
        return res;
    }

    /**
     * 提供出来让他们可以无脑Update
     *
     * @author daixian
     * @date 2021/1/25
     */
    void Update()
    {
        if (kcp == nullptr) {
            LogE("KCPClient.Update():还没有初始化,不能发送!");
            return;
        }
        ikcp_update(kcp, iclock());
    }

    /**
     * 提供出来让他们可以暴力flush
     *
     * @author daixian
     * @date 2021/1/25
     */
    void Flush()
    {
        if (kcp == nullptr) {
            LogE("KCPClient.flush():还没有初始化,不能发送!");
            return;
        }
        ikcp_flush(kcp); //尝试暴力flush
    }

    /**
     * 当前等待发送的消息计数.如果这个数量太多,那么已经拥塞.
     *
     * @author daixian
     * @date 2020/12/20
     *
     * @returns An int.
     */
    int WaitSendCount()
    {
        if (socket == nullptr || kcp == nullptr) {
            return 0;
        }

        return ikcp_waitsnd(kcp);
    }

  private:
};

} // namespace dnet