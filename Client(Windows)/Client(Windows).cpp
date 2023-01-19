#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "client.h"
using namespace std;

int main()
{
	//初始化
	if (!InitClient())	Exit();

	//连接服务器
	if (Connect_Server()){
		Connect_Result(TRUE);
		Menus();
	}else{
		Connect_Result(FALSE);
		Exit();
	}
	//创建发送和接收数据线程
	if (!Create_Threads())		Exit();
	
	//用户输入数据和显示结果
	Preprocess();
	return 0;
}
