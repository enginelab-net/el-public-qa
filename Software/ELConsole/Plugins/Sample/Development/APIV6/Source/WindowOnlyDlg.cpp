#include "stdafx.h"
#include "WindowOnlyDlg.h"
#include "Resource.h"

IMPLEMENT_DYNAMIC(WindowOnlyDlg, CDialog)

WindowOnlyDlg::WindowOnlyDlg() : CDialog(IDD_DIALOG1) {
    m_font_handle = NULL;
    _opt_value = NULL;
}

BOOL WindowOnlyDlg::Create(HWND parent) { return CDialog::Create(IDD_DIALOG1, CWnd::FromHandle(parent)); }

void WindowOnlyDlg::DoDataExchange(CDataExchange *pDX) {
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(WindowOnlyDlg, CDialog)
    ON_WM_CLOSE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()

    ON_WM_SIZE()
    ON_MESSAGE(WM_ENTERSIZEMOVE, OnEnterSizeMove)
    ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
    ON_WM_MOVING()
    ON_WM_SIZING()
END_MESSAGE_MAP()

BOOL WindowOnlyDlg::OnInitDialog() {
    CDialog::OnInitDialog();

    CFont *tmpf = GetFont();
    LOGFONT lf;
    tmpf->GetLogFont(&lf);
    m_font_height = lf.lfHeight;
    dll_layout->RegisterWindow(static_cast<pELWindow>(this));

    RECT r;
    GetClientRect(&r);

    if ( dll_host->OptionValue("control_window_font_name", &_opt_value) == DS_SUCCESS ) {
        m_control_window_font_name = _opt_value->str;
        CFont f;
        f.CreateFont(m_font_height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, proper(m_control_window_font_name).c_str());
        m_font_handle = f.Detach();
    }

    SetWindowText(_T("Window Only"));
    return TRUE;
}

void WindowOnlyDlg::OnPaint() {
    CPaintDC dc(this);

    RECT r;
    GetClientRect(&r);

    dc.FillSolidRect(&r, dll_theme->background_color);
    CString str(_T("This window acts like any other information based window a plugin can have.  It is dockable and savable but can have multiple")\
                _T(" kind of like a help window that can be docked on each page (so multiple) or undocked and only one exists.  Notice that this")\
                _T(" window does not have any controls attached.  It could, but for examples sake, no controls exist on this window."));
    dc.SetTextColor(dll_theme->text_color_primary);
    if ( m_font_handle ) {
        dc.SelectObject(m_font_handle);
    }
    dc.DrawText(str, &r, DT_LEFT | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
}

BOOL WindowOnlyDlg::OnEraseBkgnd(CDC *pDC) { return TRUE; }

void WindowOnlyDlg::PostNcDestroy() {
    CDialog::PostNcDestroy();
    delete this;
}

void WindowOnlyDlg::OnSize(UINT type, int clientw, int clienth) {
    CDialog::OnSize(type, clientw, clienth);
    Invalidate();
}
LRESULT WindowOnlyDlg::OnEnterSizeMove(WPARAM w, LPARAM l) {
    dll_layout->WindowMoveStart(static_cast<pELWindow>(this));
    return 0;
}
LRESULT WindowOnlyDlg::OnExitSizeMove(WPARAM w, LPARAM l) {
    dll_layout->WindowMoveEnd(static_cast<pELWindow>(this));
    Invalidate();
    return 0;
}
void WindowOnlyDlg::OnMoving(UINT side, LPRECT new_rect) { 
    dll_layout->WindowMoving(static_cast<pELWindow>(this));
}
void WindowOnlyDlg::OnSizing(UINT side, LPRECT new_rect) {
    dll_layout->WindowMoving(static_cast<pELWindow>(this));
}
