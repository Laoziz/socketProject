#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<windows.h>
#include<Winsock2.h>

#include<stdio.h>

enum CMD {
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT_RESULT,
	CMD_ERROR
};
struct DataHeader {
	short dataLength;
	short cmd;
};
struct Login : public DataHeader {
	Login() {
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char password[32];
};
struct LoginResult : public DataHeader {
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};
struct Logout : public DataHeader {
	Logout() {
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};
struct LogoutResult : public DataHeader {
	LogoutResult() {
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
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
		char recvBuf[1024] = {};
		int nLen = recv(_csock, (char*)&recvBuf,sizeof(DataHeader), 0);
		DataHeader * header = (DataHeader*)recvBuf;
		//printf("接收客户端命令： %d 数据长度 %d\n", recvBuf.cmd, recvBuf.dataLength);
		if (nLen < 0) {
			printf("客户端已退出， 任务结束。\n");
			break;
		}
		// 6.处理请求并发送数据
		switch (header->cmd)
		{
			case CMD_LOGIN: {
				recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
				Login* login = (Login*)recvBuf;
				printf("登陆命令： CMD_LOGIN 包体长度：%d;登陆用户名称： %s, 用户密码： %s \n", header->dataLength, login->userName, login->password);
				LoginResult logRet;
				send(_csock, (const char*)&logRet, sizeof(LoginResult), 0);
			}
			break;
			case CMD_LOGOUT: {
				recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
				Logout* logout = (Logout*)recvBuf;
				printf("登陆命令： CMD_LOGOUT 包体长度：%d;登出用户名称： %s\n", header->dataLength, logout->userName);
				LogoutResult logoutRet;
				send(_csock, (const char*)&logoutRet, sizeof(LogoutResult), 0);
			}
			break;
			default: {
				DataHeader header = {};
				header.cmd = CMD_ERROR;
				header.dataLength = sizeof(DataHeader);
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
