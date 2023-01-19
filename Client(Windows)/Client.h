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

//宏定义
#define	SERVERPORT			6666			//服务器TCP端口
#define	MAX_BUF_LENGTH			2048				//缓冲区的最大长度
#define RECVFILE			"rFile"          //接受文件命令
#define UPLOAD   	"upload"
#define DOWNLOAD   	"download"
//函数声明
BOOL InitClient(void);              //初始化
BOOL InitSocket(void);               //非阻塞套接字
BOOL Connect_Server(void);           //连接服务器
bool RecvLine(SOCKET s, char* buf); //读取一行数据
bool sendData(SOCKET s, char* str); //发送数据
void recvAndSend(void);             //数据处理函数
bool recvData(SOCKET s, char* buf);
void Connect_Result(BOOL bSuc);     //连接打印函数
void Menus();
void Exit();              //退出
DWORD __stdcall	Receive(void* Param);
DWORD __stdcall	Send(void* Param);
BOOL Create_Threads(void);
void Preprocess();
void Process(char* bufRecv, int length);
void sendFile(const char* filename);

#endif
#pragma once
