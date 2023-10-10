#include "BaseServer.h"
#include "TCPServerManager.h"
#include "configure.h"
CBaseServer::CBaseServer()
{

}

CBaseServer::~CBaseServer()
{

}

void CBaseServer::Init()
{
    InitNet();
}

void CBaseServer::Start()
{

}

void CBaseServer::InitNet()
{
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();

    pNetManager->AddTcpListenInfo(CONFIG::tcp_listen_type1,
        CONFIG::tcp_listen_addr1,
        CONFIG::tcp_listen_port1,
        CONFIG::tcp_max_connect_count1);

    pNetManager->AddTcpListenInfo(CONFIG::tcp_listen_type2,
        CONFIG::tcp_listen_addr2,
        CONFIG::tcp_listen_port2,
        CONFIG::tcp_max_connect_count2);

    pNetManager->AddWebSocketInfo(CONFIG::web_socket_listen_type1,
        CONFIG::web_socket_listen_addr1,
        CONFIG::web_socket_listen_port1,
        CONFIG::web_socket_listen_path1,
        CONFIG::web_socket_max_connect_count1);
}

void CBaseServer::StartNet()
{
    CTCPServerManager* pNetManager = CTCPServerManager::GetNetManager();
    pNetManager->Start();
}
