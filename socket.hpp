#pragma once

#include <memory>
#include <mutex>

using namespace std;

struct SocketData
{
	int socketId;
	bool active;
	mutex lock;

	SocketData();
};

class Socket
{
private:
	shared_ptr<SocketData> data;

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

	// true - все хорошо
	bool send(void* buffer, unsigned int count);
	bool recv(void* bufer, unsigned int count);
};
