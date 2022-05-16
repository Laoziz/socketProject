#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<windows.h>
#include<Winsock2.h>

#include<stdio.h>

enum CMD {
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_ERROR
};
struct DataHeader {
	short dataLength;
	short cmd;
};
struct Login {
	char userName[32];
	char password[32];
};
struct LoginResult {
	int result;
};
struct Logout {
	char userName[32];
};
struct LogoutResult {
	int result;
};
struct DataPackage {
	int age;
	char name[32];
};

int main() {
	// 启动windows socket 2.x环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	// 用socket API建立简易TCP客户端
	// 1.建立一个socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (_sock == INVALID_SOCKET) {
		printf("错误，建立socket成功\n");
	}
	else {
		printf("建立socket网络成功\n");
	}
	// 2.连接服务器 connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret) {
		printf("错误，连接服务器失败...\n");
	}
	else {
		printf("连接服务器成功 \n");
	}
	while (true) {
		// 3.输入请求命令
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);
		// 处理请求命令
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("收到退出命令");
			break;
		}
		else if(0 == strcmp(cmdBuf, "login")){
			Login login = {"tony", "zhou123456"};
			DataHeader header = {sizeof(Login), CMD_LOGIN};
			send(_sock, (const char*)&header, sizeof(DataHeader), 0);
			send(_sock, (const char*)&login, sizeof(Login), 0);
			DataHeader headRet = {};
			LoginResult ret = {};
			recv(_sock, (char*)&headRet, sizeof(DataHeader), 0);
			recv(_sock, (char*)&ret, sizeof(LoginResult), 0);
			printf("登陆收到服务器返回结果： %d \n", ret.result);
		}
		else if (0 == strcmp(cmdBuf, "logout")) {
			Logout logout = { "tony" };
			DataHeader header = { sizeof(Login), CMD_LOGOUT };
			send(_sock, (const char*)&header, sizeof(DataHeader), 0);
			send(_sock, (const char*)&logout, sizeof(Logout), 0);
			DataHeader headRet = {};
			LogoutResult ret = {};
			recv(_sock, (char*)&headRet, sizeof(DataHeader), 0);
			recv(_sock, (char*)&ret, sizeof(LogoutResult), 0);
			printf("登出收到服务器返回结果： %d \n", ret.result);
		}
		else {
			printf("不支持的命令，请重新输入。\n");
		}
	}
	// 4.关闭套接字socket
	closesocket(_sock);
	WSACleanup();
	printf("任务结束，程序退出\n");
	getchar();
	return 0;
}
