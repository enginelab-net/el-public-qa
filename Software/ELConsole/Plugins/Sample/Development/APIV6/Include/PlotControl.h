#pragma once
/*
 PlotControl:
 Holds the example of displaying log data in both gdi and d3d.
 The host does its best to optimize viewing of log data, so there are only ever 1000 datapoints to draw.
 There are methods to retrieve raw log data, but as the log grows in size those methods are not good to use during each render.
 Best to save those functions for when the user wants to wait a while for post processing or other statistics.
*/
#include "helpers.h"

#include <vector>
#include <bitset>
#include <algorithm>

pELControlWindow CreatePlotGDI();
pELControlWindow CreatePlotD3D();

class PlotControl : public CDialog, public ELControlWindow, public ELControl {
    DECLARE_DYNAMIC(PlotControl)
public:
    PlotControl(bool with_gdi);
    virtual ~PlotControl();

public: // ELControlWindow
    STDMETHODIMP_(pELHost) Host();
    STDMETHODIMP_(ELWINDOWTYPE) Type();
    STDMETHODIMP_(void) Destroy();
    STDMETHODIMP_(HWND) GetHWND();
    STDMETHODIMP_(BOOL) CanResize(RECT *);
    STDMETHODIMP_(BOOL) IsSavable();
    STDMETHODIMP_(void) BeginModal();
    STDMETHODIMP_(void) EndModal();
    STDMETHODIMP_(const char *) TypeName();
    STDMETHODIMP_(void) LoadSettingsFromString(const char *);
    STDMETHODIMP_(pELString) SaveSettingsToString(void);
    STDMETHODIMP_(void) GetStrings(pELStringArray *sa, BOOL *release);
    STDMETHODIMP_(void) SetStrings(pELStringArray sa);
    STDMETHODIMP_(void) ThemeChanged(const pELThemeColors);
    STDMETHODIMP_(void) HostStateChanged(const pELHostInfo, HOSTSTATE);
    STDMETHODIMP_(void) ApplicationOptionsChanged();
    STDMETHODIMP_(pELControl) Control();
    STDMETHODIMP_(void) DestroyHWND();
    STDMETHODIMP_(HWND) CreateHWND(HWND);

public: // ELControl
    STDMETHODIMP_(void) Update() { } // Do nothing, we do not process in the background
    STDMETHODIMP_(UINT) AddChannel(pELChannelInfo);
    STDMETHODIMP_(void) RemoveChannel(pELChannelInfo);
    STDMETHODIMP_(UINT) ChannelCount();
    STDMETHODIMP_(pELChannelInfo) Channel(UINT);
    STDMETHODIMP_(BOOL) ChannelIndex(pELChannelInfo, UINT *);
    STDMETHODIMP_(void) ChannelsChanged();
    STDMETHODIMP_(void) ChannelModified(MODIFYOPTION, pELChannelInfo, INFOSELECT);
    STDMETHODIMP_(const pELChannelSelectInfo) Info();
    STDMETHODIMP_(pELWindow) Window();
    STDMETHODIMP_(void) Render();
    STDMETHODIMP_(SYSMENUSUPPORT) SysMenuSupport();
    STDMETHODIMP_(pELChannelInfo) ActiveChannel();
    STDMETHODIMP_(void) UserMathExpression(const char *);
    STDMETHODIMP_(BOOL) ShowSelectChannelDlg();
    STDMETHODIMP_(BOOL) ShowPropertiesDlg();
    STDMETHODIMP_(void) GetChannelOptions(pELChannelInfo channel, pELStringArray * options, BOOL * release) { *options = NULL;  *release = FALSE; }
    STDMETHODIMP_(void) SetChannelOptions(pELChannelInfo channel, pELStringArray options) { }

    void RenderGDI(RECT &render_rect);
    void RenderD3D(RECT &render_rect);
public:
    bool ValidateWindow();

    LOGTIMESTAMP TimeAtPoint(int x);

public: // CDialog
    DECLARE_MESSAGE_MAP()
//    BOOL PreTranslateMessage(MSG *pMsg);
    BOOL OnInitDialog();
    void PostNcDestroy();
    bool OnExit();
    BOOL OnEraseBkgnd(CDC *pDC);
    void OnNcRButtonDown(UINT nHitTest, CPoint point);

    void OnLButtonDown(UINT flags, CPoint point);
    void OnMouseMove(UINT flags, CPoint point);
    void OnLButtonUp(UINT flags, CPoint point);

    void OnSize(UINT type, int client_width, int client_height);
    void OnSysCommand(UINT nID, LPARAM lParam);
    LRESULT OnEnterSizeMove(WPARAM w, LPARAM l);
    LRESULT OnExitSizeMove(WPARAM w, LPARAM l);
    void OnMoving(UINT side, LPRECT new_rect);
    void OnSizing(UINT side, LPRECT new_rect);

public:
    struct line_vertex {
        D3DXVECTOR3 pos;
        D3DCOLOR color;
    };
    
    static line_vertex _line_vertices[LOG_NUM_TIMESTEPS << 1];

    static POINT _line_points[LOG_NUM_TIMESTEPS << 1];

protected:
    // Internal flags.
    struct component {
        enum Enum : size_t {
            window_initialized,
            window_visible,
            keep_elwindow,
            received_WM_CLOSE,

            reposition_controls,

            validate_window,

            theme_changed,
            channels_changed,

            initial_value,
            mouse_dragging,

            no_data,

            _count
        };
    };
    std::bitset<component::_count> m_flags;
    std::string m_control_window_font_name;
    std::vector<pELChannelInfo> m_channels;

    std::string _stdstring;
    pELString _opt_value;
    int m_active_channel;
    D3DVIEWPORT9 m_viewport;

    bool m_draw_with_gdi;
    CBitmap _bitmap;
    bool _bitmap_created;
    CPoint m_mouse_drag_start_point;

private:
    // These overrides prevent ENTER and ESC from closing the window, but still allow normal closing via ALT + F4, etc...
    // Perform any additional cleanup in OnExit()
    void OnOK() {}
    void OnCancel() { if (m_flags[component::received_WM_CLOSE] && OnExit()) { CDialog::OnCancel(); } }
    void OnClose() { m_flags.set(component::received_WM_CLOSE); CDialog::OnClose(); }
};