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

CTCPServerManager::CTCPServerManager(int nWorkSize)
	: m_workBaseVec(1)
{
	m_uCurSocketSize = 0;
}

CTCPServerManager::~CTCPServerManager()
{
	m_threadAccept.join();
	m_threadSendMsg.join();
}

bool
CTCPServerManager::init(IServerSocketHander *pService, int maxCount, int port,
	const char *ip, int socketType)
{
	INFO_LOG("service CTCPServerManager init begin...");

	if (!pService || maxCount <= 0 || port <= 1000) {
		ERROR_LOG("invalid params input pService=%p maxCount=%d port=%d",
			pService, maxCount, port);
		return false;
	}

	m_pService = pService;
	m_uMaxSocketSize = maxCount;
	if (ip && strlen(ip) < sizeof(m_bindIP)) {
		strcpy(m_bindIP, ip);
	}
	m_port = port;
	m_nSocketType = socketType;

	return true;
}


void client_read_cb(struct bufferevent* bev, void* ctx)
{
	char msg[1024] = {};
	bufferevent_read(bev, msg, sizeof(msg));

	printf("���յ��ͻ�����Ϣ=[%d:%s]\n",bufferevent_getfd(bev), msg);
}

void client_write_cb(struct bufferevent* bev, void* ctx)
{

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

static void
callback3(evutil_socket_t , short, void *arg)
{
	WorkThreadInfo *pthread_info = (WorkThreadInfo *)arg;

	std::vector<int> vecFd;
	do {
		std::lock_guard<std::mutex> lck(pthread_info->mtx);
		vecFd = pthread_info->vecFd;
		pthread_info->vecFd.clear();
	} while (false);

	for (auto _fd : vecFd)
	{
		struct event_base *base = pthread_info->base;
		struct bufferevent *bev= bufferevent_socket_new(base, _fd, BEV_OPT_CLOSE_ON_FREE);
		if (!bev) {
			fprintf(stderr, "Error constructing bufferevent!");
			event_base_loopbreak(base);
			return;
		}
		bufferevent_setcb(bev, client_read_cb, NULL, client_event_cb, arg);
		bufferevent_enable(bev, EV_WRITE | EV_READ);
	}

}

bool
CTCPServerManager::Start()
{
	INFO_LOG("service TCPSocketManage start begin...");

	// ���տͻ���������Ϣ, �����ⲿ����ת����Ϣ
	m_threadAccept = std::thread(ThreadAccept, this);

	// ��ʼ�����߳���Ϣ
	int index = 0;
	for (auto &_item : m_workBaseVec) {
		_item.base = event_base_new();
		if (!_item.base) {
			CON_ERROR_LOG("TCP Could not initialize libevent!");
			exit(1);
		}

		_item.event_notify = evuser_new(_item.base, callback3, &_item);

		_item.thread = std::thread(ThreadRSSocket, &_item);
		_item.pThis = this;
	}

	// �ⲿ����д��ͻ��˵���Ϣ�ڴ��߳�д�뵽 buffer
	m_threadSendMsg = std::thread(ThreadSendMsg, this);

	return true;
}

bool
CTCPServerManager::Stop()
{
	INFO_LOG("service TCPSocketManage stop begin...");

	event_base_loopbreak(evconnlistener_get_base(m_listenThreadInfo.listener));
	for (size_t i = 0; i < m_workBaseVec.size(); i++) {
		event_base_loopbreak(m_workBaseVec[i].base);
	}

	INFO_LOG("service TCPSocketManage stop end...");

	return true;
}

static void
htt_common_cb(struct evhttp_request *req, void *arg)
{
	evhttp_send_error(req, 200, "��ȷ�����������");
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

void
CTCPServerManager::AddWebSocket(const char *ip, int port)
{
	m_WebSocketInfo.base = event_base_new();
	m_WebSocketInfo.http_server = evhttp_new(m_WebSocketInfo.base);
	evhttp_bind_socket_with_handle(m_WebSocketInfo.http_server, ip, port);

	evhttp_set_cb(m_WebSocketInfo.http_server, "/", htt_common_cb, NULL);
	evhttp_set_cb(m_WebSocketInfo.http_server, "/ws", on_ws, NULL);

	do {
		(&clients)->tqh_first = 0;
		(&clients)->tqh_last = &(&clients)->tqh_first;
	} while (0);

	event_base_dispatch(m_WebSocketInfo.base);
}

void
CTCPServerManager::ThreadAccept(CTCPServerManager *pThis)
{
	INFO_LOG("ThreadAccept thread begin...");

	pThis->m_listenThreadInfo.base = event_base_new();
	if (!pThis->m_listenThreadInfo.base) {
		CON_ERROR_LOG("TCP Could not initialize libevent!");
		exit(1);
	}

	{
		auto v = evtimer_new(
			pThis->m_listenThreadInfo.base, time_cb, event_self_cbarg());
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = rand_int(50000);
		evtimer_add(v, &tv);
	}

	// ��ʼ����
	sockaddr_in sin = {};
	sin.sin_family = AF_INET;
	sin.sin_port = htons(pThis->m_port);
	sin.sin_addr.s_addr =
		strlen(pThis->m_bindIP) == 0 ? INADDR_ANY : inet_addr(pThis->m_bindIP);

	pThis->m_listenThreadInfo.listener = evconnlistener_new_bind(
		pThis->m_listenThreadInfo.base, ListenerCB, (void *)pThis,
		LEV_OPT_REUSEABLE | LEV_OPT_REUSEABLE_PORT | LEV_OPT_CLOSE_ON_FREE, -1,
		(struct sockaddr *)&sin, sizeof(sin));

	if (!pThis->m_listenThreadInfo.listener) {
		printf("Could not create a listener! ���Ի����˿ڻ����Ե�һ�ᡣ");
		exit(1);
	}

	evconnlistener_set_error_cb(
		pThis->m_listenThreadInfo.listener, AcceptErrorCB);

	// ��ʼ����
	event_base_dispatch(pThis->m_listenThreadInfo.base);

	evconnlistener_free(pThis->m_listenThreadInfo.listener);
	event_base_free(pThis->m_listenThreadInfo.base);


	INFO_LOG("ThreadAccept thread exit.");
}

void
CTCPServerManager::ThreadRSSocket(WorkThreadInfo *pThreadData)
{
	// ���ڼ���״̬
	 event_base_loop(pThreadData->base, EVLOOP_NO_EXIT_ON_EMPTY);
	//event_base_dispatch(pThreadData->base);

	//
	event_base_free(pThreadData->base);
}


void
CTCPServerManager::ThreadSendMsg(CTCPServerManager *pThreadData)
{
}

void
CTCPServerManager::ListenerCB(struct evconnlistener *listener,
	evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
{
	CTCPServerManager *pThis = (CTCPServerManager *)user_data;

	{
		struct sockaddr_in listendAddr = {}, connectedAddr = {}, peerAddr = {};
		int listendAddrLen = sizeof(listendAddr),
			connectedAddrLen = sizeof(connectedAddr),
			peerLen = sizeof(peerAddr);

		int ret = getsockname(
			fd, (struct sockaddr *)&connectedAddr, &connectedAddrLen);
		if (ret != 0) {
			perror("��ȡ���ص�ַ����");
		} else {
			printf("connected address = %s:%d\n",
				inet_ntoa(connectedAddr.sin_addr),
				ntohs(connectedAddr.sin_port));
		}

		ret = getpeername(fd, (struct sockaddr *)&peerAddr, &peerLen);
		if (ret != 0) {
			perror("��ȡ�Զ�ַ����");
		} else {
			printf("peerAddr address = %s:%d\n", inet_ntoa(peerAddr.sin_addr),
				ntohs(peerAddr.sin_port));
		}
	}

	// ����������ж�
	if (pThis->GetCurSocketSize() >= pThis->m_uMaxSocketSize) {
		ERROR_LOG("�������Ѿ�����fd=%Id [%s][������%u/%u]", fd,
			CTCPServerManager::GetSocketNameIpAndPort(fd).c_str(),
			pThis->GetCurSocketSize(), pThis->m_uMaxSocketSize);

		// ����ʧ��
		NetMessageHead netHead = {};
		netHead.uMessageSize = sizeof(NetMessageHead);
		netHead.uMainID = 100;
		netHead.uAssistantID = 3;
		netHead.uHandleCode = 15; // ��������������

		send(fd, (char *)&netHead, sizeof(NetMessageHead), 0);

		evutil_closesocket(fd);

		return;
	}

	//
	static int lastThreadIndex = 0;
	lastThreadIndex = lastThreadIndex % pThis->m_workBaseVec.size();

	// Ͷ�ݵ������߳�
	do {
		std::lock_guard<std::mutex> lck(pThis->m_workBaseVec[lastThreadIndex].mtx);
		pThis->m_workBaseVec[lastThreadIndex].vecFd.push_back(fd);
	} while (false);

	evuser_trigger(pThis->m_workBaseVec[lastThreadIndex].event_notify);

	lastThreadIndex++;
}

void
CTCPServerManager::ReadCB(struct bufferevent *bev, void *data)
{
	WorkThreadInfo *pWorkInfo = (WorkThreadInfo *)data;
	CTCPServerManager *pThis = pWorkInfo->pThis;

	// �������ݣ���ͷ����
	pThis->RecvData(bev);
}

void
CTCPServerManager::EventCB(struct bufferevent *, short, void *)
{
}

void
CTCPServerManager::AcceptErrorCB(struct evconnlistener *listener, void *)
{
}

void
CTCPServerManager::ThreadLibeventProcess(
	evutil_socket_t readfd, short which, void *arg)
{
	/*
		�ú����ᱻ����߳�ִ��
	*/

	WorkThreadInfo *workInfo = (WorkThreadInfo *)arg;

	std::vector<int> vecFd;
	do {
		std::lock_guard<std::mutex> lck(workInfo->mtx);
		vecFd = workInfo->vecFd;
		workInfo->vecFd.clear();

	} while (false);

	for (auto fd : vecFd) {
		// ��������
		workInfo->pThis->AddTCPSocketInfo(fd, workInfo);
	}
}

int
CTCPServerManager::GetCurSocketSize() const
{
	return m_uCurSocketSize;
}

void
CTCPServerManager::AddTCPSocketInfo(int fd, WorkThreadInfo *workInfo)
{
	/*
		�ú����ᱻ����߳�ִ��
	*/
	struct event_base *base = workInfo->base;


	struct bufferevent *bev = bufferevent_socket_new(
		base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
	if (!bev) {
		ERROR_LOG("Error constructing bufferevent!,fd=%d,ip=%s", fd,
			CTCPServerManager::GetSocketNameIpAndPort(fd).c_str());
		evutil_closesocket(fd);
		return;
	}

	// ����¼��������úûص�����
	bufferevent_setcb(bev, ReadCB, NULL, EventCB, (void *)workInfo);
	if (bufferevent_enable(bev, EV_READ | EV_ET) < 0) {
		ERROR_LOG("add event fail!!!,fd=%d,ip=%s", fd,
			CTCPServerManager::GetSocketNameIpAndPort(fd).c_str());
		evutil_closesocket(fd);
		bufferevent_free(bev);
		return;
	}

	// ���ö���ʱ, ��ʹ���ڲ�����Ҳ��Ҫ�������ж��Ƿ񻹴��(����û�п���)
	timeval tvRead;
	tvRead.tv_sec = 15 * 3;
	tvRead.tv_usec = 0;
	bufferevent_set_timeouts(bev, &tvRead, NULL);

	// ������Ϣ
	{
		SocketInfo sock_info;
		sock_info.bev = bev;

		std::lock_guard<std::mutex> lck(workInfo->pThis->m_mtxFd2SocketInfo);
		m_mapFd2SocketInfo[fd] = std::move(sock_info);
	}

	{
		std::lock_guard<std::mutex> lck(workInfo->pThis->m_mtxSocketSize);
		m_uCurSocketSize++;
	}
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

	//// ��ǰ��ȡλ��, �������һ���Ը���
	// evbuffer_ptr buff_ptr = {};

	//// �ܵ��ֽ���
	// size_t nAllSize = evbuffer_get_length(input);

	// char buff[MAX_PACKAGE_SIZE] = {};

	// while (true)
	//{
	//     // ʣ���ֽ�С�ڰ�ͷ
	//     if (nAllSize- buff_ptr.pos < sizeof(NetMessageHead))
	//         break;

	//    // ��Ϣͷ
	//    NetMessageHead tmpHead = {};

	//    // �ȶ�ȡ��Ϣͷ
	//    size_t head_size = evbuffer_copyout_from(input, &buff_ptr, &tmpHead,
	//    sizeof(NetMessageHead)); if (head_size <= sizeof(NetMessageHead))
	//        break;


	//    // ���峬����󳤶�����
	//    if (tmpHead.uMessageSize > MAX_PACKAGE_SIZE)
	//    {
	//        CloseSocket(bev);
	//        return false;
	//    }

	//    // ����δ��ȫ����
	//    if(nAllSize-buff_ptr.pos-sizeof(NetMessageHead) <
	//    tmpHead.uMessageSize)
	//        break;

	//    evbuffer_ptr _tmp_ptr = {};
	//    _tmp_ptr.pos = sizeof(NetMessageHead) + buff_ptr.pos;


	//    // ��ȡ����
	//    size_t body_size = evbuffer_copyout_from(input, &_tmp_ptr, &buff,
	//    tmpHead.uMessageSize); if (body_size <= tmpHead.uMessageSize)
	//        break;


	//    // ճ������
	//    while (handleRemainSize >= sizeof(NetMessageHead) && handleRemainSize
	//    >= pNetHead->uMessageSize)
	//    {
	//        unsigned int messageSize = pNetHead->uMessageSize;
	//        if (messageSize > MAX_TEMP_SENDBUF_SIZE)
	//        {
	//            // ��Ϣ��ʽ����ȷ
	//            CloseSocket(index);
	//            ERROR_LOG("close socket
	//            ���������������ֵ,index=%d,messageSize=%u", index,
	//            messageSize); return false;
	//        }

	//        int realSize = messageSize - sizeof(NetMessageHead);
	//        if (realSize < 0)
	//        {
	//            // ���ݰ�������ͷ
	//            CloseSocket(index);
	//            ERROR_LOG("close socket ���ݰ�������ͷ");
	//            return false;
	//        }

	//        void* pData = NULL;
	//        if (realSize > 0)
	//        {
	//            // û���ݾ�ΪNULL
	//            pData = (void*)(buf + realAllSize - handleRemainSize +
	//            sizeof(NetMessageHead));
	//        }

	//        // �ɷ�����
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


bool
CTCPServerManager::CloseSocket(bufferevent *bev)
{
	int fd = bufferevent_getfd(bev);

	{
		std::lock_guard<std::mutex> lck(m_mtxSocketSize);
		m_uCurSocketSize--;
	}

	{
		std::lock_guard<std::mutex> lck(m_mtxFd2SocketInfo);
		m_mapFd2SocketInfo.erase(fd);
	}

	bufferevent_free(bev);


	// �ص�ҵ���
	if (m_pService) {
		m_pService->OnSocketCloseEvent(m_nSocketType, fd);
	}

	return true;
}
