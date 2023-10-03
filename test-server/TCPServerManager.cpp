#include "TCPServerManager.h"
#include "log.h"
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/event.h>
#include <map>
#include "event-internal.h"
#include <event2/http.h>
#include <event2/ws.h>

static void
time_cb(evutil_socket_t fd, short event, void *arg)
{
}

static int
rand_int(int n)
{
	return n;
}


#define log_d(...) fprintf(stderr, __VA_ARGS__)

typedef struct client {
	struct evws_connection *evws;
	char name[INET6_ADDRSTRLEN];
	TAILQ_ENTRY(client) next;
} client_t;
typedef struct clients_s {
	struct client *tqh_first;
	struct client **tqh_last;
} clients_t;
static clients_t clients;

CTCPServerManager::CTCPServerManager()
{

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

void
client_event_cb(struct bufferevent *bev, short events, void *ctx)
{
	if (events & BEV_EVENT_EOF) {
		printf("Connection closed=[%d]\n", bufferevent_getfd(bev));
	} else if (events & BEV_EVENT_ERROR) {
		printf("Got an error on the connection: %s\n",
			strerror(errno)); /*XXX win32*/
	}
	/* None of the other events can happen here, since we haven't enabled
	 * timeouts */
	bufferevent_free(bev);
}

//static void
//callback3(evutil_socket_t , short, void *arg)
//{
//	WorkThreadInfo *pthread_info = (WorkThreadInfo *)arg;
//
//	std::vector<int> vecFd;
//	do {
//		//std::lock_guard<std::mutex> lck(pthread_info->mtx);
//		vecFd = pthread_info->vecFd;
//		pthread_info->vecFd.clear();
//	} while (false);
//
//	for (auto _fd : vecFd)
//	{
//		struct event_base *base = pthread_info->base;
//		struct bufferevent *bev= bufferevent_socket_new(base, _fd, BEV_OPT_CLOSE_ON_FREE);
//		if (!bev) {
//			fprintf(stderr, "Error constructing bufferevent!");
//			event_base_loopbreak(base);
//			return;
//		}
//		bufferevent_setcb(bev, client_read_cb, NULL, client_event_cb, arg);
//		bufferevent_enable(bev, EV_WRITE | EV_READ);
//	}
//
//}

bool
CTCPServerManager::Start()
{
	for (int i=0; i!=m_vecTcpSocketInfo.size(); i++)
	{
		std::thread thread = std::thread(CTCPServerManager::ThreadTcpDispatch, i);
		thread.detach();
	}

	return true;
}

bool
CTCPServerManager::Stop()
{
    for (int i = 0; i != m_vecTcpSocketInfo.size(); i++)
    {
		event_base_loopbreak(m_vecTcpSocketInfo[i]->base);
    }

    return true;
}




CTCPServerManager* CTCPServerManager::GetNetManager()
{
	return &m_netManager;
}

static void
htt_common_cb(struct evhttp_request *req, void *arg)
{
	evhttp_send_error(req, 200, "正确返回输出测试");
}

static void
on_close_cb(struct evws_connection *evws, void *arg)
{
	client_t *client = (client_t *)arg;
	printf("'%s' disconnected\n", client->name);
	TAILQ_REMOVE(&clients, client, next);
	free(arg);
}

static const char *
nice_addr(const char *addr)
{
	if (strncmp(addr, "::ffff:", 7) == 0)
		addr += 7;

	return addr;
}

static void
addr2str(struct sockaddr *sa, char *addr, size_t len)
{
	const char *nice;
	unsigned short port;
	size_t adlen;

	if (sa->sa_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)sa;
		port = ntohs(s->sin_port);
		evutil_inet_ntop(AF_INET, &s->sin_addr, addr, len);
	} else { // AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)sa;
		port = ntohs(s->sin6_port);
		evutil_inet_ntop(AF_INET6, &s->sin6_addr, addr, len);
		nice = nice_addr(addr);
		if (nice != addr) {
			size_t len = strlen(addr) - (nice - addr);
			memmove(addr, nice, len);
			addr[len] = 0;
		}
	}
	adlen = strlen(addr);
	snprintf(addr + adlen, len - adlen, ":%d", port);
}


static void
broadcast_msg(char *msg)
{
	struct client *client;

	TAILQ_FOREACH (client, &clients, next) {
		evws_send(client->evws, msg, strlen(msg));
	}
	log_d("%s\n", msg);
}

static void
on_msg_cb(struct evws_connection *evws, int type, const unsigned char *data,
	size_t len, void *arg)
{
	struct client *self = (client *)arg;
	char buf[4096];
	const char *msg = (const char *)data;

	snprintf(buf, sizeof(buf), "%.*s", (int)len, msg);
	if (len == 5 && memcmp(buf, "/quit", 5) == 0) {
		evws_close(evws, WS_CR_NORMAL);
		snprintf(buf, sizeof(buf), "'%s' left the chat", self->name);
	} else if (len > 6 && strncmp(msg, "/name ", 6) == 0) {
		const char *new_name = (const char *)msg + 6;
		int name_len = len - 6;

		snprintf(buf, sizeof(buf), "'%s' renamed itself to '%.*s'", self->name,
			name_len, new_name);
		snprintf(
			self->name, sizeof(self->name) - 1, "%.*s", name_len, new_name);
	} else {
		snprintf(buf, sizeof(buf), "[%s] %.*s", self->name, (int)len, msg);
	}

	broadcast_msg(buf);
}


static void
on_ws(struct evhttp_request *req, void *arg)
{
	struct client *cli = (client *)malloc(sizeof(client));
	evutil_socket_t fd;
	struct sockaddr_storage addr;
	socklen_t len;


	cli->evws = evws_new_session(req, on_msg_cb, cli, 0);
	if (!cli->evws) {
		free(cli);
		log_d("Failed to create session\n");
		return;
	}

	fd = bufferevent_getfd(evws_connection_get_bufferevent(cli->evws));

	len = sizeof(addr);
	getpeername(fd, (struct sockaddr *)&addr, &len);
	addr2str((struct sockaddr *)&addr, cli->name, sizeof(cli->name));

	log_d("New client joined from %s\n", cli->name);

	evws_connection_set_closecb(cli->evws, on_close_cb, cli);

	do {
		(cli)->next.tqe_next = 0;
		(cli)->next.tqe_prev = (&clients)->tqh_last;
		*(&clients)->tqh_last = (cli);
		(&clients)->tqh_last = &(cli)->next.tqe_next;
	} while (0);
}


int
CTCPServerManager::AddWebSocket(const char *ip, int port,
	const std::vector<HttpPathCallBack> &path_cb)
{
	//WebSocketInfo tmp = {};
	//
	//memcpy(tmp.bind_addr, ip, strlen(ip));
	//tmp.bind_port = port;
	//tmp.path_cb = path_cb;

	//m_vecWebSocketInfo.emplace_back(std::move(tmp));

	return 0;
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

void
CTCPServerManager::ThreadSendMsg(CTCPServerManager *pThreadData)
{
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

    bufferevent_setcb(bev, CTCPServerManager::ReadCB, NULL, client_event_cb, user_data);
    bufferevent_enable(bev, EV_WRITE | EV_READ);

	pListenInfo->connectd_info[fd] = bev;
}

void
CTCPServerManager::ReadCB(struct bufferevent *bev, void *data)
{
    char msg[1024] = {};
    bufferevent_read(bev, msg, sizeof(msg));

    printf("client msg=[%d:%d:%s]\n", std::this_thread::get_id(), bufferevent_getfd(bev), msg);
}

void
CTCPServerManager::EventCB(struct bufferevent *, short, void *)
{
}

void
CTCPServerManager::AcceptErrorCB(struct evconnlistener *listener, void *)
{
	int a = 0;
}


bool
CTCPServerManager::RecvData(bufferevent *bev)
{
	// if (bev == NULL)
	//{
	//     ERROR_LOG("RecvData error bev == NULL");
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


CTCPServerManager CTCPServerManager::m_netManager;

bool
CTCPServerManager::CloseSocket(bufferevent *bev)
{
	int fd = bufferevent_getfd(bev);


	bufferevent_free(bev);


	return true;
}
