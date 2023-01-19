#include "server.h"
int main(int argc, char* argv[]){
	if (!InitSever()){			//初始化服务器
		Exit();	return 1;
	}
	if (!Start()){				//启动服务器
		Connect_Result(FALSE);
		Exit();	return 1;
	}
	Input();					//处理信息
	Exit();
	return 0;
}
