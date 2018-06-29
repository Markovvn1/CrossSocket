#include "socket.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#define SOCKET_CHANK_SIZE 2048

struct SocketData
{
	int socketId = -1;
	bool active = false;
	mutex lock;
};

inline int closeSocket(int socketId)
{
	return close(socketId);
}

inline int bindSocket(int socketId, __CONST_SOCKADDR_ARG addr, socklen_t len)
{
	return bind(socketId, addr, len);
}

inline int listenSocket(int fd, int n)
{
	return listen(fd, n);
}

inline int acceptSocket(int fd, __SOCKADDR_ARG addr, socklen_t *__restrict addr_len)
{
	return accept(fd, addr, addr_len);
}

inline ssize_t sendSocket(int fd, const void *buf, size_t n, int flags)
{
	return send(fd, buf, n, flags);
}

inline ssize_t recvSocket(int fd, void *buf, size_t n, int flags)
{
	return recv(fd, buf, n, flags);
}

inline int connectSocket(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len)
{
	return connect(fd, addr, len);
}



Socket::Socket()
{
	data = shared_ptr<SocketData>(new SocketData);
}

Socket::Socket(int socketId)
{
	data = shared_ptr<SocketData>(new SocketData);
	data->socketId = socketId;
	data->active = isOpen();
}

bool Socket::open()
{
	if (isOpen()) return true;

	data->lock.lock();
	data->socketId = socket(AF_INET, SOCK_STREAM, 0);
	data->lock.unlock();

	return isOpen();
}

bool Socket::close()
{
	if (!isOpen()) return true;
	closeSocket(data->socketId);

	data->lock.lock();

	data->socketId = -1;
	data->active = false;

	data->lock.unlock();

	return true;
}

bool Socket::isOpen()
{
	return data->socketId >= 0;
}

bool Socket::isActive()
{
	return data->active;
}

bool Socket::bind(int port)
{
	if (!isOpen() || isActive()) return false;

	data->lock.lock();

	fcntl(data->socketId, F_SETFL, O_NONBLOCK);

	struct sockaddr_in serverData;
	bzero((char*) &serverData, sizeof(serverData));

	serverData.sin_family = AF_INET;
	serverData.sin_addr.s_addr = INADDR_ANY;
	serverData.sin_port = htons(port);


	// Разрешить повторное использование сокета
	int yes = 1;
	setsockopt(data->socketId, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	// Связывание сокета с портом
	data->active = bindSocket(data->socketId, (struct sockaddr*)&serverData, sizeof(serverData)) >= 0;

	data->lock.unlock();

	return isActive();
}

bool Socket::connect(const char* host, int port)
{
	if (!isOpen() || isActive()) return false;

	data->lock.lock();

	struct sockaddr_in serverData;
	bzero((char*) &serverData, sizeof(serverData));

	serverData.sin_family = AF_INET;
	serverData.sin_port = htons(port);

	data->active = inet_pton(AF_INET, host, &serverData.sin_addr) >= 0;
	if (!data->active)
	{
		data->lock.unlock();
		return isActive();
	}

	data->active = connectSocket(data->socketId, (struct sockaddr*)&serverData, sizeof(serverData)) >= 0;

	data->lock.unlock();

	return isActive();
}

void Socket::listen(int count)
{
	if (!isActive()) return;

	data->lock.lock();
	listenSocket(data->socketId, count);
	data->lock.unlock();
}

Socket Socket::accept()
{
	if (!isActive()) return Socket();

	data->lock.lock();
	int newSocketId = acceptSocket(data->socketId, NULL, NULL);
	data->lock.unlock();

	if (newSocketId > 0)
		fcntl(newSocketId, F_SETFL, O_NONBLOCK);

	return Socket(newSocketId);
}

bool Socket::send(void* buffer, unsigned int count)
{
	if (!isActive()) return 0;

	unsigned int sending = 0;

	data->lock.lock();

	while (sending < count)
	{
		int result = sendSocket(data->socketId, buffer, min(count - sending, (unsigned int)SOCKET_CHANK_SIZE), MSG_NOSIGNAL);

		if (result == 0)
		{
			// Клиент отключился
			data->lock.unlock();
			close();
			return false;
		}
		else if (result < 0)
		{
			if (errno == EINTR || errno == EWOULDBLOCK) continue;
			// Ошибка при чтении
			data->lock.unlock();
			close();
			return false;
		}

		sending += result;
		buffer = (char*)buffer + result;
	}

	data->lock.unlock();

	return true;
}

bool Socket::recv(void* buffer, unsigned int count)
{
	if (!isActive()) return 0;

	unsigned int reading = 0;

	data->lock.lock();

	while (reading < count)
	{
		int result = recvSocket(data->socketId, buffer, min(count - reading, (unsigned int)SOCKET_CHANK_SIZE), 0);

		if (result == 0)
		{
			// Клиент отключился
			data->lock.unlock();
			close();
			return false;
		}
		else if (result < 0)
		{
			if (errno == EINTR || errno == EWOULDBLOCK) continue;
			// Ошибка при чтении
			data->lock.unlock();
			close();
			return false;
		}

		reading += result;
		buffer = (char*)buffer + result;
	}

	data->lock.unlock();

	return true;
}

