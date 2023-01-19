#define _CRT_SECURE_NO_WARNINGS
#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#define	MAX_NUM_CLIENT 5		//最大Client连接数量
#define	MAX_BUFFER_LENGTH 2048	//缓冲区的最大长度
#define ADDRESS_LENGTH 16		//IPV4地址长度
using namespace std;

typedef struct DATA{
	char buffer[MAX_BUFFER_LENGTH];
}DATA_BUFFER;

class Client{
public:
	Client(const SOCKET s, const sockaddr_in &address);
	virtual ~Client();
	BOOL Start();
	BOOL Is_Connect() {	return Connect_Status;	}			//取连接状态
	void Disconnect() {	Connect_Status = FALSE;	}			//断开与Client的连接	
	void Is_Send() {	c_Send_Flag = TRUE;	}
	u_short GetPort() {	return this->client_address.sin_port;	}
	SOCKET GetSocket() {	return this->Socket;	}
	static DWORD __stdcall	 Receive(void* Parameter);		//接收Client的信息
	static DWORD __stdcall	 Send(void* Parameter);			//向Client发送信息

private:
	Client();
	SOCKET Socket;					//套接字
	BOOL Connect_Status;			//Client连接状态
	BOOL c_Send_Flag;				//发送状态
	sockaddr_in	client_address;		//Client的IP地址	
	HANDLE Event;					//事件句柄
	HANDLE Receive_Thread;			//接收线程句柄
	HANDLE Send_Thread;				//发送线程句柄	
	CRITICAL_SECTION c_mutex;		//锁
	DATA_BUFFER client_data;		//信息数据
};
#endif
