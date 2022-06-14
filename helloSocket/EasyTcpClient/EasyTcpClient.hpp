#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
#define FD_SETSIZE 20000
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
			printf("关闭旧链接<socket=%d> \n", (int)_sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (_sock == INVALID_SOCKET) {
			printf("错误，建立socket失败\n");
		}
		else {
			//printf("建立socket网络成功\n");
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
			//printf("连接服务器成功%d \n", ret);
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
				printf("<socket=%d>select任务结束\n", (int)_sock);
				Close();
				return false;
			}

			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				if (SOCKET_ERROR == RecvData()) {
					printf("<socket=%d>select任务结束2\n", (int)_sock);
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
	// 缓冲区最小单元大小
#define RECV_BUFF_SIZE 10240
	// 接收缓冲区
	char _szRecv[RECV_BUFF_SIZE] = {};
	// 第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10] = {};
	// 消息缓冲区尾部位置
	int _lastPos = 0;
	// 接收数据
	int RecvData() {
		
		// 5.接收服务端数据
		int nLen = (int)recv(_sock, (char*)&_szRecv, RECV_BUFF_SIZE, 0);
		if (nLen < 0) {
			printf("<socket=%d>与服务器端口连接， 任务结束\n", (int)_sock);
			return -1;
		}
		// 将收取的数据拷贝到消息缓冲区
		memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);
		// 消息缓冲区的数据尾部位置后移
		_lastPos += nLen;
		// 判断消息缓冲区的数据长度大于消息头DataHeader长度
		while (_lastPos >= sizeof(DataHeader)) {
			// 这时就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)_szMsgBuf;
			// 判断消息缓冲区的数据长度大于消息长度
			if (_lastPos >= header->dataLength) {
				// 消息缓冲区剩余未处理数据的长度
				int nSize = _lastPos - header->dataLength;
				// 处理网络消息
				OnNetMsg(header);
				// 将消息缓冲区剩余未处理的数据前移
				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
				_lastPos = nSize;
			}
			else {
				// 消息缓冲区剩余数据不够一条完整消息
				break;
			}
		}
		return 0;
	}
	virtual void OnNetMsg(DataHeader* header) {
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT: {

			LoginResult* loginRet = (LoginResult*)header;
			//printf("登陆命令： CMD_LOGIN_RESULT 包体长度：%d;登陆结果： %d \n", header->dataLength, loginRet->result);
		}
							   break;
		case CMD_LOGOUT_RESULT: {

			LogoutResult* logoutRet = (LogoutResult*)header;
			//printf("登出命令： CMD_LOGOUT_RESULT 包体长度：%d;登陆结果： %d \n", header->dataLength, logoutRet->result);
		}
								break;
		case CMD_NEW_USER_JOIN: {
			NewUserJoin* newUserJoin = (NewUserJoin*)header;
			printf("新用户加入命令： CMD_NEW_USER_JOIN 包体长度：%d;登陆结果： %d \n", header->dataLength, newUserJoin->result);
		}
								break;
		case CMD_ERROR: {
			printf("<socket=%d>收到服务端消息： CMD_ERROR 包体长度：%d\n", (int)_sock, header->dataLength);
		}
						break;
		default: {
			printf("<socket=%d>收到未定义消息 包体长度：%d\n", (int)_sock, header->dataLength);
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
