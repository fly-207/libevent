#include "TCPServerManager.h"
#include "log.h"
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/event.h>
#include <map>
//#include "event-internal.h"
#include <event2/http.h>
#include <event2/ws.h>
#include <cstring>

#ifndef _WIN32
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#endif

CTCPServerManager::CTCPServerManager()
{
	m_pConnectBase = nullptr;
}

CTCPServerManager::~CTCPServerManager()
{

}


int CTCPServerManager::AddTcpListenInfo(int socketType, const char* addr, int port, int maxCount)
{
    event_base* base = event_base_new();
    if (!base)
    {
        exit(1);
    }

    // 
    sockaddr_in sin = {};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(addr);

    evconnlistener* listener = evconnlistener_new_bind(base, 
		TcpListenCB, 
		(void*)m_vecTcpSocketInfo.size(),
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
        -1, (struct sockaddr*)&sin, sizeof(sin));

    if (!listener) {
        perror("Could not create a listener!");
        exit(1);
    }

    evconnlistener_set_error_cb(listener, AcceptErrorCB);

	std::shared_ptr<TcpListenInfo> ptr_listen_info = std::make_shared<TcpListenInfo>();
	ptr_listen_info->base = base;
	ptr_listen_info->max_connect = maxCount;
	ptr_listen_info->socket_type = socketType;
	ptr_listen_info->listener = listener;
	m_vecTcpSocketInfo.emplace_back(ptr_listen_info);

    return 0;
}



bool
CTCPServerManager::Start()
{
	for (int i=0; i!=m_vecTcpSocketInfo.size(); i++)
	{
		std::thread thread = std::thread(CTCPServerManager::ThreadTcpDispatch, i);
		thread.detach();
	}

    for (int i = 0; i != m_vecHttpWorkerInfo.size(); i++)
    {
        std::thread thread = std::thread(CTCPServerManager::ThreadHttpDispatch, i);
        thread.detach();
    }

    for (int i = 0; i != m_vecWebSocketWorkerInfo.size(); i++)
    {
        std::thread thread = std::thread(CTCPServerManager::ThreadWebSocketDispatch, i);
        thread.detach();
    }

	m_pConnectTimeOut = event_new(m_pConnectBase, -1, EV_PERSIST, CTCPServerManager::ConnectBaseTimeOutCB, NULL);
	timeval tv_onesec = { 1,0 };
	event_add(m_pConnectTimeOut, &tv_onesec);
	std::thread(CTCPServerManager::ThreadConnectDispatch).detach();

	return true;
}

bool
CTCPServerManager::Stop()
{
    for (auto& _item : m_vecTcpSocketInfo)
    {
        event_base_loopbreak(_item->base);
    }

    for (auto &_item: m_vecHttpWorkerInfo)
    {
        event_base_loopbreak(_item->base);
    }

    for (auto& _item : m_vecWebSocketWorkerInfo)
    {
        event_base_loopbreak(_item->base);
    }

    return true;
}


CTCPServerManager* CTCPServerManager::GetNetManager()
{

    static CTCPServerManager *pNetManager = NULL;
	if (!pNetManager)
	{
		pNetManager = new CTCPServerManager();
		pNetManager->InitConnectBase();
	}

	return pNetManager;
}


void CTCPServerManager::TcpConnectRecvCB(struct bufferevent* bev, void* ctx)
{

}

void CTCPServerManager::TcpConnectEventCB(struct bufferevent* bev, short events, void* ctx)
{
	// 链接成功
	if (events & BEV_EVENT_CONNECTED)
    {
		printf("client connect success[fd=%d]\n", bufferevent_getfd(bev));
        return;
    }
	// 超时事件
	else if (events & BEV_EVENT_TIMEOUT)
	{
        printf("time out event[fd=%d]\n", bufferevent_getfd(bev));
	}
	else
	{
	    bufferevent_free(bev);
        CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();
		int nConnectIndex = (int)ctx;

		pNetManager->m_vecConnectInfo[nConnectIndex]->bev = NULL;
	}
}

int CTCPServerManager::AddHttpInfo(const char * addr, int port,
	const std::vector<HttpPathCallBack> &path_cb)
{
    event_base* base = event_base_new();
    if (!base)
    {
        exit(1);
    }

    auto *http = evhttp_new(base);
    if (!http) {
		exit(1);
    }
	evhttp_bind_socket(http, addr, port);

	for (auto& _item : path_cb)
	{
        evhttp_set_cb(http, _item.path.c_str(), _item.cb, _item.args);
	}

    std::shared_ptr<HttpWorkerInfo> ptr_listen_info = std::make_shared<HttpWorkerInfo>();
    ptr_listen_info->base = base;
    ptr_listen_info->http = http;
    ptr_listen_info->path = path_cb;
	m_vecHttpWorkerInfo.emplace_back(ptr_listen_info);

    return 0;
}


int CTCPServerManager::AddWebSocketInfo(int socketType, const char* addr, int port, const char * ws_path, int maxCount)
{
    event_base* base = event_base_new();
    if (!base)
    {
        exit(1);
    }

    auto* http = evhttp_new(base);
    if (!http) {
        exit(1);
    }
    evhttp_bind_socket(http, addr, port);

	evhttp_set_cb(http, ws_path, WebSocketFromHttpCB, (void *)m_vecWebSocketWorkerInfo.size());

    std::shared_ptr<WebSocketWorkerInfo> ptr_listen_info = std::make_shared<WebSocketWorkerInfo>();
    ptr_listen_info->base = base;
    ptr_listen_info->http = http;
	ptr_listen_info->socket_type = socketType;
    ptr_listen_info->max_connect = maxCount;
    m_vecWebSocketWorkerInfo.emplace_back(ptr_listen_info);

    return 0;
}

int CTCPServerManager::AddTcpConnectInfo(int socketType, const char* addr, int port)
{
	std::shared_ptr<TcpClientInfo> _tmp = std::make_shared<TcpClientInfo>();
	memcpy(_tmp->addr, addr, strlen(addr));
	_tmp->port = port;
	_tmp->socket_type = socketType;
	_tmp->bev = NULL;
	m_vecConnectInfo.emplace_back(std::move(_tmp));

	return 0;
}

void CTCPServerManager::InitConnectBase()
{
	if (m_pConnectBase)
		return;

	m_pConnectBase = event_base_new();
}

void CTCPServerManager::ThreadTcpDispatch(int nTcpInfoIndex)
{
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();
    std::shared_ptr<TcpListenInfo> pListenInfo = pNetManager->m_vecTcpSocketInfo[nTcpInfoIndex];

    printf("event loop start[%d]================================\n", std::this_thread::get_id());

    event_base_dispatch(pListenInfo->base);

    printf("event loop end[%d]================================\n", std::this_thread::get_id());

    //
	evconnlistener_free(pListenInfo->listener);
    event_base_free(pListenInfo->base);
}

void CTCPServerManager::ThreadHttpDispatch(int nHttpInfoIndex)
{
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();
	auto pInfo = pNetManager->m_vecHttpWorkerInfo[nHttpInfoIndex];

    printf("event loop for http start[%d]================================\n", std::this_thread::get_id());

    event_base_dispatch(pInfo->base);

    printf("event loop for http end[%d]================================\n", std::this_thread::get_id());

    // 内部会释放已有的链接
	evhttp_free(pInfo->http);

    event_base_free(pInfo->base);
}

void CTCPServerManager::ThreadWebSocketDispatch(int nWebSocketInfoIndex)
{
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();
    auto pInfo = pNetManager->m_vecWebSocketWorkerInfo[nWebSocketInfoIndex];

    printf("event loop for web socket start[%d]================================\n", std::this_thread::get_id());

    event_base_dispatch(pInfo->base);

    printf("event loop for web socket end[%d]================================\n", std::this_thread::get_id());

    // 内部会释放已有的链接
    evhttp_free(pInfo->http);

    event_base_free(pInfo->base);
}

void CTCPServerManager::ThreadConnectDispatch()
{
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();

    printf("event loop for connect start[%d]================================\n", std::this_thread::get_id());

    event_base_dispatch(pNetManager->m_pConnectBase);

    printf("event loop for connect end[%d]================================\n", std::this_thread::get_id());

    event_base_free(pNetManager->m_pConnectBase);
}

void
CTCPServerManager::ThreadSendMsg(CTCPServerManager *pThreadData)
{
}

void CTCPServerManager::ConnectBaseTimeOutCB(evutil_socket_t fd, short event, void* arg)
{
	printf("connect timeout callback [timestamp=%d]\n", time(0));
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();

	if (!pNetManager->m_pConnectBase)
		return;

	for (int i=0; i< pNetManager->m_vecConnectInfo.size();i++)
	{
		auto& _item = pNetManager->m_vecConnectInfo[i];
		if(_item->bev)
			continue;


        struct bufferevent* bev = bufferevent_socket_new(pNetManager->m_pConnectBase, -1, BEV_OPT_CLOSE_ON_FREE);
        if (!bev)
        {
			continue;
        }

        //
        sockaddr_in sin = {};
        sin.sin_addr.s_addr = inet_addr(_item->addr);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(_item->port);

        // 内部会自动创建 fd
        if (bufferevent_socket_connect(bev, (struct sockaddr*)&sin, sizeof(sin)) != 0)
        {
			bufferevent_free(bev);
			continue;
        }


        bufferevent_setcb(bev, CTCPServerManager::TcpConnectRecvCB, NULL, CTCPServerManager::TcpConnectEventCB, (void*)i);
        bufferevent_enable(bev, EV_READ | EV_WRITE);

		_item->bev = bev;

        //CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();

        //timeval tv_onesec = { 1,0 };
        //event_add(pNetManager->m_pConnectTimeOut, &tv_onesec);
	}

}

void
CTCPServerManager::TcpListenCB(struct evconnlistener *listener,
	evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
{
	int nSockInfoIndex = (int)user_data;
	CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();
	std::shared_ptr<TcpListenInfo> pListenInfo = pNetManager->m_vecTcpSocketInfo[nSockInfoIndex];


	bufferevent* bev = bufferevent_socket_new(pListenInfo->base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(pListenInfo->base);
        return;
    }

    bufferevent_setcb(bev, CTCPServerManager::TcpReadCB, NULL, CTCPServerManager::TcpEventCB, user_data);
    bufferevent_enable(bev, EV_WRITE | EV_READ);

	pListenInfo->connectd_info[fd] = bev;

}

void
CTCPServerManager::TcpReadCB(struct bufferevent *bev, void *data)
{
    int nSockInfoIndex = (int)data;
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();
    std::shared_ptr<TcpListenInfo> pListenInfo = pNetManager->m_vecTcpSocketInfo[nSockInfoIndex];

    char msg[1024] = {};
    bufferevent_read(bev, msg, sizeof(msg));

    printf("client msg=[%d:%d:%s]\n", std::this_thread::get_id(), bufferevent_getfd(bev), msg);
}


void
CTCPServerManager::AcceptErrorCB(struct evconnlistener *listener, void *)
{
	int a = 0;
}


void CTCPServerManager::TcpEventCB(struct bufferevent* bev, short events, void* ctx)
{
    int nSockInfoIndex = (int)ctx;
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();
    auto pInfo = pNetManager->m_vecTcpSocketInfo[nSockInfoIndex];

	auto fd = bufferevent_getfd(bev);
	pInfo->connectd_info.erase(fd);

	bufferevent_free(bev);
}

void CTCPServerManager::WebSocketFromHttpCB(struct evhttp_request* req, void* arg)
{
    int nInfoIndex = (int)arg;
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();
    auto pInfo = pNetManager->m_vecWebSocketWorkerInfo[nInfoIndex];

    auto evws = evws_new_session(req, CTCPServerManager::WebSocketRecvData, arg, 0);
    if (!evws) {
        return;
    }

	evws_connection_set_closecb(evws, CTCPServerManager::WebSocketClosed, arg);

	evutil_socket_t fd = bufferevent_getfd(evws_connection_get_bufferevent(evws));
	pInfo->connectd_info[fd] = evws;
}

bool
CTCPServerManager::TcpRecvData(bufferevent *bev)
{
	// if (bev == NULL)
	//{
	//     ERROR_LOG("TcpRecvData error bev == NULL");
	//     return false;
	// }

	// struct evbuffer* input = bufferevent_get_input(bev);

	//// 当前读取位置, 方便后面一次性更新
	// evbuffer_ptr buff_ptr = {};

	//// 总的字节数
	// size_t nAllSize = evbuffer_get_length(input);

	// char buff[MAX_PACKAGE_SIZE] = {};

	// while (true)
	//{
	//     // 剩余字节小于包头
	//     if (nAllSize- buff_ptr.pos < sizeof(NetMessageHead))
	//         break;

	//    // 消息头
	//    NetMessageHead tmpHead = {};

	//    // 先读取消息头
	//    size_t head_size = evbuffer_copyout_from(input, &buff_ptr, &tmpHead,
	//    sizeof(NetMessageHead)); if (head_size <= sizeof(NetMessageHead))
	//        break;


	//    // 包体超过最大长度限制
	//    if (tmpHead.uMessageSize > MAX_PACKAGE_SIZE)
	//    {
	//        CloseSocket(bev);
	//        return false;
	//    }

	//    // 包体未完全接收
	//    if(nAllSize-buff_ptr.pos-sizeof(NetMessageHead) <
	//    tmpHead.uMessageSize)
	//        break;

	//    evbuffer_ptr _tmp_ptr = {};
	//    _tmp_ptr.pos = sizeof(NetMessageHead) + buff_ptr.pos;


	//    // 读取包体
	//    size_t body_size = evbuffer_copyout_from(input, &_tmp_ptr, &buff,
	//    tmpHead.uMessageSize); if (body_size <= tmpHead.uMessageSize)
	//        break;


	//    // 粘包处理
	//    while (handleRemainSize >= sizeof(NetMessageHead) && handleRemainSize
	//    >= pNetHead->uMessageSize)
	//    {
	//        unsigned int messageSize = pNetHead->uMessageSize;
	//        if (messageSize > MAX_TEMP_SENDBUF_SIZE)
	//        {
	//            // 消息格式不正确
	//            CloseSocket(index);
	//            ERROR_LOG("close socket
	//            超过单包数据最大值,index=%d,messageSize=%u", index,
	//            messageSize); return false;
	//        }

	//        int realSize = messageSize - sizeof(NetMessageHead);
	//        if (realSize < 0)
	//        {
	//            // 数据包不够包头
	//            CloseSocket(index);
	//            ERROR_LOG("close socket 数据包不够包头");
	//            return false;
	//        }

	//        void* pData = NULL;
	//        if (realSize > 0)
	//        {
	//            // 没数据就为NULL
	//            pData = (void*)(buf + realAllSize - handleRemainSize +
	//            sizeof(NetMessageHead));
	//        }

	//        // 派发数据
	//        DispatchPacket(bev, index, pNetHead, pData, realSize);

	//        handleRemainSize -= messageSize;

	//        pNetHead = (NetMessageHead*)(buf + realAllSize -
	//        handleRemainSize);
	//    }

	//    evbuffer_drain(input, realAllSize - handleRemainSize);

	//    return true;
	//}

	return true;
}


void CTCPServerManager::WebSocketRecvData(struct evws_connection* evws, int type, const unsigned char* data, size_t len, void* arg)
{
	char szRecv[1024] = {};
	memcpy(szRecv, data, len);

    evutil_socket_t fd = bufferevent_getfd(evws_connection_get_bufferevent(evws));

    printf("websocket recv:[fd=%d type=%d len=%d data=%s]\n", fd,type, len, szRecv);

	evws_close(evws, WS_CR_NORMAL);
}

void CTCPServerManager::WebSocketClosed(struct evws_connection* evws, void* arg)
{
	/*
		evws 不需要手动关闭, 当对应的 http-connection 释放时会随着释放
	*/

    int nInfoIndex = (int)arg;
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();
    auto pInfo = pNetManager->m_vecWebSocketWorkerInfo[nInfoIndex];

    evutil_socket_t fd = bufferevent_getfd(evws_connection_get_bufferevent(evws));

    printf("websocket closed [fd=%d]\n", fd);

	pInfo->connectd_info.erase(fd);
}

std::string
CTCPServerManager::GetSocketNameIpAndPort(int fd)
{
	struct sockaddr_in addr = {};
	int addr_len = sizeof(addr);

	int ret = getsockname(fd, (struct sockaddr *)&addr, &addr_len);

	char out[1024] = {};
	int realLen = snprintf(out, sizeof(out), "connected address = %s:%d\n",
		inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	return out;
}

std::string
CTCPServerManager::GetSocketPeerIpAndPort(int fd)
{
	struct sockaddr_in addr = {};
	int addr_len = sizeof(addr);

	int ret = getpeername(fd, (struct sockaddr *)&addr, &addr_len);

	char out[1024] = {};
	int realLen = snprintf(out, sizeof(out), "connected address = %s:%d\n",
		inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	return out;
}
