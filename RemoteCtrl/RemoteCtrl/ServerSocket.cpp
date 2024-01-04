#include "pch.h"
#include "ServerSocket.h"

//静态变量在类外初始化，显示初始化
CServerSocket* CServerSocket::m_instance = NULL;

//调用静态变量类的构造函数
CServerSocket::C_Helper CServerSocket::m_helper;