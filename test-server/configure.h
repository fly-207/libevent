#ifndef _CONFIG_HEAD_FILE_
#define _CONFIG_HEAD_FILE_
#pragma once

namespace CONFIG
{
    const int tcp_listen_type1 = 1;
    const int tcp_max_connect_count1 = 10;
    const int tcp_listen_port1 = 46000;
    const char tcp_listen_addr1[] = "0.0.0.0";

    const int tcp_listen_type2 = 2;
    const int tcp_max_connect_count2 = 10;
    const int tcp_listen_port2 = 46001;
    const char tcp_listen_addr2[] = "0.0.0.0";

    const int web_socket_listen_type1 = 3;
    const int web_socket_max_connect_count1 = 10;
    const int web_socket_listen_port1 = 46000;
    const char web_socket_listen_addr1[] = "0.0.0.0";
    const char web_socket_listen_path1[] = "/ws";

}


#endif

