#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>


#pragma pack(push)
#pragma pack(1)
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

	//�������
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		sSum = 0;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else
		{
			strData.clear();
		}
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += (BYTE)strData[j] & 0xFF;
		}

	}

	//����������
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;   //��ͷ��WORD��2���ֽ�
				break;
			}
		}
		//��������DWORD,4���ֽڣ�����������WORD��2���ֽڣ�У�����WORD��2���ֽ�
		//û�����ݣ�����û�н�������ͷ����ô���˳�
		if (i + 8 > nSize) {
			nSize = 0;
			return;
		}
		nLength = *(WORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) {  //�����ݲ�ȫ���˳�
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2); //��һ��2��sCmd���ڶ���2��sSum
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
		strOut.resize(nLength + 6);
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
	WORD sHead;            //��ͷ         
	DWORD nLength;         //������
	WORD sCmd;             //��������
	std::string strData;   //����
	WORD sSum;             //У���
	std::string strOut;    //������������
};
#pragma pack(pop)

//����¼��ṹ��
typedef struct MouseEvent
{
	MouseEvent() {
		nAction = 0;
		nButton = -1;   //���-1�ĺô����������д�������⣬��ĳ����
		ptxy.x = 0;
		ptxy.y = 0;
	}
	WORD nAction;  //������ƶ���˫��
	WORD nButton;  //������Ҽ����м�
	POINT ptxy;    //���λ��

} MOUSEEV, * PMOUSEEV;


std::string GetErrorInfo(int wsaErrCode);

class CClientSocket
{
public:
	//��̬���������Բ����������󣬾Ϳ���ͨ���ú���ֱ�ӵ��ã�һ��Ҫ�ŵ�public
	//���þ�̬����ʱ������ʹ��this��this����Զ���ģ�û�ж���Ȼ����this��
	static CClientSocket* getInstacne() {
		if (m_instance == NULL)
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}

	bool InitSocket(std::string& strIPAddress)
	{
		if (m_socket != INVALID_SOCKET) CloseSocket();
		m_socket = socket(PF_INET, SOCK_STREAM, 0);
		if (m_socket == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(strIPAddress.c_str());
		serv_addr.sin_port = htons(9527);

		if (serv_addr.sin_addr.s_addr == INVALID_SOCKET)
		{
			AfxMessageBox("���ӵ�IP��Ч");
			return false;
		}

		int ret = connect(m_socket, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (ret == -1)
		{
			AfxMessageBox("����ʧ��");
			TRACE("����ʧ�ܣ�%d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());//��������Ա���´�������
			return false;
		}

		return true;
	}



#define BUFFER_SIZE 4096
	int DealCommand()
	{
		if (m_socket == -1) return -1;
		//TODO:��������
		char* buffer = m_buffer.data();
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_socket, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0)
				return -1;
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			//���len����0���ű�ʾ�������ɹ�
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return 0;
			}
		}
		return -1;
	}

	bool Send(const char* pdata, int psize) {
		if (m_socket == -1) return false;
		return send(m_socket, pdata, psize, 0) > 0; // ���send<=0 ���صľ���ʧ��ֵ
	}

	bool Send(CPacket& pack) {
		if (m_socket == -1) return false;
		return send(m_socket, pack.Data(), pack.nLength + 6, 0) > 0;
	}

	bool GetFilePath(std::string& strPath)
	{
		//ֻ�е�������2ʱ������ȡ��ȡ�ļ�Ŀ¼
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(mouse));
			return true;
		}
		return false;
	}

	CPacket& GetPacket()
	{
		return m_packet;
	}

	void CloseSocket()
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}

private:
	//��vector�������ʹ��m_buffer.data()ȡ��ַ
	std::vector<char> m_buffer;
	SOCKET m_socket;
	CPacket m_packet;

	CClientSocket& operator=(const CClientSocket& one) {

	}


	CClientSocket(const CClientSocket& ss)
	{
		m_socket = ss.m_socket;
		
	}

	CClientSocket() {
		
		if (!InitSocketEnv())
		{
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);

		//�ͻ��˲���������һ��������������socket
		//m_socket�ڽ�����ʱ�򱻷�������close���ˣ����º���m_socket�޷����������������ǲ���
		//�����ﴴ��socket
		//m_socket = socket(PF_INET, SOCK_STREAM, 0);
	}

	~CClientSocket() {
		WSACleanup();
	}

	bool InitSocketEnv() {
		WSAData data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
			return false;
		return true;
	}

	//��̬����
	static CClientSocket* m_instance;

	static void releaseInstance()
	{
		if (m_instance == NULL)
		{
			return;
		}
		CClientSocket* tmp = m_instance;
		m_instance = NULL;
		delete tmp;
	}

	//�������ڶ���һ���࣬����������������̬��Ա����
	class C_Helper {
	public:
		C_Helper()
		{
			CClientSocket::getInstacne();
		}
		~C_Helper()
		{
			CClientSocket::releaseInstance();
		}
	};

	//��Ϊ����û�취ֱ�ӵ���˽����C_Helper,���������ٴ���һ����̬��Ա
	static C_Helper m_helper;
};

extern CClientSocket client;
