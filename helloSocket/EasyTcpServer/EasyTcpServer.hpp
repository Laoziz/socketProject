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
		printf("��ʼ��socket=<%d> %d\n", _sock, INVALID_SOCKET);
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
		SOCKET _csock = INVALID_SOCKET;
#ifdef _WIN32
		_csock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		_csock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif

		if (INVALID_SOCKET == _csock) {
			printf("socket=<%d>���󣬽��յ���Ч�ͻ���SOCKET...\n", (int)_sock);
		}
		else {
			printf("socket=<%d>�¿ͻ��˼��룺 socket=<%d>,IP=<%s> \n", (int)_sock, (int)_csock, inet_ntoa(clientAddr.sin_addr));
			NewUserJoin newUserJoin;
			SendDataToAll(&newUserJoin);
			g_clients.push_back(_csock);
		}
		return _csock;
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
			for (int i = (int)g_clients.size() - 1; i >= 0; i--)
			{
				FD_SET(g_clients[i], &fdRead);
				if (maxSocket < g_clients[i]) {
					maxSocket = g_clients[i];
				}
			}
			// nfds��һ����������ָfd_set������������������socket���ķ�Χ������������
			// �����������������ֵ��1����windows�����ָ����д0
			timeval t = { 1, 0 };
			int ret = select(maxSocket + 1, &fdRead, &fdWrite, &fdExp, &t);
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
		// 5.���տͻ�������
		char recvBuf[1024] = {};
		int nLen = (int)recv(_csock, (char*)&recvBuf, sizeof(DataHeader), 0);
		DataHeader * header = (DataHeader*)recvBuf;
		//printf("���տͻ������ %d ���ݳ��� %d\n", recvBuf.cmd, recvBuf.dataLength);
		if (nLen < 0) {
			printf("�ͻ������˳��� ���������\n");
			return -1;
		}
		recv(_csock, recvBuf + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(_csock, header);
		return 0;
	}
	virtual void OnNetMsg(SOCKET _csock, DataHeader* header) {
		// 6.�������󲢷�������
		switch (header->cmd)
		{
		case CMD_LOGIN: {
			Login* login = (Login*)header;
			printf("��½��� CMD_LOGIN ���峤�ȣ�%d;��½�û����ƣ� %s, �û����룺 %s \n", header->dataLength, login->userName, login->password);
			LoginResult logRet;
			SendData(_csock, &logRet);
		}
						break;
		case CMD_LOGOUT: {

			Logout* logout = (Logout*)header;
			printf("�ǳ���� CMD_LOGOUT ���峤�ȣ�%d;�ǳ��û����ƣ� %s\n", header->dataLength, logout->userName);
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
	// ����ָ���û�
	int SendData(SOCKET _csock, DataHeader* header) {
		if (isRun() && header) {
			return send(_csock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
	// ����ָ���û�
	int SendDataToAll(DataHeader* header) {
		for (int i = (int)g_clients.size() - 1; i >= 0; i--)
		{
			SendData(g_clients[i], header);
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
			for (size_t i = g_clients.size() - 1; i >= 0; i--)
			{
				closesocket(g_clients[i]);
			}
			// 6.�ر��׽���socket
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