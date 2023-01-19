#define _CRT_SECURE_NO_WARNINGS
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
using namespace std;

//变量
SOCKET	Socket;							//套接字
HANDLE	Send_Thread;						//发送数据线程
HANDLE	Receive_Thread;						//接收数据线程
char    Send_Buffer[MAX_BUF_LENGTH];				//发送数据缓冲区
BOOL    Send_Flag = FALSE;                      //发送标记位
BOOL	conn_status;						//与服务器的连接状态
HANDLE	Threads[2];						//子线程数组
CRITICAL_SECTION mutex;					//临界区对象，锁定Send_Buffer
string sfile;
char SERVER_IP[15];
/**
 *	初始化
 */
BOOL InitClient(void){
	InitializeCriticalSection(&mutex);
	Socket = INVALID_SOCKET;	//套接字
	conn_status = FALSE;		//连接状态
	Receive_Thread = NULL;			//接收数据线程句柄
	Send_Thread = NULL;			//发送数据线程句柄

	//初始化缓冲区
	memset(Send_Buffer, 0, MAX_BUF_LENGTH);
	memset(Threads, 0, 2);
	if (!InitSocket())	return FALSE;
	return TRUE;
}

/**
 * 创建非阻塞套接字
 */
BOOL  InitSocket(void){
	WSADATA		ws;	//WSADATA变量
	int flag = WSAStartup(MAKEWORD(2, 2), &ws);//初始化Windows Socket
	Socket = socket(AF_INET, SOCK_STREAM, 0);	//创建套接字
	if (Socket == INVALID_SOCKET)	return FALSE;
	unsigned long a = 1;
	flag = ioctlsocket(Socket, FIONBIO, (unsigned long*)&a);	//设置套接字非阻塞模式
	if (flag == SOCKET_ERROR)	return FALSE;
	return TRUE;
}

/**
 * 连接服务器
 */
BOOL Connect_Server(void){
	sockaddr_in Addr;//服务器地址
	//输入要连接的主机地址
	Addr.sin_family = AF_INET;
	Addr.sin_port = htons(SERVERPORT);
	cout << "Please input Server's IP address: ";
	cin.getline(SERVER_IP, 15);
	inet_pton(AF_INET, SERVER_IP, &Addr.sin_addr.S_un.S_addr);
	while (true){
		//连接服务器
		int flag = connect(Socket, (struct sockaddr*)&Addr, sizeof(Addr));
		if (flag == SOCKET_ERROR){
			int Error = WSAGetLastError();
			if (Error == WSAEWOULDBLOCK || Error == WSAEINVAL)	continue;    //连接还没有完成
			else if (Error == WSAEISCONN)	break;//连接已经完成
			else	return FALSE;//其它原因，连接失败
		}
		if (!flag)	break;//连接成功
	}
	conn_status = TRUE;
	return TRUE;
}
/**
 * 创建发送和接收数据线程
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
 * 接收数据线程
 */
DWORD __stdcall	Receive(void* Param){
	char    Receive_Buffer[MAX_BUF_LENGTH + 10];   //接收数据缓冲区
	while (conn_status){			    //连接状态
		memset(Receive_Buffer, 0, MAX_BUF_LENGTH);
		int Length = recv(Socket, Receive_Buffer, MAX_BUF_LENGTH, 0);//接收数据
		if (SOCKET_ERROR == Length){
			if (WSAGetLastError() == WSAEWOULDBLOCK)			//接受数据缓冲区不可用
				continue;
			else {
				conn_status = FALSE;
				return 0;							//线程退出
			}
		}
		if (Length == 0)							//服务器关闭了连接
		{
			Send_Flag = FALSE;
			conn_status = FALSE;
			Connect_Result(FALSE);
			memset(Receive_Buffer, 0, MAX_BUF_LENGTH);		//清空接收缓冲区
			Exit();
			return 0;								//线程退出
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
	if ((str[0] == 'E' || str[0] == 'e') && strlen(str) == 1){     //判断是否退出
		Connect_Result(FALSE);
		Send_Flag = FALSE;
		conn_status = FALSE;
		memset(str, 0, MAX_BUF_LENGTH);		//清空接收缓冲区
		Exit();
	}else if (!strncmp(RECVFILE, str, strlen(RECVFILE))) {
		const char* filename = sfile.c_str();
		EnterCriticalSection(&mutex);
		cout << "!";
		str += strlen(RECVFILE);
		++str;
		strcpy(bufFile, str);
		length -= (strlen(RECVFILE) + 1);
		FILE* fp = fopen(filename, "ab");  //以二进制方式打开（创建）文件
		if (!fp) {
			puts("Can't open the file!");
			system("pause");
		}
		//循环接收数据，直到文件传输完毕
		fwrite(bufFile, length, 1, fp);
		puts("The file transfered successfully!");
		//文件接收完毕后直接关闭套接字，无需调用shutdown()
		fclose(fp);
		LeaveCriticalSection(&mutex);
	}else	cout << "You've received: " << str << endl;		//显示数据
}

/**
 * 发送数据线程
 */
DWORD __stdcall	Send(void* Param){
	while (conn_status){						//连接状态
		if (Send_Flag){						//发送数据
			EnterCriticalSection(&mutex);	//进入临界区
			while (TRUE){
				int error = send(Socket, Send_Buffer, strlen(Send_Buffer), 0);
				//处理返回错误
				if (error == SOCKET_ERROR){
					if (WSAGetLastError() == WSAEWOULDBLOCK)		//发送缓冲区不可用
						continue;
					else{
						LeaveCriticalSection(&mutex);	//离开临界区
						return 0;
					}
				}
				Send_Flag = FALSE;			//发送状态
				break;					//跳出for
			}
			LeaveCriticalSection(&mutex);	//离开临界区
		}
	}
	return 0;
}
/**
 * 输入数据和显示结果
 */
void Preprocess(){
	char Input_Buffer[MAX_BUF_LENGTH];	//用户输入缓冲区
	while (conn_status){			//连接状态
		memset(Input_Buffer, 0, MAX_BUF_LENGTH);
		cin.getline(Input_Buffer, MAX_BUF_LENGTH);	//输入数据
		char* str = Input_Buffer;
		if (!strncmp(DOWNLOAD, str, strlen(DOWNLOAD))) {
			EnterCriticalSection(&mutex);
			memcpy(Send_Buffer, Input_Buffer, strlen(Input_Buffer));
			LeaveCriticalSection(&mutex);
			Send_Flag = TRUE;
			str += strlen(DOWNLOAD);
			++str;
			string temp(str);//上传的文件名
			sfile = "./" + temp;
		}else if (!strncmp(UPLOAD, str, strlen(UPLOAD))) {
			EnterCriticalSection(&mutex);
			memcpy(Send_Buffer, Input_Buffer, strlen(Input_Buffer));
			LeaveCriticalSection(&mutex);
			Send_Flag = TRUE;
			str += strlen(UPLOAD);
			++str;
			string a(str);//上传的文件名
			string b = "./" + a;
			const char* filename = b.c_str();
			sendFile(filename);
		}else {
			EnterCriticalSection(&mutex);		//进入临界区
			memcpy(Send_Buffer, Input_Buffer, strlen(Input_Buffer));
			LeaveCriticalSection(&mutex);		//离开临界区
			Send_Flag = TRUE;
		}
	}
}
void sendFile(const char* filename) {
	char buffer[MAX_BUF_LENGTH];  //文件缓冲区
	int nCount;

	FILE* fp = fopen(filename, "rb");  //以二进制方式打开文件
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
	recv(Socket, buffer, MAX_BUF_LENGTH, 0);  //阻塞，等待客户端接收完毕
	fclose(fp);
}

/**
 * 客户端退出
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
 * 显示连接服务器信息
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
