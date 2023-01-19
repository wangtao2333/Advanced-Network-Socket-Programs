#include "server.h"
#include "base.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * 全局变量
 */
char Buffer[MAX_BUFFER_LENGTH];			//缓冲区
BOOL connect_status;					//与Client的连接状态
BOOL Send_Flag;
BOOL Connect_Flag;						//连接标记
SOCKET Sokcet;
CRITICAL_SECTION mutex;			        //锁
HANDLE Accept_Thread;					//接收线程句柄
HANDLE Clean_Thread;					//线程清理句柄
ClIENT_VECTOR client_vector;			//存client信息
string Pre_File = "";
BOOL InitSever(){
	InitializeCriticalSection(&mutex);
	memset(Buffer, 0, MAX_BUFFER_LENGTH);
	Sokcet = INVALID_SOCKET;
	Send_Flag = FALSE;
	Connect_Flag = FALSE;
	connect_status = FALSE;	
	Accept_Thread = NULL;
	Clean_Thread = NULL;
	client_vector.clear();
	if (!InitSocket())	return FALSE;
	return TRUE;
}

BOOL InitSocket(){										//初始化SOCKET
	WSADATA  ws;
	int flag = WSAStartup(MAKEWORD(2, 2), &ws);
	Sokcet = socket(AF_INET, SOCK_STREAM, 0);			//创建socket
	if (INVALID_SOCKET == Sokcet)	return FALSE;
	unsigned long temp = 1;
	if (ioctlsocket(Sokcet, FIONBIO, (unsigned long *)&temp) == SOCKET_ERROR)	return FALSE;
	sockaddr_in Address;								//设置socket
	Address.sin_family = AF_INET;	Address.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, "192.168.2.208", &Address.sin_addr.S_un.S_addr);
	if (bind(Sokcet, (struct sockaddr *)&Address, sizeof(Address)) == SOCKET_ERROR)	return FALSE;
	if (listen(Sokcet, CLIENT_NUM_UP) == SOCKET_ERROR)	return FALSE;
	return TRUE;
}

BOOL Start(){			//启动
	BOOL flag = TRUE;
	Menus(SERVER_START);
	char c;	
	while (true){
		cin >> c;
		if ((c == 'e' || c == 'E'))	return FALSE;
		else if ((c == 's' || c == 'S')) {
			if (Create_Thread())	Connect_Result(TRUE);		//成功创建线程
			else	flag = FALSE;
			break;
		}else{
			puts("Start Fail!");
			Menus(SERVER_START);
		}
	}
	cin.sync();                     //清空输入缓冲区
	return flag;
}

BOOL Create_Thread(){												//创建线程
	connect_status = TRUE;											//设置Server为运行状态
	unsigned long temp;
	Accept_Thread = CreateThread(NULL, 0, Accept, NULL, 0, &temp);	//创建接收Client请求线程
	if (!Accept_Thread){
		connect_status = FALSE;
		return FALSE;
	}else	CloseHandle(Accept_Thread);
	Clean_Thread = CreateThread(NULL, 0, Clean, NULL, 0, &temp);	//创建清理线程
	if (!Clean_Thread)	return FALSE;
	else	CloseHandle(Clean_Thread);
	return TRUE;
}

DWORD __stdcall Accept(void *Parameter){
	sockaddr_in Address;														//Client地址
	while (connect_status){														//连接状态
		int	length = sizeof(sockaddr_in);
		memset(&Address, 0, length);
		SOCKET Accept_Socket = accept(Sokcet, (sockaddr *)&Address, &length);	//和Client连接的套接字
		if (Accept_Socket == INVALID_SOCKET){
			if (WSAGetLastError() == WSAEWOULDBLOCK){
				Sleep(600);	continue;											//继续等待
			}else	return 0;
		}else{																	//接受Client的请求
			Connect_Flag = TRUE;
			Client *ptr = new Client(Accept_Socket, Address);
			EnterCriticalSection(&mutex);		
			char Client_IP[ADDRESS_LENGTH];
			inet_ntop(AF_INET, &Address.sin_addr, Client_IP, sizeof(Client_IP));
			cout << "Connect a client IP: " << Client_IP << "\tPort: " << ntohs(Address.sin_port) << endl;	//显示Client的IP和端口
			client_vector.push_back(ptr);										//添加Client
			LeaveCriticalSection(&mutex);
			ptr->Start();
		}
	}return 0;
}

/**
 * 清理资源线程
 */
DWORD __stdcall Clean(void *Parameter){
	while (connect_status){
		EnterCriticalSection(&mutex);							//加锁
		for (ClIENT_VECTOR::iterator iter = client_vector.begin(); iter != client_vector.end();){
			Client* pClient = (Client*)*iter;
			if (!pClient->Is_Connect()){						//Client已经退出
				client_vector.erase(iter);
				delete pClient;									//释放内存
				pClient = NULL;
			}else	iter++;										//指针后移
		}
		if (client_vector.size() == 0)	Connect_Flag = FALSE;
		LeaveCriticalSection(&mutex);
		Sleep(CLIENT_EXIT);										//给Client线程响应退出时间
	}
	if (!connect_status){										//连接断开
		EnterCriticalSection(&mutex);
		for (ClIENT_VECTOR::iterator iter = client_vector.begin(); iter != client_vector.end();){
			Client* pClient = (Client*)*iter;	
			if (pClient->Is_Connect())	pClient->Disconnect();	//断开连接，线程退出
			++iter;
		}
		LeaveCriticalSection(&mutex);							//解锁
		Sleep(CLIENT_EXIT);
	}
	client_vector.clear();
	Connect_Flag = FALSE;
	return 0;
}

/**
 * 处理数据
 */
void Input(){
	char Temp_Buffer[MAX_BUFFER_LENGTH];
	Menus(TIPS);
	while (connect_status){
		memset(Temp_Buffer, 0, MAX_BUFFER_LENGTH);		//清空缓冲区
		cin.getline(Temp_Buffer, MAX_BUFFER_LENGTH);	//输入
		Preprocess(Temp_Buffer);						//发送信息
	}
}

void Process(char *str, char &c, int &num){
	if (!strncmp(SEND, str, strlen(SEND))){
		EnterCriticalSection(&mutex);
		str += strlen(SEND);
		c = *++str;
		num = c - '1';
		if (num > -1 && num < client_vector.size()){
			Client* sClient = client_vector.at(num);			//发给指定Client
			str += 2;											//空格和数字处理
			strcpy(Buffer, str);
			sClient->Is_Send();
			cout << "向Client" << num + 1 << "发送了消息： " << str << endl;
			LeaveCriticalSection(&mutex);
		}
		else {
			cout << "不存在Client" << num + 1 << "!!!" << endl;
			LeaveCriticalSection(&mutex);
		}
	}
	else if (!strncmp(BROADCAST, str, strlen(BROADCAST))){
		EnterCriticalSection(&mutex);
		str += strlen(BROADCAST);
		strcpy(Buffer, ++str);
		Send_Flag = TRUE;
		cout << "给所有Client发送消息： " << str << endl;		//Server发给所有Client	
		LeaveCriticalSection(&mutex);
	}
}
void Process(char *str, char &c, int &num, int local) {
	if (!strncmp(str, SEND, strlen(SEND))) {								//send
		EnterCriticalSection(&mutex);
		str += strlen(SEND);
		c = *++str;
		num = c - '1';
		if (num < client_vector.size() && num > -1) {
			str += 2;														//空格和数字处理
			strcpy(Buffer, str);
			client_vector.at(num)->Is_Send();								//发给指定Client
			cout << "向Client" << num + 1 << "发送消息： " << str << endl;
			LeaveCriticalSection(&mutex);
		}else{
			cout << "没有该Client!!!" << endl;
			LeaveCriticalSection(&mutex);
		}
	}else if (!strncmp(str, BROADCAST, strlen(BROADCAST))){					//broadcast
		EnterCriticalSection(&mutex);
		str += strlen(BROADCAST);
		strcpy(Buffer, ++str);
		cout << "向所有Client发送了消息： " << str << endl;					//发给所有Client
		int num = 0;
		for (ClIENT_VECTOR::iterator iter = client_vector.begin(); iter != client_vector.end(); num++, iter++) {
			if (num == local)	continue;
			client_vector.at(num)->Is_Send();
		}
		LeaveCriticalSection(&mutex);
	}else if (!strncmp(str, PRE, strlen(PRE))){								//文件发送处理
		EnterCriticalSection(&mutex);
		str++;
		int length = strlen(str);
		FILE* fp = fopen(Pre_File.c_str(), "wb");							//二进制处理文件
		if (!fp)		puts("Cannot open file!");
		else {
			fwrite(str, length, 1, fp);
			fclose(fp);
			cout << "接收Client" << local + 1 << "发来的文件：" << Pre_File << endl;
			strcpy(Buffer, "Success!");
			client_vector.at(local)->Is_Send();
		}
		Pre_File = "";
		LeaveCriticalSection(&mutex);
	}else if (!strncmp(str, UPLOAD, strlen(UPLOAD))){						//upload
		EnterCriticalSection(&mutex);
		str += strlen(UPLOAD) + 1;
		string a(str);														//上传的文件名
		Pre_File = "./" + a;
		memset(Buffer, 0, MAX_BUFFER_LENGTH);
		LeaveCriticalSection(&mutex);
	}else if (!strncmp(str, DOWNLOAD, strlen(DOWNLOAD))){					//download
		EnterCriticalSection(&mutex);
		str += strlen(DOWNLOAD) + 1;
		string a(str);
		memset(Buffer, 0, MAX_BUFFER_LENGTH);
		if (Send_File(("./" + a).c_str(), local))		cout << "向Client" << local + 1 << "发送文件：" << a << endl;
		else	cout << "文件传输错误" << endl;
		LeaveCriticalSection(&mutex);	
	}else{
		cout << str << endl;
		LeaveCriticalSection(&mutex);
	}
}

void Preprocess(char *str){
	char c = NULL;
	int num = 0;
	if (str){
		Process(str, c, num);
		if (strlen(str) == 1 && (str[0] == 'e' || str[0] == 'E')){
			connect_status = FALSE;
			Exit();
		}
	}
}

void Process_Client(char *str, u_short localPort){						//处理Client传来的数据
	char c = NULL;
	int num = -1, local = -1;
	for (ClIENT_VECTOR::iterator iter = client_vector.begin(); iter != client_vector.end(); iter++){
		local++;
		if (ntohs(((Client *)*iter)->GetPort()) == localPort)	break;
	}
	if (str)	Process(str, c, num, local);
}

BOOL Send_File(const char *File, int local){
	FILE *fp = fopen(File, "rb");  //以二进制方式打开文件
	if (!fp){
		puts("Can't open file!");
		return FALSE;
	}
	if (local < client_vector.size()){
		EnterCriticalSection(&mutex);
		Client *client = client_vector.at(local);					//发送到指定Client
		char buffer[MAX_BUFFER_LENGTH] = {0};
		int length;
		while ((length = fread(buffer, 1, MAX_BUFFER_LENGTH, fp)) > 0){
			string temp(buffer);
			strcpy(Buffer, ("r_F-" + temp).c_str());
			client->Is_Send();
		}
		recv(client->GetSocket(), buffer, MAX_BUFFER_LENGTH, 0);	 //阻塞等待Client返回接收成功的消息
		fclose(fp);
		LeaveCriticalSection(&mutex);
	}
	return TRUE;
}
/**
 * 输出显示
 */
void Menus(BOOL flag){
	EnterCriticalSection(&mutex);
	if (flag == SERVER_START){
		cout << "======================" << endl;
		cout << "| s(S): Start server |" << endl;
		cout << "| e(E): Exit  server |" << endl;
		cout << "======================" << endl;
		cout << "Input:";
	}else if (flag == TIPS){
		cout << "===========================================" << endl;
		cout << "| please connect clients,then send data   |" << endl;
		cout << "| write+num+data:Send data to client-num  |" << endl;
		cout << "|   all+data:Send data to all clients     |" << endl;
		cout << "|          e(E): Exit  server             |" << endl;
		cout << "===========================================" << endl;
		cout << "Input:" << endl;
	}LeaveCriticalSection(&mutex);
}
//显示启动Server结果
void  Connect_Result(BOOL bSuc)
{
	if (bSuc){
		cout << "######################" << endl;
		cout << "# Server succeeded!  #" << endl;
		cout << "######################" << endl;
	}else {
		cout << "######################" << endl;
		cout << "# Server failed   !  #" << endl;
		cout << "######################" << endl;
	}
}

void  Exit(){		//退出
	cout << "######################" << endl;
	cout << "#   Server exit...   #" << endl;
	cout << "######################" << endl;
	Sleep(6000);
	closesocket(Sokcet);					//关闭SOCKET
	WSACleanup();							//清理Windows Sockets
}