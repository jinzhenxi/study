#include "pch.h"
#include "ClientSocket.h"


//静态变量在类外初始化，显示初始化
CClientSocket* CClientSocket::m_instance = NULL;

//调用静态变量类的构造函数
CClientSocket::C_Helper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstacne();


std::string GetErrorInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf;   //LPVOID是无类型指针，可以将任意类型的指针传给它
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);//该函数将GetLastError得到的错误信息转化为字符串信息
	ret = (char*)lpMsgBuf;    //string可以直接等于char*,这点要记住
	LocalFree(lpMsgBuf);  //释放内存
	return ret;
}