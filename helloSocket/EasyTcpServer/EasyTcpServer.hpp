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

// 缓冲区最小单元大小
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

class ClientSocket {
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET) {
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}
	SOCKET sockfd() {
		return _sockfd;
	}
	char* msgBuf() {
		return _szMsgBuf;
	}
	int getLastPos() {
		return _lastPos;
	}
	void setLastPost(int pos) {
		_lastPos = pos;
	}
private:
	// socket fd_set file desc set
	SOCKET _sockfd;
	// 第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	// 消息缓冲区尾部位置
	int _lastPos;
};
class EasyTcpServer {
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
		_port = -1;
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
		SOCKET csock = INVALID_SOCKET;
#ifdef _WIN32
		csock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		csock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif

		if (INVALID_SOCKET == csock) {
			printf("socket=<%d>错误，接收到无效客户端SOCKET...\n", (int)_sock);
		}
		else {
			printf("socket=<%d>新客户端加入： socket=<%d>,IP=<%s> \n", (int)_sock, (int)csock, inet_ntoa(clientAddr.sin_addr));
			NewUserJoin newUserJoin;
			SendDataToAll(&newUserJoin);
			_clients.push_back(new ClientSocket(csock));
		}
		return csock;
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
			for (int i = (int)_clients.size() - 1; i >= 0; i--)
			{
				FD_SET(_clients[i]->sockfd(), &fdRead);
				if (maxSocket < _clients[i]->sockfd()) {
					maxSocket = _clients[i]->sockfd();
				}
			}
			// nfds是一个整数，是指fd_set集合中所有描述符（socket）的范围，而不是数量
			// 既是所有描述符最大值加1，在windows上这个指可以写0
			timeval t = { 1, 0 };
			int ret = (int)select(maxSocket + 1, &fdRead, &fdWrite, &fdExp, &t);
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
			for (int i = (int)_clients.size() - 1; i >= 0; i--)
			{
				if (FD_ISSET(_clients[i]->sockfd(), &fdRead)) {
					if (-1 == RecvData(_clients[i])) {
						auto iter = _clients.begin() + i;
						if (iter != _clients.end()) {
							delete _clients[i];
							_clients.erase(iter);
						}
					}
				}
			}
			return true;
		}
		return false;
	}
	// 接收缓冲区
	char _szRecv[RECV_BUFF_SIZE] = {};

	int RecvData(ClientSocket* pClient) {
		int nLen = (int)recv(pClient->sockfd(), (char*)&_szRecv, RECV_BUFF_SIZE, 0);

		if (nLen < 0) {
			printf("客户端已退出， 任务结束。\n");
			return -1;
		}
		
		// 将收取的数据拷贝到消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		// 消息缓冲区的数据尾部位置后移
		pClient->setLastPost(pClient->getLastPos() + nLen);
		// 判断消息缓冲区的数据长度大于消息头DataHeader长度
		while (pClient->getLastPos() >= sizeof(DataHeader)) {
			// 这时就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			// 判断消息缓冲区的数据长度大于消息长度
			if (pClient->getLastPos() >= header->dataLength) {
				// 消息缓冲区剩余未处理数据的长度
				int nSize = pClient->getLastPos() - header->dataLength;
				// 处理网络消息
				OnNetMsg(pClient->sockfd(), header);
				// 将消息缓冲区剩余未处理的数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				pClient->setLastPost(nSize);
			}
			else {
				// 消息缓冲区剩余数据不够一条完整消息
				break;
			}
		}
		return 0;
	}
	virtual void OnNetMsg(SOCKET csock, DataHeader* header) {
		// 6.处理请求并发送数据
		switch (header->cmd)
		{
		case CMD_LOGIN: {
			Login* login = (Login*)header;
			//printf("登陆命令： CMD_LOGIN 包体长度：%d;登陆用户名称： %s, 用户密码： %s \n", header->dataLength, login->userName, login->password);
			LoginResult logRet;
			SendData(csock, &logRet);
		}
						break;
		case CMD_LOGOUT: {

			Logout* logout = (Logout*)header;
			//printf("登出命令： CMD_LOGOUT 包体长度：%d;登出用户名称： %s\n", header->dataLength, logout->userName);
			LogoutResult logoutRet;
			SendData(csock, &logoutRet);
		}
						 break;
		default: {
			printf("<socket=%d>收到未定义消息 包体长度：%d\n", (int)csock, header->dataLength);
			DataHeader dateHeader;
			SendData(csock, &dateHeader);
		}
		}
	}
	// 发送指定用户
	int SendData(SOCKET csock, DataHeader* header) {
		if (isRun() && header) {
			return send(csock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
	// 发送指定用户
	int SendDataToAll(DataHeader* header) {
		for (int i = (int)_clients.size() - 1; i >= 0; i--)
		{
			SendData(_clients[i]->sockfd(), header);
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
			for (size_t i = _clients.size() - 1; i >= 0; i--)
			{
				closesocket(_clients[i]->sockfd());
				delete _clients[i];
			}
			// 6.关闭套接字socket
			closesocket(_sock);
			WSACleanup();
#else
			for (int i = (int)_clients.size() - 1; i >= 0; i--)
			{
				close(_clients[i]->sockfd());
				delete _clients[i];
			}
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
			_clients.clear();
		}
	}
private:
	SOCKET _sock;
	int _port;
	std::vector<ClientSocket*> _clients;
};