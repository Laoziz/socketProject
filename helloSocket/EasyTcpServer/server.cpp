#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "EasyTcpServer.hpp"

int main() {
	EasyTcpServer server;
	server.Bind(nullptr, 4567);
	server.Listen(5);
	while (server.isRun()) {
		server.OnRun();
		//printf("����ʱ�䴦������ҵ�񡣡�����\n");
	}
	server.Close();
	printf("��������������˳� \n");
	getchar();
	return 0;
}
