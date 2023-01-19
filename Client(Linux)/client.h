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
#define	SERVER_PORT			6666			//服务器端口
#define	MAX_BUF_LENGTH			2048				//缓冲区上限
#define FILE_PRE			"r_F-"          //文件前缀标识
#define UPLOAD   	"upload"
#define DOWNLOAD   	"download"
using namespace std;
typedef unsigned int SOCKET;

bool InitClient();              //初始化
bool InitSocket();               //初始化套接字
bool Connect_Server();           //连服务器
void *Receive_Thread(void *Parameter);	//接收线程函数
void *Send_Thread(void *Parameter);		//发送线程函数
bool Create_Threads();			//创线程
void Preprocess();				//预处理
void Process(char* buffer, int length);	//数据处理
void Send_File(const char* File);	//发送文件
void Connect_Result(bool flag);
void Menus();				//菜单
void Exit(void);			//退出
#endif
#pragma once