#pragma once
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
#include<vector>
#include<map>
#include<thread>
#include<mutex>
#include<atomic>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
// ��������С��Ԫ��С
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE
#define _CellServer_THREAD_COUNT 4

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
	// ����ָ���û�
	int SendData(DataHeader* header) {
		if (header) {
			return send(_sockfd, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
private:
	// socket fd_set file desc set
	SOCKET _sockfd;
	// �ڶ������� ��Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	// ��Ϣ������β��λ��
	int _lastPos;
};

// �����¼��ӿ�
class INetEvent {
public:
	// ���麯��
	// �ͻ��˼����¼�
	virtual void OnJoin(ClientSocket* pClient) = 0;
	// �ͻ����뿪�¼�
	virtual void OnLeave(ClientSocket* pClient) = 0;
	// �ͻ�����Ϣ�¼�
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) = 0;
};

// 
class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET) {
		_sock = sock;
		_pINetEvent = nullptr;
	};
	~CellServer() {
		Close();
		_sock = INVALID_SOCKET;
	}
	// ���ûص�����
	void setEventObj(INetEvent* event) {
		_pINetEvent = event;
	}
	// ���ջ�����
	char _szRecv[RECV_BUFF_SIZE] = {};

	int RecvData(ClientSocket* pClient) {
		int nLen = (int)recv(pClient->sockfd(), (char*)&_szRecv, RECV_BUFF_SIZE, 0);

		if (nLen < 0) {
			//printf("�ͻ������˳��� ���������\n");
			return -1;
		}

		// ����ȡ�����ݿ�������Ϣ������
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		// ��Ϣ������������β��λ�ú���
		pClient->setLastPost(pClient->getLastPos() + nLen);
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
				pClient->setLastPost(nSize);
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
		_pINetEvent->OnNetMsg(pClient, header);
		//// 6.�������󲢷�������
		//switch (header->cmd)
		//{
		//case CMD_LOGIN: {
		//	Login* login = (Login*)header;
		//	//printf("��½��� CMD_LOGIN ���峤�ȣ�%d;��½�û����ƣ� %s, �û����룺 %s \n", header->dataLength, login->userName, login->password);
		//	//LoginResult logRet;
		//	//pClient->SendData(&logRet);
		//}
		//				break;
		//case CMD_LOGOUT: {

		//	Logout* logout = (Logout*)header;
		//	//printf("�ǳ���� CMD_LOGOUT ���峤�ȣ�%d;�ǳ��û����ƣ� %s\n", header->dataLength, logout->userName);
		//	//LogoutResult logoutRet;
		//	//SendData(csock, &logoutRet);
		//}
		//				 break;
		//default: {
		//	//printf("<socket=%d>�յ�δ������Ϣ ���峤�ȣ�%d\n", (int)csock, header->dataLength);
		//	DataHeader dateHeader;
		//	//SendData(csock, &dateHeader);
		//}
		//}
	}
	bool _clients_change;
	fd_set _fdRead_bak;
	SOCKET _maxSocket;
	// ����������Ϣ
	void OnRun() {
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
				std::chrono::microseconds t(1);
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
				_maxSocket = 0;
				for (auto iter : _clients)
				{
					FD_SET(iter.first, &fdRead);
					if (_maxSocket < iter.first) {
						_maxSocket = iter.first;
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			// nfds��һ����������ָfd_set������������������socket���ķ�Χ������������
			// �����������������ֵ��1����windows�����ָ����д0
			timeval t = { 0, 0 };
			int ret = (int)select(_maxSocket + 1, &fdRead, nullptr, nullptr, &t);
			//// ����ģʽ
			//int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, NULL);
			if (ret < 0) {
				printf("select ��������� \n");
				Close();
				return ;
			}
			else if (ret == 0) {
				continue;
			}
#ifdef _WIN32
			for (size_t i = 0; i < fdRead.fd_count; i++) {
				auto iter = _clients.find(fdRead.fd_array[i]);
				if (iter != _clients.end()) {
					if (-1 == RecvData(iter->second)) {
						_clients_change = true;
						if (_pINetEvent) {
							_pINetEvent->OnLeave(iter->second);
						}
						delete iter->second;
						_clients.erase(iter->first);
					}
				}
				else {
					printf("err find no client\n");
				}
			}
#else
			std::vector<ClientSocket*> temp;
			for (auto iter : _clients) {
				if (FD_ISSET(iter.first, &fdRead)) {
					if (-1 == RecvData(iter.second)) {
						_clients_change = false;
						if (_pINetEvent) {
							_pINetEvent->OnLeave(iter.second);
						}
						temp.push_back(iter.second);
					}
				}
			}
			for (auto iter : temp) {
				_clients.erase(iter->sockfd());
				delete iter;
			}
#endif // _WIN32
		}
	}
	// �Ƿ�����
	bool isRun() {
		return INVALID_SOCKET != _sock;
	}
	// �ر�����
	void Close() {
		if (INVALID_SOCKET != _sock) {
#ifdef _WIN32
			for (auto iter : _clients)
			{
				closesocket(iter.first);
				delete iter.second;
			}
#else
			for (auto iter : _clients)
			{
				close(iter.first);
				delete iter.second;
			}
#endif
			_sock = INVALID_SOCKET;
			_clients.clear();
		}
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
	INetEvent* _pINetEvent;
};

class EasyTcpServer : public INetEvent {
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
		_port = -1;
		_recvCount = 0;
		_ClientCount = 0;
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
		if (_sock == INVALID_SOCKET) {
			printf("���󣬽���socketʧ��\n");
		}
		else {
			printf("����socket����ɹ�\n");
		}
		return _sock;
	}
	// �󶨶˿ں�
	int Bind(const char* ip, unsigned short port) {
		if (INVALID_SOCKET == _sock) {
			InitSocket();
		}
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
			return -1;
		}
		else {
			printf("socket=<%d>�ɹ�����������˿�port=<%d>�ɹ�...\n", (int)_sock, _port);
		}
		return 0;
	}
	// ���ܿͻ�������
	SOCKET Accept() {
		// 4.�ȴ����ܿͻ�������
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET csock = INVALID_SOCKET;
#ifdef _WIN32
		csock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		csock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif

		if (INVALID_SOCKET == csock) {
			printf("socket=<%d>���󣬽��յ���Ч�ͻ���SOCKET...\n", (int)_sock);
		}
		else {
			// ��ȡip��ַ inet_ntoa(clientAddr.sin_addr)
			// ���¿ͻ��˷�����ͻ��������ٵ�cellServer
			addClientToCellServer(new ClientSocket(csock));
		}
		return csock;
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
		OnJoin(pClient);
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
			timeval t = {0, 0 };
			int ret = (int)select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			//// ����ģʽ
			//int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, NULL);
			if (ret < 0) {
				//printf("select ��������� \n");
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
	void Start(int tCount) {
		for (int i = 0; i < tCount; i++) {
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			// ע�������¼����ܶ���
			ser->setEventObj(this);
			ser->Start();
		}
	}
	// ��Ӧ������Ϣ
	virtual void time4msg() {
		auto tl = _tTime.getElapsedSecond();
		if (tl >= 1.0) {
			printf("thread<%d>time<%lf>,socket<%d>,clientCount<%d>,recvCount<%d> \n",_cellServers.size(), tl, _sock, (int)_ClientCount, (int)(_recvCount / tl));
			_recvCount = 0;
			_tTime.update();
		}
	}
	// �Ƿ�����
	bool isRun() {
		return INVALID_SOCKET != _sock;
	}
	// �ر�����
	void Close() {
		if (INVALID_SOCKET != _sock) {
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
	// ֻ�ᱻһ���̵߳��� ��ȫ
	virtual void OnJoin(ClientSocket* pClient) {
		_ClientCount++;
	}
	// cellServer 4 �̲߳���ȫ
	// ���߳�Ϊ��ȫ
	virtual void OnLeave(ClientSocket* pClient) {
		_ClientCount--;
	}
	// cellServer 4 ����̴߳���������ȫ
	// ���߳�Ϊ��ȫ
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) {
		_recvCount++;
	}
private:
	SOCKET _sock;
	int _port;
	// ��Ϣ��������ڲ������߳�
	std::vector<CellServer*> _cellServers;
	CELLTimestamp _tTime;
protected:
	// �յ���Ϣ����
	std::atomic_int _recvCount;
	// �ͻ��˼���
	std::atomic_int _ClientCount;
};

class MyServer : public EasyTcpServer {
	// ֻ�ᱻһ���̵߳��� ��ȫ
	virtual void OnJoin(ClientSocket* pClient) {
		_ClientCount++;
		//printf("socket<=%d> join \n", pClient->sockfd());
	}
	// cellServer 4 �̲߳���ȫ
	// ���߳�Ϊ��ȫ
	virtual void OnLeave(ClientSocket* pClient) {
		_ClientCount--;
		//printf("socket<=%d> leave \n", pClient->sockfd());
	}
	// cellServer 4 ����̴߳���������ȫ
	// ���߳�Ϊ��ȫ
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) {
		_recvCount++;
		// 6.�������󲢷�������
		switch (header->cmd)
		{
		case CMD_LOGIN: {
			Login* login = (Login*)header;
			//printf("��½��� CMD_LOGIN ���峤�ȣ�%d;��½�û����ƣ� %s, �û����룺 %s \n", header->dataLength, login->userName, login->password);
			LoginResult logRet;
			pClient->SendData(&logRet);
		}
						break;
		case CMD_LOGOUT: {

			Logout* logout = (Logout*)header;
			//printf("�ǳ���� CMD_LOGOUT ���峤�ȣ�%d;�ǳ��û����ƣ� %s\n", header->dataLength, logout->userName);
			//LogoutResult logoutRet;
			//SendData(csock, &logoutRet);
		}
						 break;
		default: {
			//printf("<socket=%d>�յ�δ������Ϣ ���峤�ȣ�%d\n", (int)csock, header->dataLength);
			DataHeader dateHeader;
			//SendData(csock, &dateHeader);
		}
		}
	}
};