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
#include <event2/ws.h>
#include <event2/http.h>
#include <list>
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
struct WebSocketWorkerInfo
{
    int max_connect;
    int socket_type;
    event_base* base;
    evhttp* http;
    std::map<int, evws_connection*> connectd_info;
};


struct HttpWorkerInfo
{
    event_base* base;
    evhttp* http;
    std::vector< HttpPathCallBack> path;
};


struct TcpListenInfo
{
    int max_connect;
    int socket_type;
    event_base* base;
    evconnlistener* listener;
    std::map<int, bufferevent*> connectd_info;
};

struct TcpClientInfo
{
    int socket_type;
    char addr[32];
    int port;

    bufferevent* bev;
};

struct ConnectedInfo
{
    int socket_type;
    int fd;
    int index;
};

struct RecvMsg
{
    ConnectedInfo connect_info;
    evbuffer* buff;
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
	int AddHttpInfo(const char * addr, int port, const std::vector<HttpPathCallBack>& path_cb);
    // 添加 websocket
    int AddWebSocketInfo(int socketType, const char* addr, int port, const char * ws_path, int maxCount);
    //
    int AddTcpConnectInfo(int socketType, const char* addr, int port);
    
    // 开始服务
    bool Start();
    // 停止服务
    bool Stop();
    //
    static CTCPServerManager* GetNetManager();


private:
    //
    static void TcpConnectRecvCB(struct bufferevent* bev, void* ctx);
    //
    static void TcpConnectEventCB(struct bufferevent* bev, short what, void* ctx);

protected:
    //
    void InitConnectBase();


protected:
    // SOCKET 数据接收线程
    static void ThreadTcpDispatch(int nTcpInfoIndex);
    //
    static void ThreadHttpDispatch(int nHttpInfoIndex);
    //
    static void ThreadWebSocketDispatch(int nWebSocketInfoIndex);
    //
    static void ThreadConnectDispatch();

public:
    // 外部需要发送的消息, 由此线程写入 buff
    static void ThreadSendMsg(CTCPServerManager* pThreadData);

public:
    //
    static void ConnectBaseTimeOutCB(evutil_socket_t fd, short event, void* arg);

protected:
    // 新的连接到来，ThreadAccept线程函数
    static void TcpListenCB(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_data);
    // 新的数据到来
    static void TcpReadCB(struct bufferevent*, void*);
    // accept失败，ThreadAccept线程函数
    static void AcceptErrorCB(struct evconnlistener* listener, void*);
    //
    static void TcpEventCB(struct bufferevent* bev, short events, void* ctx);
    // 最底层处理收到的数据函数
    bool TcpRecvData(bufferevent* bev);

protected:
    // 
    static void WebSocketFromHttpCB(struct evhttp_request* req, void* arg);

private:
    //
    static void WebSocketRecvData(struct evws_connection* evws, int type, const unsigned char* data, size_t len, void* arg);
    //
    static void WebSocketClosed(struct evws_connection* evws, void* arg);

private:
    // 返回 socket 本地的 xx.xx.xx.xx:xxxx 形式的字符串串
    static std::string GetSocketNameIpAndPort(int fd);
    // 返回 socket 对端的 xx.xx.xx.xx:xxxx 形式的字符串串
    static std::string GetSocketPeerIpAndPort(int fd);

private:
    //
    event_base* m_pConnectBase;
    event* m_pConnectTimeOut;

	// 注册的 tcp 监听信息
	std::vector<std::shared_ptr<TcpListenInfo>> m_vecTcpSocketInfo;

    // 注册的 http 信息
	std::vector<std::shared_ptr<HttpWorkerInfo>> m_vecHttpWorkerInfo;

    // 注册的 web socket 信息
    std::vector<std::shared_ptr<WebSocketWorkerInfo>> m_vecWebSocketWorkerInfo;

    // 注册的 client 信息
    std::vector<std::shared_ptr<TcpClientInfo>> m_vecConnectInfo;

    // 被动关闭 fd 列表, 等待逻辑处理
    std::mutex  m_mtxClosedFd;
    std::list<ConnectedInfo> m_vecClosedFd;

    // 主动关闭 fd 列表, 等待底层 socket 关闭
    std::mutex m_mtxWaitCloseFd;
    std::list<ConnectedInfo> m_vecWaitCloseFd;

    // 等待逻辑处理所有消息, 消息是单线程处理的, 消息要有顺序
    std::mutex m_mtxRecvMsg;
    std::list<RecvMsg> m_vecRecvMsg;
};


#endif
