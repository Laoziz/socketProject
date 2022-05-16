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
	// 1. ����һ��socket �׽���
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 2. bind �����ڽ��ܿͻ������ӵ�����˿�
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);// host to net unsigned short
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;
	
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		printf("���󣬰�����˿�4567ʧ��...\n");
		return 0;
	}
	else {
		printf("�ɹ���������˿�4567�ɹ�...\n");
	}

	// 3. listen ��������˿�
	if (SOCKET_ERROR == listen(_sock,5)) {
		printf("���󣬼�������˿�4567ʧ��...\n");
		return 0;
	}
	else {
		printf("�ɹ�����������˿�4567�ɹ�...\n");
	}

	// 4.�ȴ����ܿͻ�������
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(sockaddr_in);
	SOCKET _csock = INVALID_SOCKET;

	_csock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _csock) {
		printf("���󣬽��յ���Ч�ͻ���SOCKET...\n");
	}
	printf("�¿ͻ��˼��룺 %d IP = %s \n", (int)_csock, inet_ntoa(clientAddr.sin_addr));
	
	while (true) {
		// 5.���տͻ�������
		DataHeader header = {};
		int nLen = recv(_csock, (char*)&header,sizeof(DataHeader), 0);
		printf("���տͻ������ %d ���ݳ��� %d\n", header.cmd, header.dataLength);
		if (nLen < 0) {
			printf("�ͻ������˳��� ���������\n");
			break;
		}
		// 6.�������󲢷�������
		switch (header.cmd)
		{
			case CMD_LOGIN: {
				Login login = {};
				recv(_csock, (char*)&login, sizeof(Login), 0);
				printf("��½�û����ƣ� %s, �û����룺 %s \n", login.userName, login.password);
				LoginResult logRet = { 0 };
				send(_csock, (const char*)&header, sizeof(DataHeader), 0);
				send(_csock, (const char*)&logRet, sizeof(LoginResult), 0);
			}
			break;
			case CMD_LOGOUT: {
				Logout logout = {};
				recv(_csock, (char*)&logout, sizeof(Logout), 0);
				printf("�ǳ��û����ƣ� %s\n", logout.userName);
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

	// 6.�ر��׽���socket
	closesocket(_sock);
	WSACleanup();
	printf("��������������˳�\n");
	getchar();
	return 0;
}
