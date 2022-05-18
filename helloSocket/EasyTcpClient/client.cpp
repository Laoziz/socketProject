#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<windows.h>
#include<Winsock2.h>

#include<stdio.h>
#include<thread>

enum CMD {
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
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
struct NewUserJoin : public DataHeader {
	NewUserJoin() {
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		result = 0;
	}
	int result;
};

bool g_Exit = true;
void cmdThread (SOCKET _sock) {
	while (true) {
		// 3.输入请求命令
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);
		// 处理请求命令
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("收到退出命令");
			g_Exit = false;
			return;
		}
		else if (0 == strcmp(cmdBuf, "login")) {
			Login login;
			strcpy(login.userName, "tony");
			strcpy(login.password, "zhou123456");
			send(_sock, (const char*)&login, sizeof(Login), 0);
		}
		else if (0 == strcmp(cmdBuf, "logout")) {
			Logout logout;
			strcpy(logout.userName, "tony");
			send(_sock, (const char*)&logout, sizeof(Logout), 0);
		}
		else {
			printf("不支持的命令，请重新输入。\n");
		}
	}
}

int processor(SOCKET _csock) { 
	// 5.接收服务端数据
	char recvBuf[1024] = {};
	int nLen = recv(_csock, (char*)&recvBuf, sizeof(DataHeader), 0);
	DataHeader * header = (DataHeader*)recvBuf;
	//printf("接收服务端命令： %d 数据长度 %d\n", recvBuf.cmd, recvBuf.dataLength);
	if (nLen < 0) {
		printf("客户端已退出， 任务结束。\n");
		return -1;
	}
	// 6.处理请求并发送数据
	printf("收到服务端命令：%d", header->cmd);
	switch (header->cmd)
	{
		case CMD_LOGIN_RESULT: {
			recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			LoginResult* loginRet = (LoginResult*)recvBuf;
			printf("登陆命令： CMD_LOGIN_RESULT 包体长度：%d;登陆结果： %d \n", header->dataLength, loginRet->result);
		}
		break;
		case CMD_LOGOUT_RESULT: {
			recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			LogoutResult* logoutRet = (LogoutResult*)recvBuf;
			printf("登出命令： CMD_LOGOUT_RESULT 包体长度：%d;登陆结果： %d \n", header->dataLength, logoutRet->result);
		}
		break;
		case CMD_NEW_USER_JOIN: {
			recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			NewUserJoin* newUserJoin = (NewUserJoin*)recvBuf;
			printf("新用户加入命令： CMD_NEW_USER_JOIN 包体长度：%d;登陆结果： %d \n", header->dataLength, newUserJoin->result);
		}
		break;
	}
	return 0;
}
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
	std::thread td(cmdThread, _sock);
	td.detach();
	while (g_Exit) {

		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);

		timeval t = { 1, 0 };
		int ret = select(_sock, &fdRead, 0, 0, &t);
		if (ret < 0) {
			printf("select任务结束\n");
			break;
		}

		if (FD_ISSET(_sock, &fdRead)) {
			FD_CLR(_sock, &fdRead);
			processor(_sock);
		}
		//printf("空闲时间处理其它业务。。。。\n");
	}
	// 4.关闭套接字socket
	closesocket(_sock);
	WSACleanup();
	printf("任务结束，程序退出\n");
	getchar();
	return 0;
}
