#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "EasyTcpServer.hpp"
#include<thread>

bool isRun = true;
void cmdThread() {
	while (true) {
		// 3.������������
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);
		// ������������
		if (0 == strcmp(cmdBuf, "exit")) {
			isRun = false;
			break;
		}
		else {
			printf("��֧�ֵ�������������롣\n");
		}
	}
	printf("�˳��̡߳�\n");
}

int main() {
	EasyTcpServer server;
	server.Bind(nullptr, 3001);
	server.Listen(5);

	std::thread td(cmdThread);
	td.detach();

	while (isRun) {
		server.OnRun();
	}
	server.Close();
	printf("��������������˳� \n");
	getchar();
	return 0;
}
