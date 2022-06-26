#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "EasyTcpServer.hpp"

bool isRun = true;
void cmdThread() {
	while (true) {
		// 3.输入请求命令
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);
		// 处理请求命令
		if (0 == strcmp(cmdBuf, "exit")) {
			isRun = false;
			break;
		}
		else {
			printf("不支持的命令，请重新输入。\n");
		}
	}
	printf("退出线程。\n");
}

int main() {
#ifdef _WIN32
	SetConsoleTitle(L"EasyTcpServer");
#endif // _WIN32

	MyServer server;
	server.Bind(nullptr, 3001);
	server.Listen(5);
	server.Start(4);
	std::thread td(cmdThread);
	td.detach();

	while (isRun) {
		server.OnRun();
	}
	server.Close();
	printf("任务结束，程序退出 \n");
	system("pause");
	return 0;
}
