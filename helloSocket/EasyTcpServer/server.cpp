#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<windows.h>
#include<Winsock2.h>

#include<stdio.h>

enum CMD{
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

struct DataPackage{
	int age;
	char name[32];
};

int main() {
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	// ----------------------
	// 1. 建立一个socket 套接字
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 2. bind 绑定用于接受客户端连接的网络端口
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);// host to net unsigned short
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;
	
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		printf("错误，绑定网络端口4567失败...\n");
		return 0;
	}
	else {
		printf("成功，绑定网络端口4567成功...\n");
	}

	// 3. listen 监听网络端口
	if (SOCKET_ERROR == listen(_sock,5)) {
		printf("错误，监听网络端口4567失败...\n");
		return 0;
	}
	else {
		printf("成功，监听网络端口4567成功...\n");
	}

	// 4.等待接受客户端连接
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(sockaddr_in);
	SOCKET _csock = INVALID_SOCKET;

	_csock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _csock) {
		printf("错误，接收到无效客户端SOCKET...\n");
	}
	printf("新客户端加入： %d IP = %s \n", (int)_csock, inet_ntoa(clientAddr.sin_addr));
	
	while (true) {
		// 5.接收客户端数据
		DataHeader header = {};
		int nLen = recv(_csock, (char*)&header,sizeof(DataHeader), 0);
		printf("接收客户端命令： %d 数据长度 %d\n", header.cmd, header.dataLength);
		if (nLen < 0) {
			printf("客户端已退出， 任务结束。\n");
			break;
		}
		// 6.处理请求并发送数据
		switch (header.cmd)
		{
			case CMD_LOGIN: {
				Login login = {};
				recv(_csock, (char*)&login, sizeof(Login), 0);
				printf("登陆用户名称： %s, 用户密码： %s \n", login.userName, login.password);
				LoginResult logRet = { 0 };
				send(_csock, (const char*)&header, sizeof(DataHeader), 0);
				send(_csock, (const char*)&logRet, sizeof(LoginResult), 0);
			}
			break;
			case CMD_LOGOUT: {
				Logout logout = {};
				recv(_csock, (char*)&logout, sizeof(Logout), 0);
				printf("登出用户名称： %s\n", logout.userName);
				LogoutResult logoutRet = { 1 };
				send(_csock, (const char*)&header, sizeof(DataHeader), 0);
				send(_csock, (const char*)&logoutRet, sizeof(LogoutResult), 0);
			}
			break;
			default: {
				header.cmd = CMD_ERROR;
				header.dataLength = 0;
				send(_csock, (const char*)&header, sizeof(DataHeader), 0);
			}
			break;
		}
	}

	// 6.关闭套接字socket
	closesocket(_sock);
	WSACleanup();
	printf("任务结束，程序退出\n");
	getchar();
	return 0;
}
