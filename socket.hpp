#pragma once

#include <memory>

struct __Socket;

class Socket
{
private:
	std::shared_ptr<__Socket> data;

public:
	Socket();
	Socket(unsigned int socketId);
	virtual ~Socket();

	// true - все хорошо

	bool open();
	bool close();
	bool isOpen();
	bool isActive();

	bool bind(int port);
	bool connect(const char* host, int port);

	void listen(int count);
	Socket accept();

	bool send(const char* buffer, unsigned int count);
	bool recv(char* buffer, unsigned int count);
};
