#include "pch.h"
#include "ClientSocket.h"


//��̬�����������ʼ������ʾ��ʼ��
CClientSocket* CClientSocket::m_instance = NULL;

//���þ�̬������Ĺ��캯��
CClientSocket::C_Helper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstacne();


std::string GetErrorInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf;   //LPVOID��������ָ�룬���Խ��������͵�ָ�봫����
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);//�ú�����GetLastError�õ��Ĵ�����Ϣת��Ϊ�ַ�����Ϣ
	ret = (char*)lpMsgBuf;    //string����ֱ�ӵ���char*,���Ҫ��ס
	LocalFree(lpMsgBuf);  //�ͷ��ڴ�
	return ret;
}