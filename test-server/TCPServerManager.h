#ifndef _TCP_SERVER_HEADER_
#define _TCP_SERVER_HEADER_
#pragma once

#include "ServerHandler.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <thread>
#include <event2/util.h>
#include <mutex>
#include <string>
#include "evconfig-private.h"
#include <map>
struct bufferevent;


const int MAX_PACKAGE_SIZE = 1024 * 8;


struct event_base;
struct event;
struct evconnlistener;
class CTCPServerManager;

typedef void (*http_path_cb)(struct evhttp_request *, void *);


//
struct HttpPathCallBack
{
    std::string path;
    http_path_cb cb;
    void* args;
};

// 工作线程信息
struct WorkThreadInfo
{
    struct event_base* base;
	struct event *event_notify;

    CTCPServerManager* pThis;
};


struct TcpListenInfo
{
    int max_connect;
    int socket_type;
    event_base* base;
    evconnlistener* listener;
    std::map<int, bufferevent*> connectd_info;
};


class CTCPServerManager
{
public:
    //
	CTCPServerManager();
	//
    ~CTCPServerManager();

public:
	// 添加监听信息
	int AddTcpListenInfo(int socketType, const char* addr, int port, int maxCount);
	// 添加监听信息
	int AddWebSocket(const char *ip, int port, const std::vector<HttpPathCallBack>& path_cb);
    // 开始服务
    bool Start();
    // 停止服务
    bool Stop();
    //
    static CTCPServerManager* GetNetManager();


protected:
    // SOCKET 数据接收线程
    static void ThreadTcpDispatch(int nTcpInfoIndex);
    // 外部需要发送的消息, 由此线程写入 buff
    static void ThreadSendMsg(CTCPServerManager* pThreadData);

protected:
    // 新的连接到来，ThreadAccept线程函数
    static void TcpListenCB(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_data);
    // 新的数据到来
    static void ReadCB(struct bufferevent*, void*);
    // 连接关闭等等错误消息
    static void EventCB(struct bufferevent*, short, void*);
    // accept失败，ThreadAccept线程函数
    static void AcceptErrorCB(struct evconnlistener* listener, void*);

private:
    // 最底层处理收到的数据函数
    bool RecvData(bufferevent* bev);
    // 关闭socket
    bool CloseSocket(bufferevent* bev);

private:
    // 返回 socket 本地的 xx.xx.xx.xx:xxxx 形式的字符串串
    static std::string GetSocketNameIpAndPort(int fd);
    // 返回 socket 对端的 xx.xx.xx.xx:xxxx 形式的字符串串
    static std::string GetSocketPeerIpAndPort(int fd);

private:
    event_base* m_LibeventListenBase;

    // m_mapFd2SocketInfo 在监听线程会更改, 在 sendMsg 中可能会关闭连接时更改
    std::mutex m_mtxFd2SocketInfo;

    // accept 线程信息
    std::thread m_threadAccept;

    // 
    std::thread m_threadSendMsg;

	// 注册的 tcp 监听信息
	std::vector<std::shared_ptr<TcpListenInfo>> m_vecTcpSocketInfo;

    // 注册的 web socket 信息
	//std::vector<WebSocketInfo> m_vecWebSocketInfo;
	// 工作线程信息
	std::vector<WorkThreadInfo> m_workBaseVec;

private:
    static CTCPServerManager m_netManager;

};


#endif
