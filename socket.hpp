#pragma once

#include <memory>

using namespace std;

class Socket
{
private:
	shared_ptr<int> socketId;
	shared_ptr<bool> active;

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
