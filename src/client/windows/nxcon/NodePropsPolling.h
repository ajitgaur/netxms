#if !defined(AFX_NODEPROPSPOLLING_H__EC1C3F19_71F1_43F3_9572_568534EA16FF__INCLUDED_)
#define AFX_NODEPROPSPOLLING_H__EC1C3F19_71F1_43F3_9572_568534EA16FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NodePropsPolling.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNodePropsPolling dialog

class CNodePropsPolling : public CPropertyPage
{
	DECLARE_DYNCREATE(CNodePropsPolling)

// Construction
public:
	DWORD m_dwPollerNode;
	CNodePropsPolling();
	~CNodePropsPolling();

// Dialog Data
	//{{AFX_DATA(CNodePropsPolling)
	enum { IDD = IDD_OBJECT_NODE_POLL };
	BOOL	m_bDisableAgent;
	BOOL	m_bDisableICMP;
	BOOL	m_bDisableSNMP;
	BOOL	m_bDisableConfPolls;
	BOOL	m_bDisableDataCollection;
	BOOL	m_bDisableRoutePolls;
	BOOL	m_bDisableStatusPolls;
	int		m_nUseIfXTable;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNodePropsPolling)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNodePropsPolling)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelectPoller();
	afx_msg void OnCheckDisableAgent();
	afx_msg void OnCheckDisableIcmp();
	afx_msg void OnCheckDisableSnmp();
	afx_msg void OnCheckDisableConfPolls();
	afx_msg void OnCheckDisableDatacoll();
	afx_msg void OnCheckDisableRoutePolls();
	afx_msg void OnCheckDisableStatusPoll();
	afx_msg void OnRadioDefault();
	afx_msg void OnRadioDisable();
	afx_msg void OnRadioEnable();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE *m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODEPROPSPOLLING_H__EC1C3F19_71F1_43F3_9572_568534EA16FF__INCLUDED_)