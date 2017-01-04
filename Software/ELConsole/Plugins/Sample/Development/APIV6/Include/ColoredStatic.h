#pragma once

// CColored Static is a CStatic control that supports changing the foreground/background colors
// It does not support text rotation or any horizontal alignment other than left, but it does support multi-line word wrap.
class CColoredStatic : public CStatic {
public:
    CColoredStatic();
    virtual ~CColoredStatic();

    void SetTextColor(COLORREF color);
    void SetBkColor(COLORREF color);

protected:
    COLORREF m_text_color;
    COLORREF m_back_color;
    CBitmap m_bitmap;

    bool m_created_bitmap;

    void PreSubclassWindow();
    void DoTextOutput(CDC *pDC, CString &str, RECT *r);

    DECLARE_MESSAGE_MAP()

    // Message map functions
    afx_msg void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};
