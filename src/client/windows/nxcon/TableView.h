#if !defined(AFX_TABLEVIEW_H__3C012EC0_F4A4_4A42_B5DC_1FE384A4B130__INCLUDED_)
#define AFX_TABLEVIEW_H__3C012EC0_F4A4_4A42_B5DC_1FE384A4B130__INCLUDED_

#include "WaitView.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TableView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTableView frame

class CTableView : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CTableView)
protected:
	CTableView();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	void RequestData(void);
   CTableView(DWORD dwNodeId, DWORD dwToolId);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTableView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
   Table *m_pData;
	int m_iSortMode;
	int m_iSortDir;
	CImageList m_imageList;
	CWaitView m_wndWaitView;
	BOOL m_bIsBusy;
	int m_iStatusBarHeight;
	CStatusBarCtrl m_wndStatusBar;
	CListCtrl m_wndListCtrl;
	DWORD m_dwToolId;
	DWORD m_dwNodeId;
	virtual ~CTableView();

	// Generated message map functions
	//{{AFX_MSG(CTableView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewRefresh();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnClose();
	//}}AFX_MSG
   afx_msg LRESULT OnTableData(WPARAM wParam, LPARAM lParam);
   afx_msg void OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()

public:
	int CompareItems(int nItem1, int nItem2);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABLEVIEW_H__3C012EC0_F4A4_4A42_B5DC_1FE384A4B130__INCLUDED_)