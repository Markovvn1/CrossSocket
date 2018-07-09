#pragma once

#include <memory>

struct SocketData;

class Socket
{
private:
	std::shared_ptr<SocketData> data;

public:
	Socket();
	Socket(int socketId);
	~Socket();

	bool open();
	bool close();
	bool isOpen();
	bool isActive();

	bool bind(int port);
	bool connect(const char* host, int port);

	void listen(int count);
	Socket accept();

	// true - все хорошо
	bool send(void* buffer, unsigned int count);
	bool recv(void* buffer, unsigned int count);
};
