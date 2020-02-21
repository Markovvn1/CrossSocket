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

Socket::~Socket()
{
	if (data.use_count() != 1) return;

	close();
}

bool Socket::open()
{
	if (isOpen()) return true;

	data->lock.lock();
#ifdef _WIN32
		check_start();
		data->socketId = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (data->socketId == INVALID_SOCKET) check_close();
#else
		data->socketId = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
	data->lock.unlock();

	return isOpen();
}

bool Socket::close()
{
	if (!isOpen()) return true;

#ifdef _WIN32
	shutdown(data->socketId, SD_SEND);
	closesocket(data->socketId);
	check_close();
#else
	::close(data->socketId);
#endif

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

bool Socket::bind(uint port)
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
	data->active = ::bind(data->socketId, (SOCKADDR*)&serverData, sizeof(serverData)) >= 0;

	data->lock.unlock();

	return isActive();
}

bool Socket::connect(const char* host, uint port)
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

	data->active = ::connect(data->socketId, (SOCKADDR*)&serverData, sizeof(serverData)) >= 0;

	data->lock.unlock();

	return isActive();
}

void Socket::listen(uint count)
{
	if (!isActive()) return;

	data->lock.lock();
	::listen(data->socketId, count);
	data->lock.unlock();
}

AcceptedClient Socket::accept()
{
	if (!isActive()) return AcceptedClient();

	data->lock.lock();
	SOCKET newSocketId = ::accept(data->socketId, NULL, NULL);
	data->lock.unlock();

	if (newSocketId != INVALID_SOCKET)
		setNonBlockSocket(newSocketId);

	// Создание AcceptedClient
	AcceptedClient ac;
	ac.data->socketId = newSocketId;
	ac.data->active = ac.isOpen();
	return ac;
}

bool Socket::send(const char* buffer, uint count)
{
	if (!isActive()) return false;

	uint sending = 0;

	data->lock.lock();

	while (sending < count)
	{
		int result = ::send(data->socketId, buffer, min(count - sending, (uint)SOCKET_CHANK_SIZE), 0);

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

bool Socket::recv(char* buffer, uint count)
{
	if (!isActive()) return false;

	uint reading = 0;

	data->lock.lock();

	while (reading < count)
	{
		int result = ::recv(data->socketId, buffer, min(count - reading, (uint)SOCKET_CHANK_SIZE), 0);

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
