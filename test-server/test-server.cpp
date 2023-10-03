// login.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "TCPServerManager.h"
#ifdef _WIN32
#include <event2/thread.h>
#endif /* _WIN32 */
#include <event2/http.h>

// 服务器网络服务接口
class ServerSocketHander : public IServerSocketHander {
	/// 接口函数
public:
	// 网络关闭处理
	virtual bool
	OnSocketCloseEvent(int nServerType, int nSocket)
	{
		return true;
	}

	// 网络消息处理
	virtual bool
	OnSocketReadEvent(int nServerType, int nSocket, NetMessageHead *pNetHead,
		void *pData, int nSize)
	{
		return true;
	}
};

void cb1(struct evhttp_request* req, void*)
{
	evhttp_send_error(req, 0, "cb1");
}


void cb2(struct evhttp_request* req, void*)
{
    evhttp_send_error(req, 0, "cb2");
}


int
main()
{
#ifdef _WIN32
	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
	//evthread_use_windows_threads();
#else
	evthread_use_pthreads();

#endif

	CTCPServerManager *a = CTCPServerManager::GetNetManager();

	ServerSocketHander b;

    a->AddTcpListenInfo(1, "0.0.0.0", 49980, 2);
    a->AddTcpListenInfo(1, "0.0.0.0", 49981, 2);
    a->AddTcpListenInfo(2, "0.0.0.0", 49990, 3);
    a->AddTcpListenInfo(2, "0.0.0.0", 49991, 3);

    //a.AddTcpListenInfo(10, "0.0.0.0", 50000, 2, 2);

	//HttpPathCallBack c1 = { "/cb1", cb1, 0 };
 //   HttpPathCallBack c2 = { "/cb2", cb2, 0 };
	//a.AddWebSocket("0.0.0.0", 50001, {c1 ,c2});

	a->Start();

	Sleep(1000*3);

	a->Stop();

    Sleep(1000 * 3000);

	return 0;
}
