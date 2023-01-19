#include "server.h"
#include "base.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * ȫ�ֱ���
 */
char Buffer[MAX_BUFFER_LENGTH];			//������
BOOL connect_status;					//��Client������״̬
BOOL Send_Flag;
BOOL Connect_Flag;						//���ӱ��
SOCKET Sokcet;
CRITICAL_SECTION mutex;			        //��
HANDLE Accept_Thread;					//�����߳̾��
HANDLE Clean_Thread;					//�߳�������
ClIENT_VECTOR client_vector;			//��client��Ϣ
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

BOOL InitSocket(){										//��ʼ��SOCKET
	WSADATA  ws;
	int flag = WSAStartup(MAKEWORD(2, 2), &ws);
	Sokcet = socket(AF_INET, SOCK_STREAM, 0);			//����socket
	if (INVALID_SOCKET == Sokcet)	return FALSE;
	unsigned long temp = 1;
	if (ioctlsocket(Sokcet, FIONBIO, (unsigned long *)&temp) == SOCKET_ERROR)	return FALSE;
	sockaddr_in Address;								//����socket
	Address.sin_family = AF_INET;	Address.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, "192.168.2.208", &Address.sin_addr.S_un.S_addr);
	if (bind(Sokcet, (struct sockaddr *)&Address, sizeof(Address)) == SOCKET_ERROR)	return FALSE;
	if (listen(Sokcet, CLIENT_NUM_UP) == SOCKET_ERROR)	return FALSE;
	return TRUE;
}

BOOL Start(){			//����
	BOOL flag = TRUE;
	Menus(SERVER_START);
	char c;	
	while (true){
		cin >> c;
		if ((c == 'e' || c == 'E'))	return FALSE;
		else if ((c == 's' || c == 'S')) {
			if (Create_Thread())	Connect_Result(TRUE);		//�ɹ������߳�
			else	flag = FALSE;
			break;
		}else{
			puts("Start Fail!");
			Menus(SERVER_START);
		}
	}
	cin.sync();                     //������뻺����
	return flag;
}

BOOL Create_Thread(){												//�����߳�
	connect_status = TRUE;											//����ServerΪ����״̬
	unsigned long temp;
	Accept_Thread = CreateThread(NULL, 0, Accept, NULL, 0, &temp);	//��������Client�����߳�
	if (!Accept_Thread){
		connect_status = FALSE;
		return FALSE;
	}else	CloseHandle(Accept_Thread);
	Clean_Thread = CreateThread(NULL, 0, Clean, NULL, 0, &temp);	//���������߳�
	if (!Clean_Thread)	return FALSE;
	else	CloseHandle(Clean_Thread);
	return TRUE;
}

DWORD __stdcall Accept(void *Parameter){
	sockaddr_in Address;														//Client��ַ
	while (connect_status){														//����״̬
		int	length = sizeof(sockaddr_in);
		memset(&Address, 0, length);
		SOCKET Accept_Socket = accept(Sokcet, (sockaddr *)&Address, &length);	//��Client���ӵ��׽���
		if (Accept_Socket == INVALID_SOCKET){
			if (WSAGetLastError() == WSAEWOULDBLOCK){
				Sleep(600);	continue;											//�����ȴ�
			}else	return 0;
		}else{																	//����Client������
			Connect_Flag = TRUE;
			Client *ptr = new Client(Accept_Socket, Address);
			EnterCriticalSection(&mutex);		
			char Client_IP[ADDRESS_LENGTH];
			inet_ntop(AF_INET, &Address.sin_addr, Client_IP, sizeof(Client_IP));
			cout << "Connect a client IP: " << Client_IP << "\tPort: " << ntohs(Address.sin_port) << endl;	//��ʾClient��IP�Ͷ˿�
			client_vector.push_back(ptr);										//���Client
			LeaveCriticalSection(&mutex);
			ptr->Start();
		}
	}return 0;
}

/**
 * ������Դ�߳�
 */
DWORD __stdcall Clean(void *Parameter){
	while (connect_status){
		EnterCriticalSection(&mutex);							//����
		for (ClIENT_VECTOR::iterator iter = client_vector.begin(); iter != client_vector.end();){
			Client* pClient = (Client*)*iter;
			if (!pClient->Is_Connect()){						//Client�Ѿ��˳�
				client_vector.erase(iter);
				delete pClient;									//�ͷ��ڴ�
				pClient = NULL;
			}else	iter++;										//ָ�����
		}
		if (client_vector.size() == 0)	Connect_Flag = FALSE;
		LeaveCriticalSection(&mutex);
		Sleep(CLIENT_EXIT);										//��Client�߳���Ӧ�˳�ʱ��
	}
	if (!connect_status){										//���ӶϿ�
		EnterCriticalSection(&mutex);
		for (ClIENT_VECTOR::iterator iter = client_vector.begin(); iter != client_vector.end();){
			Client* pClient = (Client*)*iter;	
			if (pClient->Is_Connect())	pClient->Disconnect();	//�Ͽ����ӣ��߳��˳�
			++iter;
		}
		LeaveCriticalSection(&mutex);							//����
		Sleep(CLIENT_EXIT);
	}
	client_vector.clear();
	Connect_Flag = FALSE;
	return 0;
}

/**
 * ��������
 */
void Input(){
	char Temp_Buffer[MAX_BUFFER_LENGTH];
	Menus(TIPS);
	while (connect_status){
		memset(Temp_Buffer, 0, MAX_BUFFER_LENGTH);		//��ջ�����
		cin.getline(Temp_Buffer, MAX_BUFFER_LENGTH);	//����
		Preprocess(Temp_Buffer);						//������Ϣ
	}
}

void Process(char *str, char &c, int &num){
	if (!strncmp(SEND, str, strlen(SEND))){
		EnterCriticalSection(&mutex);
		str += strlen(SEND);
		c = *++str;
		num = c - '1';
		if (num > -1 && num < client_vector.size()){
			Client* sClient = client_vector.at(num);			//����ָ��Client
			str += 2;											//�ո�����ִ���
			strcpy(Buffer, str);
			sClient->Is_Send();
			cout << "��Client" << num + 1 << "��������Ϣ�� " << str << endl;
			LeaveCriticalSection(&mutex);
		}
		else {
			cout << "������Client" << num + 1 << "!!!" << endl;
			LeaveCriticalSection(&mutex);
		}
	}
	else if (!strncmp(BROADCAST, str, strlen(BROADCAST))){
		EnterCriticalSection(&mutex);
		str += strlen(BROADCAST);
		strcpy(Buffer, ++str);
		Send_Flag = TRUE;
		cout << "������Client������Ϣ�� " << str << endl;		//Server��������Client	
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
			str += 2;														//�ո�����ִ���
			strcpy(Buffer, str);
			client_vector.at(num)->Is_Send();								//����ָ��Client
			cout << "��Client" << num + 1 << "������Ϣ�� " << str << endl;
			LeaveCriticalSection(&mutex);
		}else{
			cout << "û�и�Client!!!" << endl;
			LeaveCriticalSection(&mutex);
		}
	}else if (!strncmp(str, BROADCAST, strlen(BROADCAST))){					//broadcast
		EnterCriticalSection(&mutex);
		str += strlen(BROADCAST);
		strcpy(Buffer, ++str);
		cout << "������Client��������Ϣ�� " << str << endl;					//��������Client
		int num = 0;
		for (ClIENT_VECTOR::iterator iter = client_vector.begin(); iter != client_vector.end(); num++, iter++) {
			if (num == local)	continue;
			client_vector.at(num)->Is_Send();
		}
		LeaveCriticalSection(&mutex);
	}else if (!strncmp(str, PRE, strlen(PRE))){								//�ļ����ʹ���
		EnterCriticalSection(&mutex);
		str++;
		int length = strlen(str);
		FILE* fp = fopen(Pre_File.c_str(), "wb");							//�����ƴ����ļ�
		if (!fp)		puts("Cannot open file!");
		else {
			fwrite(str, length, 1, fp);
			fclose(fp);
			cout << "����Client" << local + 1 << "�������ļ���" << Pre_File << endl;
			strcpy(Buffer, "Success!");
			client_vector.at(local)->Is_Send();
		}
		Pre_File = "";
		LeaveCriticalSection(&mutex);
	}else if (!strncmp(str, UPLOAD, strlen(UPLOAD))){						//upload
		EnterCriticalSection(&mutex);
		str += strlen(UPLOAD) + 1;
		string a(str);														//�ϴ����ļ���
		Pre_File = "./" + a;
		memset(Buffer, 0, MAX_BUFFER_LENGTH);
		LeaveCriticalSection(&mutex);
	}else if (!strncmp(str, DOWNLOAD, strlen(DOWNLOAD))){					//download
		EnterCriticalSection(&mutex);
		str += strlen(DOWNLOAD) + 1;
		string a(str);
		memset(Buffer, 0, MAX_BUFFER_LENGTH);
		if (Send_File(("./" + a).c_str(), local))		cout << "��Client" << local + 1 << "�����ļ���" << a << endl;
		else	cout << "�ļ��������" << endl;
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

void Process_Client(char *str, u_short localPort){						//����Client����������
	char c = NULL;
	int num = -1, local = -1;
	for (ClIENT_VECTOR::iterator iter = client_vector.begin(); iter != client_vector.end(); iter++){
		local++;
		if (ntohs(((Client *)*iter)->GetPort()) == localPort)	break;
	}
	if (str)	Process(str, c, num, local);
}

BOOL Send_File(const char *File, int local){
	FILE *fp = fopen(File, "rb");  //�Զ����Ʒ�ʽ���ļ�
	if (!fp){
		puts("Can't open file!");
		return FALSE;
	}
	if (local < client_vector.size()){
		EnterCriticalSection(&mutex);
		Client *client = client_vector.at(local);					//���͵�ָ��Client
		char buffer[MAX_BUFFER_LENGTH] = {0};
		int length;
		while ((length = fread(buffer, 1, MAX_BUFFER_LENGTH, fp)) > 0){
			string temp(buffer);
			strcpy(Buffer, ("r_F-" + temp).c_str());
			client->Is_Send();
		}
		recv(client->GetSocket(), buffer, MAX_BUFFER_LENGTH, 0);	 //�����ȴ�Client���ؽ��ճɹ�����Ϣ
		fclose(fp);
		LeaveCriticalSection(&mutex);
	}
	return TRUE;
}
/**
 * �����ʾ
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
//��ʾ����Server���
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

void  Exit(){		//�˳�
	cout << "######################" << endl;
	cout << "#   Server exit...   #" << endl;
	cout << "######################" << endl;
	Sleep(6000);
	closesocket(Sokcet);					//�ر�SOCKET
	WSACleanup();							//����Windows Sockets
}