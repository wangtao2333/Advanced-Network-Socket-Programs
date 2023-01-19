#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "client.h"
using namespace std;

int main(){
	if (!InitClient())	Exit();	//初始化
	if (Connect_Server()){	//连接服务器
		Connect_Result(1);
		Menus();
	}else{
		Connect_Result(0);
		Exit();

	}
	if (!Create_Threads())		Exit();	//创建线程
	Preprocess();
	return 0;
}
