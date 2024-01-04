#pragma once
#include "pch.h"
#include "framework.h"








class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(const CPacket& one)
	{
		sHead = one.sHead;
		nLength = one.nLength;
		sCmd = one.sCmd;
		strData = one.strData;
		sSum = one.sSum;
	}
	CPacket& operator=(const CPacket& one) {
		if (this == &one) return *this;
		sHead = one.sHead;
		nLength = one.nLength;
		sCmd = one.sCmd;
		strData = one.strData;
		sSum = one.sSum;
		return *this;
	}

	//打包数据
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		strData.resize(nSize);
		memcpy((void*)strData.c_str(), pData, nSize);
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += (BYTE)strData[j] & 0xFF;
		}

	}

	//解析包数据
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) {
				i += 2;   //包头是WORD，2个字节
				break;
			}
		}
		//包长度是DWORD,4个字节；控制命令是WORD，2个字节；校验和是WORD，2个字节
		//没有数据，或者没有解析到包头，那么就退出
		if (i + 8 > nSize) {
			nSize = 0;
			return;
		}
		nLength = *(WORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) {  //包数据不全，退出
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2); //第一个2是sCmd，第二个2是sSum
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}

		sSum = *(WORD*)(pData + i);
		i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += (BYTE)strData[j] & 0xFF;
		}
		if (sum == sSum)
		{
			nSize = i;
			return;
		}

		nSize = 0;

	}
	~CPacket() {}

	int Size()
	{
		return nLength + 6;
	}
	const char* Data()
	{
		strOut.resize(nLength+6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;
		pData += 2;
		*(DWORD*)pData = nLength;
		pData += 4;
		*(WORD*)pData = sCmd;
		pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
	
public:
	WORD sHead;            //包头         
	DWORD nLength;         //包长度
	WORD sCmd;             //控制命令
	std::string strData;   //数据
	WORD sSum;             //校验和
	std::string strOut;    //整个包的数据
};





//因为像WSAData，WSAStartup(),WSACleanup()全过程只会调用一次，因此我们选择
//创建一个类，当类被创建和被销毁时，都只会调用一次。
class CServerSocket
{
public:
	//静态函数，可以不用声明对象，就可以通过该函数直接调用，一定要放到public
	//调用静态对象时，不能使用this，this是针对对象的，没有对象当然不用this了
	static CServerSocket* getInstacne() {
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	bool InitSocket()
	{
		if (m_socket == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(9527);

		bind(m_socket, (sockaddr*)&serv_addr, sizeof(serv_addr));

		if (listen(m_socket, 1) == -1)
			return false;

		return true;
	}

	bool AcceptClient()
	{
		sockaddr_in client_addr;
		int client_size = sizeof(client_addr);
		m_client = accept(m_socket, (sockaddr*)&client_addr, &client_size);
		if (m_client == -1) return false;
		closesocket(m_socket);
		return true;
	}

#define BUFFER_SIZE 4096
	int DealCommand()
	{
		if (m_client == -1) return -1;
		//TODO:处理命令
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0)
				return -1;
			index += len;
			m_packet = CPacket((BYTE*)buffer, len);
			//如何len大于0，才表示包解析成功
			if (len > 0)
			{
				memmove(buffer, buffer + index, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pdata, int psize) {
		if (m_client == -1) return false;
		return send(m_client, pdata, psize, 0) > 0; // 如果send<=0 返回的就是失败值
	}

	bool Send(const CPacket& pack) {
		if (m_client == -1) return false;
		return send(m_client, (const char*)&pack, pack.nLength+6, 0 ) > 0;
	}

	bool GetFilePath(std::string& strPath)
	{
		//只有当命令是2时，才能取获取文件目录
		if (m_packet.sCmd == 2)
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

private:
	SOCKET m_socket;
	SOCKET m_client;
	CPacket m_packet;

	CServerSocket& operator=(const CServerSocket& one) {

	}


	CServerSocket(const CServerSocket& ss)
	{
		m_socket = ss.m_socket;
		m_client = ss.m_client;
	}

	CServerSocket() {
		m_client = INVALID_SOCKET;
		if (!InitSocketEnv())
		{
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_socket = socket(PF_INET, SOCK_STREAM, 0);
	}

	~CServerSocket() {
		WSACleanup();
	}

	bool InitSocketEnv() {
		WSAData data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
			return false;
		return true;
	}

	//静态对象
	static CServerSocket* m_instance;

	static void releaseInstance()
	{
		if (m_instance == NULL)
		{
			return;
		}
		CServerSocket* tmp = m_instance;
		m_instance = NULL;
		delete tmp;
	}

	//在类内在定义一个类，用来帮助和析构静态成员变量
	class C_Helper {
	public:
		C_Helper()
		{
			CServerSocket::getInstacne();
		}
		~C_Helper()
		{
			CServerSocket::releaseInstance();
		}
	};

	//因为类外没办法直接调用私有类C_Helper,所以我们再创建一个静态成员
	static C_Helper m_helper;
};

extern CServerSocket server;


