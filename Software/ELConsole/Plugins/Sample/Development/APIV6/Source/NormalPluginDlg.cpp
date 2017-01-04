#include "stdafx.h"
#include "NormalPluginDlg.h"
#include "Resource.h"
#include <float.h>
#include <limits>

IMPLEMENT_DYNAMIC(NormalPluginDlg, CDialog)

NormalPluginDlg::NormalPluginDlg() {
    m_font_handle = NULL;
    m_control_font_handle = NULL;
    _opt_value = NULL;
    _window._data = this;
    m_items[0]._data = this;
    m_items[0].index = 0;
    m_items[0].channel = NULL;
    m_items[1]._data = this;
    m_items[1].index = 1;
    m_items[2]._data = this;
    m_items[2].index = 2;
}

BOOL NormalPluginDlg::Create(HWND parent) { return CDialog::Create(IDD_DIALOG1, CWnd::FromHandle(parent)); }

void NormalPluginDlg::DoDataExchange(CDataExchange *pDX) {
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(NormalPluginDlg, CDialog)
    ON_WM_CLOSE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()

    ON_WM_SIZE()
    ON_MESSAGE(WM_ENTERSIZEMOVE, OnEnterSizeMove)
    ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
    ON_WM_MOVING()
    ON_WM_SIZING()
END_MESSAGE_MAP()

BOOL NormalPluginDlg::OnInitDialog() {
    CDialog::OnInitDialog();

    m_font_handle = NULL;
    m_control_font_handle = NULL;
    _opt_value = NULL;

    CFont *tmpf = GetFont();
    LOGFONT lf;
    tmpf->GetLogFont(&lf);
    m_font_height = lf.lfHeight;
    // Make sure the layout knows about us for docking and such.
    dll_layout->RegisterWindow(&_window);

    // We display information about 3 channels.  Be sure to manually register each child control.
    RECT r;
    GetClientRect(&r);
    m_items[0].name = "NoOp_Channel_1";
    if ( dll_host->AllocateChannelInfo(m_items[0].name.c_str(), &m_items[0].channel, FALSE) == DS_SUCCESS ) {
        m_items[0].label.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTER, r, this);
        m_items[0].label.SetTextColor(dll_theme->text_color_primary);
        m_items[0].label.SetBkColor(dll_theme->background_color);
        dll_layout->RegisterControl(&m_items[0]);
    }
    m_items[1].name = "NoOp_Channel_2";
    if ( dll_host->AllocateChannelInfo(m_items[1].name.c_str(), &m_items[1].channel, FALSE) == DS_SUCCESS ) {
        m_items[1].label.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTER, r, this);
        m_items[1].label.SetTextColor(dll_theme->text_color_primary);
        m_items[1].label.SetBkColor(dll_theme->background_color);
        dll_layout->RegisterControl(&m_items[1]);
    }
    m_items[2].name = "NoOp_Channel_3";
    if ( dll_host->AllocateChannelInfo(m_items[2].name.c_str(), &m_items[2].channel, FALSE) == DS_SUCCESS ) {
        m_items[2].label.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTER, r, this);
        m_items[2].label.SetTextColor(dll_theme->text_color_primary);
        m_items[2].label.SetBkColor(dll_theme->background_color);
        dll_layout->RegisterControl(&m_items[2]);
    }

    if ( dll_host->OptionValue("control_window_font_name", &_opt_value) == DS_SUCCESS ) {
        m_control_window_font_name = _opt_value->str;
        CFont f;
        f.CreateFont(m_font_height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, proper(m_control_window_font_name).c_str());
        m_font_handle = f.Detach();
    }

    RepositionControls(r.right - r.left, r.bottom - r.top);

    SetWindowText(_T("Normal Plugin Window"));
    return TRUE;
}

void NormalPluginDlg::RenderItem(size_t index) {
    Invalidate(FALSE);
}

void NormalPluginDlg::OnPaint() {
    CPaintDC dc(this);

    RECT r;
    GetClientRect(&r);

    RECT rects[4];
    int section_height = (r.bottom - r.top) / 4;
    rects[3] = r;
    rects[3].top = rects[3].bottom - section_height;
    rects[2] = r;
    rects[2].bottom = rects[3].top;
    rects[2].top = rects[2].bottom - section_height;
    rects[1] = r;
    rects[1].bottom = rects[2].top;
    rects[1].top = rects[1].bottom - section_height;
    rects[0] = r;
    rects[0].bottom = rects[1].top;

    dc.FillSolidRect(&rects[0], dll_theme->background_color);
    CString str("This is a window that acts like a normal plugin window.  Notice that this window captures theme changes"\
                " and is dockable.  This window is unique, in that only one instance can exist, and if it becomes docked"\
                " to a page that page becomes active whenever this window is picked again via the menu.  Try it.");
    dc.SetTextColor(dll_theme->text_color_primary);
    if ( m_font_handle ) {
        dc.SelectObject(m_font_handle);
    }
    dc.DrawText(str, &rects[0], DT_LEFT | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);

    for (int i = 0; i < 3; ++i) {
        dc.FillSolidRect(&m_items[i].rect, dll_theme->background_color);
        dc.SetTextColor(dll_theme->text_color_primary);
        dc.SelectObject(m_items[i].label.GetFont()->operator HGDIOBJ());
        dc.DrawText(proper(m_items[i].name).c_str(), -1, &m_items[i].rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE);
    }
}

BOOL NormalPluginDlg::OnEraseBkgnd(CDC *pDC) { return TRUE; }

extern NormalPluginDlg *g_normal_plugin_dlg;
void NormalPluginDlg::PostNcDestroy() {
    CDialog::PostNcDestroy();
    delete this;
    g_normal_plugin_dlg = NULL;
}

void NormalPluginDlg::OnSize(UINT type, int clientw, int clienth) {
    CDialog::OnSize(type, clientw, clienth);
    if ( ::IsWindow(m_items[0].label.GetSafeHwnd()) &&
         ::IsWindow(m_items[1].label.GetSafeHwnd()) &&
         ::IsWindow(m_items[2].label.GetSafeHwnd()) ) {
        RepositionControls(clientw, clienth);
    }
    Invalidate();
}
void NormalPluginDlg::RepositionControls(int clientw, int clienth) {
    CFont f;

    RECT r;
    GetClientRect(&r);

    RECT rects[4];
    int section_height = (r.bottom - r.top) / 4;
    rects[3] = r;
    rects[3].top = rects[3].bottom - section_height;
    rects[2] = r;
    rects[2].bottom = rects[3].top;
    rects[2].top = rects[2].bottom - section_height;
    rects[1] = r;
    rects[1].bottom = rects[2].top;
    rects[1].top = rects[1].bottom - section_height;
    rects[0] = r;
    rects[0].bottom = rects[1].top;

    f.CreateFont(section_height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, proper(m_control_window_font_name).c_str());
    m_items[0].label.SetFont(&f);
    m_items[1].label.SetFont(&f);
    m_items[2].label.SetFont(&f);
    if ( m_control_font_handle != NULL ) {
        ::DeleteObject(m_control_font_handle);
    }
    m_control_font_handle = f.Detach();

    RECT half[2];
    for (int i = 0; i < 3; ++i) {
        half[0] = rects[i + 1];
        half[1] = rects[i + 1];
        half[0].right = half[0].left + ((r.right - r.left) / 2);
        half[1].left = half[0].right;
        m_items[i].rect = half[0];
        m_items[i].label.MoveWindow(half[1].left, half[1].top, half[1].right - half[1].left, half[1].bottom - half[1].top);
    }
}

LRESULT NormalPluginDlg::OnEnterSizeMove(WPARAM w, LPARAM l) {
    dll_layout->WindowMoveStart(&_window);
    return 0;
}
LRESULT NormalPluginDlg::OnExitSizeMove(WPARAM w, LPARAM l) {
    dll_layout->WindowMoveEnd(&_window);
    Invalidate();
    return 0;
}
void NormalPluginDlg::OnMoving(UINT side, LPRECT new_rect) { 
    dll_layout->WindowMoving(&_window);
}
void NormalPluginDlg::OnSizing(UINT side, LPRECT new_rect) {
    dll_layout->WindowMoving(&_window);
}
