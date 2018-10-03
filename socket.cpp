#include "socket.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <cstring>
#include <mutex>

#ifdef _WIN32
#pragma comment (lib, "Ws2_32.lib")
#endif

#define SOCKET_CHANK_SIZE 2048

using namespace std;

#ifndef _WIN32
typedef unsigned int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

#define INVALID_SOCKET  (SOCKET)(~0)
#endif

#ifdef _WIN32

mutex lock_open_socket;
int count_socket = 0;

void check_start()
{
	lock_open_socket.lock();
	if (count_socket == 0)
	{
		WSADATA WSAData;
		WSAStartup(MAKEWORD(2, 2), &WSAData);
	}
	count_socket++;
	lock_open_socket.unlock();
}

void check_close()
{
	lock_open_socket.lock();
	count_socket--;
	if (count_socket == 0)
	{
		WSACleanup();
	}
	lock_open_socket.unlock();
}

#endif

inline SOCKET openSocket()
{
#ifdef _WIN32
	check_start();
	SOCKET res = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (res < 0) check_close();

	return res;
#else
	return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
}

inline int closeSocket(SOCKET socketId)
{
#ifdef _WIN32
	shutdown(socketId, SD_SEND);
	int res = closesocket(socketId);
	check_close();
	return res;
#else
	return close(socketId);
#endif
}

inline int bindSocket(SOCKET socketId, const SOCKADDR* addr, int len)
{
	return bind(socketId, addr, len);
}

inline int listenSocket(SOCKET fd, int n)
{
	return listen(fd, n);
}

inline int acceptSocket(SOCKET fd, SOCKADDR* addr, int* addr_len)
{
#ifdef _WIN32
	return accept(fd, addr, addr_len);
#else
	return accept(fd, addr, (socklen_t*)addr_len);
#endif
}

inline int sendSocket(SOCKET fd, const char *buf, size_t n, int flags)
{
	return send(fd, buf, n, flags);
}

inline int recvSocket(SOCKET fd, char *buf, size_t n, int flags)
{
	return recv(fd, buf, n, flags);
}

inline int connectSocket(SOCKET fd, const SOCKADDR* addr, int len)
{
	return connect(fd, addr, len);
}

void setNonBlockSocket(SOCKET socketId)
{
#ifdef _WIN32
	u_long nonblocking_enabled = true;
	ioctlsocket(socketId, FIONBIO, &nonblocking_enabled);
#else
	fcntl(socketId, F_SETFL, O_NONBLOCK);
#endif
}


struct __Socket
{
	SOCKET socketId;
	bool active;
	mutex lock;
};


Socket::Socket()
{
	data = shared_ptr<__Socket>(new __Socket);
	data->socketId = INVALID_SOCKET;
	data->active = false;
}

Socket::Socket(unsigned int socketId)
{
	data = shared_ptr<__Socket>(new __Socket);
	data->socketId = socketId;
	data->active = isOpen();
}

Socket::~Socket()
{
	if (data.use_count() != 1) return;

	close();
}

bool Socket::open()
{
	if (isOpen()) return true;

	data->lock.lock();
	data->socketId = openSocket();
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
	return data->socketId != INVALID_SOCKET;
}

bool Socket::isActive()
{
	return data->active;
}

bool Socket::bind(int port)
{
	if (!isOpen() || isActive()) return false;

	data->lock.lock();

	setNonBlockSocket(data->socketId);

	SOCKADDR_IN serverData;

#ifdef _WIN32
	ZeroMemory((char*)&serverData, sizeof(serverData));
#else
	bzero((char*)&serverData, sizeof(serverData));
#endif

	serverData.sin_family = AF_INET;
	serverData.sin_addr.s_addr = INADDR_ANY;
	serverData.sin_port = htons(port);


	// Разрешить повторное использование сокета
	int yes = 1;
	setsockopt(data->socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int));

	// Связывание сокета с портом
	data->active = bindSocket(data->socketId, (SOCKADDR*)&serverData, sizeof(serverData)) >= 0;

	data->lock.unlock();

	return isActive();
}

bool Socket::connect(const char* host, int port)
{
	if (!isOpen() || isActive()) return false;

	data->lock.lock();

	SOCKADDR_IN serverData;

#ifdef _WIN32
	ZeroMemory((char*)&serverData, sizeof(serverData));
#else
	bzero((char*)&serverData, sizeof(serverData));
#endif

	serverData.sin_family = AF_INET;
	serverData.sin_port = htons(port);

	data->active = inet_pton(AF_INET, host, &serverData.sin_addr) >= 0;
	if (!data->active)
	{
		data->lock.unlock();
		return isActive();
	}

	data->active = connectSocket(data->socketId, (SOCKADDR*)&serverData, sizeof(serverData)) >= 0;

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
	SOCKET newSocketId = acceptSocket(data->socketId, NULL, NULL);
	data->lock.unlock();

	if (newSocketId != INVALID_SOCKET)
		setNonBlockSocket(newSocketId);

	return Socket(newSocketId);
}

bool Socket::send(const char* buffer, unsigned int count)
{
	if (!isActive()) return 0;

	unsigned int sending = 0;

	data->lock.lock();

	while (sending < count)
	{
		int result = sendSocket(data->socketId, buffer, min(count - sending, (unsigned int)SOCKET_CHANK_SIZE), 0);

		if (result == 0)
		{
			// Клиент отключился
			data->lock.unlock();
			close();
			return false;
		}
		else if (result < 0)
		{
			if (errno == EINTR || errno == EWOULDBLOCK || errno == 0) continue;
			// Ошибка при чтении
			data->lock.unlock();
			close();
			return false;
		}

		sending += result;
		buffer = buffer + result;
	}

	data->lock.unlock();

	return true;
}

bool Socket::recv(char* buffer, unsigned int count)
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
			if (errno == EINTR || errno == EWOULDBLOCK || errno == 0) continue;
			// Ошибка при чтении
			data->lock.unlock();
			close();
			return false;
		}

		reading += result;
		buffer = buffer + result;
	}

	data->lock.unlock();

	return true;
}
