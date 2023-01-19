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
#define SERVER_PORT			6666			//服务器TCP端口
#define CLIENT_NUM_UP		5				//连接Client上限
#define CLIENT_EXIT			1600			//Client退出等待时间
#define ADDRESS_LENGTH		16
#define BROADCAST		"broadcast"
#define SEND			"send"
#define DOWNLOAD		"download"
#define UPLOAD   		"upload"
#define PRE   			"$"
using namespace std;
typedef vector<Client*> ClIENT_VECTOR;				//Client容器
extern char	Buffer[MAX_BUFFER_LENGTH];				//缓冲区
extern BOOL Send_Flag;                              //发送标记位

BOOL InitSever();							//初始化
BOOL InitSocket();							//初始化Socket
BOOL Start();								//启动服
BOOL Create_Thread();
BOOL Send_File(const char *File, int local);
void Connect_Result(BOOL flag);				//显示连接结果
void Menus(BOOL flag);						//显示菜单提示
void Process(char *str, char &c, int &num);
void Process(char *str, char &c, int &num, int localPort);
void Input();								//处理输入
void Preprocess(char *str);                
void Process_Client(char *str, u_short localPort);
void Exit();								//退出
DWORD __stdcall Accept(void *Parameter);	//开启Client请求线程
DWORD __stdcall Clean(void *Parameter);
#endif