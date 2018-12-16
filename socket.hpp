#pragma once

#include <memory>

struct __Socket;

class Socket
{
private:
	std::shared_ptr<__Socket> data;

public:
	Socket();
	Socket(unsigned int socketId); // Системный конструктор
	virtual ~Socket();

	// Вернет true, если все хорошо
	bool open();
	bool close();

	// Проверка, открыт ли socket
	bool isOpen();

	// Проверка, установленно ли соединение на этом socket'е
	bool isActive();

	// Связать socket с портом
	bool bind(int port);

	// Подключиться к серверу
	bool connect(const char* host, int port);

	// Установить максимальное количество человек в очереди
	void listen(int count);

	// Вернет открытый socket, если соединение с клиентом было установленно
	// Если клиентов нет, то вернет закрытый socket
	Socket accept();

	// Отправить count байт данных, лежащих по адресу buffer
	bool send(const char* buffer, unsigned int count);

	// Принять count байт данных и положить их по адресу buffer
	bool recv(char* buffer, unsigned int count);
};
