#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<windows.h>
#include<Winsock2.h>

#include<stdio.h>
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
	// 3.���շ�������Ϣ recv
	char recvBuf[256] = {};
	int nlen = recv(_sock, recvBuf, 256, 0);
	if (nlen > 0) {
		printf("recv %s \n", recvBuf);
	}
	
	// 4.�ر��׽���socket
	closesocket(_sock);
	WSACleanup();
	getchar();
}