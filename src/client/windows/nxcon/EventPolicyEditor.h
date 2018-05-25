#if !defined(AFX_EVENTPOLICYEDITOR_H__E3E2B03F_812E_4289_8595_F36FCD0072F8__INCLUDED_)
#define AFX_EVENTPOLICYEDITOR_H__E3E2B03F_812E_4289_8595_F36FCD0072F8__INCLUDED_

#include "RuleList.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EventPolicyEditor.h : header file
//

#include "ObjectSelDlg.h"
#include "EventSelDlg.h"
#include "RuleCommentDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CEventPolicyEditor frame

class CEventPolicyEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CEventPolicyEditor)
protected:
	CEventPolicyEditor();           // protected constructor used by dynamic creation
   void Modify(void) { if (!m_bIsModified) ModifyTitle(); m_bIsModified = TRUE; }

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventPolicyEditor)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void DeleteSelectedRules();
	void CopySelectedRules();
	NXC_EPP* m_clipboard;
	void EditSituation(int row);
	void EditOptions(int row);
	int m_iImageOptionsBase;
	void EditScript(int iRow);
	void ModifyTitle(void);
	BOOL m_bIsModified;
	void AddAction(void);
	int m_iImageAny;
	CImageList *m_pImageList;
	void AddEvent(void);
	void EditComment(int iRow);
	void EditAlarm(int iRow);
	void EditSeverity(int iRow);
	void AddSource(void);
	void EnableSelectedRows(BOOL bEnable);
	void UpdateRow(int iRow);
	void InsertNewRule(int iInsertBefore);
	CRuleList m_wndRuleList;
	virtual ~CEventPolicyEditor();

	// Generated message map functions
	//{{AFX_MSG(CEventPolicyEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnPolicyInsertruleTop();
	afx_msg void OnPolicyInsertruleBottom();
	afx_msg void OnPolicyInsertruleAbove();
	afx_msg void OnPolicyInsertruleBelow();
	afx_msg void OnUpdatePolicyInsertruleBelow(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePolicyInsertruleAbove(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePolicyNegatecell(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePolicyDisablerule(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePolicyDeleterule(CCmdUI* pCmdUI);
	afx_msg void OnPolicyDisablerule();
	afx_msg void OnPolicyEnablerule();
	afx_msg void OnUpdatePolicyEnablerule(CCmdUI* pCmdUI);
	afx_msg void OnPolicyAdd();
	afx_msg void OnUpdatePolicyAdd(CCmdUI* pCmdUI);
	afx_msg void OnPolicyDelete();
	afx_msg void OnUpdatePolicyDelete(CCmdUI* pCmdUI);
	afx_msg void OnPolicyDeleterule();
	afx_msg void OnPolicyEdit();
	afx_msg void OnPolicySave();
	afx_msg void OnUpdatePolicySave(CCmdUI* pCmdUI);
	afx_msg void OnPolicyNegatecell();
	afx_msg void OnPolicyCopy();
	afx_msg void OnUpdatePolicyCopy(CCmdUI* pCmdUI);
	afx_msg void OnPolicyPaste();
	afx_msg void OnUpdatePolicyPaste(CCmdUI* pCmdUI);
	afx_msg void OnPolicyCut();
	afx_msg void OnUpdatePolicyCut(CCmdUI* pCmdUI);
	//}}AFX_MSG
   afx_msg void OnRuleListDblClk(LPNMHDR pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
private:
	int m_iImageActionsBase;
	int m_iImageSeverityBase;
	NXC_EPP *m_pEventPolicy;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTPOLICYEDITOR_H__E3E2B03F_812E_4289_8595_F36FCD0072F8__INCLUDED_)