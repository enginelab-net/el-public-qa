#include "stdafx.h"
#include "ColoredStatic.h"

CColoredStatic::CColoredStatic() {
    m_text_color = GetSysColor(COLOR_WINDOWTEXT);
    m_back_color = GetSysColor(COLOR_3DFACE);
    m_created_bitmap = false;
}
CColoredStatic::~CColoredStatic() {
    if ( m_created_bitmap ) { m_bitmap.DeleteObject(); }
}
void CColoredStatic::PreSubclassWindow() {
    CStatic::PreSubclassWindow();
    // Remove the type flags and replace them with SS_OWNERDRAW
    ModifyStyle(SS_TYPEMASK, SS_OWNERDRAW);
}
void CColoredStatic::SetTextColor(COLORREF color) {
    if ( color != m_text_color ) {
        m_text_color = color;
        Invalidate(FALSE);
    }
}
void CColoredStatic::SetBkColor(COLORREF color) {
    if ( color != m_back_color ) {
        m_back_color = color;
        Invalidate(FALSE);
    }
}
void CColoredStatic::DoTextOutput(CDC *pDC, CString &str, RECT *r) {
    pDC->DrawText(str, r, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
}

BEGIN_MESSAGE_MAP(CColoredStatic, CStatic)
    ON_WM_DRAWITEM()
END_MESSAGE_MAP()

void CColoredStatic::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) {
    CDC* dc = CDC::FromHandle(lpDrawItemStruct->hDC);
    RECT r = lpDrawItemStruct->rcItem;
    CDC memdc;
    memdc.CreateCompatibleDC(dc);
    if ( m_created_bitmap ) {
        CSize s = m_bitmap.GetBitmapDimension();
        if (s.cx < r.right - r.left || s.cy < r.bottom - r.top) {
            m_bitmap.DeleteObject();
            m_created_bitmap = false;
        }
    }
    if ( !m_created_bitmap ) {
        // Create the off-screen bitmap
        // Make the size larger than necessary, in both directions, to avoid constantly recreating it
        m_bitmap.CreateCompatibleBitmap(dc, 30 + r.right - r.left, 30 + r.bottom - r.top);
        m_bitmap.SetBitmapDimension(30 + r.right - r.left, 30 + r.bottom - r.top);
        m_created_bitmap = true;
    }
    memdc.SelectObject(&m_bitmap);
    memdc.FillSolidRect(&r, m_back_color);
    memdc.SetBkMode(TRANSPARENT);
    memdc.SetTextColor(m_text_color);
    memdc.SelectObject(GetFont());
    CString str;
    GetWindowText(str);
    DoTextOutput(&memdc, str, &r);
    dc->BitBlt(r.left, r.top, r.right - r.left, r.bottom - r.top, &memdc, r.left, r.top, SRCCOPY);
}
