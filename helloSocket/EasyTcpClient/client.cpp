
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "EasyTcpClient.hpp"

void cmdThread (EasyTcpClient* client) {
	while (true) {
		// 3.������������
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);
		// ������������
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("�յ��˳�����");
			client->Close();
			break;
		}
		else if (0 == strcmp(cmdBuf, "login")) {
			Login login;
			strcpy(login.userName, "tony");
			strcpy(login.password, "zhou123456");
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "logout")) {
			Logout logout;
			strcpy(logout.userName, "tony");
			client->SendData(&logout);
		}
		else {
			printf("��֧�ֵ�������������롣\n");
		}
	}
	printf("�˳��̡߳�\n");
}

int main() {

	EasyTcpClient client;
	client._sock;
	client.Connect("192.168.31.164", 4567);// win 192.168.31.68,mac 192.168.31.126,ubuntu 192.168.31.164
	std::thread td(cmdThread, &client);
	td.detach();
	while (client.isRun()) {
		client.onRun();
	}
	client.Close();

	printf("��������������˳�\n");
	getchar();
	return 0;
}
