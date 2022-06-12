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

// ��������С��Ԫ��С
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
	// �ڶ������� ��Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	// ��Ϣ������β��λ��
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
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
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
			printf("socket=<%d>�¿ͻ��˼��룺 socket=<%d>,IP=<%s> \n", (int)_sock, (int)csock, inet_ntoa(clientAddr.sin_addr));
			NewUserJoin newUserJoin;
			SendDataToAll(&newUserJoin);
			_clients.push_back(new ClientSocket(csock));
		}
		return csock;
	}
	// ����������Ϣ
	bool OnRun() {
		if (isRun()) {
			// �������׽��� BSD socket
			fd_set fdRead; // ����������
			fd_set fdWrite;
			fd_set fdExp;

			// ������
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);

			// ��������(socket)���뼯��
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
			// nfds��һ����������ָfd_set������������������socket���ķ�Χ������������
			// �����������������ֵ��1����windows�����ָ����д0
			timeval t = { 1, 0 };
			int ret = (int)select(maxSocket + 1, &fdRead, &fdWrite, &fdExp, &t);
			//// ����ģʽ
			//int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, NULL);
			if (ret < 0) {
				printf("select ��������� \n");
				Close();
				return false;
			}
			// �ж�������(socket)�Ƿ��ڼ�����
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
	// ���ջ�����
	char _szRecv[RECV_BUFF_SIZE] = {};

	int RecvData(ClientSocket* pClient) {
		int nLen = (int)recv(pClient->sockfd(), (char*)&_szRecv, RECV_BUFF_SIZE, 0);

		if (nLen < 0) {
			printf("�ͻ������˳��� ���������\n");
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
				OnNetMsg(pClient->sockfd(), header);
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
	virtual void OnNetMsg(SOCKET csock, DataHeader* header) {
		// 6.�������󲢷�������
		switch (header->cmd)
		{
		case CMD_LOGIN: {
			Login* login = (Login*)header;
			//printf("��½��� CMD_LOGIN ���峤�ȣ�%d;��½�û����ƣ� %s, �û����룺 %s \n", header->dataLength, login->userName, login->password);
			LoginResult logRet;
			SendData(csock, &logRet);
		}
						break;
		case CMD_LOGOUT: {

			Logout* logout = (Logout*)header;
			//printf("�ǳ���� CMD_LOGOUT ���峤�ȣ�%d;�ǳ��û����ƣ� %s\n", header->dataLength, logout->userName);
			LogoutResult logoutRet;
			SendData(csock, &logoutRet);
		}
						 break;
		default: {
			printf("<socket=%d>�յ�δ������Ϣ ���峤�ȣ�%d\n", (int)csock, header->dataLength);
			DataHeader dateHeader;
			SendData(csock, &dateHeader);
		}
		}
	}
	// ����ָ���û�
	int SendData(SOCKET csock, DataHeader* header) {
		if (isRun() && header) {
			return send(csock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
	// ����ָ���û�
	int SendDataToAll(DataHeader* header) {
		for (int i = (int)_clients.size() - 1; i >= 0; i--)
		{
			SendData(_clients[i]->sockfd(), header);
		}
		return SOCKET_ERROR;
	}
	// �Ƿ�����
	bool isRun() {
		return INVALID_SOCKET != _sock;
	}
	// �ر�����
	void Close() {
		if (INVALID_SOCKET != _sock) {
#ifdef _WIN32
			for (size_t i = _clients.size() - 1; i >= 0; i--)
			{
				closesocket(_clients[i]->sockfd());
				delete _clients[i];
			}
			// 6.�ر��׽���socket
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