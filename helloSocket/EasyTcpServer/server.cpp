#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifdef _WIN32
#include<windows.h>
#include<Winsock2.h>
#else
#include<unistd.h> // uni std
#include<arpa/inet.h>
#include<string.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR -1
#endif

#include<stdio.h>
#include<vector>
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
std::vector<SOCKET> g_clients;

int processor(SOCKET _csock) {
	// 5.���տͻ�������
	char recvBuf[1024] = {};
	int nLen = (int)recv(_csock, (char*)&recvBuf, sizeof(DataHeader), 0);
	DataHeader * header = (DataHeader*)recvBuf;
	//printf("���տͻ������ %d ���ݳ��� %d\n", recvBuf.cmd, recvBuf.dataLength);
	if (nLen < 0) {
		printf("�ͻ������˳��� ���������\n");
		return -1;
	}
	// 6.�������󲢷�������
	switch (header->cmd)
	{
	case CMD_LOGIN: {
		recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		Login* login = (Login*)recvBuf;
		printf("��½��� CMD_LOGIN ���峤�ȣ�%d;��½�û����ƣ� %s, �û����룺 %s \n", header->dataLength, login->userName, login->password);
		LoginResult logRet;
		send(_csock, (const char*)&logRet, sizeof(LoginResult), 0);
	}
					break;
	case CMD_LOGOUT: {
		recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		Logout* logout = (Logout*)recvBuf;
		printf("��½��� CMD_LOGOUT ���峤�ȣ�%d;�ǳ��û����ƣ� %s\n", header->dataLength, logout->userName);
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
	return 0;
}

int main() {
#ifdef _WIN32
	// ����windows socket 2.x����
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif

	// ----------------------
	// 1. ����һ��socket �׽���
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 2. bind �����ڽ��ܿͻ������ӵ�����˿�
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);// host to net unsigned short
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
	_sin.sin_addr.s_addr = INADDR_ANY;
#endif

	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		printf("���󣬰�����˿�4567ʧ��...\n");
		return 0;
	}
	else {
		printf("�ɹ���������˿�4567�ɹ�...\n");
	}

	// 3. listen ��������˿�
	if (SOCKET_ERROR == listen(_sock, 5)) {
		printf("���󣬼�������˿�4567ʧ��...\n");
		return 0;
	}
	else {
		printf("�ɹ�����������˿�4567�ɹ�...\n");
	}

	while (true) {
		// �������׽��� BSD socket
		fd_set fdRead; // ����������
		fd_set fdWrite;
		fd_set fdExp;

		// ������
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		// ��������(socket)���뼯��
		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);

		SOCKET maxSocket = _sock;
		for (int i = (int)g_clients.size() - 1; i >= 0; i--)
		{
			FD_SET(g_clients[i], &fdRead);
			if (maxSocket < g_clients[i]) {
				maxSocket = g_clients[i];
			}
		}
		// nfds��һ����������ָfd_set������������������socket���ķ�Χ������������
		// �����������������ֵ��1����windows�����ָ����д0
		timeval t = { 1, 0 };
		int ret = select(maxSocket + 1, &fdRead, &fdWrite, &fdExp, &t);
		//// ����ģʽ
		//int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, NULL);
		if (ret < 0) {
			printf("select ��������� \n");
			break;
		}
		// �ж�������(socket)�Ƿ��ڼ�����
		if (FD_ISSET(_sock, &fdRead)) {
			FD_CLR(_sock, &fdRead);

			// 4.�ȴ����ܿͻ�������
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(sockaddr_in);
			SOCKET _csock = INVALID_SOCKET;
#ifdef _WIN32
			_csock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
			_csock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif


			if (INVALID_SOCKET == _csock) {
				printf("���󣬽��յ���Ч�ͻ���SOCKET...\n");
			}
			else {
				printf("�¿ͻ��˼��룺 %d IP = %s \n", (int)_csock, inet_ntoa(clientAddr.sin_addr));
				NewUserJoin newUserJoin;
				for (int i = (int)g_clients.size() - 1; i >= 0; i--)
				{
					send(g_clients[i], (const char*)&newUserJoin, sizeof(NewUserJoin), 0);
				}
				g_clients.push_back(_csock);
			}
		}
		for (int i = (int)g_clients.size() - 1; i >= 0; i--)
		{
			if (FD_ISSET(g_clients[i], &fdRead)) {
				if (-1 == processor(g_clients[i])) {
					auto iter = g_clients.begin() + i;
					if (iter != g_clients.end()) {
						g_clients.erase(iter);
					}
				}
			}
		}
		//printf("����ʱ�䴦������ҵ�񡣡�����\n");
	}

#ifdef _WIN32
	for (size_t i = g_clients.size() - 1; i >= 0; i--)
	{
		closesocket(g_clients[i]);
	}
	// 6.�ر��׽���socket
	closesocket(_sock);
	WSACleanup();
#else
	for (int i = (int)g_clients.size() - 1; i >= 0; i--)
	{
		close(g_clients[i]);
	}
	close(_sock);
#endif
	printf("��������������˳� \n");
	getchar();
	return 0;
}
