#include <iostream>
#include <unistd.h>

#include "../socket.hpp"

int main()
{
	// Create and confinure server
	ServerSocket server;
	server.open();
	server.bind(1111);
	server.listen(5); // Maximum amount of clients

	while (true)
	{
		// Check if client can be accepted (non-blocking)
		AcceptedClient client = server.accept();

		// If client was not accepted
		if (!client.isActive())
		{
			usleep(10000);
			continue; // continue waiting for client
		}

		// Send some data
		const char* dataToSend = "Hello from server!";
		client.send(dataToSend, 18 + 1);

		// Receive some data
		char dataForReceive[16];
		client.recv(dataForReceive, 16);
		std::cout << dataForReceive << std::endl;

		// Close connection (optionally: it closes automatically when object will be destroyed)
		client.close();

		break;
	}

	// Close server (optionally: it closes automatically when object will be destroyed)
	server.close();

	return 0;
}
