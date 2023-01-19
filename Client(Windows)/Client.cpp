#define _CRT_SECURE_NO_WARNINGS
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
using namespace std;

//����
SOCKET	Socket;							//�׽���
HANDLE	Send_Thread;						//���������߳�
HANDLE	Receive_Thread;						//���������߳�
char    Send_Buffer[MAX_BUF_LENGTH];				//�������ݻ�����
BOOL    Send_Flag = FALSE;                      //���ͱ��λ
BOOL	conn_status;						//�������������״̬
HANDLE	Threads[2];						//���߳�����
CRITICAL_SECTION mutex;					//�ٽ�����������Send_Buffer
string sfile;
char SERVER_IP[15];
/**
 *	��ʼ��
 */
BOOL InitClient(void){
	InitializeCriticalSection(&mutex);
	Socket = INVALID_SOCKET;	//�׽���
	conn_status = FALSE;		//����״̬
	Receive_Thread = NULL;			//���������߳̾��
	Send_Thread = NULL;			//���������߳̾��

	//��ʼ��������
	memset(Send_Buffer, 0, MAX_BUF_LENGTH);
	memset(Threads, 0, 2);
	if (!InitSocket())	return FALSE;
	return TRUE;
}

/**
 * �����������׽���
 */
BOOL  InitSocket(void){
	WSADATA		ws;	//WSADATA����
	int flag = WSAStartup(MAKEWORD(2, 2), &ws);//��ʼ��Windows Socket
	Socket = socket(AF_INET, SOCK_STREAM, 0);	//�����׽���
	if (Socket == INVALID_SOCKET)	return FALSE;
	unsigned long a = 1;
	flag = ioctlsocket(Socket, FIONBIO, (unsigned long*)&a);	//�����׽��ַ�����ģʽ
	if (flag == SOCKET_ERROR)	return FALSE;
	return TRUE;
}

/**
 * ���ӷ�����
 */
BOOL Connect_Server(void){
	sockaddr_in Addr;//��������ַ
	//����Ҫ���ӵ�������ַ
	Addr.sin_family = AF_INET;
	Addr.sin_port = htons(SERVERPORT);
	cout << "Please input Server's IP address: ";
	cin.getline(SERVER_IP, 15);
	inet_pton(AF_INET, SERVER_IP, &Addr.sin_addr.S_un.S_addr);
	while (true){
		//���ӷ�����
		int flag = connect(Socket, (struct sockaddr*)&Addr, sizeof(Addr));
		if (flag == SOCKET_ERROR){
			int Error = WSAGetLastError();
			if (Error == WSAEWOULDBLOCK || Error == WSAEINVAL)	continue;    //���ӻ�û�����
			else if (Error == WSAEISCONN)	break;//�����Ѿ����
			else	return FALSE;//����ԭ������ʧ��
		}
		if (!flag)	break;//���ӳɹ�
	}
	conn_status = TRUE;
	return TRUE;
}
/**
 * �������ͺͽ��������߳�
 */
BOOL Create_Threads(void){
	unsigned long temp;
	Receive_Thread = CreateThread(NULL, 0, Receive, NULL, 0, &temp);
	if (!Receive_Thread)	return FALSE;
	Send_Thread = CreateThread(NULL, 0, Send, NULL, 0, &temp);
	if (!Send_Thread)	return FALSE;
	Threads[0] = Receive_Thread;
	Threads[1] = Send_Thread;
	return TRUE;
}
/**
 * ���������߳�
 */
DWORD __stdcall	Receive(void* Param){
	char    Receive_Buffer[MAX_BUF_LENGTH + 10];   //�������ݻ�����
	while (conn_status){			    //����״̬
		memset(Receive_Buffer, 0, MAX_BUF_LENGTH);
		int Length = recv(Socket, Receive_Buffer, MAX_BUF_LENGTH, 0);//��������
		if (SOCKET_ERROR == Length){
			if (WSAGetLastError() == WSAEWOULDBLOCK)			//�������ݻ�����������
				continue;
			else {
				conn_status = FALSE;
				return 0;							//�߳��˳�
			}
		}
		if (Length == 0)							//�������ر�������
		{
			Send_Flag = FALSE;
			conn_status = FALSE;
			Connect_Result(FALSE);
			memset(Receive_Buffer, 0, MAX_BUF_LENGTH);		//��ս��ջ�����
			Exit();
			return 0;								//�߳��˳�
		}
		if (Length > 0)
		{
			EnterCriticalSection(&mutex);
			Process(Receive_Buffer, Length);
			LeaveCriticalSection(&mutex);
			memset(Receive_Buffer, 0, MAX_BUF_LENGTH);
		}
	}
	return 0;
}
void Process(char* str, int length) {
	char    bufFile[MAX_BUF_LENGTH];
	if ((str[0] == 'E' || str[0] == 'e') && strlen(str) == 1){     //�ж��Ƿ��˳�
		Connect_Result(FALSE);
		Send_Flag = FALSE;
		conn_status = FALSE;
		memset(str, 0, MAX_BUF_LENGTH);		//��ս��ջ�����
		Exit();
	}else if (!strncmp(RECVFILE, str, strlen(RECVFILE))) {
		const char* filename = sfile.c_str();
		EnterCriticalSection(&mutex);
		cout << "!";
		str += strlen(RECVFILE);
		++str;
		strcpy(bufFile, str);
		length -= (strlen(RECVFILE) + 1);
		FILE* fp = fopen(filename, "ab");  //�Զ����Ʒ�ʽ�򿪣��������ļ�
		if (!fp) {
			puts("Can't open the file!");
			system("pause");
		}
		//ѭ���������ݣ�ֱ���ļ��������
		fwrite(bufFile, length, 1, fp);
		puts("The file transfered successfully!");
		//�ļ�������Ϻ�ֱ�ӹر��׽��֣��������shutdown()
		fclose(fp);
		LeaveCriticalSection(&mutex);
	}else	cout << "You've received: " << str << endl;		//��ʾ����
}

/**
 * ���������߳�
 */
DWORD __stdcall	Send(void* Param){
	while (conn_status){						//����״̬
		if (Send_Flag){						//��������
			EnterCriticalSection(&mutex);	//�����ٽ���
			while (TRUE){
				int error = send(Socket, Send_Buffer, strlen(Send_Buffer), 0);
				//�����ش���
				if (error == SOCKET_ERROR){
					if (WSAGetLastError() == WSAEWOULDBLOCK)		//���ͻ�����������
						continue;
					else{
						LeaveCriticalSection(&mutex);	//�뿪�ٽ���
						return 0;
					}
				}
				Send_Flag = FALSE;			//����״̬
				break;					//����for
			}
			LeaveCriticalSection(&mutex);	//�뿪�ٽ���
		}
	}
	return 0;
}
/**
 * �������ݺ���ʾ���
 */
void Preprocess(){
	char Input_Buffer[MAX_BUF_LENGTH];	//�û����뻺����
	while (conn_status){			//����״̬
		memset(Input_Buffer, 0, MAX_BUF_LENGTH);
		cin.getline(Input_Buffer, MAX_BUF_LENGTH);	//��������
		char* str = Input_Buffer;
		if (!strncmp(DOWNLOAD, str, strlen(DOWNLOAD))) {
			EnterCriticalSection(&mutex);
			memcpy(Send_Buffer, Input_Buffer, strlen(Input_Buffer));
			LeaveCriticalSection(&mutex);
			Send_Flag = TRUE;
			str += strlen(DOWNLOAD);
			++str;
			string temp(str);//�ϴ����ļ���
			sfile = "./" + temp;
		}else if (!strncmp(UPLOAD, str, strlen(UPLOAD))) {
			EnterCriticalSection(&mutex);
			memcpy(Send_Buffer, Input_Buffer, strlen(Input_Buffer));
			LeaveCriticalSection(&mutex);
			Send_Flag = TRUE;
			str += strlen(UPLOAD);
			++str;
			string a(str);//�ϴ����ļ���
			string b = "./" + a;
			const char* filename = b.c_str();
			sendFile(filename);
		}else {
			EnterCriticalSection(&mutex);		//�����ٽ���
			memcpy(Send_Buffer, Input_Buffer, strlen(Input_Buffer));
			LeaveCriticalSection(&mutex);		//�뿪�ٽ���
			Send_Flag = TRUE;
		}
	}
}
void sendFile(const char* filename) {
	char buffer[MAX_BUF_LENGTH];  //�ļ�������
	int nCount;

	FILE* fp = fopen(filename, "rb");  //�Զ����Ʒ�ʽ���ļ�
	if (!fp) {
		puts("Can't open file!");
		return;
	}
	cout << "Transfer File: " << filename << endl;
	while ((nCount = fread(buffer, 1, MAX_BUF_LENGTH, fp)) > 0) {
		string temp(buffer);
		string c = "$-" + temp;
		const char* command = c.c_str();
		memset(Send_Buffer, 0, MAX_BUF_LENGTH);
		memcpy(Send_Buffer, command, nCount + 2);
		Send_Flag = TRUE;
	}
	recv(Socket, buffer, MAX_BUF_LENGTH, 0);  //�������ȴ��ͻ��˽������
	fclose(fp);
}

/**
 * �ͻ����˳�
 */
void Exit(){
	DeleteCriticalSection(&mutex);
	CloseHandle(Receive_Thread);
	CloseHandle(Send_Thread);
	memset(Send_Buffer, 0, MAX_BUF_LENGTH);
	closesocket(Socket);
	WSACleanup();
}
/**
 * ��ʾ���ӷ�������Ϣ
 */
void Connect_Result(BOOL flag){
	if (flag)	cout << "## Succeed to connect server! ##" << endl;
	else	cout << "## Client has to exit! ##" << endl;
}
void Menus() {
	cout << "=====================================================" << endl;
	cout << "|      please connect clients,then send data        |" << endl;
	cout << "| send+num+message:Send message to specified client |" << endl;
	cout << "|      all+message:Send message to all clients      |" << endl;
	cout << "|    download+filename: download file from server   |" << endl;
	cout << "|       upload+filename: upload file to server      |" << endl;
	cout << "=====================================================" << endl;
	cout << "You can input:" << endl;
}
