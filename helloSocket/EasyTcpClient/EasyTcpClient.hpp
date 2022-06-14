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
	// ��ʼ��socket
	void initSocket() {
		// ����Win, Sock 2.x����
#ifdef _WIN32
		// ����windows socket 2.x����
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		// ��socket API��������TCP�ͻ���
		// 1.����һ��socket
		if (INVALID_SOCKET != _sock) {
			printf("�رվ�����<socket=%d> \n", (int)_sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (_sock == INVALID_SOCKET) {
			printf("���󣬽���socketʧ��\n");
		}
		else {
			//printf("����socket����ɹ�\n");
		}
	}
	// ���ӷ�����
	int Connect(const char* ip, unsigned short port) {
		if (INVALID_SOCKET == _sock) {
			initSocket();
		}
		// 2.���ӷ����� connect
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
			printf("�������ӷ�����ʧ��...\n");
		}
		else {
			//printf("���ӷ������ɹ�%d \n", ret);
		}
		return ret;
	}
	// �ر�socket
	void Close() {
		if (INVALID_SOCKET != _sock) {
#ifdef _WIN32
			// 4.�ر��׽���socket
			closesocket(_sock);
			WSACleanup();
#else
			close(_sock);
#endif
		}
		_sock = INVALID_SOCKET;
	}
	// ����������Ϣ
	bool onRun() {
		if (isRun()) {
			fd_set fdRead;
			FD_ZERO(&fdRead);
			FD_SET(_sock, &fdRead);

			timeval t = { 1, 0 };
			int ret = (int)select(_sock + 1, &fdRead, 0, 0, &t);
			if (ret < 0) {
				printf("<socket=%d>select�������\n", (int)_sock);
				Close();
				return false;
			}

			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				if (SOCKET_ERROR == RecvData()) {
					printf("<socket=%d>select�������2\n", (int)_sock);
					Close();
					return false;
				}
			}
			return true;
		}
		return false;
	}
	// �Ƿ�����
	bool isRun() {
		return INVALID_SOCKET != _sock;
	}
	// ��������С��Ԫ��С
#define RECV_BUFF_SIZE 10240
	// ���ջ�����
	char _szRecv[RECV_BUFF_SIZE] = {};
	// �ڶ������� ��Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE * 10] = {};
	// ��Ϣ������β��λ��
	int _lastPos = 0;
	// ��������
	int RecvData() {
		
		// 5.���շ��������
		int nLen = (int)recv(_sock, (char*)&_szRecv, RECV_BUFF_SIZE, 0);
		if (nLen < 0) {
			printf("<socket=%d>��������˿����ӣ� �������\n", (int)_sock);
			return -1;
		}
		// ����ȡ�����ݿ�������Ϣ������
		memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);
		// ��Ϣ������������β��λ�ú���
		_lastPos += nLen;
		// �ж���Ϣ�����������ݳ��ȴ�����ϢͷDataHeader����
		while (_lastPos >= sizeof(DataHeader)) {
			// ��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
			DataHeader* header = (DataHeader*)_szMsgBuf;
			// �ж���Ϣ�����������ݳ��ȴ�����Ϣ����
			if (_lastPos >= header->dataLength) {
				// ��Ϣ������ʣ��δ�������ݵĳ���
				int nSize = _lastPos - header->dataLength;
				// ����������Ϣ
				OnNetMsg(header);
				// ����Ϣ������ʣ��δ���������ǰ��
				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
				_lastPos = nSize;
			}
			else {
				// ��Ϣ������ʣ�����ݲ���һ��������Ϣ
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
			//printf("��½��� CMD_LOGIN_RESULT ���峤�ȣ�%d;��½����� %d \n", header->dataLength, loginRet->result);
		}
							   break;
		case CMD_LOGOUT_RESULT: {

			LogoutResult* logoutRet = (LogoutResult*)header;
			//printf("�ǳ���� CMD_LOGOUT_RESULT ���峤�ȣ�%d;��½����� %d \n", header->dataLength, logoutRet->result);
		}
								break;
		case CMD_NEW_USER_JOIN: {
			NewUserJoin* newUserJoin = (NewUserJoin*)header;
			printf("���û�������� CMD_NEW_USER_JOIN ���峤�ȣ�%d;��½����� %d \n", header->dataLength, newUserJoin->result);
		}
								break;
		case CMD_ERROR: {
			printf("<socket=%d>�յ��������Ϣ�� CMD_ERROR ���峤�ȣ�%d\n", (int)_sock, header->dataLength);
		}
						break;
		default: {
			printf("<socket=%d>�յ�δ������Ϣ ���峤�ȣ�%d\n", (int)_sock, header->dataLength);
		}
				 break;
		}
	}
	// ��������
	int SendData(DataHeader* header) {
		if (isRun() && header) {
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
private:
};
#endif // _EasyTcpClient_hpp_
