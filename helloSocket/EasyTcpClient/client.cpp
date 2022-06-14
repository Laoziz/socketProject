
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "EasyTcpClient.hpp"

bool g_run = true;
void cmdThread (EasyTcpClient* client) {
	while (true) {
		// 3.输入请求命令
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);
		// 处理请求命令
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("收到退出命令");
			g_run = false;
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
			printf("不支持的命令，请重新输入。\n");
		}
	}
	printf("退出线程。\n");
}

int main() {
#ifdef _WIN32
	SetConsoleTitle(L"EasyTcpClient");
#endif // _WIN32

	const int cCount = 10000;
	EasyTcpClient* client[cCount];
	for (int i = 0; i < cCount; i++) {
		client[i] = new EasyTcpClient();
		client[i]->Connect("127.0.0.1", 3001);// win 192.168.31.68,mac 192.168.31.126,ubuntu 192.168.31.164 centos 49.235.145.15
		printf("Connect count<%d> \n",i);
	}

	//EasyTcpClient client;
	//client.Connect("127.0.0.1", 3001);
	
	//std::thread td(cmdThread, &client);
	//td.detach();

	Login login;
	strcpy(login.userName, "tony");
	strcpy(login.password, "zhou123456");
	while (g_run) {
		for (int i = 0; i < cCount; i++) {
			client[i]->SendData(&login);
			//client[i]->onRun();
		}
		//client.SendData(&login);
		//client.onRun();
	}
	for (int i = 0; i < cCount; i++) {
		client[i]->Close();
	}
	//client.Close();
	printf("任务结束，程序退出\n");
	getchar();
	return 0;
}
