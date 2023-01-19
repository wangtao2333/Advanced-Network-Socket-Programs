#include <process.h>
#include <stdio.h>
#include "base.h"
#include "server.h"

Client::Client(const SOCKET s, const sockaddr_in& addrress){	//构造函数
	Receive_Thread = NULL;
	Send_Thread = NULL;
	Socket = s;
	client_address = addrress;
	Connect_Status = FALSE;
	c_Send_Flag = FALSE;
	memset(client_data.buffer, 0, MAX_BUFFER_LENGTH);
	Event = CreateEvent(NULL, FALSE, FALSE, NULL);//初始化为无信号状态
	InitializeCriticalSection(&c_mutex);
}

Client::~Client(){							//析构函数
	closesocket(Socket);					//关闭socket
	Socket = INVALID_SOCKET;				//作废socket
	DeleteCriticalSection(&c_mutex);
	CloseHandle(Event);						//释放事件对象
}

BOOL Client::Start(){													//创建线程
	Connect_Status = TRUE;												//设置连接状态
	unsigned long temp;
	Receive_Thread = CreateThread(NULL, 0, Receive, this, 0, &temp);	//创建接收线程
	if (!Receive_Thread)	return FALSE;
	else	CloseHandle(Receive_Thread);
	Send_Thread = CreateThread(NULL, 0, Send, this, 0, &temp);			//创建发送线程
	if (!Send_Thread)	return FALSE;
	else	CloseHandle(Send_Thread);
	return TRUE;
}

DWORD Client::Receive(void *Parameter){											//接收信息
	Client *ptr = (Client *)Parameter;
	char temp[MAX_BUFFER_LENGTH];
	while (ptr->Connect_Status){
		memset(temp, 0, MAX_BUFFER_LENGTH);
		int length = recv(ptr->Socket, temp, MAX_BUFFER_LENGTH, 0);				//接收信息
		if (length == 0)		break;
		else if (length == SOCKET_ERROR){										//错误处理
			int Error = WSAGetLastError();
			if (Error == WSAEWOULDBLOCK)		continue;						//继续尝试连接
			else if (Error == WSAETIMEDOUT || Error == WSAECONNRESET || Error == WSAENETDOWN)	break;							//线程退出
		}else if (length > 0){													//收到信息
			EnterCriticalSection(&ptr->c_mutex);
			char Client_IP[ADDRESS_LENGTH];
			inet_ntop(AF_INET, &ptr->client_address.sin_addr, Client_IP, sizeof(Client_IP));
			cout << "IP: " << Client_IP << "\tPort: " << ntohs(ptr->client_address.sin_port) << "发来消息" << endl;
			Process_Client(temp, ntohs(ptr->client_address.sin_port));			//向其他Client发送信息
			LeaveCriticalSection(&ptr->c_mutex);
			memset(temp, 0, MAX_BUFFER_LENGTH);
		}
	}
	ptr->Connect_Status = FALSE;												//与Client断开连接
	return 0;
}

DWORD Client::Send(void *Parameter){																//发送信息
	Client *ptr = (Client *)Parameter;																//转换数据类型为Client指针
	while (ptr->Connect_Status){	
		if (Send_Flag || ptr->c_Send_Flag){
			EnterCriticalSection(&ptr->c_mutex);													//加锁
			if (send(ptr->Socket, Buffer, strlen(Buffer), 0) == SOCKET_ERROR){						//发送信息，并处理错误
				int Error = WSAGetLastError();
				if (Error == WSAEWOULDBLOCK)	continue;											//缓冲区不可用
				else if (Error == WSAENETDOWN || Error == WSAECONNRESET ||Error == WSAETIMEDOUT){	//关闭连接
					LeaveCriticalSection(&ptr->c_mutex);
					ptr->c_Send_Flag = FALSE;
					ptr->Connect_Status = FALSE;													//断开连接
					break;
				}else {
					LeaveCriticalSection(&ptr->c_mutex);
					ptr->Connect_Status = FALSE;													//断开连接
					ptr->c_Send_Flag = FALSE;	break;
				}
			}
			memset(Buffer, 0, MAX_BUFFER_LENGTH);
			LeaveCriticalSection(&ptr->c_mutex);
			Send_Flag = FALSE;
			ptr->c_Send_Flag = FALSE;			
		}
	}
	return 0;
}

