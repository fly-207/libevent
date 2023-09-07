/*
  This example program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef _WIN32
#include <netinet/in.h>
#ifdef _XOPEN_SOURCE_EXTENDED
#include <arpa/inet.h>
#endif
#include <sys/socket.h>
#endif

#include <thread>

#include <event2/util.h>
#include <event2/event.h>
#include <string>


static const char MESSAGE[] = "Hello, World!\n";

static const unsigned short PORT = 9995;



void
read_cb1(evutil_socket_t fd, short nEvnet, void *arg)
{

}

void
read_cb2(evutil_socket_t fd, short nEvnet, void *arg) 
{
	std::string a;

	if (nEvnet & EV_ET)
	{
		a += "ET "
	}


	printf()
}


void
foo(event* ev1, event* ev2)
{
	Sleep(1);


	auto fd1 = event_get_fd(ev1);

	char msg[] = "123";
	send(fd1, msg, sizeof(msg), 0);

	Sleep(10000000);
}


int
main(int argc, char **argv)
{
#ifdef _WIN32
	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif

	event_base *base = event_base_new();


	evutil_socket_t fd[2] = {};
	evutil_socketpair(AF_INET, SOCK_STREAM, 0, fd);

	auto ev1 = event_new(base, fd[0], EV_READ | EV_ET | EV_PERSIST, read_cb1, event_self_cbarg());
	auto ev2 = event_new(base, fd[1], EV_READ | EV_ET | EV_PERSIST, read_cb2, event_self_cbarg());

	event_add(ev1, NULL);
	event_add(ev2, NULL);

	std::thread a(foo, ev1, ev2);
	a.detach();

	event_base_dispatch(base);
	event_base_free(base);

	printf("done\n");
	return 0;
}

