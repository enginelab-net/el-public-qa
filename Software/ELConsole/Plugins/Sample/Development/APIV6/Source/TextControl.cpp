#include "stdafx.h"
#include "TextControl.h"
#include "Resource.h"

#define IDC_TEXT_EDIT 1000

pELControlWindow CreateText() {
    TextControl *tmp = NULL;
    try {
      tmp = new TextControl();
      if ( !tmp ) { return NULL; }
      return static_cast<pELControlWindow>(tmp);
    }
    catch (...) { }
    if ( tmp ) { delete tmp; }
    return NULL;
}

IMPLEMENT_DYNAMIC(TextControl, CDialog)

HWND TextControl::CreateHWND(HWND parent) {
    if ( ::IsWindow(m_hWnd) || CDialog::Create(IDD_DIALOG1, CWnd::FromHandle(parent)) == TRUE ) {
        return GetSafeHwnd();
    }
    return NULL;
}

bool TextControl::ValidateWindow() {
    if ( !m_flags[component::window_initialized] ) {
        m_flags.set(component::validate_window);
        return false;
    }
    m_flags.reset(component::validate_window);
    m_flags.reset(component::window_visible);
    Invalidate();

    m_label.ShowWindow(SW_HIDE);
    if ( !dll_host_info->connected ) {
        SetWindowText(_T("Not connected"));
        return false;
    }

    if ( m_channels.empty() ) {
        SetWindowText(_T("No channel"));
        return false;
    }

    pELChannelInfo info = ActiveChannel();
    if ( !info || !type(info, _info.types_allowed) ) {
        SetWindowText(_T("Invalid channel"));
        return false;
    }

    m_flags.set(component::window_visible);
    return true;
}

BEGIN_MESSAGE_MAP(TextControl, CDialog)
    ON_WM_CLOSE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()

    ON_WM_SIZE()
    ON_WM_SYSCOMMAND()
    ON_MESSAGE(WM_ENTERSIZEMOVE, OnEnterSizeMove)
    ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
    ON_WM_MOVING()
    ON_WM_SIZING()

    ON_WM_LBUTTONDBLCLK()
    ON_EN_KILLFOCUS(IDC_TEXT_EDIT, OnEditKillFocus)
END_MESSAGE_MAP()

BOOL TextControl::PreTranslateMessage(MSG *pMsg) {
    if ( pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN ) {
        if ( pMsg->wParam == VK_RETURN ) {
            if ( m_edit.IsWindowVisible() ) {
                // Apply the value
                CString str;
                m_edit.GetWindowText(str);
                OnEditKillFocus();

                float val = 0.0f;
                dll_gen->Scan(to_api(str).c_str(), &val);
                dll_host->WriteChannel(ActiveChannel(), val, m_flags[component::initial_value] ? TRUE : FALSE);
            }
            else {
                // Activate edit control
                OnLButtonDblClk(0, CPoint(0, 0));
            }

            return TRUE;
        }
        else if ( pMsg->wParam == VK_ESCAPE && m_edit.IsWindowVisible() ) {
            // Cancel editing
            OnEditKillFocus();
            return TRUE;
        }
    }

    // If hotkeys are enabled, give the host a chance to handle the hotkey, if handled early out.
    if ( EnableHotKeys() && dll_host->HandleKeyPressed(pMsg) ) { return TRUE; }
    return CDialog::PreTranslateMessage(pMsg);
}

BOOL TextControl::OnInitDialog() {
    CDialog::OnInitDialog();

    RECT r;
    GetClientRect(&r);

    GetSystemMenu(FALSE)->InsertMenu(1, MF_BYPOSITION, SC_INITIAL_VALUE, _T("&Initial value"));
    GetSystemMenu(FALSE)->InsertMenu(2, MF_BYPOSITION | MF_SEPARATOR, 0, _T(""));

    m_label.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTER, r, this);
    m_edit.Create(WS_CHILD | ES_AUTOHSCROLL, r, this, IDC_TEXT_EDIT);
    m_edit.ModifyStyleEx(0, WS_EX_CLIENTEDGE);

    m_label.SetTextColor(dll_theme->text_color_primary);
    m_label.SetBkColor(dll_theme->background_color);

    bool initial = m_flags[component::initial_value];
    m_flags.set();
    m_flags.set(component::initial_value, initial);
    m_flags.reset(component::received_WM_CLOSE);
    m_flags.reset(component::keep_elwindow);

    return TRUE;
}

void TextControl::OnPaint() {
    CPaintDC dc(this);

    RECT r;
    GetClientRect(&r);

    RECT rects[2];
    int section_height = (r.bottom - r.top) / 2;
    rects[1] = r;
    rects[1].top = rects[1].bottom - section_height;
    rects[0] = r;
    rects[0].bottom = rects[1].top;

    dc.FillSolidRect(&rects[0], dll_theme->background_color);
    CString str("This window acts like a control.  Notice that it exists in both the add control menu and in its own little plugin menu."
                "This control overwrites the default Text control.");
    dc.SetTextColor(dll_theme->text_color_primary);
    if ( m_font_handle ) {
        dc.SelectObject(m_font_handle);
    }
    dc.DrawText(str, &rects[0], DT_LEFT | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
}

void TextControl::OnLButtonDblClk(UINT flags, CPoint point) {
    if ( !m_label.IsWindowVisible() ) { return; }

    pELChannelInfo info = ActiveChannel();
    if ( !info || !::flags(info, CHANNELFLAGS_VALID) || !type(info, _info.types_allowed) ) { return; }

    // Copy the current label text to the edit control
    CString str;
    m_label.GetWindowText(str);
    m_edit.SetWindowText(str);

    m_label.ShowWindow(SW_HIDE);
    m_edit.ShowWindow(SW_SHOWNORMAL);

    m_edit.SetSel(0, -1);
    m_edit.SetFocus();
}

void TextControl::OnEditKillFocus() {
    m_label.ShowWindow(SW_SHOWNORMAL);
    m_edit.ShowWindow(SW_HIDE);
}

BOOL TextControl::OnEraseBkgnd(CDC *pDC) {
    if ( !m_flags[component::window_initialized] || !m_label.IsWindowVisible() ) { return CDialog::OnEraseBkgnd(pDC); }
    return TRUE;
}

void TextControl::RepositionControls(int client_width, int client_height) {
    if ( !m_flags[component::window_initialized] ) {
        m_flags.set(component::reposition_controls);
        return;
    }
    m_flags.reset(component::reposition_controls);

    CFont f;
    RECT r;
    GetClientRect(&r);

    RECT rects[2];
    int section_height = (r.bottom - r.top) / 2;
    rects[1] = r;
    rects[1].top = rects[1].bottom - section_height;
    rects[0] = r;
    rects[0].bottom = rects[1].top;

    f.CreateFont(section_height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, proper(m_control_window_font_name).c_str());
    m_label.SetFont(&f);
    m_edit.SetFont(&f);

    if ( m_control_font_handle ) { ::DeleteObject(m_control_font_handle); }

    m_control_font_handle = f.Detach();

    m_label.MoveWindow(rects[1].left, rects[1].top, client_width, section_height);
    m_edit.MoveWindow(rects[1].left, rects[1].top, client_width, section_height);

}

void TextControl::OnSysCommand(UINT nID, LPARAM lParam) {
    if ( dll_host->HandleSystemMenu(static_cast<pELControl>(this), nID, ActiveChannel()) ) { return; }

    // Windows has a problem with the last nibble, so no menu option so use the last 4 bits.
    switch (nID & 0xFFF0) {
    case SC_INITIAL_VALUE:
        m_flags.flip(component::initial_value);
        ChannelsChanged();
        return;
    }

    CDialog::OnSysCommand(nID, lParam);
}
