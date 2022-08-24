#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
	#define FD_SETSIZE      2506
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<unistd.h> // uni std
	#include<arpa/inet.h>
	#include<string.h>
	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR (-1)
#endif

#include<stdio.h>
#include<functional>
#include<vector>
#include<map>
#include<thread>
#include<mutex>
#include<atomic>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
// 缓冲区最小单元大小
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240 * 5
#define SEND_BUFF_SIZE RECV_BUFF_SIZE
#endif // !RECV_BUFF_SIZE
#define _CellServer_THREAD_COUNT 4

class ClientSocket {
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET) {
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
		_lastPos = 0;
		memset(_szSendBuf, 0, SEND_BUFF_SIZE);
		_lastSendPos = 0;
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
	void _setLastPos(int pos) {
		_lastPos = pos;
	}

	//发送数据
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//要发送的数据长度
		int nSendLen = header->dataLength;
		//要发送的数据
		const char* pSendData = (const char*)header;

		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
			{
				//计算可拷贝的数据长度
				int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
				//拷贝数据
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
				//计算剩余数据位置
				pSendData += nCopyLen;
				//计算剩余数据长度
				nSendLen -= nSendLen;
				//发送数据
				ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
				//数据尾部位置清零
				_lastSendPos = 0;
				//发送错误
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
			}else {
				//将要发送的数据 拷贝到发送缓冲区尾部
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
				//计算数据尾部位置
				_lastSendPos += nSendLen;
				break;
			}
		}
		return ret;
	}
private:
	// socket fd_set file desc set
	SOCKET _sockfd;
	// 第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE];
	// 消息缓冲区尾部位置
	int _lastPos;
	//第二缓冲区 发送缓冲区
	char _szSendBuf[SEND_BUFF_SIZE];
	//发送缓冲区的数据尾部位置
	int _lastSendPos;
};

// 网络事件接口
class INetEvent {
public:
	// 纯虚函数
	// 客户端加入事件
	virtual void OnNetJoin(ClientSocket* pClient) = 0;
	// 客户端离开事件
	virtual void OnNetLeave(ClientSocket* pClient) = 0;
	// 客户端消息事件
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) = 0;
	//recv事件
	virtual void OnNetRecv(ClientSocket* pClient) = 0;
private:

};

// 
class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET) {
		_sock = sock;
		_pNetEvent = nullptr;
	};
	~CellServer() {
		Close();
		_sock = INVALID_SOCKET;
	}
	// 设置回调钩子
	void setEventObj(INetEvent* event) {
		_pNetEvent = event;
	}
	// 关闭网络
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			for (auto iter : _clients)
			{
				closesocket(iter.second->sockfd());
				delete iter.second;
			}
			//关闭套节字closesocket
			closesocket(_sock);
#else
			for (auto iter : _clients)
			{
				close(iter.second->sockfd());
				delete iter.second;
			}
			//关闭套节字closesocket
			close(_sock);
#endif
			_clients.clear();
		}
	}
	
	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//处理网络消息
	//备份客户socket fd_set
	fd_set _fdRead_bak;
	//客户列表是否有变化
	bool _clients_change;
	SOCKET _maxSock;
	// 处理网络消息
	bool OnRun() {
		_clients_change = true;
		while (isRun()) {
			if (_clientsBuff.size() > 0) {
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff) {
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = true;
			}
			// 如果没有需要处理的客户端，就跳过
			if (_clients.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			// 伯克利套接字 BSD socket
			fd_set fdRead; // 描述符集合

			// 清理集合
			FD_ZERO(&fdRead);

			// 将描述符(socket)加入集合
			if (_clients_change) {
				_clients_change = false;
				//将描述符（socket）加入集合
				_maxSock = _clients.begin()->second->sockfd();
				for (auto iter : _clients)
				{
					FD_SET(iter.second->sockfd(), &fdRead);
					if (_maxSock < iter.second->sockfd())
					{
						_maxSock = iter.second->sockfd();
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			// nfds是一个整数，是指fd_set集合中所有描述符（socket）的范围，而不是数量
			// 既是所有描述符最大值加1，在windows上这个指可以写0
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0) {
				printf("select 任务结束。 \n");
				Close();
				return false;
			}
			else if (ret == 0) {
				continue;
			}
#ifdef _WIN32
			for (int i = 0; i < fdRead.fd_count; i++) {
				auto iter = _clients.find(fdRead.fd_array[i]);
				if (iter != _clients.end()) {
					if (-1 == RecvData(iter->second)) {
						if (_pNetEvent) 
							_pNetEvent->OnNetLeave(iter->second);
						_clients_change = true;
						_clients.erase(iter->first);
					}
				}
				else {
					printf("error. if (iter != _clients.end())\n");
				}
			}
#else
			std::vector<ClientSocket*> temp;
			for (auto iter : _clients) {
				if (FD_ISSET(iter.second->sockfd(), &fdRead)) {
					if (-1 == RecvData(iter.second)) {
						if (_pNetEvent) {
							_pNetEvent->OnNetLeave(iter.second);
						}
						_clients_change = false;

						temp.push_back(iter.second);
					}
				}
			}
			for (auto pClient : temp)
			{
				_clients.erase(pClient->sockfd());
				delete pClient;
			}
#endif // _WIN32
		}
		return false;
	}

	int RecvData(ClientSocket* pClient) {
		char* szRecv = pClient->msgBuf() + pClient->getLastPos();
		int nLen = (int)recv(pClient->sockfd(), szRecv, (SEND_BUFF_SIZE)- pClient->getLastPos(), 0);
		_pNetEvent->OnNetRecv(pClient);
		if (nLen <= 0) {
			//printf("客户端已退出， 任务结束。\n", pClient->sockfd());
			return -1;
		}

		// 将收取的数据拷贝到消息缓冲区
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		// 消息缓冲区的数据尾部位置后移
		pClient->_setLastPos(pClient->getLastPos() + nLen);
		// 判断消息缓冲区的数据长度大于消息头DataHeader长度
		while (pClient->getLastPos() >= sizeof(DataHeader)) {
			// 这时就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			// 判断消息缓冲区的数据长度大于消息长度
			if (pClient->getLastPos() >= header->dataLength) {
				// 消息缓冲区剩余未处理数据的长度
				int nSize = pClient->getLastPos() - header->dataLength;
				// 处理网络消息
				OnNetMsg(pClient, header);
				// 将消息缓冲区剩余未处理的数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				pClient->_setLastPos(nSize);
			}
			else {
				// 消息缓冲区剩余数据不够一条完整消息
				break;
			}
		}
		return 0;
	}
	// 相应网络消息
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) {
		_pNetEvent->OnNetMsg(pClient, header);
	}
	// 添加客户端
	void addClient(ClientSocket* pClient) {
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	// 启动
	void Start() {
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
	}
	// 获取客户端数量
	size_t getClientCount() {
		return _clients.size() + _clientsBuff.size();
	}
private:
	SOCKET _sock;
	std::map<SOCKET, ClientSocket*> _clients;
	std::vector<ClientSocket*> _clientsBuff;
	// 缓冲队列锁
	std::mutex _mutex;
	std::thread _thread;
	// 网络事件对象
	INetEvent* _pNetEvent;
};

class EasyTcpServer : public INetEvent {
private:
	SOCKET _sock;
	int _port;
	// 消息处理对象，内部会创建线程
	std::vector<CellServer*> _cellServers;
	//每秒消息计时
	CELLTimestamp _tTime;
protected:
	// 收到消息计数
	std::atomic_int _recvCount;
	std::atomic_int _msgCount;
	// 客户端计数
	std::atomic_int _clientCount;
	
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
		_port = -1;
		_recvCount = 0;
		_msgCount = 0;
		_clientCount = 0;
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
		if (INVALID_SOCKET == _sock) {
			printf("错误，建立socket失败\n");
		}
		else {
			printf("建立socket=<%d>成功...\n", (int)_sock);
		}
		return _sock;
	}
	// 绑定端口号
	int Bind(const char* ip, unsigned short port) {
		//if (INVALID_SOCKET == _sock) {
		//	InitSocket();
		//}
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
		int ret = ::bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
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
		}
		else {
			printf("socket=<%d>成功，监听网络端口port=<%d>成功...\n", (int)_sock, _port);
		}
		return ret;
	}
	// 接受客户端链接
	SOCKET Accept() {
		// 4.等待接受客户端连接
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif

		if (INVALID_SOCKET == cSock) {
			printf("socket=<%d>错误，接收到无效客户端SOCKET...\n", (int)_sock);
		}
		else {
			// 获取ip地址 inet_ntoa(clientAddr.sin_addr)
			// 将新客户端分配给客户数量最少的cellServer
			addClientToCellServer(new ClientSocket(cSock));
		}
		return cSock;
	}
	void addClientToCellServer(ClientSocket* pClient) {
		// 查找客户数量最少的CellServer消息处理对象
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers) {
			if (pMinServer->getClientCount() > pCellServer->getClientCount()) {
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);
		OnNetJoin(pClient);
	}
	void Start(int nCellServer) {
		for (int i = 0; i < nCellServer; i++) {
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			// 注册网络事件接受对象
			ser->setEventObj(this);
			ser->Start();
		}
	}
	// 关闭网络
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			// 关闭套接字socket
			closesocket(_sock);
			// 清除windows socket环境
			WSACleanup();
#else
			// 关闭套接字closesocket
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
	}
	// 处理网络消息
	bool OnRun() {
		if (isRun()) {
			time4msg();
			// 伯克利套接字 BSD socket
			fd_set fdRead; // 描述符集合

			// 清理集合
			FD_ZERO(&fdRead);

			// 将描述符(socket)加入集合
			FD_SET(_sock, &fdRead);

			// nfds是一个整数，是指fd_set集合中所有描述符（socket）的范围，而不是数量
			// 既是所有描述符最大值加1，在windows上这个指可以写0
			timeval t = { 0,10};
			int ret = select(_sock + 1, &fdRead, 0, 0, &t); //
			if (ret < 0)
			{
				printf("Accept Select任务结束。\n");
				Close();
				return false;
			}
			// 判断描述符(socket)是否在集合中
			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				Accept();
				return true;
			}
			return true;
		}
		return false;
	}
	
		// 是否工作中
	bool isRun() {
		return INVALID_SOCKET != _sock;
	}
	// 相应网络消息
	void time4msg() {
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0) {
			printf("thread<%d>time<%lf>,socket<%d>,clientCount<%d>,recvCount<%d>,msg<%d> \n",_cellServers.size(), t1, _sock, (int)_clientCount, (int)(_recvCount/ t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}

	// 只会被一个线程调用 安全
	virtual void OnNetJoin(ClientSocket* pClient) {
		_clientCount++;
	}
	// cellServer 4 线程不安全
	// 单线程为安全
	virtual void OnNetLeave(ClientSocket* pClient) {
		_clientCount--;
	}
	// cellServer 4 多个线程触发，不安全
	// 单线程为安全
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) {
		_recvCount++;
	}
};
#endif
//class MyServer : public EasyTcpServer {
//	// 只会被一个线程调用 安全
//	virtual void OnNetJoin(ClientSocket* pClient) {
//		_clientCount++;
//		//printf("socket<=%d> join \n", pClient->sockfd());
//	}
//	// cellServer 4 线程不安全
//	// 单线程为安全
//	virtual void OnNetLeave(ClientSocket* pClient) {
//		_clientCount--;
//		//printf("socket<=%d> leave \n", pClient->sockfd());
//	}
//	// cellServer 4 多个线程触发，不安全
//	// 单线程为安全
//	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) {
//		_recvCount++;
//		// 6.处理请求并发送数据
//		switch (header->cmd)
//		{
//		case CMD_LOGIN: {
//			Login* login = (Login*)header;
//			//printf("登陆命令： CMD_LOGIN 包体长度：%d;登陆用户名称： %s, 用户密码： %s \n", header->dataLength, login->userName, login->password);
//			LoginResult logRet;
//			pClient->SendData(&logRet);
//		}
//						break;
//		case CMD_LOGOUT: {
//
//			Logout* logout = (Logout*)header;
//			//printf("登出命令： CMD_LOGOUT 包体长度：%d;登出用户名称： %s\n", header->dataLength, logout->userName);
//			//LogoutResult logoutRet;
//			//SendData(csock, &logoutRet);
//		}
//						 break;
//		default: {
//			//printf("<socket=%d>收到未定义消息 包体长度：%d\n", (int)csock, header->dataLength);
//			DataHeader dateHeader;
//			//SendData(csock, &dateHeader);
//		}
//		}
//	}
//};