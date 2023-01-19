#define _CRT_SECURE_NO_WARNINGS
#include "client.h"
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
using namespace std;
typedef unsigned int SOCKET;
//����
SOCKET	Socket;							//�׽���
char    Send_Buffer[MAX_BUF_LENGTH];				//�������ݻ�����
bool    Send_Flag = false;                      //���ͱ��λ
bool	conn_status;						//�������������״̬
int* p_id = (int*)malloc(sizeof(int) * 2);
pthread_t threads[2];
pthread_mutex_t mutex;					//�ٽ�����������Send_Buffer
string Pre_File;
char SERVER_IP[15];
/**
 *	��ʼ��
 */
bool InitClient(){
	pthread_mutex_init(&mutex, NULL);
	Socket = -1;				//�׽���
	conn_status = false;		//����״̬
	memset(Send_Buffer, 0, MAX_BUF_LENGTH);		//��ʼ�����ݻ�����
	memset(threads, 0, 2);
	memset(p_id, 0, 2);
	if (!InitSocket())	return false;	//����SOCKET
	return true;
}

/**
 * �����׽���
 */
bool  InitSocket(){
	Socket = socket(AF_INET, SOCK_STREAM, 0);	//�����׽���
	if (Socket == -1)	return false;
	if (fcntl(Socket, F_SETFL, O_NONBLOCK) == -1)	return false;	//�����׽��ַ�����
	return true;
}

/**
 * ���ӷ�����
 */
bool Connect_Server(){
	sockaddr_in Addr;					//��������ַ
	Addr.sin_family = AF_INET;
	Addr.sin_port = htons(SERVER_PORT);
	cout << "Please input Server's IP address: ";
	cin.getline(SERVER_IP, 15);
	inet_pton(AF_INET, SERVER_IP, &Addr.sin_addr.s_addr);
	inet_aton(SERVER_IP, &Addr.sin_addr);
	while (true){
		int flag = connect(Socket, (const struct sockaddr*)&Addr, sizeof(struct sockaddr));		//���ӷ�����
		if (flag == -1){						//���Ӵ���
			if (errno == EISCONN)		break;	//�����Ѿ�����
			else		continue;				//����ʧ��	
		}
		if (!flag)		break;					//���ӳɹ�
	}
	conn_status = true;
	return true;
}
/**
 * �����߳�
 */
bool Create_Threads(){
	p_id[0] = 0; p_id[1] = 1;
	if (pthread_create(&threads[0], NULL, Receive_Thread, (void*)&p_id[0]))	return false;	//�����������ݵ��߳�
	if (pthread_create(&threads[1], NULL, Send_Thread, (void*)&p_id[1]))	return false;	//�����������ݵ��߳�
	return true;
}

void *Receive_Thread(void *Parameter){			//�����߳�
	char Receive_Bufer[MAX_BUF_LENGTH + 10];   //���ջ�����
	while (conn_status){
		memset(Receive_Bufer, 0, MAX_BUF_LENGTH);
		int Length = recv(Socket, Receive_Bufer, MAX_BUF_LENGTH, 0);	//��������
		if (Length == -1){
			if (errno == EAGAIN)	continue;						//������������
			else{
				conn_status = true;	break;							//�߳��˳�
			}
		}else if (Length == 0){											//���ӹر�
			Connect_Result(false);
			conn_status = false;
			Send_Flag = false;
			memset(Receive_Bufer, 0, MAX_BUF_LENGTH);
			Exit();break;											//Client�˳�
		}else if (Length > 0){											//���ճɹ�
			char bufer[MAX_BUF_LENGTH];
			pthread_mutex_lock(&mutex);
			strcpy(bufer, Receive_Bufer);
			pthread_mutex_unlock(&mutex);
			Process(bufer, Length);
			memset(Receive_Bufer, 0, MAX_BUF_LENGTH);
		}
	}
}

void *Send_Thread(void *Parameter){				//���������߳�
	while (conn_status){
		if (Send_Flag){
			pthread_mutex_lock(&mutex);					//����
			while (true){			
				if (send(Socket, Send_Buffer, strlen(Send_Buffer), 0) == -1){			
					if (errno == EAGAIN)		continue;		//���ͻ�����������
					else{
						pthread_mutex_unlock(&mutex);
						return 0;
					}
				}Send_Flag = false;break;
			}pthread_mutex_unlock(&mutex);				//����
		}
	}
}
void Process(char *str, int length){
	char    File_Buffer[MAX_BUF_LENGTH];
	if ((str[0] == 'E' || str[0] == 'e') && strlen(str) == 1) {		 //�Ƿ��˳�
		Connect_Result(false);
		conn_status = false;
		Send_Flag = false;
		memset(str, 0, MAX_BUF_LENGTH);
		Exit();
	}else if (!strncmp(FILE_PRE, str, strlen(FILE_PRE))){
		pthread_mutex_lock(&mutex);
		cout << "!";
		str += strlen(FILE_PRE);
		strcpy(File_Buffer, str);
		length -= strlen(FILE_PRE);
		FILE *fp = fopen(Pre_File.c_str(), "ab");					//�����ƴ����ļ�
		if (!fp){
			puts("Can't open the file!");
			system("pause");
		}
		fwrite(File_Buffer, length, 1, fp);
		puts("The file transfered successfully!");
		fclose(fp);
		pthread_mutex_unlock(&mutex);
	}else	cout << "You've received: " << str << endl;				//��ʾ��Ϣ
}
/**
 * �������ݺ���ʾ���
 */
void Preprocess(){	
	while (conn_status){
		char Input_Buffer[MAX_BUF_LENGTH];						//���뻺����
		memset(Input_Buffer, 0, MAX_BUF_LENGTH);
		cin.getline(Input_Buffer, MAX_BUF_LENGTH);				//��������
		pthread_mutex_lock(&mutex);
		memcpy(Send_Buffer, Input_Buffer, strlen(Input_Buffer));
		pthread_mutex_unlock(&mutex);
		char *input = Input_Buffer;
		bool temp = false;
		if (!strncmp(UPLOAD, input, strlen(UPLOAD))){
			temp = true;
			Send_Flag = true;
			input += strlen(UPLOAD);
			string a(++input);
			string b = "./" + a;
			if (temp)	Send_File((b).c_str());
		}else if (!strncmp(DOWNLOAD, input, strlen(DOWNLOAD))){
			Send_Flag = true;
			input += strlen(DOWNLOAD);
			string a(++input);
			Pre_File = "./" + a;
		}else	Send_Flag = true;
	}
}
void Send_File(const char *File) {
	char buffer[MAX_BUF_LENGTH];
	int length;
	FILE *fp = fopen(File, "rb");								//������ֻ��
	if (!fp) {
		puts("Can't open file!");
		return;
	}
	cout << "Transfer File: " << File + 2 << endl;
	while ((length = fread(buffer, 1, MAX_BUF_LENGTH, fp)) > 0) {
		pthread_mutex_lock(&mutex);
		string temp(buffer);
		memset(Send_Buffer, 0, MAX_BUF_LENGTH);
		memcpy(Send_Buffer, ("$" + temp).c_str(), length + 1);
		Send_Flag = true;
		pthread_mutex_unlock(&mutex);
	}
	recv(Socket, buffer, MAX_BUF_LENGTH, 0);					//�����ȴ�������ϵ��ź�
	fclose(fp);
}

void Exit(void){
	pthread_mutex_destroy(&mutex);
	free(p_id);
	memset(Send_Buffer, 0, MAX_BUF_LENGTH);
	close(Socket);
}

void Connect_Result(bool flag){
	if (flag)	cout << "## Succeed to connect server! ##" << endl;
	else	cout << "## Client has to exit! ##" << endl;
}
void Menus() {
	cout << "=====================================================" << endl;
	cout << "|     please connect clients,then send message      |" << endl;
	cout << "| send+num+message:Send message to specified client |" << endl;
	cout << "|   broadcast+message:Send message to all clients   |" << endl;
	cout << "|      download+file: download file from server     |" << endl;
	cout << "|         upload+file: upload file to server        |" << endl;
	cout << "=====================================================" << endl;
	cout << "You can input:" << endl;
}