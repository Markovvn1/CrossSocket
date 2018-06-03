#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include "socket.hpp"

#define SOCKET_CHANK_SIZE 2048

int closeSocket(int socketId)
{
	return close(socketId);
}

int bindSocket(int socketId, __CONST_SOCKADDR_ARG addr, socklen_t len)
{
	return bind(socketId, addr, len);
}

int listenSocket(int fd, int n)
{
	return listen(fd, n);
}

int acceptSocket(int fd, __SOCKADDR_ARG addr, socklen_t *__restrict addr_len)
{
	return accept(fd, addr, addr_len);
}

ssize_t sendSocket(int fd, const void *buf, size_t n, int flags)
{
	return send(fd, buf, n, flags);
}

ssize_t recvSocket(int fd, void *buf, size_t n, int flags)
{
	return recv(fd, buf, n, flags);
}

int connectSocket(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len)
{
	return connect(fd, addr, len);
}



Socket::Socket()
{
	socketId = shared_ptr<int>(new int); *socketId = -1;
	active = shared_ptr<bool>(new bool); *active = false;
}

Socket::Socket(int socketId)
{
	this->socketId = shared_ptr<int>(new int); *this->socketId = socketId;
	this->active = shared_ptr<bool>(new bool); *this->active = isOpen();
}

void Socket::open()
{
	if (isOpen()) return;

	*socketId = socket(AF_INET, SOCK_STREAM, 0);
}

void Socket::close()
{
	if (!isOpen()) return;

	closeSocket(*socketId);
	*socketId = -1;

	*active = false;
}

bool Socket::isOpen()
{
	return *socketId >= 0;
}

bool Socket::isActive()
{
	return *active;
}

bool Socket::bind(int port)
{
	if (!isOpen() || isActive()) return false;

	fcntl(*socketId, F_SETFL, O_NONBLOCK);

	struct sockaddr_in serverData;
	bzero((char*) &serverData, sizeof(serverData));

	serverData.sin_family = AF_INET;
	serverData.sin_addr.s_addr = INADDR_ANY;
	serverData.sin_port = htons(port);


	// Разрешить повторное использование сокета
	int yes = 1;
	setsockopt(*socketId, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	// Связывание сокета с портом
	*active = bindSocket(*socketId, (struct sockaddr*)&serverData, sizeof(serverData)) >= 0;

	return isActive();
}

bool Socket::connect(const char* host, int port)
{
	if (!isOpen() || isActive()) return false;

	struct sockaddr_in serverData;
	bzero((char*) &serverData, sizeof(serverData));

	serverData.sin_family = AF_INET;
	serverData.sin_port = htons(port);

	*active = inet_pton(AF_INET, host, &serverData.sin_addr) >= 0;
	if (!(*active)) return isActive();

	*active = connectSocket(*socketId, (struct sockaddr*)&serverData, sizeof(serverData)) >= 0;

	return isActive();
}

void Socket::listen(int count)
{
	if (!isActive()) return;

	listenSocket(*socketId, count);
}

Socket Socket::accept()
{
	if (!isActive()) return Socket();

	Socket result = Socket(acceptSocket(*socketId, NULL, NULL));
	return result;
}

int Socket::send(void* buffer, unsigned int count)
{
	if (!isActive()) return 0;

	unsigned int sending = 0;

	while (sending < count)
	{
		int result = sendSocket(*socketId, buffer, min(count - sending, (unsigned int)SOCKET_CHANK_SIZE), MSG_NOSIGNAL);

		if (result == 0)
		{
			// Клиент отключился
			return sending;
		}
		else if (result < 0)
		{
			if (errno == EINTR) continue;
			// Ошибка при чтении
			return sending;
		}

		sending += result;
		buffer = (char*)buffer + result;
	}

	return sending;
}

unsigned int Socket::recv(void* buffer, unsigned int count)
{
	if (!isActive()) return 0;

	unsigned int reading = 0;

	while (reading < count)
	{
		int result = recvSocket(*socketId, buffer, min(count - reading, (unsigned int)SOCKET_CHANK_SIZE), 0);

		if (result == 0)
		{
			// Клиент отключился
			return reading;
		}
		else if (result < 0)
		{
			if (errno == EINTR) continue;
			// Ошибка при чтении
			return reading;
		}

		reading += result;
		buffer = (char*)buffer + result;
	}

	return reading;
}

