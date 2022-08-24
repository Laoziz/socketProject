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
// ��������С��Ԫ��С
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

	//��������
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//Ҫ���͵����ݳ���
		int nSendLen = header->dataLength;
		//Ҫ���͵�����
		const char* pSendData = (const char*)header;

		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
			{
				//����ɿ��������ݳ���
				int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
				//��������
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
				//����ʣ������λ��
				pSendData += nCopyLen;
				//����ʣ�����ݳ���
				nSendLen -= nSendLen;
				//��������
				ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
				//����β��λ������
				_lastSendPos = 0;
				//���ʹ���
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
			}else {
				//��Ҫ���͵����� ���������ͻ�����β��
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
				//��������β��λ��
				_lastSendPos += nSendLen;
				break;
			}
		}
		return ret;
	}
private:
	// socket fd_set file desc set
	SOCKET _sockfd;
	// �ڶ������� ��Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE];
	// ��Ϣ������β��λ��
	int _lastPos;
	//�ڶ������� ���ͻ�����
	char _szSendBuf[SEND_BUFF_SIZE];
	//���ͻ�����������β��λ��
	int _lastSendPos;
};

// �����¼��ӿ�
class INetEvent {
public:
	// ���麯��
	// �ͻ��˼����¼�
	virtual void OnNetJoin(ClientSocket* pClient) = 0;
	// �ͻ����뿪�¼�
	virtual void OnNetLeave(ClientSocket* pClient) = 0;
	// �ͻ�����Ϣ�¼�
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) = 0;
	//recv�¼�
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
	// ���ûص�����
	void setEventObj(INetEvent* event) {
		_pNetEvent = event;
	}
	// �ر�����
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			for (auto iter : _clients)
			{
				closesocket(iter.second->sockfd());
				delete iter.second;
			}
			//�ر��׽���closesocket
			closesocket(_sock);
#else
			for (auto iter : _clients)
			{
				close(iter.second->sockfd());
				delete iter.second;
			}
			//�ر��׽���closesocket
			close(_sock);
#endif
			_clients.clear();
		}
	}
	
	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//����������Ϣ
	//���ݿͻ�socket fd_set
	fd_set _fdRead_bak;
	//�ͻ��б��Ƿ��б仯
	bool _clients_change;
	SOCKET _maxSock;
	// ����������Ϣ
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
			// ���û����Ҫ����Ŀͻ��ˣ�������
			if (_clients.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			// �������׽��� BSD socket
			fd_set fdRead; // ����������

			// ������
			FD_ZERO(&fdRead);

			// ��������(socket)���뼯��
			if (_clients_change) {
				_clients_change = false;
				//����������socket�����뼯��
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

			// nfds��һ����������ָfd_set������������������socket���ķ�Χ������������
			// �����������������ֵ��1����windows�����ָ����д0
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0) {
				printf("select ��������� \n");
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
			//printf("�ͻ������˳��� ���������\n", pClient->sockfd());
			return -1;
		}

		// ����ȡ�����ݿ�������Ϣ������
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		// ��Ϣ������������β��λ�ú���
		pClient->_setLastPos(pClient->getLastPos() + nLen);
		// �ж���Ϣ�����������ݳ��ȴ�����ϢͷDataHeader����
		while (pClient->getLastPos() >= sizeof(DataHeader)) {
			// ��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			// �ж���Ϣ�����������ݳ��ȴ�����Ϣ����
			if (pClient->getLastPos() >= header->dataLength) {
				// ��Ϣ������ʣ��δ�������ݵĳ���
				int nSize = pClient->getLastPos() - header->dataLength;
				// ����������Ϣ
				OnNetMsg(pClient, header);
				// ����Ϣ������ʣ��δ���������ǰ��
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				pClient->_setLastPos(nSize);
			}
			else {
				// ��Ϣ������ʣ�����ݲ���һ��������Ϣ
				break;
			}
		}
		return 0;
	}
	// ��Ӧ������Ϣ
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) {
		_pNetEvent->OnNetMsg(pClient, header);
	}
	// ��ӿͻ���
	void addClient(ClientSocket* pClient) {
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	// ����
	void Start() {
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
	}
	// ��ȡ�ͻ�������
	size_t getClientCount() {
		return _clients.size() + _clientsBuff.size();
	}
private:
	SOCKET _sock;
	std::map<SOCKET, ClientSocket*> _clients;
	std::vector<ClientSocket*> _clientsBuff;
	// ���������
	std::mutex _mutex;
	std::thread _thread;
	// �����¼�����
	INetEvent* _pNetEvent;
};

class EasyTcpServer : public INetEvent {
private:
	SOCKET _sock;
	int _port;
	// ��Ϣ��������ڲ��ᴴ���߳�
	std::vector<CellServer*> _cellServers;
	//ÿ����Ϣ��ʱ
	CELLTimestamp _tTime;
protected:
	// �յ���Ϣ����
	std::atomic_int _recvCount;
	std::atomic_int _msgCount;
	// �ͻ��˼���
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
	// ��ʼ��socket
	SOCKET InitSocket() {
#ifdef _WIN32
		// ����windows socket 2.x����
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		// 1.����һ��socket
		if (INVALID_SOCKET != _sock) {
			printf("�رվ�����socket=<%d> \n", (int)_sock);
			Close();
		}
		// ----------------------
		// 1. ����һ��socket �׽���
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			printf("���󣬽���socketʧ��\n");
		}
		else {
			printf("����socket=<%d>�ɹ�...\n", (int)_sock);
		}
		return _sock;
	}
	// �󶨶˿ں�
	int Bind(const char* ip, unsigned short port) {
		//if (INVALID_SOCKET == _sock) {
		//	InitSocket();
		//}
		// 2. bind �����ڽ��ܿͻ������ӵ�����˿�
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
			printf("����socket=<%d>���󣬰�����˿�port=<%d>ʧ��...\n", (int)_sock, port);
			return 0;
		}
		else {
			printf("����socket=<%d>�ɹ���������˿�port=<%d>�ɹ�...\n", (int)_sock, port);
		}
		_port = port;
		return ret;
	}
	// �����˿ں�
	int Listen(int n) {
		// 3. listen ��������˿�
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			printf("socket=<%d>���󣬼�������˿�port=<%d>ʧ��...\n", (int)_sock, _port);
		}
		else {
			printf("socket=<%d>�ɹ�����������˿�port=<%d>�ɹ�...\n", (int)_sock, _port);
		}
		return ret;
	}
	// ���ܿͻ�������
	SOCKET Accept() {
		// 4.�ȴ����ܿͻ�������
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif

		if (INVALID_SOCKET == cSock) {
			printf("socket=<%d>���󣬽��յ���Ч�ͻ���SOCKET...\n", (int)_sock);
		}
		else {
			// ��ȡip��ַ inet_ntoa(clientAddr.sin_addr)
			// ���¿ͻ��˷�����ͻ��������ٵ�cellServer
			addClientToCellServer(new ClientSocket(cSock));
		}
		return cSock;
	}
	void addClientToCellServer(ClientSocket* pClient) {
		// ���ҿͻ��������ٵ�CellServer��Ϣ�������
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
			// ע�������¼����ܶ���
			ser->setEventObj(this);
			ser->Start();
		}
	}
	// �ر�����
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			// �ر��׽���socket
			closesocket(_sock);
			// ���windows socket����
			WSACleanup();
#else
			// �ر��׽���closesocket
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
	}
	// ����������Ϣ
	bool OnRun() {
		if (isRun()) {
			time4msg();
			// �������׽��� BSD socket
			fd_set fdRead; // ����������

			// ������
			FD_ZERO(&fdRead);

			// ��������(socket)���뼯��
			FD_SET(_sock, &fdRead);

			// nfds��һ����������ָfd_set������������������socket���ķ�Χ������������
			// �����������������ֵ��1����windows�����ָ����д0
			timeval t = { 0,10};
			int ret = select(_sock + 1, &fdRead, 0, 0, &t); //
			if (ret < 0)
			{
				printf("Accept Select���������\n");
				Close();
				return false;
			}
			// �ж�������(socket)�Ƿ��ڼ�����
			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				Accept();
				return true;
			}
			return true;
		}
		return false;
	}
	
		// �Ƿ�����
	bool isRun() {
		return INVALID_SOCKET != _sock;
	}
	// ��Ӧ������Ϣ
	void time4msg() {
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0) {
			printf("thread<%d>time<%lf>,socket<%d>,clientCount<%d>,recvCount<%d>,msg<%d> \n",_cellServers.size(), t1, _sock, (int)_clientCount, (int)(_recvCount/ t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}

	// ֻ�ᱻһ���̵߳��� ��ȫ
	virtual void OnNetJoin(ClientSocket* pClient) {
		_clientCount++;
	}
	// cellServer 4 �̲߳���ȫ
	// ���߳�Ϊ��ȫ
	virtual void OnNetLeave(ClientSocket* pClient) {
		_clientCount--;
	}
	// cellServer 4 ����̴߳���������ȫ
	// ���߳�Ϊ��ȫ
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) {
		_recvCount++;
	}
};
#endif
//class MyServer : public EasyTcpServer {
//	// ֻ�ᱻһ���̵߳��� ��ȫ
//	virtual void OnNetJoin(ClientSocket* pClient) {
//		_clientCount++;
//		//printf("socket<=%d> join \n", pClient->sockfd());
//	}
//	// cellServer 4 �̲߳���ȫ
//	// ���߳�Ϊ��ȫ
//	virtual void OnNetLeave(ClientSocket* pClient) {
//		_clientCount--;
//		//printf("socket<=%d> leave \n", pClient->sockfd());
//	}
//	// cellServer 4 ����̴߳���������ȫ
//	// ���߳�Ϊ��ȫ
//	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) {
//		_recvCount++;
//		// 6.�������󲢷�������
//		switch (header->cmd)
//		{
//		case CMD_LOGIN: {
//			Login* login = (Login*)header;
//			//printf("��½��� CMD_LOGIN ���峤�ȣ�%d;��½�û����ƣ� %s, �û����룺 %s \n", header->dataLength, login->userName, login->password);
//			LoginResult logRet;
//			pClient->SendData(&logRet);
//		}
//						break;
//		case CMD_LOGOUT: {
//
//			Logout* logout = (Logout*)header;
//			//printf("�ǳ���� CMD_LOGOUT ���峤�ȣ�%d;�ǳ��û����ƣ� %s\n", header->dataLength, logout->userName);
//			//LogoutResult logoutRet;
//			//SendData(csock, &logoutRet);
//		}
//						 break;
//		default: {
//			//printf("<socket=%d>�յ�δ������Ϣ ���峤�ȣ�%d\n", (int)csock, header->dataLength);
//			DataHeader dateHeader;
//			//SendData(csock, &dateHeader);
//		}
//		}
//	}
//};