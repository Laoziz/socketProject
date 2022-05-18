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
		// 3.������������
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);
		// ������������
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("�յ��˳�����");
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
			printf("��֧�ֵ�������������롣\n");
		}
	}
}

int processor(SOCKET _csock) { 
	// 5.���շ��������
	char recvBuf[1024] = {};
	int nLen = recv(_csock, (char*)&recvBuf, sizeof(DataHeader), 0);
	DataHeader * header = (DataHeader*)recvBuf;
	//printf("���շ������� %d ���ݳ��� %d\n", recvBuf.cmd, recvBuf.dataLength);
	if (nLen < 0) {
		printf("�ͻ������˳��� ���������\n");
		return -1;
	}
	// 6.�������󲢷�������
	printf("�յ���������%d", header->cmd);
	switch (header->cmd)
	{
		case CMD_LOGIN_RESULT: {
			recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			LoginResult* loginRet = (LoginResult*)recvBuf;
			printf("��½��� CMD_LOGIN_RESULT ���峤�ȣ�%d;��½����� %d \n", header->dataLength, loginRet->result);
		}
		break;
		case CMD_LOGOUT_RESULT: {
			recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			LogoutResult* logoutRet = (LogoutResult*)recvBuf;
			printf("�ǳ���� CMD_LOGOUT_RESULT ���峤�ȣ�%d;��½����� %d \n", header->dataLength, logoutRet->result);
		}
		break;
		case CMD_NEW_USER_JOIN: {
			recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			NewUserJoin* newUserJoin = (NewUserJoin*)recvBuf;
			printf("���û�������� CMD_NEW_USER_JOIN ���峤�ȣ�%d;��½����� %d \n", header->dataLength, newUserJoin->result);
		}
		break;
	}
	return 0;
}
int main() {
	// ����windows socket 2.x����
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	// ��socket API��������TCP�ͻ���
	// 1.����һ��socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (_sock == INVALID_SOCKET) {
		printf("���󣬽���socket�ɹ�\n");
	}
	else {
		printf("����socket����ɹ�\n");
	}
	// 2.���ӷ����� connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret) {
		printf("�������ӷ�����ʧ��...\n");
	}
	else {
		printf("���ӷ������ɹ� \n");
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
			printf("select�������\n");
			break;
		}

		if (FD_ISSET(_sock, &fdRead)) {
			FD_CLR(_sock, &fdRead);
			processor(_sock);
		}
		//printf("����ʱ�䴦������ҵ�񡣡�����\n");
	}
	// 4.�ر��׽���socket
	closesocket(_sock);
	WSACleanup();
	printf("��������������˳�\n");
	getchar();
	return 0;
}
