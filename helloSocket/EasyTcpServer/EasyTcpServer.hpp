#pragma once
#ifdef _WIN32
	#include<windows.h>
	#include<Winsock2.h>
	#pragma comment(lib, "Ws2_32.lib")
#else
	#include<unistd.h> // uni std
	#include<arpa/inet.h>
	#include<string.h>
	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR (-1)
#endif

#include<stdio.h>
#include<vector>
#include "MessageHeader.hpp"

class EasyTcpServer {
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
		_port = -1;
		printf("初始化socket=<%d> %d\n", _sock, INVALID_SOCKET);
	}
	virtual ~EasyTcpServer() {
		Close();
	}
	// 初始化socket
	SOCKET InitSocket() {
#ifdef _WIN32
		// 启动windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		// 1.建立一个socket
		if (INVALID_SOCKET != _sock) {
			printf("关闭旧链接socket=<%d> \n", (int)_sock);
			Close();
		}
		// ----------------------
		// 1. 建立一个socket 套接字
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_sock == INVALID_SOCKET) {
			printf("错误，建立socket失败\n");
		}
		else {
			printf("建立socket网络成功\n");
		}
		return _sock;
	}
	// 绑定端口号
	int Bind(const char* ip, unsigned short port) {
		if (INVALID_SOCKET == _sock) {
			InitSocket();
		}
		// 2. bind 绑定用于接受客户端连接的网络端口
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);// host to net unsigned short
#ifdef _WIN32
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret) {
			printf("链接socket=<%d>错误，绑定网络端口port=<%d>失败...\n", (int)_sock, port);
			return 0;
		}
		else {
			printf("链接socket=<%d>成功，绑定网络端口port=<%d>成功...\n", (int)_sock, port);
		}
		_port = port;
		return ret;
	}
	// 监听端口号
	int Listen(int n) {
		// 3. listen 监听网络端口
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			printf("socket=<%d>错误，监听网络端口port=<%d>失败...\n", (int)_sock, _port);
			return -1;
		}
		else {
			printf("socket=<%d>成功，监听网络端口port=<%d>成功...\n", (int)_sock, _port);
		}
		return 0;
	}
	// 接受客户端链接
	SOCKET Accept() {
		// 4.等待接受客户端连接
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET _csock = INVALID_SOCKET;
#ifdef _WIN32
		_csock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		_csock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif

		if (INVALID_SOCKET == _csock) {
			printf("socket=<%d>错误，接收到无效客户端SOCKET...\n", (int)_sock);
		}
		else {
			printf("socket=<%d>新客户端加入： socket=<%d>,IP=<%s> \n", (int)_sock, (int)_csock, inet_ntoa(clientAddr.sin_addr));
			NewUserJoin newUserJoin;
			SendDataToAll(&newUserJoin);
			g_clients.push_back(_csock);
		}
		return _csock;
	}
	// 处理网络消息
	bool OnRun() {
		if (isRun()) {
			// 伯克利套接字 BSD socket
			fd_set fdRead; // 描述符集合
			fd_set fdWrite;
			fd_set fdExp;

			// 清理集合
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);

			// 将描述符(socket)加入集合
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
			// nfds是一个整数，是指fd_set集合中所有描述符（socket）的范围，而不是数量
			// 既是所有描述符最大值加1，在windows上这个指可以写0
			timeval t = { 1, 0 };
			int ret = select(maxSocket + 1, &fdRead, &fdWrite, &fdExp, &t);
			//// 阻塞模式
			//int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, NULL);
			if (ret < 0) {
				printf("select 任务结束。 \n");
				Close();
				return false;
			}
			// 判断描述符(socket)是否在集合中
			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				Accept();
			}
			for (int i = (int)g_clients.size() - 1; i >= 0; i--)
			{
				if (FD_ISSET(g_clients[i], &fdRead)) {
					if (-1 == RecvData(g_clients[i])) {
						auto iter = g_clients.begin() + i;
						if (iter != g_clients.end()) {
							g_clients.erase(iter);
						}
					}
				}
			}
			return true;
		}
		return false;
	}
	int RecvData(SOCKET _csock) {
		// 5.接收客户端数据
		char recvBuf[1024] = {};
		int nLen = (int)recv(_csock, (char*)&recvBuf, sizeof(DataHeader), 0);
		DataHeader * header = (DataHeader*)recvBuf;
		//printf("接收客户端命令： %d 数据长度 %d\n", recvBuf.cmd, recvBuf.dataLength);
		if (nLen < 0) {
			printf("客户端已退出， 任务结束。\n");
			return -1;
		}
		recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(_csock, header);
		return 0;
	}
	virtual void OnNetMsg(SOCKET _csock, DataHeader* header) {
		// 6.处理请求并发送数据
		switch (header->cmd)
		{
		case CMD_LOGIN: {
			Login* login = (Login*)header;
			printf("登陆命令： CMD_LOGIN 包体长度：%d;登陆用户名称： %s, 用户密码： %s \n", header->dataLength, login->userName, login->password);
			LoginResult logRet;
			SendData(_csock, &logRet);
		}
						break;
		case CMD_LOGOUT: {

			Logout* logout = (Logout*)header;
			printf("登出命令： CMD_LOGOUT 包体长度：%d;登出用户名称： %s\n", header->dataLength, logout->userName);
			LogoutResult logoutRet;
			SendData(_csock, &logoutRet);
		}
						 break;
		default: {
			DataHeader header = {};
			header.cmd = CMD_ERROR;
			header.dataLength = sizeof(DataHeader);
			SendData(_csock, &header);
		}
				 break;
		}
	}
	// 发送指定用户
	int SendData(SOCKET _csock, DataHeader* header) {
		if (isRun() && header) {
			return send(_csock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
	// 发送指定用户
	int SendDataToAll(DataHeader* header) {
		for (int i = (int)g_clients.size() - 1; i >= 0; i--)
		{
			SendData(g_clients[i], header);
		}
		return SOCKET_ERROR;
	}
	// 是否工作中
	bool isRun() {
		return INVALID_SOCKET != _sock;
	}
	// 关闭网络
	void Close() {
		if (INVALID_SOCKET != _sock) {
#ifdef _WIN32
			for (size_t i = g_clients.size() - 1; i >= 0; i--)
			{
				closesocket(g_clients[i]);
			}
			// 6.关闭套接字socket
			closesocket(_sock);
			WSACleanup();
#else
			for (int i = (int)g_clients.size() - 1; i >= 0; i--)
			{
				close(g_clients[i]);
			}
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
	}
private:
	SOCKET _sock;
	int _port;
	std::vector<SOCKET> g_clients;
};