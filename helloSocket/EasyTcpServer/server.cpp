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
	char msgBuff[] = "Hello, I'm Server.";
	while (true) {
		_csock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
		if (INVALID_SOCKET == _csock) {
			printf("错误，接收到无效客户端SOCKET...\n");
		}
		printf("新客户端加入： IP = %s \n", inet_ntoa(clientAddr.sin_addr));
		// 5.send 向客户端发送一条数据
		send(_csock, msgBuff, strlen(msgBuff) + 1, 0);
	}

	// 6.关闭套接字socket
	closesocket(_sock);
	WSACleanup();
	return 0;
}
