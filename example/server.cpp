#include <iostream>
#include <unistd.h>
#include <cstring>

#include "socket.hpp"

using namespace std;


int main()
{
	Socket socket;
	socket.open();
	socket.bind(1111);
	socket.listen(5);

	while (true)
	{
		Socket socket1 = socket.accept();

		if (!socket1.isActive())
		{
			usleep(10000);
			continue;
		}

		const char* data1 = "Valy, Hello!";
		socket1.send(data1, strlen(data1) + 1);

		char res[13];
		socket1.recv(res, 13);

		cout << res << endl;

		break;
	}

	return 0;
}
