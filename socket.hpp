#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include <iostream>

using namespace std;

class Socket
{
private:
	int socketId = -1;
	bool active = false;

public:
	Socket();
	Socket(int socketId);

	void open();
	void close();
	bool isOpen();
	bool isActive();

	bool bind(int port);
	bool connect(const char* host, int port);

	void listen(int count);
	Socket accept();

	int send(void* buffer, unsigned int count);
	unsigned int recv(void* bufer, unsigned int count);
};
