#include "pch.h"
#include "ServerSocket.h"

//��̬�����������ʼ������ʾ��ʼ��
CServerSocket* CServerSocket::m_instance = NULL;

//���þ�̬������Ĺ��캯��
CServerSocket::C_Helper CServerSocket::m_helper;