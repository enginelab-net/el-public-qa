#pragma once

#include "helpers.h"
#include <string>

class WindowOnlyDlg : public CDialog, public ELWindow {
    DECLARE_DYNAMIC(WindowOnlyDlg)
public:
    WindowOnlyDlg();
    ~WindowOnlyDlg() {
        if ( _opt_value ) {
            dll_gen->FreeString(&_opt_value);
        }
    }

    BOOL Create(HWND parent);

    STDMETHODIMP_(pELHost) Host() { return dll_host; }
    STDMETHODIMP_(ELWINDOWTYPE) Type() { return ELWINDOWTYPE_NORMAL; }
    STDMETHODIMP_(void) Destroy() {
        dll_layout->WindowClosed(static_cast<pELWindow>(this));
        DestroyWindow();
    }
    STDMETHODIMP_(HWND) GetHWND() { return GetSafeHwnd(); }
    STDMETHODIMP_(BOOL) CanResize(RECT *r) { return TRUE; }
    STDMETHODIMP_(BOOL) IsSavable() { return TRUE; }
    STDMETHODIMP_(void) BeginModal() { BeginModalState(); }
    STDMETHODIMP_(void) EndModal() { EndModalState(); }
    STDMETHODIMP_(const char *) TypeName() { return "WindowOnly"; }
    STDMETHODIMP_(void) LoadSettingsFromString(const char *s) { }
    STDMETHODIMP_(pELString) SaveSettingsToString() { return NULL; }
    STDMETHODIMP_(void) GetStrings( pELStringArray * strings, BOOL * release) { }
    STDMETHODIMP_(void) SetStrings( pELStringArray strings) { }
    STDMETHODIMP_(void) ThemeChanged(const pELThemeColors theme) { Invalidate(); }
    STDMETHODIMP_(void) HostStateChanged( const pELHostInfo host, HOSTSTATE states_changed) { }
    STDMETHODIMP_(void) ApplicationOptionsChanged() {
        const char *opt = NULL;
        if ( (opt = dll_host->OptionChanged("control_window_font_name")) != NULL ) {
            m_control_window_font_name = opt;

            CFont f;
            f.CreateFont(m_font_height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, proper(m_control_window_font_name).c_str());
            if ( m_font_handle ) {
                ::DeleteObject(m_font_handle);
            }
            m_font_handle = f.Detach();
            Invalidate();
        }
    }

    void DoDataExchange(CDataExchange *pDX);
    DECLARE_MESSAGE_MAP()
    BOOL PreTranslateMessage(MSG *pMsg) {
        if ( dll_host && dll_host->HandleKeyPressed(pMsg) ) { return TRUE; }
        return CDialog::PreTranslateMessage(pMsg);
    }
    BOOL OnInitDialog();
    void PostNcDestroy();
    bool OnExit() {
        dll_layout->WindowClosed(static_cast<ELWindow*>(this));
        DestroyWindow();
        return false;
    }

    void OnPaint();
    BOOL OnEraseBkgnd(CDC *pDC);

    void OnSize(UINT type, int client_width, int client_height);
    void OnSysCommand(UINT nID, LPARAM lParam);
    LRESULT OnEnterSizeMove(WPARAM w, LPARAM l);
    LRESULT OnExitSizeMove(WPARAM w, LPARAM l);
    void OnMoving(UINT side, LPRECT new_rect);
    void OnSizing(UINT side, LPRECT new_rect);

private:
    std::string m_control_window_font_name;
    ELString *_opt_value;
    CBrush m_background_brush;
    HGDIOBJ m_font_handle;
    LONG m_font_height;

    bool m_received_WM_CLOSE;

    // These overrides prevent ENTER and ESC from closing the window, but still allow normal closing via ALT + F4, etc...
    // Perform any additional cleanup in OnExit()
    void OnOK() {}
    void OnCancel() { if (m_received_WM_CLOSE && OnExit()) { CDialog::OnCancel(); } }
    void OnClose() { m_received_WM_CLOSE = true; CDialog::OnClose(); }
};
