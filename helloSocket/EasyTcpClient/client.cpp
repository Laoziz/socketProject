
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "EasyTcpClient.hpp"

bool g_run = true;
void cmdThread () {
	while (true) {
		// 3.������������
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);
		// ������������
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("�յ��˳�����");
			g_run = false;
			break;
		}
		else {
			printf("��֧�ֵ�������������롣\n");
		}
	}
	printf("�˳��̡߳�\n");
}


// �ͻ�������
const int cCount = 10000;
// �����߳�����
const int tCount = 4;
// �ͻ�������
EasyTcpClient* client[cCount];

void sendThread(int id) { 
	// �߳�ID 1~4
	int c = cCount / tCount;
	int begin = (id - 1) * c;
	int end = id * c;
	for (int i = begin; i < end; i++) {
		client[i] = new EasyTcpClient();
	}
	for (int i = begin; i < end; i++) {
		client[i]->Connect("127.0.0.1", 3001);// win 192.168.31.68,mac 192.168.31.126,ubuntu 192.168.31.164 centos 49.235.145.15
		printf("Connect count<%d> \n", i);
	}

	std::chrono::microseconds t(3000);
	std::this_thread::sleep_for(t);
	Login login;
	strcpy(login.userName, "tony");
	strcpy(login.password, "zhou123456");
	while (g_run) {
		for (int i = begin; i < end; i++) {
			client[i]->SendData(&login);
			//client[i]->onRun();
		}
	}
	for (int i = begin; i < end; i++) {
		client[i]->Close();
	}
}
int main() {
#ifdef _WIN32
	SetConsoleTitle(L"EasyTcpClient");
#endif // _WIN32

	std::thread td(cmdThread);
	td.detach();

	for (int i = 0; i < tCount;i++) {
		std::thread td(sendThread, i + 1);
		td.detach();
	}
	system("pause");
	return 0;
}
