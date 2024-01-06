// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>          //跟磁盘有关
#include <atlimage.h>        //跟截图有关

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

int RunFile()
{
    std::string strPath;
    CServerSocket::getInstacne()->GetFilePath(strPath);
    ShellExecuteA(NULL,NULL,strPath.c_str(),NULL,NULL,SW_SHOWNORMAL);//打开记事本,或者执行程序
    CPacket pack(3, NULL, 0);
    CServerSocket::getInstacne()->Send(pack);
    return 0;
}

int DownloadFile()
{
    std::string strPath;
    CServerSocket::getInstacne()->GetFilePath(strPath);
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile,strPath.c_str(),"rb");
    if (err != 0)
    {
        CPacket pack(4, (BYTE*) &data, 0);
        CServerSocket::getInstacne()->Send(pack);
        return -1;
    }

    if (pFile != NULL)
    {
        //设置文件指针为文件末尾
        fseek(pFile,0, SEEK_END);
        //获取文件指针位置到文件开头的距离, 其实就是文件大小了
        data = _ftelli64(pFile);
        CPacket head(4, (BYTE*)data, 8);
        CServerSocket::getInstacne()->Send(head);
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024] = "";
        size_t rlen = 0;
        do
        {
            rlen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstacne()->Send(pack);
        } while (rlen >= 1024);
    }
    CPacket pack(4, NULL, 0);
    CServerSocket::getInstacne()->Send(pack);
    fclose(pFile);
    return 0;
}

int MouseEvent()
{
    MOUSEEV mouse;
    if (CServerSocket::getInstacne()->GetMouseEvent(mouse))
    {
        DWORD nflag = 0;
        switch (mouse.nButton)
        {
        case 0://左键
            nflag = 1;
            break;
        case 1://右键
            nflag = 2;       //二进制代表第二个比特位
            break;
        case 2://中键
            nflag = 4;       //二进制代表第三个比特位
            break;
        case 4://没有按键
            nflag = 8;       //二进制代表第四个比特位
            break;
        }

        if (nflag != 8)
        {
            SetCursorPos(0, 0);  //设置当前鼠标位置
        }

        switch (mouse.nAction)
        {
        case 0://单击
            nflag |= 0x10;
        case 1://双击
            nflag |= 0x20;
            break;
        case 2://按下
            nflag |= 0x40;
            break;
        case 3://放开
            nflag |= 0x80;
            break;
        default:
            break;
        }

        switch (nflag)
        {
        case 0x21:  //左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
        case 0x11:  //左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41:  //左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81:  //左键松开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22:  //右键双击  
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
        case 0x12:  //右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42:  //右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82:  //右键松开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24:  //中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
        case 0x14:  //中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44:  //中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84:  //中键松开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        }
        CPacket pack(5,NULL,0);
        CServerSocket::getInstacne()->Send(pack);

    }
    else
    {
        OutputDebugString(_T("获取鼠标操作参数失败！"));
        return -1;
    }

    return 0;
}


int SendScreen()
{
    CImage screen;
    HDC hscreen = ::GetDC(NULL);         //设备上下文
    int nBitPerPixel = GetDeviceCaps(hscreen, BITSPIXEL);   //位宽
    int nWidth = GetDeviceCaps(hscreen, HORZRES);           //屏幕宽 
    int nHeight = GetDeviceCaps(hscreen, VERTRES);          //屏幕高
    screen.Create(nWidth, nHeight, nBitPerPixel);  
    BitBlt(screen.GetDC(),0,0,1920,1020, hscreen, 0, 0, SRCCOPY); //1020是跳过任务栏，该函数是位块传输
    ReleaseDC(NULL,hscreen);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0); //内存是可以调整大小的
    if (hMem == NULL) return -1;
    IStream* pStream = NULL;          //内存流,可以往里写东西的缓冲区
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    if (ret == S_OK)
    {
        screen.Save(pStream,Gdiplus::ImageFormatPNG);  //图片以png格式保存到内存流中
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);      //把指针设到开头去
        PBYTE pData = (PBYTE)GlobalLock(hMem);
        SIZE_T nSize = GlobalSize(hMem);
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstacne()->Send(pack);
        GlobalUnlock(hMem);
    }
    pStream->Release();           //写IStream的时候，顺便把这个写了，免得后面忘记了
    GlobalFree(hMem);
    screen.ReleaseDC();
    return 0;
}



//锁机要求：弹出对话框，这个对话框不能被退出掉
#include "LockDialog.h"
CLockDialog dlg;
unsigned threadid = 0;

//因为有一个无限循环，除非按下按键退出。如果不用线程来的话，会影响主线程的运行
unsigned _stdcall threadLockDlg(void* arg)
{
    TRACE("%s(%d):%d",__FUNCTION__, __LINE__, GetCurrentThreadId());
    dlg.Create(IDD_DIALOG_INFO, NULL);
    dlg.ShowWindow(SW_SHOW);
    //遮蔽后台窗口
    CRect rect;  //MFC中窗口的范围
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
    rect.bottom *= 1.03; //因为这个bottom不是全部的，所以我们稍微放大一点
    dlg.MoveWindow(rect);   //设置窗口范围
    //窗口置顶
    //dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    //限制鼠标的功能
    ShowCursor(false);  //不显示鼠标
    //隐藏任务栏
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"),NULL), SW_SHOW);

    //限制鼠标移动
    rect.left = 0;
    rect.right = 1;
    rect.top = 0;
    rect.bottom = 1;
    ClipCursor(rect);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN)
        {
            //TRACE函数可以用来跟踪实例
            TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
            //if (msg.wParam == 0x1B) //按下ESC退出
            if(msg.wParam == 0x41) //按下a键退出
            {
                break;
            }
        }
    }

    dlg.DestroyWindow();
    ShowCursor(true);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
    _endthreadex(0);   //线程启动了之后要记得退出
    return 0;  //?????????
}

int LockMachine()
{
    if ((dlg.m_hWnd != NULL) || (dlg.m_hWnd != INVALID_HANDLE_VALUE))
    {
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid=%d\r\n", threadid);
    }
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstacne()->Send(pack);
    return 0;
}

int UnlockMachine()
{
    PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);   //往指定的线程里发送信号
    CPacket pack(8, NULL, 0);
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
            int nCmd = 7;
            switch (nCmd)
            {
            case 1: //查看磁盘分区
                MakeDriverInfo();
                break;
            case 2://查看指定目录下的文件
                MakeDirectoryInfo();
                break;
            case 3://打开文件
                RunFile();
                break;
            case 4://下载文件
                DownloadFile();
                break;
            case 5://鼠标操作
                MouseEvent();
                break;
            case 6://发送屏幕截图
                SendScreen();
                break;
            case 7://锁机
                LockMachine();
                break;
            case 8://解锁
                UnlockMachine();
                break;
            }
            Sleep(5000);
            UnlockMachine();
            while (dlg.m_hWnd != NULL) {
                Sleep(10);
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
