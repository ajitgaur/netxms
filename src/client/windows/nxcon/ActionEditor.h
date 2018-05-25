#if !defined(AFX_ACTIONEDITOR_H__CB1ECA05_6601_46FC_8E3D_F6130B0E2E49__INCLUDED_)
#define AFX_ACTIONEDITOR_H__CB1ECA05_6601_46FC_8E3D_F6130B0E2E49__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CActionEditor frame

class CActionEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CActionEditor)
protected:
	CActionEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	void OnActionUpdate(DWORD code, NXC_ACTION *action);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CActionEditor)
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SortList();
	int m_iSortDir;
	int m_iSortMode;
	int m_iSortImageBase;
	void ReplaceItem(int iItem, NXC_ACTION *pAction);
	int AddItem(NXC_ACTION *pAction);
	virtual ~CActionEditor();

	// Generated message map functions
	//{{AFX_MSG(CActionEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnViewRefresh();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnUpdateActionDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateActionProperties(CCmdUI* pCmdUI);
	afx_msg void OnUpdateActionRename(CCmdUI* pCmdUI);
	afx_msg void OnActionNew();
	afx_msg void OnActionProperties();
	afx_msg void OnActionDelete();
	//}}AFX_MSG
	afx_msg void OnListViewDoubleClick(NMHDR *pInfo, LRESULT *pResult);
   afx_msg void OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
private:
	CImageList m_imageList;
	CListCtrl m_wndListCtrl;

public:
   int GetSortMode(void) { return m_iSortMode; }
   int GetSortDir(void) { return m_iSortDir; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONEDITOR_H__CB1ECA05_6601_46FC_8E3D_F6130B0E2E49__INCLUDED_)