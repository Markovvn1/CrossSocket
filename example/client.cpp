/*
 *  Project: CrossSocket
 *  File: example/client.cpp
 *
 *  Date: 11.08.2018
 *  Author: Markovvn1
 */

#include <iostream>

#include "../socket.hpp"

int main()
{
	// Create client socket and connect to server
    ClientSocket socket;
    socket.open();
    socket.connect("127.0.0.1", 1111);

    // Receive some data
    char dataForReceive[19];
    socket.recv(dataForReceive, 19);
    std::cout << dataForReceive << std::endl;

    // Send some data
    const char* dataToSend = "Hi from client!";
    socket.send(dataToSend, 15 + 1);

    // Close client (optionally: it closes automatically when object will be destroyed)
    socket.close();

    return 0;
}
