// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#pragma comment( linker, "/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup")
//#pragma comment( linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment( linker, "/subsystem:console /entry:WinMainCRTStartup")


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;


void Dump(BYTE* pData, size_t nSize)
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16) == 0) strOut += "\n";
        snprintf(buf,sizeof(buf),"%02X", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}


//获取磁盘信息，因为我们有打开文件的需求，要打开文件，就要知道在
//哪个分区，因此要获取磁盘信息
int MakeDriverInfo()
{
    std::string result;
    for (int i = 1; i<= 26; i++)
    {
        if (_chdrive(i) == 0)
        {
            result += 'A' + i - 1;
        }
    }

    CPacket pack(1, (BYTE*)result.c_str(), result.size());
    // Dump((BYTE*)&pack, pack.nLength + 6);这样是错的，因为取的是pack的地址，而不是pack的内容
    Dump((BYTE*)pack.Data(), pack.nLength + 6);
    //CServerSocket::getInstacne()->Send(CPacket(1,(BYTE*)result.c_str(), result.size()));
    return 0;
}


#include<io.h>
#include<list>
//定义一个结构体来存文件或者目录的信息
typedef struct file_Info {
    file_Info()
    {
        IsInvalid = 0;
        IsDirectory = -1;
        HasNext = false;
        memset(szFileName,0,sizeof(szFileName));
    }
    bool IsInvalid;
    bool IsDirectory;
    bool HasNext;
    char szFileName[256];
}FILEINFO,*PFILEINFO;

//获取指定目录或者文件夹
int MakeDirectoryInfo()
{
    std::string strPath;
    //std::list<FILEINFO> lisFileInfos;
    if (CServerSocket::getInstacne()->GetFilePath(strPath) == false)
    {
        OutputDebugString(_T("当前指令不是获取目录,命令解析错误"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0)
    {
        FILEINFO finfo;
        finfo.IsInvalid = true;
        finfo.IsDirectory = true;
        finfo.HasNext = false;
        memcpy(finfo.szFileName,strPath.c_str(), strPath.size());
        //lisFileInfos.emplace_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstacne()->Send(pack);
        OutputDebugString(_T("当前目录或者文件没有权限！"));
        return -2;
    }
    //_finddata_t结构体用来存储文件信息
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1)
    {
        OutputDebugString(_T("没有找到任何文件!"));
        return -3;
    }
    do
    {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        //lisFileInfos.emplace_back(finfo);
        CPacket pack(2,(BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstacne()->Send(pack);
    } while (_findnext(hfind, &fdata));

    FILEINFO finfo;
    finfo.HasNext = false;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::getInstacne()->Send(pack);

    return 0;
}


//调用main前是一个主线程，会按include顺序先进行初始和实例化，然后在调用main

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            //CServerSocket* pserver = CServerSocket::getInstacne();  //创建单例
            //int count = 0;
            //if (pserver->InitSocket() == false)
            //{
            //    MessageBox(NULL, _T("无法初始化套接字环境，请检查网络环境"), _T("初始化错误"), MB_OK | MB_ICONERROR);
            //    exit(0);
            //}
            //while (pserver != NULL)
            //{
            //    if (pserver->AcceptClient() == false)
            //    {
            //        if (count >= 3)
            //        {
            //            MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败!"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("无法正常接入用户，结束程序"), _T("接入用户失败!"), MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    int ret = pserver->DealCommand();
            //    //TODO
            //}
            int nCmd = 1;
            switch (nCmd)
            {
            case 1: 
                MakeDriverInfo();
                break;
            case 2:
                MakeDirectoryInfo();
                break;
            }

        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
