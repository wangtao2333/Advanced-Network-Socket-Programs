#define _CRT_SECURE_NO_WARNINGS
#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#define	MAX_NUM_CLIENT 5		//���Client��������
#define	MAX_BUFFER_LENGTH 2048	//����������󳤶�
#define ADDRESS_LENGTH 16		//IPV4��ַ����
using namespace std;

typedef struct DATA{
	char buffer[MAX_BUFFER_LENGTH];
}DATA_BUFFER;

class Client{
public:
	Client(const SOCKET s, const sockaddr_in &address);
	virtual ~Client();
	BOOL Start();
	BOOL Is_Connect() {	return Connect_Status;	}			//ȡ����״̬
	void Disconnect() {	Connect_Status = FALSE;	}			//�Ͽ���Client������	
	void Is_Send() {	c_Send_Flag = TRUE;	}
	u_short GetPort() {	return this->client_address.sin_port;	}
	SOCKET GetSocket() {	return this->Socket;	}
	static DWORD __stdcall	 Receive(void* Parameter);		//����Client����Ϣ
	static DWORD __stdcall	 Send(void* Parameter);			//��Client������Ϣ

private:
	Client();
	SOCKET Socket;					//�׽���
	BOOL Connect_Status;			//Client����״̬
	BOOL c_Send_Flag;				//����״̬
	sockaddr_in	client_address;		//Client��IP��ַ	
	HANDLE Event;					//�¼����
	HANDLE Receive_Thread;			//�����߳̾��
	HANDLE Send_Thread;				//�����߳̾��	
	CRITICAL_SECTION c_mutex;		//��
	DATA_BUFFER client_data;		//��Ϣ����
};
#endif
