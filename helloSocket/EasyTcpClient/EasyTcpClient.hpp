#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

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
#include<thread>
#include "MessageHeader.hpp"

class EasyTcpClient {
	
public:
	SOCKET _sock;
	EasyTcpClient() {
		_sock = INVALID_SOCKET;
		printf("初始化<socket=%d> %d\n", _sock, INVALID_SOCKET);
	}
	virtual ~EasyTcpClient() {
		Close();
	}
	// 初始化socket
	void initSocket() {
		// 启动Win, Sock 2.x环境
#ifdef _WIN32
		// 启动windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		// 用socket API建立简易TCP客户端
		// 1.建立一个socket
		if (INVALID_SOCKET != _sock) {
			printf("关闭旧链接<socket=%d> \n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (_sock == INVALID_SOCKET) {
			printf("错误，建立socket失败\n");
		}
		else {
			printf("建立socket网络成功\n");
		}
	}
	// 连接服务器
	int Connect(const char* ip, unsigned short port) {
		if (INVALID_SOCKET == _sock) {
			initSocket();
		}
		// 2.连接服务器 connect
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);// win 68,mac 126,ubuntu 164
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			printf("错误，连接服务器失败...\n");
		}
		else {
			printf("连接服务器成功%d \n", ret);
		}
		return ret;
	}
	// 关闭socket
	void Close() {
		if (INVALID_SOCKET != _sock) {
#ifdef _WIN32
			// 4.关闭套接字socket
			closesocket(_sock);
			WSACleanup();
#else
			close(_sock);
#endif
		}
		_sock = INVALID_SOCKET;
	}
	// 处理网络消息
	bool onRun() {
		if (isRun()) {
			fd_set fdRead;
			FD_ZERO(&fdRead);
			FD_SET(_sock, &fdRead);

			timeval t = { 1, 0 };
			int ret = (int)select(_sock + 1, &fdRead, 0, 0, &t);
			if (ret < 0) {
				printf("<socket=%d>select任务结束\n", _sock);
				Close();
				return false;
			}

			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				if (SOCKET_ERROR == RecvData()) {
					printf("<socket=%d>select任务结束2\n", _sock);
					Close();
					return false;
				}
			}
			return true;
		}
		return false;
	}
	// 是否工作中
	bool isRun() {
		return INVALID_SOCKET != _sock;
	}
	// 接收数据
	int RecvData() {
		// 5.接收服务端数据
		char recvBuf[1024] = {};
		int nLen = (int)recv(_sock, (char*)&recvBuf, sizeof(DataHeader), 0);
		DataHeader * header = (DataHeader*)recvBuf;
		//printf("接收服务端命令： %d 数据长度 %d\n", recvBuf.cmd, recvBuf.dataLength);
		if (nLen < 0) {
			printf("客户端已退出， 任务结束。\n");
			return -1;
		}
		// 6.处理请求并发送数据
		printf("收到服务端命令：%d", header->cmd);
		recv(_sock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(header);
		return 0;
	}
	virtual void OnNetMsg(DataHeader* header) {
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT: {

			LoginResult* loginRet = (LoginResult*)header;
			printf("登陆命令： CMD_LOGIN_RESULT 包体长度：%d;登陆结果： %d \n", header->dataLength, loginRet->result);
		}
							   break;
		case CMD_LOGOUT_RESULT: {

			LogoutResult* logoutRet = (LogoutResult*)header;
			printf("登出命令： CMD_LOGOUT_RESULT 包体长度：%d;登陆结果： %d \n", header->dataLength, logoutRet->result);
		}
								break;
		case CMD_NEW_USER_JOIN: {
			NewUserJoin* newUserJoin = (NewUserJoin*)header;
			printf("新用户加入命令： CMD_NEW_USER_JOIN 包体长度：%d;登陆结果： %d \n", header->dataLength, newUserJoin->result);
		}
								break;
		}
	}
	// 发送数据
	int SendData(DataHeader* header) {
		if (isRun() && header) {
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
private:
};
#endif // _EasyTcpClient_hpp_
