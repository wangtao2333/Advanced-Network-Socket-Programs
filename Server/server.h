#define _CRT_SECURE_NO_WARNINGS
#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED
#include <Ws2tcpip.h>
#include <winsock2.h>
#include <winbase.h>
#include <process.h>
#include <iostream>
#include <vector>
#include <string>
#include "base.h"
#pragma comment(lib, "ws2_32.lib")
#define SERVER_START		1
#define TIPS				2
#define SERVER_PORT			6666			//������TCP�˿�
#define CLIENT_NUM_UP		5				//����Client����
#define CLIENT_EXIT			1600			//Client�˳��ȴ�ʱ��
#define ADDRESS_LENGTH		16
#define BROADCAST		"broadcast"
#define SEND			"send"
#define DOWNLOAD		"download"
#define UPLOAD   		"upload"
#define PRE   			"$"
using namespace std;
typedef vector<Client*> ClIENT_VECTOR;				//Client����
extern char	Buffer[MAX_BUFFER_LENGTH];				//������
extern BOOL Send_Flag;                              //���ͱ��λ

BOOL InitSever();							//��ʼ��
BOOL InitSocket();							//��ʼ��Socket
BOOL Start();								//������
BOOL Create_Thread();
BOOL Send_File(const char *File, int local);
void Connect_Result(BOOL flag);				//��ʾ���ӽ��
void Menus(BOOL flag);						//��ʾ�˵���ʾ
void Process(char *str, char &c, int &num);
void Process(char *str, char &c, int &num, int localPort);
void Input();								//��������
void Preprocess(char *str);                
void Process_Client(char *str, u_short localPort);
void Exit();								//�˳�
DWORD __stdcall Accept(void *Parameter);	//����Client�����߳�
DWORD __stdcall Clean(void *Parameter);
#endif