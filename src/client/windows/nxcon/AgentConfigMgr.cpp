// AgentConfigMgr.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AgentConfigMgr.h"
#include "AgentCfgDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAgentConfigMgr

IMPLEMENT_DYNCREATE(CAgentConfigMgr, CMDIChildWnd)

CAgentConfigMgr::CAgentConfigMgr()
{
   m_iSortMode = theApp.GetProfileInt(_T("AgentConfigMgr"), _T("SortMode"), 1);
   m_iSortDir = theApp.GetProfileInt(_T("AgentConfigMgr"), _T("SortDir"), 1);
}

CAgentConfigMgr::~CAgentConfigMgr()
{
   theApp.WriteProfileInt(_T("AgentConfigMgr"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("AgentConfigMgr"), _T("SortDir"), m_iSortDir);
}


BEGIN_MESSAGE_MAP(CAgentConfigMgr, CMDIChildWnd)
	//{{AFX_MSG_MAP(CAgentConfigMgr)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_UPDATE_COMMAND_UI(ID_CONFIG_DELETE, OnUpdateConfigDelete)
	ON_UPDATE_COMMAND_UI(ID_CONFIG_MOVEUP, OnUpdateConfigMoveup)
	ON_UPDATE_COMMAND_UI(ID_CONFIG_MOVEDOWN, OnUpdateConfigMovedown)
	ON_UPDATE_COMMAND_UI(ID_CONFIG_EDIT, OnUpdateConfigEdit)
	ON_COMMAND(ID_CONFIG_NEW, OnConfigNew)
	ON_COMMAND(ID_CONFIG_EDIT, OnConfigEdit)
	ON_COMMAND(ID_CONFIG_DELETE, OnConfigDelete)
	ON_COMMAND(ID_CONFIG_MOVEUP, OnConfigMoveup)
	ON_COMMAND(ID_CONFIG_MOVEDOWN, OnConfigMovedown)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, ID_LIST_VIEW, OnListViewDblClk)
	ON_NOTIFY(LVN_COLUMNCLICK, ID_LIST_VIEW, OnListViewColumnClick)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAgentConfigMgr message handlers

BOOL CAgentConfigMgr::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, GetSysColorBrush(COLOR_WINDOW), 
                                         theApp.LoadIcon(IDI_CONFIGS));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CAgentConfigMgr::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   LVCOLUMN lvCol;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
	
   // Create list view control
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS,
                        rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 2, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 55);
   m_wndListCtrl.InsertColumn(1, _T("Seq."), LVCFMT_LEFT, 55);
   m_wndListCtrl.InsertColumn(2, _T("Name"), LVCFMT_LEFT, 400);
   LoadListCtrlColumns(m_wndListCtrl, _T("AgentConfigMgr"), _T("ListCtrl"));

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = m_iSortDir;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   theApp.OnViewCreate(VIEW_AGENT_CONFIG_MANAGER, this);
	
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CAgentConfigMgr::OnDestroy() 
{
   SaveListCtrlColumns(m_wndListCtrl, _T("AgentConfigMgr"), _T("ListCtrl"));
   theApp.OnViewDestroy(VIEW_AGENT_CONFIG_MANAGER, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SETFOCUS message handler
//

void CAgentConfigMgr::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();	
}


//
// WM_SIZE message handler
//

void CAgentConfigMgr::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_CONTEXTMENU message handler
//

void CAgentConfigMgr::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(22);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// UI update handlers
//

void CAgentConfigMgr::OnUpdateConfigEdit(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}

void CAgentConfigMgr::OnUpdateConfigDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CAgentConfigMgr::OnUpdateConfigMoveup(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_wndListCtrl.GetSelectedCount() == 1) && (m_iSortMode == 1));
}

void CAgentConfigMgr::OnUpdateConfigMovedown(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_wndListCtrl.GetSelectedCount() == 1) && (m_iSortMode == 1));
}


//
// Menu item "Config->New..." handler
//

void CAgentConfigMgr::OnConfigNew() 
{
   NXC_AGENT_CONFIG cfg;

   memset(&cfg, 0, sizeof(NXC_AGENT_CONFIG));
   EditConfig(&cfg);
}


//
// Menu item "Config->Edit..." handler
//

void CAgentConfigMgr::OnConfigEdit() 
{
   int nItem;
   NXC_AGENT_CONFIG cfg;
   DWORD dwResult;

   nItem = m_wndListCtrl.GetSelectionMark();
   if (nItem == -1)
      return;

   dwResult = DoRequestArg3(NXCOpenAgentConfig, g_hSession,
                            (void *)m_wndListCtrl.GetItemData(nItem),
                            &cfg, _T("Loading agent configuration..."));
   if (dwResult == RCC_SUCCESS)
   {
      EditConfig(&cfg);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Cannot load agent config: %s"));
   }
}


//
// Edit config
//

void CAgentConfigMgr::EditConfig(NXC_AGENT_CONFIG *pConfig)
{
   CAgentCfgDlg dlg;
   DWORD dwResult;
   TCHAR szBuffer[32];
   BOOL bNew = (pConfig->dwId == 0);
   int nItem;

   dlg.m_strName = pConfig->szName;
   dlg.m_strText = CHECK_NULL_EX(pConfig->pszText);
   if ((pConfig->dwId == 0) && (pConfig->pszFilter == NULL))
   {
      dlg.m_strFilter = _T("// Filtering script will receive the following arguments:\n")
                        _T("//\n//   $1 - agent's IP address\n")
                        _T("//   $2 - platform name (like \"windows-i386\")\n")
                        _T("//   $3 - agent's major version number\n")
                        _T("//   $4 - agent's minor version number\n")
                        _T("//   $5 - agent's release number\n\n");
   }
   else
   {
      dlg.m_strFilter = CHECK_NULL_EX(pConfig->pszFilter);
   }
   if (dlg.DoModal() == IDOK)
   {
      safe_free(pConfig->pszFilter);
      safe_free(pConfig->pszText);

      nx_strncpy(pConfig->szName, (LPCTSTR)dlg.m_strName, MAX_DB_STRING);
      pConfig->pszFilter = (TCHAR *)((LPCTSTR)dlg.m_strFilter);
      pConfig->pszText = (TCHAR *)((LPCTSTR)dlg.m_strText);

      dwResult = DoRequestArg2(NXCSaveAgentConfig, g_hSession, pConfig,
                               _T("Saving changes in agent configs..."));
      if (dwResult == RCC_SUCCESS)
      {
         if (bNew)
         {
            _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), pConfig->dwId);
            nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, -1);
            _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), pConfig->dwSequence);
            m_wndListCtrl.SetItemText(nItem, 1, szBuffer);
            m_wndListCtrl.SetItemData(nItem, pConfig->dwId);
            SelectListViewItem(&m_wndListCtrl, nItem);
         }
         else
         {
            nItem = m_wndListCtrl.GetSelectionMark();
         }
         m_wndListCtrl.SetItemText(nItem, 2, pConfig->szName);
      }
      else
      {
         theApp.ErrorBox(dwResult, _T("Cannot save changes in agent configs: %s"));
      }
   }
   else
   {
      safe_free(pConfig->pszFilter);
      safe_free(pConfig->pszText);
   }
}


//
// Delete config records
//

static DWORD DeleteConfigRecords(CListCtrl *pListCtrl)
{
   int nItem;
   DWORD dwResult = RCC_SUCCESS;

   nItem = pListCtrl->GetNextItem(-1, LVIS_SELECTED);
   while(nItem != -1)
   {
      dwResult = NXCDeleteAgentConfig(g_hSession, pListCtrl->GetItemData(nItem));
      if (dwResult != RCC_SUCCESS)
         break;
      pListCtrl->DeleteItem(nItem);
      nItem = pListCtrl->GetNextItem(-1, LVIS_SELECTED);
   }
   return dwResult;
}


//
// Menu item "Config->Delete" handler
//

void CAgentConfigMgr::OnConfigDelete() 
{
   DWORD dwResult;

   dwResult = DoRequestArg1(DeleteConfigRecords, &m_wndListCtrl, _T("Deleting configs..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot delete agent config record: %s"));
}


//
// Handler for WM_NOTIFY::NM_DBLCLK from ID_LIST_VIEW
//

void CAgentConfigMgr::OnListViewDblClk(NMHDR *pNMHDR, LRESULT *pResult)
{
   PostMessage(WM_COMMAND, ID_CONFIG_EDIT, 0);
}


//
// Item comparision procedure for sorting
//

static int CALLBACK ItemCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CAgentConfigMgr *pWnd = (CAgentConfigMgr *)lParamSort;
	TCHAR item1[256], item2[256];
   int iResult;

	pWnd->GetItemText((int)lParam1, pWnd->GetSortMode(), item1);
	pWnd->GetItemText((int)lParam2, pWnd->GetSortMode(), item2);

	switch(pWnd->GetSortMode())
	{
		case 0:	// ID
         iResult = COMPARE_NUMBERS(_tcstol(item1, NULL, 10), _tcstol(item2, NULL, 10));
			break;
		case 1:	// Sequence
         iResult = COMPARE_NUMBERS(_tcstol(item1, NULL, 10), _tcstol(item2, NULL, 10));
			break;
		case 2:	// Name
         iResult = _tcsicmp(item1, item2);
			break;
      default:
         iResult = 0;
         break;
	}

   return iResult * pWnd->GetSortDir();
}


//
// WM_NOTIFY::LVN_COLUMNCLICK message handler
//

void CAgentConfigMgr::OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
   LVCOLUMN lvCol;

   // Unmark old sorting column
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   // Change current sort mode and resort list
   if (m_iSortMode == ((LPNMLISTVIEW)pNMHDR)->iSubItem)
   {
      // Same column, change sort direction
      m_iSortDir = -m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = ((LPNMLISTVIEW)pNMHDR)->iSubItem;
   }
   m_wndListCtrl.SortItems(ItemCompareProc, (LPARAM)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir == 1) ? 0 : 1;
   m_wndListCtrl.SetColumn(((LPNMLISTVIEW)pNMHDR)->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// Handler for "Config->Move up" menu
//

void CAgentConfigMgr::OnConfigMoveup() 
{
   int nItem;

   nItem = m_wndListCtrl.GetSelectionMark();
   if (nItem > 0)
      SwapItems(nItem, nItem - 1);
}


//
// Handler for "Config->Move down" menu
//

void CAgentConfigMgr::OnConfigMovedown() 
{
   int nItem;

   nItem = m_wndListCtrl.GetSelectionMark();
   if (nItem < m_wndListCtrl.GetItemCount() - 1)
      SwapItems(nItem, nItem + 1);
}


//
// Swap sequence numbers for two items
//

void CAgentConfigMgr::SwapItems(int nItem1, int nItem2)
{
   DWORD dwResult, dwId1, dwId2;
   TCHAR szBuffer[32], szBuf1[256], szBuf2[256];
   int nItem;

   dwId1 = (DWORD)m_wndListCtrl.GetItemData(nItem1);
   dwId2 = (DWORD)m_wndListCtrl.GetItemData(nItem2);
   dwResult = DoRequestArg3(NXCSwapAgentConfigs, g_hSession, (void *)dwId1,
                            (void *)dwId2, _T("Changing sequence of agent config..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_wndListCtrl.GetItemText(nItem1, 1, szBuf1, 256);
      m_wndListCtrl.GetItemText(nItem2, 1, szBuf2, 256);
      m_wndListCtrl.SetItemText(nItem1, 1, szBuf2);
      if (m_iSortMode == 1)
      {
         m_wndListCtrl.GetItemText(nItem2, 2, szBuf2, 256);
         m_wndListCtrl.DeleteItem(nItem2);

         _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), dwId2);
         nItem = m_wndListCtrl.InsertItem(nItem1, szBuffer, -1);
         m_wndListCtrl.SetItemText(nItem, 1, szBuf1);
         m_wndListCtrl.SetItemText(nItem, 2, szBuf2);
         m_wndListCtrl.SetItemData(nItem, dwId2);
      }
      else
      {
         m_wndListCtrl.SetItemText(nItem2, 1, szBuf1);
      }
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Cannot change sequence number of agent config: %s"));
   }
}


//
// Get list item text
//

void CAgentConfigMgr::GetItemText(int item, int col, TCHAR *buffer)
{
	int index;
	LVFINDINFO lvfi;

	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = item;
	index = m_wndListCtrl.FindItem(&lvfi);
	if (index != -1)
	{
		m_wndListCtrl.GetItemText(index, col, buffer, 256);
	}
	else
	{
		*buffer = 0;
	}
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CAgentConfigMgr::OnViewRefresh() 
{
   DWORD i, dwResult, dwNumCfg;
   NXC_AGENT_CONFIG_INFO *pList;
   TCHAR szBuffer[32];
   int iItem;

   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg3(NXCGetAgentConfigList, g_hSession, &dwNumCfg, &pList,
                            _T("Loading list of agent configurations..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < dwNumCfg; i++)
      {
         _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), pList[i].dwId);
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, -1);
         if (iItem != -1)
         {
            m_wndListCtrl.SetItemData(iItem, pList[i].dwId);
            _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), pList[i].dwSequence);
            m_wndListCtrl.SetItemText(iItem, 1, szBuffer);
            m_wndListCtrl.SetItemText(iItem, 2, pList[i].szName);
         }
      }
      safe_free(pList);
	   m_wndListCtrl.SortItems(ItemCompareProc, (LPARAM)this);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Cannot load list of agent configurations: %s"));
   }
}