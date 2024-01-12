
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_serv_address(0)
	, m_Port(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_serv_address);
	DDX_Text(pDX, IDC_EDIT_Port, m_Port);
	DDX_Control(pDX, IDC_TREE1, m_Tree);
	DDX_Control(pDX, IDC_LIST1, m_List);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_BN_CLICKED(IDC_BtnFileinfo, &CRemoteClientDlg::OnBnClickedBtnfileinfo)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, &CRemoteClientDlg::OnTvnSelchangedTree1)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE1, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE1, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CRemoteClientDlg::OnNMRClickListFile)
END_MESSAGE_MAP()


int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();
	CClientSocket* pclient = CClientSocket::getInstacne();
	//记得在控件里添加变量m_serv_address,m_Port
	bool ret = pclient->InitSocket(m_serv_address, atoi((LPCTSTR)m_Port));
	if (!ret)
	{
		AfxMessageBox("网络初始化失败！");
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	pclient->Send(pack);
	int cmd = pclient->DealCommand();
	TRACE("ack:%d\r\n", pclient->GetPacket().sCmd);
	if(bAutoClose)
	   pclient->CloseSocket();
	return cmd;
}



// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_Port = _T("9527");
	m_serv_address = 0x7F000001;
	UpdateData(FALSE); //把值赋给控件


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	// TODO: 在此添加控件通知处理程序代码
	//UpdateData，true把控件的值赋给变量，false把变量的值赋给控件
	//UpdateData();

	//CClientSocket* pclient = CClientSocket::getInstacne();

	////记得在控件里添加变量m_serv_address,m_Port
	//bool ret = pclient->InitSocket(m_serv_address, atoi((LPCTSTR)m_Port));
	//if (!ret)
	//{
	//	AfxMessageBox("网络初始化失败！");
	//	return;
	//}
	//CPacket pack(1981, NULL, 0);

	//pclient->Send(pack);
	//pclient->DealCommand();
	//TRACE("ack:%d\r\n", pclient->GetPacket().sCmd);
	//pclient->CloseSocket();
	
	SendCommandPacket(1981);

}


void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CRemoteClientDlg::OnBnClickedBtnfileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = SendCommandPacket(1);
	if (ret == -1)
	{
		AfxMessageBox(_T("命令处理失败！！"));
		return;
	}
	
	CClientSocket* pClient = CClientSocket::getInstacne();
	std::string drivers = pClient->GetPacket().strData;
	std::string dr;
	//如果没delete所有的item的话，每点一下这个按钮，tree就会扩张一下，这是不合理的
	m_Tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] == ',')
		{
			dr += ":";
			//目录下一定有子节点
			HTREEITEM htemp =  m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);  //TVI_ROOT是根目录，TVI_LAST是最后一个
			m_Tree.InsertItem("", htemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}

}


void CRemoteClientDlg::OnTvnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


//获取tree里结点的路径。
CString CRemoteClientDlg::GetPath(HTREEITEM htree)
{
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(htree);
		strRet = strTmp + "\\" + strRet;
		htree = m_Tree.GetParentItem(htree);    //拿父结点信息
	} while (htree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildItem(HTREEITEM htree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(htree);
		if (hSub != NULL) m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint pMouse;
	GetCursorPos(&pMouse);
	m_Tree.ScreenToClient(&pMouse);     //把屏幕鼠标转成客户端鼠标
	HTREEITEM htreeSelected = m_Tree.HitTest(pMouse, 0);
	if (htreeSelected == nullptr)
		return;    //鼠标什么都没点到，就退出
	//每有子节点，就说明是文件
	if (m_Tree.GetChildItem(htreeSelected) == NULL)
		return;


	//如果不删除子item，每双击一下，就会扩张，这是不合理的
	DeleteTreeChildItem(htreeSelected);
	m_List.DeleteAllItems();
	CString cPath = GetPath(htreeSelected);
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)cPath, cPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstacne()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstacne();
	while (pInfo->HasNext)
	{
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory)
		{
			//如果是.和..，就是当前目录和上一级目录，就会不断重复，所以我们应当排除这种情况
			if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..")
			{
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) break;
				pInfo = (PFILEINFO)CClientSocket::getInstacne()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM htemp = m_Tree.InsertItem(pInfo->szFileName, htreeSelected, TVI_LAST);
			//目录下一定有子节点
			m_Tree.InsertItem("", htemp, TVI_LAST);
		}
		else
		{
			//文件的话就把它放在List里
			m_List.InsertItem(0,pInfo->szFileName);
		}
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstacne()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL)
	{
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}
