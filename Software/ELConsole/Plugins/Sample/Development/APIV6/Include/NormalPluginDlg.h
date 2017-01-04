#pragma once
/*
  NormalPluginDlg:
  This window is single instance, and contains serparate controls on its window.
*/
#include "helpers.h"
#include <string>
#include "ColoredStatic.h"

// The longest a float can be.
#define MAX_FIXED_BUFSIZE (1 + 1 + FLT_MAX_10_EXP + 1 + 9 + 1)

template <typename _Class>
struct control_item : public ELControl {
    control_item() {
        channel = NULL;
        index = 0;
        _info.types_allowed = CHANNELTYPE_SIMPLE;
        _info.min_allowed = 1;
        _info.max_allowed = 1;
        _info.flags = SELECTFLAGS_NONE;
    }
    STDMETHODIMP_(void) Update() { }
    STDMETHODIMP_(UINT) AddChannel(pELChannelInfo channel) { return 0; }
    STDMETHODIMP_(void) RemoveChannel(pELChannelInfo channel) { }
    STDMETHODIMP_(UINT) ChannelCount() { return 1;}
    STDMETHODIMP_(pELChannelInfo) Channel(UINT index) { return channel; }
    STDMETHODIMP_(BOOL) ChannelIndex(pELChannelInfo ch, UINT * out_index) { *out_index = 0; return ch == channel ? TRUE : FALSE; }
    STDMETHODIMP_(void) ChannelsChanged() { }
    STDMETHODIMP_(void) ChannelModified(MODIFYOPTION option, pELChannelInfo channel, INFOSELECT which) { }
    STDMETHODIMP_(const pELChannelSelectInfo) Info() { return &_info; }
    STDMETHODIMP_(pELWindow) Window() { return &_data->_window; }
    STDMETHODIMP_(void) Render() {
        if ( !channel || !flags(channel, CHANNELFLAGS_VALID) || !channel->primary.value ) { return; }
        COLORREF bkcolor = color_at(channel->primary.info, channel->primary.info.color_gradient, *channel->primary.value);
        label.SetBkColor(bkcolor);
        label.SetTextColor(brightness(bkcolor) < 0.5f ? dll_theme->text_light_color : dll_theme->text_dark_color);
        dll_gen->Print(&channel->primary.info.number_format, *channel->primary.value, &_data->_opt_value);
        label.SetWindowText(proper(_data->_opt_value->str).c_str());
    }
    STDMETHODIMP_(SYSMENUSUPPORT) SysMenuSupport() { return 0; /*SYSMENUSUPPORT_NONE;*/ }
    STDMETHODIMP_(pELChannelInfo) ActiveChannel() { return channel; }
    STDMETHODIMP_(void) UserMathExpression(const char * expression) { }
    STDMETHODIMP_(BOOL) ShowSelectChannelDlg() { return FALSE; }
    STDMETHODIMP_(BOOL) ShowPropertiesDlg() { return FALSE; }
    STDMETHODIMP_(void) GetChannelOptions(pELChannelInfo channel, pELStringArray * options, BOOL * release) { *options = NULL;  *release = FALSE; }
    STDMETHODIMP_(void) SetChannelOptions(pELChannelInfo channel, pELStringArray options) { }

    ELChannelSelectInfo _info;
    _Class *_data;
    std::string name;
    pELChannelInfo channel;
    size_t index;
    RECT rect;
    CColoredStatic label;
};

class NormalPluginDlg : public CDialog {
    DECLARE_DYNAMIC(NormalPluginDlg)
public:
    my_window<NormalPluginDlg> _window;
    ELString *_opt_value;

    NormalPluginDlg();
    ~NormalPluginDlg() {
        if ( _opt_value ) { dll_gen->FreeString(&_opt_value); }
    }

    BOOL Create(HWND parent);

    pELHost Host() { return dll_host; }
    ELWINDOWTYPE Type() { return ELWINDOWTYPE_NORMAL; }
    void Destroy() {
        dll_layout->WindowClosed(&_window);
        DestroyWindow();
    }
    HWND GetHWND() { return GetSafeHwnd(); }
    BOOL CanResize( RECT * new_rect) { return TRUE; }
    BOOL IsSavable() { return TRUE; }
    void BeginModal() { BeginModalState(); }
    void EndModal() { EndModalState(); }
    const char *TypeName() { return "NormalPlugIn_v6"; }
    void LoadSettingsFromString(const char *s) { }
    ELString *SaveSettingsToString() { return NULL; }
    void GetStrings(pELStringArray * strings, BOOL * release) { }
    void SetStrings(pELStringArray strings) { }
    void ThemeChanged(const pELThemeColors theme) { Invalidate(); }
    void HostStateChanged(const pELHostInfo host, HOSTSTATE states_changed) { }
    void ApplicationOptionsChanged() {
        const char *opt = NULL;
        if ( (opt = dll_host->OptionChanged("control_window_font_name")) != NULL ) {
            m_control_window_font_name = opt;
            CFont f;
            f.CreateFont(m_font_height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, proper(m_control_window_font_name).c_str());
            if ( m_font_handle ) {
                ::DeleteObject(m_font_handle);
            }
            m_font_handle = f.Detach();
            RECT r;
            GetClientRect(&r);
            RepositionControls(r.right - r.left, r.bottom - r.top);
            Invalidate();
        }
    }

    void RenderItem(size_t index);

    void DoDataExchange(CDataExchange *pDX);
    DECLARE_MESSAGE_MAP()
    BOOL PreTranslateMessage(MSG *pMsg) {
        if ( dll_host && dll_host->HandleKeyPressed(pMsg) ) { return TRUE; }
        return CDialog::PreTranslateMessage(pMsg);
    }
    BOOL OnInitDialog();
    void PostNcDestroy();
    bool OnExit() {
        dll_layout->WindowClosed(&_window);
        DestroyWindow();
        return false;
    }

    void OnPaint();
    BOOL OnEraseBkgnd(CDC *pDC);

    void RepositionControls(int clientw, int clienth);
    void OnSize(UINT type, int client_width, int client_height);
    LRESULT OnEnterSizeMove(WPARAM w, LPARAM l);
    LRESULT OnExitSizeMove(WPARAM w, LPARAM l);
    void OnMoving(UINT side, LPRECT new_rect);
    void OnSizing(UINT side, LPRECT new_rect);

private:
    std::string m_control_window_font_name;
    CBrush m_background_brush;
    HGDIOBJ m_font_handle;
    HGDIOBJ m_control_font_handle;
    LONG m_font_height;

    control_item<NormalPluginDlg> m_items[3];

    bool m_received_WM_CLOSE;

    // These overrides prevent ENTER and ESC from closing the window, but still allow normal closing via ALT + F4, etc...
    // Perform any additional cleanup in OnExit()
    void OnOK() {}
    void OnCancel() { if (m_received_WM_CLOSE && OnExit()) { CDialog::OnCancel(); } }
    void OnClose() { m_received_WM_CLOSE = true; CDialog::OnClose(); }
};
