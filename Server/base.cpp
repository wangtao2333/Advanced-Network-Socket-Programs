#include <process.h>
#include <stdio.h>
#include "base.h"
#include "server.h"

Client::Client(const SOCKET s, const sockaddr_in& addrress){	//���캯��
	Receive_Thread = NULL;
	Send_Thread = NULL;
	Socket = s;
	client_address = addrress;
	Connect_Status = FALSE;
	c_Send_Flag = FALSE;
	memset(client_data.buffer, 0, MAX_BUFFER_LENGTH);
	Event = CreateEvent(NULL, FALSE, FALSE, NULL);//��ʼ��Ϊ���ź�״̬
	InitializeCriticalSection(&c_mutex);
}

Client::~Client(){							//��������
	closesocket(Socket);					//�ر�socket
	Socket = INVALID_SOCKET;				//����socket
	DeleteCriticalSection(&c_mutex);
	CloseHandle(Event);						//�ͷ��¼�����
}

BOOL Client::Start(){													//�����߳�
	Connect_Status = TRUE;												//��������״̬
	unsigned long temp;
	Receive_Thread = CreateThread(NULL, 0, Receive, this, 0, &temp);	//���������߳�
	if (!Receive_Thread)	return FALSE;
	else	CloseHandle(Receive_Thread);
	Send_Thread = CreateThread(NULL, 0, Send, this, 0, &temp);			//���������߳�
	if (!Send_Thread)	return FALSE;
	else	CloseHandle(Send_Thread);
	return TRUE;
}

DWORD Client::Receive(void *Parameter){											//������Ϣ
	Client *ptr = (Client *)Parameter;
	char temp[MAX_BUFFER_LENGTH];
	while (ptr->Connect_Status){
		memset(temp, 0, MAX_BUFFER_LENGTH);
		int length = recv(ptr->Socket, temp, MAX_BUFFER_LENGTH, 0);				//������Ϣ
		if (length == 0)		break;
		else if (length == SOCKET_ERROR){										//������
			int Error = WSAGetLastError();
			if (Error == WSAEWOULDBLOCK)		continue;						//������������
			else if (Error == WSAETIMEDOUT || Error == WSAECONNRESET || Error == WSAENETDOWN)	break;							//�߳��˳�
		}else if (length > 0){													//�յ���Ϣ
			EnterCriticalSection(&ptr->c_mutex);
			char Client_IP[ADDRESS_LENGTH];
			inet_ntop(AF_INET, &ptr->client_address.sin_addr, Client_IP, sizeof(Client_IP));
			cout << "IP: " << Client_IP << "\tPort: " << ntohs(ptr->client_address.sin_port) << "������Ϣ" << endl;
			Process_Client(temp, ntohs(ptr->client_address.sin_port));			//������Client������Ϣ
			LeaveCriticalSection(&ptr->c_mutex);
			memset(temp, 0, MAX_BUFFER_LENGTH);
		}
	}
	ptr->Connect_Status = FALSE;												//��Client�Ͽ�����
	return 0;
}

DWORD Client::Send(void *Parameter){																//������Ϣ
	Client *ptr = (Client *)Parameter;																//ת����������ΪClientָ��
	while (ptr->Connect_Status){	
		if (Send_Flag || ptr->c_Send_Flag){
			EnterCriticalSection(&ptr->c_mutex);													//����
			if (send(ptr->Socket, Buffer, strlen(Buffer), 0) == SOCKET_ERROR){						//������Ϣ�����������
				int Error = WSAGetLastError();
				if (Error == WSAEWOULDBLOCK)	continue;											//������������
				else if (Error == WSAENETDOWN || Error == WSAECONNRESET ||Error == WSAETIMEDOUT){	//�ر�����
					LeaveCriticalSection(&ptr->c_mutex);
					ptr->c_Send_Flag = FALSE;
					ptr->Connect_Status = FALSE;													//�Ͽ�����
					break;
				}else {
					LeaveCriticalSection(&ptr->c_mutex);
					ptr->Connect_Status = FALSE;													//�Ͽ�����
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

