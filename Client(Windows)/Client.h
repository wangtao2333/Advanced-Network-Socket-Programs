#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED
#include <iostream>
#include <winsock2.h>
#include <process.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "WS2_32.lib")
using namespace std;

//�궨��
#define	SERVERPORT			6666			//������TCP�˿�
#define	MAX_BUF_LENGTH			2048				//����������󳤶�
#define RECVFILE			"rFile"          //�����ļ�����
#define UPLOAD   	"upload"
#define DOWNLOAD   	"download"
//��������
BOOL InitClient(void);              //��ʼ��
BOOL InitSocket(void);               //�������׽���
BOOL Connect_Server(void);           //���ӷ�����
bool RecvLine(SOCKET s, char* buf); //��ȡһ������
bool sendData(SOCKET s, char* str); //��������
void recvAndSend(void);             //���ݴ�����
bool recvData(SOCKET s, char* buf);
void Connect_Result(BOOL bSuc);     //���Ӵ�ӡ����
void Menus();
void Exit();              //�˳�
DWORD __stdcall	Receive(void* Param);
DWORD __stdcall	Send(void* Param);
BOOL Create_Threads(void);
void Preprocess();
void Process(char* bufRecv, int length);
void sendFile(const char* filename);

#endif
#pragma once
