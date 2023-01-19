#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED
#include <iostream>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define	SERVER_PORT			6666			//�������˿�
#define	MAX_BUF_LENGTH			2048				//����������
#define FILE_PRE			"r_F-"          //�ļ�ǰ׺��ʶ
#define UPLOAD   	"upload"
#define DOWNLOAD   	"download"
using namespace std;
typedef unsigned int SOCKET;

bool InitClient();              //��ʼ��
bool InitSocket();               //��ʼ���׽���
bool Connect_Server();           //��������
void *Receive_Thread(void *Parameter);	//�����̺߳���
void *Send_Thread(void *Parameter);		//�����̺߳���
bool Create_Threads();			//���߳�
void Preprocess();				//Ԥ����
void Process(char* buffer, int length);	//���ݴ���
void Send_File(const char* File);	//�����ļ�
void Connect_Result(bool flag);
void Menus();				//�˵�
void Exit(void);			//�˳�
#endif
#pragma once