#include <iostream>
#include <cstring>
#include "socket.hpp"

using namespace std;

int main()
{
    Socket socket;
    socket.open();
    socket.connect("192.168.15.169", 1111);

    char data1[13];
    socket.recv(data1, 13);
    cout << data1 << endl;

    const char* data2 = "Hello, Vova!";
    socket.send(data2, strlen(data2) + 1);

    return 0;
}
