#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<windows.h>
#include<Winsock2.h>

#include<stdio.h>
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
	char msgBuff[] = "Hello, I'm Server.";
	while (true) {
		_csock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
		if (INVALID_SOCKET == _csock) {
			printf("���󣬽��յ���Ч�ͻ���SOCKET...\n");
		}
		printf("�¿ͻ��˼��룺 IP = %s \n", inet_ntoa(clientAddr.sin_addr));
		// 5.send ��ͻ��˷���һ������
		send(_csock, msgBuff, strlen(msgBuff) + 1, 0);
	}

	// 6.�ر��׽���socket
	closesocket(_sock);
	WSACleanup();
	return 0;
}
