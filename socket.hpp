#pragma once

#include <memory>

class Socket;
class ServerSocket;
class ClientSocket;
class AcceptedClient;

struct __Socket;

// Класс, реализующий весь основной функционал для работы с сокетами.
class Socket
{
private:
	std::shared_ptr<__Socket> data;

public:
	Socket();
	virtual ~Socket();

	// Вернет true, если все хорошо
	bool open();
	bool close();

	// Проверка, открыт ли socket
	bool isOpen();

	// Проверка, установленно ли соединение на этом socket'е
	bool isActive();

	// Связать socket с портом
	bool bind(uint port);

	// Подключиться к серверу
	bool connect(const char* host, uint port);

	// Установить максимальное количество человек в очереди
	void listen(uint count);

	// Вернет открытый socket, если соединение с клиентом было установленно
	// Если клиентов нет, то вернет закрытый socket
	AcceptedClient accept();

	// Отправить count байт данных, лежащих по адресу buffer
	bool send(const char* buffer, uint count);

	// Принять count байт данных и положить их по адресу buffer
	bool recv(char* buffer, uint count);
};

// Класс, релизующий весь необходимый функционал для создания сервера
class ServerSocket : public Socket
{
	bool connect() = delete;
	bool send() = delete;
	bool recv() = delete;
};

// Класс, релизующий весь необходимый функционал для создания клиента
class ClientSocket : public Socket
{
	bool bind() = delete;
	bool listen() = delete;
	AcceptedClient accept() = delete;
};

class AcceptedClient : public Socket
{
	bool bind() = delete;
	bool connect() = delete;
	bool listen() = delete;
	AcceptedClient accept() = delete;
};
