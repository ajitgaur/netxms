// RuleList.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Constants
//

#define RULE_HEADER_HEIGHT    30
#define EMPTY_ROW_HEIGHT      20
#define CELL_TEXT_Y_MARGIN    5
#define CELL_TEXT_Y_SPACING   0
#define CELL_TEXT_X_MARGIN    8


/////////////////////////////////////////////////////////////////////////////
// Supplementary classes


//
// RL_Row
//

RL_Row::RL_Row(int iNumCells)
{
   int i;

   m_dwFlags = 0;
   m_iHeight = EMPTY_ROW_HEIGHT;
   m_iNumCells = iNumCells;
   m_ppCellList = (RL_Cell **)malloc(sizeof(RL_Cell *) * m_iNumCells);
   memset(m_ppCellList, 0, sizeof(RL_Cell *) * m_iNumCells);
   for(i = 0; i < m_iNumCells; i++)
      m_ppCellList[i] = new RL_Cell;
}

RL_Row::~RL_Row()
{
   int i;

   for(i = 0; i < m_iNumCells; i++)
      delete m_ppCellList[i];
   safe_free(m_ppCellList);
}

void RL_Row::InsertCell(int iPos)
{
   m_ppCellList = (RL_Cell **)realloc(m_ppCellList, sizeof(RL_Cell *) * (m_iNumCells + 1));
   if (iPos < m_iNumCells)
      memmove(&m_ppCellList[iPos + 1], &m_ppCellList[iPos], 
              sizeof(RL_Cell *) * (m_iNumCells - iPos));
   m_iNumCells++;
   m_ppCellList[iPos] = new RL_Cell;

}

void RL_Row::RecalcHeight(int iTextHeight)
{
   int i, iCellHeight;

   for(i = 0, m_iHeight = 0; i < m_iNumCells; i++)
   {
      iCellHeight = (m_ppCellList[i]->m_iNumLines == 0) ? EMPTY_ROW_HEIGHT :
                     (CELL_TEXT_Y_MARGIN * 2 + m_ppCellList[i]->m_iNumLines * iTextHeight +
                        (m_ppCellList[i]->m_iNumLines - 1) * CELL_TEXT_Y_SPACING);
      if (iCellHeight > m_iHeight)
         m_iHeight = iCellHeight;
   }
}

//
// RL_Cell
//

RL_Cell::RL_Cell()
{
   m_iNumLines = 0;
   m_pszTextList = NULL;
   m_phIconList = NULL;
}

RL_Cell::~RL_Cell()
{
   int i;

   for(i = 0; i < m_iNumLines; i++)
      safe_free(m_pszTextList[i]);
    safe_free(m_pszTextList);
    safe_free(m_phIconList);
}

RL_Cell::AddLine(char *pszText, HICON hIcon)
{
   int iPos;

   iPos = m_iNumLines++;
   m_pszTextList = (char **)realloc(m_pszTextList, sizeof(char *) * m_iNumLines);
   m_phIconList = (HICON *)realloc(m_phIconList, sizeof(HICON) * m_iNumLines);
   m_pszTextList[iPos] = strdup(pszText);
   m_phIconList[iPos] = hIcon;
   return iPos;
}

/////////////////////////////////////////////////////////////////////////////
// CRuleList

CRuleList::CRuleList()
{
   m_pColList = NULL;
   m_ppRowList = NULL;
   m_iNumColumns = 0;
   m_iNumRows = 0;
   m_iTotalWidth = 0;
   m_iTotalHeight = 0;

   m_bHScroll = FALSE;
   m_bVScroll = FALSE;

   // Default colors
   m_rgbActiveBkColor = RGB(255, 255, 255);
   m_rgbNormalBkColor = RGB(206, 206, 206);
   m_rgbTextColor = RGB(0, 0, 0);
   m_rgbTitleBkColor = RGB(0, 115, 230);
   m_rgbTitleTextColor = RGB(255, 255, 255);
}

CRuleList::~CRuleList()
{
   safe_free(m_pColList);
   safe_free(m_ppRowList);
}


BEGIN_MESSAGE_MAP(CRuleList, CWnd)
	//{{AFX_MSG_MAP(CRuleList)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
   ON_NOTIFY(HDN_BEGINTRACK, ID_HEADER_CTRL, OnHeaderBeginTrack)
   ON_NOTIFY(HDN_TRACK, ID_HEADER_CTRL, OnHeaderTrack)
   ON_NOTIFY(HDN_ENDTRACK, ID_HEADER_CTRL, OnHeaderEndTrack)
END_MESSAGE_MAP()


//
// Simplified Create() method
//

BOOL CRuleList::Create(DWORD dwStyle, const RECT &rect, CWnd *pwndParent, UINT nId)
{
   return CWnd::Create(NULL, "", dwStyle, rect, pwndParent, nId);
}


//
// Redefined PreCreateWindow()
//

BOOL CRuleList::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW),
                                         CreateSolidBrush(RGB(255, 255, 255)), NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CRuleList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   CDC *pDC;
	
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create font for elements
   m_fontNormal.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, "Verdana");
   pDC = GetDC();
   m_iTextHeight = pDC->GetTextExtent("gqhXQ|", 6).cy;
   ReleaseDC(pDC);

   // Create header
   GetClientRect(&rect);
   rect.bottom = RULE_HEADER_HEIGHT;
   m_wndHeader.SetColors(m_rgbTitleTextColor, m_rgbTitleBkColor);
   m_wndHeader.Create(WS_CHILD | WS_VISIBLE | CCS_NODIVIDER | HDS_HORZ, rect, this, ID_HEADER_CTRL);

   // Create scroll bars
   GetClientRect(&rect);
   rect.right -= GetSystemMetrics(SM_CXVSCROLL);
   m_wndHScroll.Create(SBS_HORZ | SBS_BOTTOMALIGN | WS_CHILD, rect, this, ID_HSCROLL);

   GetClientRect(&rect);
   rect.bottom -= GetSystemMetrics(SM_CYHSCROLL);
   m_wndVScroll.Create(SBS_VERT | SBS_RIGHTALIGN | WS_CHILD, rect, this, ID_VSCROLL);
	
   GetClientRect(&rect);
   m_wndSizeBox.Create(SBS_SIZEBOX | SBS_SIZEBOXBOTTOMRIGHTALIGN | WS_CHILD, rect, this, ID_SIZEBOX);

   return 0;
}


//
// WM_PAINT message handler
//

void CRuleList::OnPaint() 
{
   RECT rect, rcClient, rcText;
   int i, j, iLine;
   CFont *pOldFont;
   CBrush brTitle, brActive, brNormal, *pOldBrush;

	CPaintDC dc(this); // device context for painting

   // Create brushes
   brTitle.CreateSolidBrush(m_rgbTitleBkColor);
   brActive.CreateSolidBrush(m_rgbActiveBkColor);
   brNormal.CreateSolidBrush(m_rgbNormalBkColor);

   // Setup DC
   pOldFont = dc.SelectObject(&m_fontNormal);
   pOldBrush = dc.GetCurrentBrush();
	
   // Calculate drawing rect
   GetClientRect(&rcClient);
   rcClient.top = RULE_HEADER_HEIGHT;

   // Draw table
   for(i = 0, rect.bottom = rcClient.top; i < m_iNumRows; i++)
   {
      rect.top = rect.bottom;
      rect.bottom += m_ppRowList[i]->m_iHeight;
      for(j = 0, rect.right = rcClient.left; j < m_iNumColumns; j++)
      {
         // Draw background and bounding rectangle for the cell
         rect.left = rect.right;
         rect.right += m_pColList[j].m_iWidth;
         dc.SelectObject((m_pColList[j].m_dwFlags & CF_TITLE_COLOR) ? &brTitle : 
                           ((m_ppRowList[i]->m_dwFlags & RF_SELECTED) ? &brActive : &brNormal));
         dc.Rectangle(&rect);
         dc.Draw3dRect(&rect, RGB(128, 128, 128), RGB(255, 255, 255));

         // Prepare for drawing cell text
         memcpy(&rcText, &rect, sizeof(RECT));
         rcText.top += CELL_TEXT_Y_MARGIN;
         rcText.bottom = rcText.top + m_iTextHeight;
         rcText.left += CELL_TEXT_X_MARGIN;
         rcText.right -= CELL_TEXT_X_MARGIN;
         dc.SetTextColor((m_pColList[j].m_dwFlags & CF_TITLE_COLOR) ? m_rgbTitleTextColor : m_rgbTextColor);
         dc.SetBkColor((m_pColList[j].m_dwFlags & CF_TITLE_COLOR) ? m_rgbTitleBkColor :
                         ((m_ppRowList[i]->m_dwFlags & RF_SELECTED) ? m_rgbActiveBkColor : m_rgbNormalBkColor));

         // Walk through cell's items
         for(iLine = 0; iLine < m_ppRowList[i]->m_ppCellList[j]->m_iNumLines; iLine++)
         {
            dc.DrawText(m_ppRowList[i]->m_ppCellList[j]->m_pszTextList[iLine],
                        strlen(m_ppRowList[i]->m_ppCellList[j]->m_pszTextList[iLine]),
                        &rcText, DT_SINGLELINE | DT_VCENTER | 
                                    (m_pColList[j].m_dwFlags & CF_CENTER ? DT_CENTER : DT_LEFT));
            rcText.top = rcText.bottom + CELL_TEXT_Y_SPACING;
            rcText.bottom = rcText.top + m_iTextHeight;
         }
      }
   }

   // Draw frame around the table
   rect.left = rcClient.left;
   rect.top = 0;
   rect.bottom++;
   rect.right += 2;
   dc.DrawEdge(&rect, EDGE_RAISED, BF_BOTTOMRIGHT);

   // Cleanup
   dc.SelectObject(pOldFont);
   dc.SelectObject(pOldBrush);
}


//
// Insert new column
//

int CRuleList::InsertColumn(int iInsertBefore, char *pszText, int iWidth, DWORD dwFlags)
{
   int i, iNewCol;
   HDITEM hdi;

   // Calculate position for new item
   iNewCol = (iInsertBefore >= m_iNumColumns) ? m_iNumColumns : 
               (iInsertBefore >= 0 ? iInsertBefore : 0);

   // Insert new item into internal column list
   m_pColList = (RL_COLUMN *)realloc(m_pColList, sizeof(RL_COLUMN) * (m_iNumColumns + 1));
   if (iNewCol < m_iNumColumns)
      memmove(&m_pColList[iNewCol + 1], &m_pColList[iNewCol], 
              sizeof(RL_COLUMN) * (m_iNumColumns - iNewCol));
   m_iNumColumns++;
   m_pColList[iNewCol].m_iWidth = iWidth;
   strncpy(m_pColList[iNewCol].m_szName, pszText, MAX_COLUMN_NAME);
   m_pColList[iNewCol].m_dwFlags = dwFlags;

   // Insert new item into header control
   hdi.mask = HDI_TEXT | HDI_WIDTH;
   hdi.pszText = pszText;
   hdi.cxy = iWidth;
   m_wndHeader.InsertItem(iNewCol, &hdi);

   // Update all existing rows
   for(i = 0; i < m_iNumRows; i++)
      m_ppRowList[i]->InsertCell(iNewCol);
   
   RecalcWidth();
   if (UpdateScrollBars())
   {
      RECT rect;

      GetClientRect(&rect);
      AlignScrollBars(rect.right, rect.bottom);
   }
   m_wndHeader.SetWindowPos(NULL, 0, 0, m_iTotalWidth, RULE_HEADER_HEIGHT, SWP_NOZORDER);
   return iNewCol;
}


//
// Insert new row
//

int CRuleList::InsertRow(int iInsertBefore)
{
   int iNewRow;

   // Calculate position for new item
   iNewRow = (iInsertBefore >= m_iNumRows) ? m_iNumRows : 
               (iInsertBefore >= 0 ? iInsertBefore : 0);

   // Insert new item into internal row list
   m_ppRowList = (RL_Row **)realloc(m_ppRowList, sizeof(RL_Row *) * (m_iNumRows + 1));
   if (iNewRow < m_iNumRows)
      memmove(&m_ppRowList[iNewRow + 1], &m_ppRowList[iNewRow], 
              sizeof(RL_Row *) * (m_iNumRows - iNewRow));
   m_iNumRows++;
   m_ppRowList[iNewRow] = new RL_Row(m_iNumColumns);

   RecalcHeight();
   if (UpdateScrollBars())
   {
      RECT rect;

      GetClientRect(&rect);
      AlignScrollBars(rect.right, rect.bottom);
   }

   return iNewRow;
}


//
// Add new item to cell
//

int CRuleList::AddItem(int iRow, int iColumn, char *pszText, HICON hIcon)
{
   int iPos;

   if ((iRow < 0) || (iRow >= m_iNumRows) || (iColumn < 0) || (iColumn >= m_iNumColumns))
      return -1;

   iPos = m_ppRowList[iRow]->m_ppCellList[iColumn]->AddLine(pszText, hIcon);
   m_ppRowList[iRow]->RecalcHeight(m_iTextHeight);
   RecalcHeight();
   if (UpdateScrollBars())
   {
      RECT rect;

      GetClientRect(&rect);
      AlignScrollBars(rect.right, rect.bottom);
   }
   InvalidateRect(NULL);
   return iPos;
}


//
// Recalculate control's width
//

void CRuleList::RecalcWidth()
{
   int i;

   m_iTotalWidth = 0;
   for(i = 0; i < m_iNumColumns; i++)
      m_iTotalWidth += m_pColList[i].m_iWidth;
}


//
// Recalculate control's height
//

void CRuleList::RecalcHeight()
{
   int i;

   m_iTotalHeight = 0;
   for(i = 0; i < m_iNumRows; i++)
      m_iTotalHeight += m_ppRowList[i]->m_iHeight;
}


//
// Process HDN_BEGINTRACK notification from header control
//

void CRuleList::OnHeaderBeginTrack(NMHEADER *pHdrInfo, LRESULT *pResult)
{
   int x;

   x = GetColumnStartPos(pHdrInfo->iItem) + pHdrInfo->pitem->cxy;
   DrawShadowLine(x, RULE_HEADER_HEIGHT, x, m_iTotalHeight + RULE_HEADER_HEIGHT);
   *pResult = 0;
}


//
// Process HDN_TRACK notification from header control
//

void CRuleList::OnHeaderTrack(NMHEADER *pHdrInfo, LRESULT *pResult)
{
   int x, sx;

   // Draw line at old and new column width
   sx = GetColumnStartPos(pHdrInfo->iItem);
   x = sx + m_pColList[pHdrInfo->iItem].m_iWidth;
   DrawShadowLine(x, RULE_HEADER_HEIGHT, x, m_iTotalHeight + RULE_HEADER_HEIGHT);
   x = sx + pHdrInfo->pitem->cxy;
   DrawShadowLine(x, RULE_HEADER_HEIGHT, x, m_iTotalHeight + RULE_HEADER_HEIGHT);

   // New column width
   m_pColList[pHdrInfo->iItem].m_iWidth = pHdrInfo->pitem->cxy;

   *pResult = 0;
}


//
// Process HDN_ENDTRACK notification from header control
//

void CRuleList::OnHeaderEndTrack(NMHEADER *pHdrInfo, LRESULT *pResult)
{
   RecalcWidth();
   m_wndHeader.SetWindowPos(NULL, 0, 0, m_iTotalWidth, RULE_HEADER_HEIGHT, SWP_NOZORDER);
   if (UpdateScrollBars())
   {
      RECT rect;

      GetClientRect(&rect);
      AlignScrollBars(rect.right, rect.bottom);
   }
   InvalidateRect(NULL);
}


//
// Get start X position for column
//

int CRuleList::GetColumnStartPos(int iColumn)
{
   int i, x;

   for(i = 0, x = 0; i < iColumn; i++)
      x += m_pColList[i].m_iWidth;
   return x;
}


//
// Draw shadow line
//

void CRuleList::DrawShadowLine(int x1, int y1, int x2, int y2)
{
   CDC *pDC;
   int iDrawMode;

   pDC = GetDC();
   iDrawMode = pDC->SetROP2(R2_NOT);
   pDC->MoveTo(x1, y1);
   pDC->LineTo(x2, y2);
   pDC->SetROP2(iDrawMode);
   ReleaseDC(pDC);
}


//
// Clear selection flag from all rows
//

void CRuleList::ClearSelection(BOOL bRedraw)
{
   int i;

   for(i = 0; i < m_iNumRows; i++)
      m_ppRowList[i]->m_dwFlags &= ~RF_SELECTED;
   if (bRedraw)
      InvalidateRect(NULL, FALSE);
}


//
// Get row from given point
//

int CRuleList::RowFromPoint(int x, int y)
{
   int i, cy;

   if ((x >= m_iTotalWidth) || (y >= m_iTotalHeight + RULE_HEADER_HEIGHT))
      return - 1;

   for(i = 0, cy = RULE_HEADER_HEIGHT; i < m_iNumRows; i++)
   {
      if ((y >= cy) && (y < cy + m_ppRowList[i]->m_iHeight))
         return i;
      cy += m_ppRowList[i]->m_iHeight;
   }
   return -1;
}


//
// WM_LBUTTONDOWN message handler
//

void CRuleList::OnLButtonDown(UINT nFlags, CPoint point) 
{
   int iRow;

   iRow = RowFromPoint(point.x, point.y);
   if (iRow != -1)
   {
      if (nFlags & MK_CONTROL)
      {
         m_ppRowList[iRow]->m_dwFlags |= RF_SELECTED;
      }
      else if (nFlags & MK_SHIFT)
      {
      }
      else
      {
         ClearSelection(FALSE);
         m_ppRowList[iRow]->m_dwFlags |= RF_SELECTED;
      }
      InvalidateRect(NULL, FALSE);
   }
   else
   {
      ClearSelection();
   }
}


//
// WM_SIZE message handler
//

void CRuleList::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);

   UpdateScrollBars();
   AlignScrollBars(cx, cy);
}


//
// Update scroll bars state and position
//

BOOL CRuleList::UpdateScrollBars(void)
{
   RECT rect;
   BOOL bChanged = FALSE;

   GetClientRect(&rect);

   // Check if we need vertical scroll bar
   if ((rect.bottom < m_iTotalHeight + RULE_HEADER_HEIGHT + 
        (m_bHScroll ? GetSystemMetrics(SM_CYHSCROLL) : 0)) && (!m_bVScroll))
   {
      m_wndVScroll.ShowScrollBar(TRUE);
      m_bVScroll = TRUE;
      if (m_bHScroll)
         m_wndSizeBox.ShowScrollBar(TRUE);
      bChanged = TRUE;
   }
   else if ((rect.bottom >= m_iTotalHeight + RULE_HEADER_HEIGHT) && (m_bVScroll))
   {
      m_wndVScroll.ShowScrollBar(FALSE);
      m_bVScroll = FALSE;
      m_wndSizeBox.ShowScrollBar(FALSE);
      bChanged = TRUE;
   }

   // Check if we need horizontal scroll bar
   if ((rect.right < m_iTotalWidth + 
        (m_bVScroll ? GetSystemMetrics(SM_CXVSCROLL) : 0)) && (!m_bHScroll))
   {
      m_wndHScroll.ShowScrollBar(TRUE);
      m_bHScroll = TRUE;
      if (m_bVScroll)
         m_wndSizeBox.ShowScrollBar(TRUE);
      bChanged = TRUE;
   }
   else if ((rect.right >= m_iTotalWidth) && (m_bHScroll))
   {
      m_wndHScroll.ShowScrollBar(FALSE);
      m_bHScroll = FALSE;
      m_wndSizeBox.ShowScrollBar(FALSE);
      bChanged = TRUE;
   }

   return bChanged;
}


//
// Align scroll bars
//

void CRuleList::AlignScrollBars(int cx, int cy)
{
   CSize cs;

   if (m_bVScroll)
   {
      cs = GetWindowSize(&m_wndVScroll);
      m_wndVScroll.SetWindowPos(NULL, cx - cs.cx + 1, 0, cs.cx, 
                                m_bHScroll ? (cy - GetSystemMetrics(SM_CYHSCROLL)) : cy, 
                                SWP_NOZORDER);
   }

   if (m_bHScroll)
   {
      cs = GetWindowSize(&m_wndHScroll);
      m_wndHScroll.SetWindowPos(NULL, 0, cy - cs.cy + 1, 
                                m_bVScroll ? (cx - GetSystemMetrics(SM_CXVSCROLL)) : cx,
                                cs.cy, SWP_NOZORDER);
   }

   if (m_bVScroll && m_bHScroll)
   {
      cs = GetWindowSize(&m_wndSizeBox);
      m_wndSizeBox.SetWindowPos(NULL, cx - cs.cx, cy - cs.cy, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
   }
}
