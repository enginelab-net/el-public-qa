#include "stdafx.h"
#include "PlotControl.h"
#include "Resource.h"

// Unused margin between the window border and plot area
#define PLOT_MARGIN_PIXELS 6
// Minimum line thickness in pixels
#define MIN_LINE_THICKNESS 3.0f

// Z coordinates for ordering layers
#define BORDERS_Z 0.01f
#define LEGEND_Z 0.02f
#define CURSOR_Z 0.03f
#define FIRST_PLOTLINE_Z 0.05f
#define TIMELINES_Z 0.99f

#define CAMERA_Z -1.0f
#define NEAR_Z BORDERS_Z - CAMERA_Z
#define FAR_Z TIMELINES_Z - CAMERA_Z

inline void swap_argb_abgr(DWORD &c) {
    DWORD swap = c & 0x000000FF;

    c &= 0xFFFFFF00;
    c |= (c & 0x00FF0000) >> 16;
    c &= 0xFF00FFFF;
    c |= swap << 16;
}

inline DWORD swap_argb_abgr_const(const DWORD &c) {
    DWORD ret = c;

    swap_argb_abgr(ret);

    return ret;
}

inline void BeginRendering(LPDIRECT3DDEVICE9 device, RECT &render_rect, D3DVIEWPORT9 &viewport) {
    // Setup a "viewport" to restrict drawing to a smaller rectangle within the backbuffer
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = render_rect.right - render_rect.left;
    viewport.Height = render_rect.bottom - render_rect.top;
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;

    device->SetViewport(&viewport);
    // Clear the backbuffer
    device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, swap_argb_abgr_const(dll_theme->background_color), 1.0f, 0);
    device->BeginScene();
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
}

inline void EndRendering(LPDIRECT3DDEVICE9 device, HWND hndl, RECT &render_rect) {
    device->EndScene();
    // If the Present() call fails, the only thing that can be done is to reset the device, so treat all failures as D3DERR_DEVICELOST
    if ( device->Present(&render_rect, &render_rect, hndl, NULL) != S_OK ) { dll_host->D3DDeviceLost(); }
}

inline bool empty(const ELLogInfo &i) { return i.length == 0 || i.display_end_time - i.display_start_time < 100ui64; }

// A divide-and-conquer logarithm function for 64-bit values that returns just the number of decimal digits - 1
unsigned int log10_int64(unsigned __int64 x) {
    if ( x > 9999999999ui64 ) {
        if ( x > 999999999999999ui64 ) {
            if ( x > 99999999999999999ui64 ) {
                if ( x > 9999999999999999999ui64 ) {
                    return 19;
                }
                else {
                    if ( x > 999999999999999999ui64 ) {
                        return 18;
                    }
                    else {
                        return 17;
                    }
                }
            }
            else {
                if ( x > 9999999999999999ui64 ) {
                    return 16;
                }
                else {
                    return 15;
                }
            }
        }
        else {
            if ( x > 999999999999ui64 ) {
                if ( x > 99999999999999ui64 ) {
                    return 14;
                }
                else {
                    if ( x > 9999999999999ui64 ) {
                        return 13;
                    }
                    else {
                        return 12;
                    }
                }
            }
            else {
                if ( x > 99999999999ui64 ) {
                    return 11;
                }
                else {
                    return 10;
                }
            }
        }
    }
    else {
        if ( x > 99999ui64 ) {
            if ( x > 99999999ui64 ) {
                if ( x > 999999999ui64 ) {
                    return 9;
                }
                else {
                    return 8;
                }
            }
            else {
                if ( x > 9999999ui64 ) {
                    return 7;
                }
                else {
                    if ( x > 999999ui64 ) {
                        return 6;
                    }
                    else {
                        return 5;
                    }
                }
            }
        }
        else {
            if ( x > 999ui64 ) {
                if ( x > 9999ui64 )  {
                    return 4;
                }
                else {
                    return 3;
                }
            }
            else {
                if ( x > 99ui64 ) {
                    return 2;
                }
                else {
                    if ( x > 9ui64 ) {
                        return 1;
                    }
                    else {
                        return 0;
                    }
                }
            }
        }
    }
}

// Constant-time 10^x for 64-bit integers
// Undefined behavior if p > 19
unsigned __int64 pow10_int64(unsigned int x) {
    static unsigned __int64 powers[20] = { 1ui64, 10ui64, 100ui64, 1000ui64, 10000ui64, 100000ui64, 1000000ui64, 10000000ui64, 100000000ui64, 1000000000ui64, 10000000000ui64, 100000000000ui64,
        1000000000000ui64, 10000000000000ui64, 100000000000000ui64, 1000000000000000ui64, 10000000000000000ui64, 100000000000000000ui64, 1000000000000000000ui64, 10000000000000000000ui64 };

    return powers[x];
}

// Clamp a float to within the min/max and DO NOT check for NaN (not a number)
// Uses a templated function instead of a #define macro to avoid side effects
template <typename T>
inline T clamp(const T &val, const T &min, const T &max) {
    return (val < min ? min : (val > max ? max : val));
}

float find_interpolation_delta(float x_a, float x_b, float x) {
    // Don't allow division-by-zero
    if ( x_a == x_b ) {
        return x_a;
    }

    return (x - x_a) / (x_b - x_a);
}

float linear_interpolate(float a, float b, float delta) {
    return delta * b + (1.0f - delta) * a;
}

float linear_interpolate(float y_a, float y_b, float x_a, float x_b, float x) {
    return linear_interpolate(y_a, y_b, find_interpolation_delta(x_a, x_b, x));
}

PlotControl::line_vertex PlotControl::_line_vertices[LOG_NUM_TIMESTEPS << 1];
POINT PlotControl::_line_points[LOG_NUM_TIMESTEPS << 1];

pELControlWindow CreatePlotGDI() {
    PlotControl *tmp = NULL;
    try {
      tmp = new PlotControl(true);
      if ( !tmp ) { return NULL; }
      return static_cast<pELControlWindow>(tmp);
    }
    catch (...) { }
    if ( tmp ) { delete tmp; }
    return NULL;
}

pELControlWindow CreatePlotD3D() {
    PlotControl *tmp = NULL;
    try {
        tmp = new PlotControl(false);
        if ( !tmp ) { return NULL; }
        return static_cast<pELControlWindow>(tmp);
    }
    catch (...) { }
    if ( tmp ) { delete tmp; }
    return NULL;
}

IMPLEMENT_DYNAMIC(PlotControl, CDialog)

PlotControl::PlotControl(bool with_gdi) {
    m_active_channel = -1;
    m_draw_with_gdi = with_gdi;

    _bitmap_created = false;
    _opt_value = NULL;

    if ( dll_host->OptionValue("control_window_font_name", &_opt_value) == DS_SUCCESS ) {
        m_control_window_font_name = _opt_value->str;
    }

    m_flags.reset();
}

PlotControl::~PlotControl() {
    dll_gen->FreeString(&_opt_value);
    if ( _bitmap_created ) {
        _bitmap.DeleteObject();
        _bitmap_created = false;
    }
}

/*** ELControlWindow functions ***/
pELHost PlotControl::Host() { return dll_host; }
ELWINDOWTYPE PlotControl::Type() { return ELWINDOWTYPE_CONTROL; }
void PlotControl::Destroy() {
    dll_layout->WindowClosed(static_cast<pELWindow>(this));
    if ( m_hWnd ) {
        DestroyWindow();
    }
    else {
        delete this;
    }
}
HWND PlotControl::GetHWND() { return GetSafeHwnd(); }
BOOL PlotControl::CanResize(RECT *new_rect) {
    return new_rect->right - new_rect->left >= EL_MINIMUM_CONTOL_WINDOW_WIDTH && new_rect->bottom - new_rect->top >= EL_MINIMUM_CONTOL_WINDOW_HEIGHT;
}
// Handled by my_control_window implementation.
BOOL PlotControl::IsSavable() { return TRUE; }
void PlotControl::BeginModal() { BeginModalState(); }
void PlotControl::EndModal() { EndModalState(); }
const char *PlotControl::TypeName() { return m_draw_with_gdi ? "Plot GDI" : "Plot D3D"; }
void PlotControl::LoadSettingsFromString(const char *settings) { }
pELString PlotControl::SaveSettingsToString() { return NULL; }
void PlotControl::GetStrings(pELStringArray *sa, BOOL *release) { *release = FALSE; *sa = NULL; }
void PlotControl::SetStrings(pELStringArray sa) { }
void PlotControl::ThemeChanged(const pELThemeColors theme) {
    if ( !m_flags[component::window_initialized] ) {
        m_flags.set(component::theme_changed);
        return;
    }
    m_flags.reset(component::theme_changed);

    ChannelsChanged();
}
void PlotControl::HostStateChanged(const pELHostInfo host, HOSTSTATE states_changed) {
    ValidateWindow();
    m_flags.set(component::channels_changed);
}
void PlotControl::ApplicationOptionsChanged() {
    const char *opt = NULL;
    if ( (opt = dll_host->OptionChanged("control_window_font_name")) != NULL ) {
        m_control_window_font_name = opt;
    }
}
pELControl PlotControl::Control() { return static_cast<pELControl>(this); }
void PlotControl::DestroyHWND() {
    if ( !::IsWindow(m_hWnd) ) { return; }
    m_flags.set(component::keep_elwindow);
    DestroyWindow();
}
HWND PlotControl::CreateHWND(HWND parent) {
    if ( ::IsWindow(m_hWnd) || CDialog::Create(IDD_DIALOG1, CWnd::FromHandle(parent)) == TRUE ) {
        return GetSafeHwnd();
    }
    return NULL;
}

/*** ELControl functions ***/
// The normal Plot control on the Host supports multiple channels, this example (GDI or D3D) supports only a single channel.
UINT PlotControl::AddChannel(pELChannelInfo channel) {
    try {
        m_channels.push_back(channel);
        if ( m_channels.size() == 1 ) { m_active_channel = 0; }
        return (UINT)m_channels.size() - 1;
    }
    catch (...) { }
    return (UINT)-1;
}
void PlotControl::RemoveChannel(pELChannelInfo channel) {
    m_channels.erase(std::find(m_channels.begin(), m_channels.end(), channel));
    if ( m_channels.empty() ) {
        m_active_channel = -1;
    }
}
UINT PlotControl::ChannelCount() { return (UINT)m_channels.size(); }
pELChannelInfo PlotControl::Channel(UINT index) {
    if ( index < m_channels.size() ) { return m_channels[index]; }
    return NULL;
}
BOOL PlotControl::ChannelIndex(pELChannelInfo channel, UINT *index) {
    for (size_t i = 0, end = m_channels.size(); i < end; ++i) {
        if ( m_channels[i] == channel ) {
            *index = (UINT)i;
            return TRUE;
        }
    }

    return FALSE;
}

bool PlotControl::ValidateWindow() {
    if ( !m_flags[component::window_initialized] ) {
        m_flags.set(component::validate_window);
        return false;
    }
    m_flags.reset(component::validate_window);
    m_flags.reset(component::window_visible);
    Invalidate();

    if ( !dll_host_info->connected ) {
        SetWindowText(_T("Not connected"));
        return false;
    }

    if ( m_channels.empty() ) {
        SetWindowText(_T("No channel"));
        return false;
    }

    m_flags.set(component::window_visible);
    return true;
}

void PlotControl::ChannelsChanged() {
    if ( !m_flags[component::window_initialized] ) {
        m_flags.set(component::channels_changed);
        return;
    }
    m_flags.reset(component::channels_changed);

    if ( !ValidateWindow() ) { return; }

    pELChannelInfo info = ActiveChannel();

    m_flags.reset(component::no_data);
    if ( !info || !flags(info, CHANNELFLAGS_VALID) ) {
        SetWindowText(_T("Invalid Channel"));
    }
    else {
        adjust_name(*info, INFOSELECT_PRIMARY, _opt_value, _stdstring, false);

        if ( !dll_host_info->logging && dll_log->length == 0 ) {
            _stdstring += " [No Log Data]";
        }
        SetWindowText(proper(_stdstring).c_str());
    }
}
void PlotControl::ChannelModified(MODIFYOPTION option, pELChannelInfo channel, INFOSELECT which) {
    switch (option) {
    case MODIFYOPTION_SELECTION: 
    case MODIFYOPTION_FOLLOWECU: 
    case MODIFYOPTION_FORMATTING:
    case MODIFYOPTION_DATA:
        return;

    case MODIFYOPTION_COLOR:
    case MODIFYOPTION_VALIDITY:
    case MODIFYOPTION_UNIT:
    case MODIFYOPTION_LOGGED:
        if ( which & INFOSELECT_PRIMARY ) {
            m_flags.set(component::channels_changed);
            if ( !dll_host_info->connected ) { ChannelsChanged(); }
        }
        break;
    }
}

// The plots allow for all channel types, a single channel, and no multiple select options.
ELChannelSelectInfo _plot_channel_select_info = {CHANNELTYPE_ALL, 1, 1, SELECTFLAGS_NONE};
const pELChannelSelectInfo PlotControl::Info() { return &_plot_channel_select_info; }
pELWindow PlotControl::Window() { return this; }
void PlotControl::Render() {
    if ( !m_flags[component::window_initialized] || !m_flags[component::window_visible] ) { return; }

    if ( m_flags[component::theme_changed] ) { ThemeChanged((const pELThemeColors)dll_theme); }
    if ( m_flags[component::validate_window] || m_flags[component::channels_changed] ) { ChannelsChanged(); }
    if ( m_channels.empty() ) { return; }

    RECT r;
    GetClientRect(&r);

    if ( m_draw_with_gdi ) {
        RenderGDI(r);
    }
    else {
        RenderD3D(r);
    }
}

// Plot supports user selection of channel and they do display log data.
SYSMENUSUPPORT PlotControl::SysMenuSupport() { return SYSMENUSUPPORT_SELECT_CHANNEL | SYSMENUSUPPORT_DISPLAYS_LOG; }
pELChannelInfo PlotControl::ActiveChannel() {
    if ( m_active_channel >= 0 && (size_t)m_active_channel < m_channels.size() ) { return m_channels[m_active_channel]; }
    return NULL;
}
void PlotControl::UserMathExpression(const char *expression) { }
BOOL PlotControl::ShowSelectChannelDlg() { return dll_dialogs->DisplayChannelList(static_cast<pELControl>(this), INFOSELECT_PRIMARY, 0, NULL) == DS_SUCCESS; }
BOOL PlotControl::ShowPropertiesDlg() { return TRUE; }

// Only called if the plot is drawn is good ole windows gdi
void PlotControl::RenderGDI(RECT &render_rect) {
    const ELLogInfo &log = *dll_log;
    const ELThemeColors &theme = *dll_theme;

    RECT r = render_rect;
    CDC *dc = GetDC();
    CDC mem;
    mem.CreateCompatibleDC(GetDC());
    if (_bitmap_created) {
        CSize sz = _bitmap.GetBitmapDimension();
        if (sz.cx < r.right - r.left || sz.cy < r.bottom - r.top) {
            _bitmap.DeleteObject();
            _bitmap_created = false;
        }
    }
    
    if (!_bitmap_created) {
        _bitmap.CreateCompatibleBitmap(dc, 30 + (r.right - r.left), 30 + (r.bottom - r.top));
        _bitmap_created = true;
    }
    mem.SelectObject(&_bitmap);

    mem.FillSolidRect(&r, dll_theme->background_color);
    mem.SetBkMode(TRANSPARENT);

    r.left += PLOT_MARGIN_PIXELS;
    r.top += PLOT_MARGIN_PIXELS;
    r.right -= PLOT_MARGIN_PIXELS;
    r.bottom -= PLOT_MARGIN_PIXELS;
    
    CBrush border(dll_theme->grid_lines_color);
    CBrush *old_border = mem.SelectObject(&border);
    mem.FrameRect(&r, mem.GetCurrentBrush());
    mem.SelectObject(old_border);

    LOGTIMESTAMP span = log.display_end_time - log.display_start_time;

    pELChannelInfo channel = ActiveChannel();
    if ( !channel || empty(log) || !logged(channel) ) {
        return;
    }

    {
        mem.SetDCPenColor(RGB(255, 0, 0));
        for (size_t t = 0; t < LOG_NUM_TIMESTEPS; ++t) {
            size_t t_doubled = t << 1;
            POINT &p1 = _line_points[t_doubled];
            POINT &p2 = _line_points[t_doubled+1];
            p1.x = r.left + ((r.right - r.left) * ((float)t / (float)LOG_NUM_TIMESTEPS) + 0.5f);
            p2.x = p1.x;

            p1.y = linear_interpolate(r.bottom, r.top, 0.0f, 1.0f, channel->log.max_array[t]) + 0.5f;
            p2.y = linear_interpolate(r.bottom, r.top, 0.0f, 1.0f, channel->log.min_array[t]) + 0.5f;
        }
        CPen line(PS_SOLID, 0, RGB(255,0,0));
        CPen *old = mem.SelectObject(&line);
        mem.Polyline(_line_points, LOG_NUM_TIMESTEPS << 1);
        mem.SelectObject(old);
    }

    if ( log.display_cursor > log.display_start_time && log.display_cursor < log.display_end_time ) {
        CBrush cursor(dll_theme->indicator_color);
        CBrush *old = mem.SelectObject(&cursor);
        int cursor_x = r.left + ((r.right - r.left) * ((float)(log.display_cursor - log.display_start_time) / (float)span));
        mem.MoveTo(cursor_x, r.top);
        mem.LineTo(cursor_x, r.bottom);
        mem.SelectObject(old);
    }

    dc->BitBlt(render_rect.left, render_rect.top, render_rect.right - render_rect.left, render_rect.bottom - render_rect.top, &mem, render_rect.left, render_rect.top, SRCCOPY);
}

// Called only if the plot if drawn using direct x.
void PlotControl::RenderD3D(RECT &r) {
    LPDIRECT3DDEVICE9 device = dll_3d->device;
    const ELLogInfo &log = *dll_log;
    const ELThemeColors &theme = *dll_theme;
    BeginRendering(device, r, m_viewport);

    D3DXMATRIX transform;

    // World
    D3DXMatrixIdentity(&transform);
    device->SetTransform(D3DTS_WORLD, &transform);

    // Projection
    float w_pixels = (float)(r.right - r.left);
    float h_pixels = (float)(r.bottom - r.top);

    float margin = PLOT_MARGIN_PIXELS / (w_pixels - 2.0f * PLOT_MARGIN_PIXELS);
    float projection_w = 1.0f + 2.0f * margin;
    float projection_h = projection_w * h_pixels / w_pixels;

    float plot_area_top = projection_h - 2.0f * margin;

    float single_pixel = projection_w / w_pixels;
    float min_thickness = single_pixel * MIN_LINE_THICKNESS;

    D3DXMatrixOrthoLH(&transform, projection_w, projection_h, NEAR_Z, FAR_Z);
    device->SetTransform(D3DTS_PROJECTION, &transform);

    // View
    // Set the view so that the y coordinates go from zero (bottom) to projection_h
    D3DXMatrixLookAtLH(&transform, &D3DXVECTOR3(0.5f, projection_h / 2.0f - margin, CAMERA_Z), &D3DXVECTOR3(0.5f, projection_h / 2.0f - margin, (NEAR_Z + FAR_Z) / 2.0f), &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
    device->SetTransform(D3DTS_VIEW, &transform);

    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);

    // Draw borders
    D3DCOLOR grid_color = swap_argb_abgr_const(theme.grid_lines_color);
    line_vertex border_vertices[4] = {
        {D3DXVECTOR3(0.0f, 0.0f, BORDERS_Z), grid_color},
        {D3DXVECTOR3(1.0f, 0.0f, BORDERS_Z), grid_color},
        {D3DXVECTOR3(1.0f, plot_area_top, BORDERS_Z), grid_color},
        {D3DXVECTOR3(0.0f, plot_area_top, BORDERS_Z), grid_color} 
    };
    WORD border_indices[5] = { 0, 1, 2, 3, 0 };

    device->DrawIndexedPrimitiveUP(D3DPT_LINESTRIP, 0, 4, 4, border_indices, D3DFMT_INDEX16, border_vertices, sizeof(line_vertex));

    // Only the difference between start and end should be used as a float because these are 64-bit values and precision is lost when casting to float
    LOGTIMESTAMP span = log.display_end_time - log.display_start_time;

    pELChannelInfo channel = ActiveChannel();
    // Don't attempt to draw if the time span is less than 100 ticks (10us) because the interpolation below might not work correctly
    if ( !channel || empty(log) || !logged(channel) ) {
        EndRendering(device, GetSafeHwnd(), r);
        return;
    }

    // Draw time divisions
    line_vertex time_division_vertices[2];

    unsigned int log_span = log10_int64(span);
    LOGTIMESTAMP pow = pow10_int64(log_span);

    float delta = clamp((float)(span - pow) / (float)(pow * 9ui64), 0.0f, 1.0f);

    LOGTIMESTAMP first_major = (log.display_start_time / pow) * pow;
    LOGTIMESTAMP last_major = (log.display_end_time / pow) * pow;

    UINT line_count = 0;

    LOGTIMESTAMP pow_over_10 = pow / 10ui64;

    // Draw the cursor
    float cursor_x = -1.0f;
    D3DCOLOR clr = swap_argb_abgr_const(theme.indicator_color);

    if ( log.display_cursor > log.display_start_time && log.display_cursor < log.display_end_time ) {
        cursor_x = (float)(log.display_cursor - log.display_start_time) / (float)span;
        {
            line_vertex &lv = time_division_vertices[line_count++];
            lv.pos.x = cursor_x;
            lv.pos.y = -margin;
            lv.pos.z = CURSOR_Z;
            lv.color = clr;
        }
        {
            line_vertex &lv = time_division_vertices[line_count++];
            lv.pos.x = cursor_x;
            lv.pos.y = projection_h - margin;
            lv.pos.z = CURSOR_Z;
            lv.color = clr;
        }
    }

    device->DrawPrimitiveUP(D3DPT_LINELIST, line_count / 2, (void *)time_division_vertices, sizeof(line_vertex));

    // Draw the channel plot data
    float z = FIRST_PLOTLINE_Z;

    {
        float bottom = 0.0f, top = 1.0f;

        for (size_t t = 0; t < LOG_NUM_TIMESTEPS; ++t) {
            size_t t_doubled = t << 1;
            float x = (float)t / (float)LOG_NUM_TIMESTEPS;
            line_vertex &lower_vert = _line_vertices[t_doubled];
            line_vertex &upper_vert = _line_vertices[t_doubled + 1];
            lower_vert.pos.x = x;
            upper_vert.pos.x = x;

            lower_vert.pos.y = linear_interpolate(0.0f, plot_area_top, bottom, top, channel->log.min_array[t]);
            upper_vert.pos.y = linear_interpolate(0.0f, plot_area_top, bottom, top, channel->log.max_array[t]);

            lower_vert.pos.z = z;
            upper_vert.pos.z = z;

            lower_vert.color = 0x00FF00;
            upper_vert.color = lower_vert.color;

            // Make the line thick enough vertically
            float thickness = upper_vert.pos.y - lower_vert.pos.y;

            if ( thickness < min_thickness ) {
                float adjustment = (min_thickness - thickness) / 2.0f;

                upper_vert.pos.y += adjustment;
                lower_vert.pos.y -= adjustment;
            }
        }

        // Make the line thick enough horizontally
        for (size_t t = LOG_NUM_TIMESTEPS - 1; t > 0; --t) {
            size_t t_doubled = t << 1;
            line_vertex &lower_vert = _line_vertices[t_doubled];
            line_vertex &upper_vert = _line_vertices[t_doubled + 1];
            line_vertex &prev_lower_vert = _line_vertices[t_doubled - 2];
            line_vertex &prev_upper_vert = _line_vertices[t_doubled - 1];

            if ( lower_vert.pos.y > prev_upper_vert.pos.y ) {
                lower_vert.pos.y = prev_lower_vert.pos.y;
            }
            else if ( upper_vert.pos.y < prev_lower_vert.pos.y ) {
                upper_vert.pos.y = prev_upper_vert.pos.y;
            }
        }

        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, (LOG_NUM_TIMESTEPS - 1) * 2, (void *)_line_vertices, sizeof(line_vertex));

        z += 0.01f;
    }

    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    EndRendering(device, GetSafeHwnd(), r);
}

LOGTIMESTAMP PlotControl::TimeAtPoint(int x) {
    RECT r;
    GetClientRect(&r);

    LOGTIMESTAMP span = dll_log->display_end_time - dll_log->display_start_time;

    return dll_log->display_start_time + (LOGTIMESTAMP)(span * find_interpolation_delta((float)(r.left + PLOT_MARGIN_PIXELS), (float)(r.right - PLOT_MARGIN_PIXELS), (float)x));
}

BEGIN_MESSAGE_MAP(PlotControl, CDialog)
    ON_WM_ERASEBKGND()
    ON_WM_CLOSE()
    ON_WM_SIZE()
    ON_WM_SYSCOMMAND()
    ON_WM_NCRBUTTONDOWN()

    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()

    ON_MESSAGE(WM_ENTERSIZEMOVE, OnEnterSizeMove)
    ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
    ON_WM_MOVING()
    ON_WM_SIZING()
END_MESSAGE_MAP()

BOOL PlotControl::OnInitDialog() {
    CDialog::OnInitDialog();

    m_flags.set();
    m_flags.reset(component::keep_elwindow);
    m_flags.reset(component::mouse_dragging);
    m_flags.reset(component::received_WM_CLOSE);

    return TRUE;
}

BOOL PlotControl::OnEraseBkgnd(CDC *pDC) {
    if ( !m_flags[component::window_visible] ) { return CDialog::OnEraseBkgnd(pDC); }
    return TRUE;
}

void PlotControl::PostNcDestroy() {
    CDialog::PostNcDestroy();
    bool keep = m_flags[component::keep_elwindow];
    m_flags.reset(component::keep_elwindow);
    m_flags.reset(component::window_initialized);
    if ( !keep ) {
        // This looks ugly, but this is the recommended place to delete the object
        delete this;
    }
}

bool PlotControl::OnExit() {
    dll_layout->WindowClosed(static_cast<pELWindow>(this));
    DestroyWindow();
    return false;
}

void PlotControl::OnSize(UINT type, int client_width, int client_height) {
    CDialog::OnSize(type, client_width, client_height);
    if ( !m_flags[component::window_initialized] ) { return; }
    Render();
}

void PlotControl::OnSysCommand(UINT nID, LPARAM lParam) {
    if ( dll_host->HandleSystemMenu(static_cast<pELControl>(this), nID, ActiveChannel()) ) { return; }
    CDialog::OnSysCommand(nID, lParam);
}

void PlotControl::OnNcRButtonDown(UINT nHitTest, CPoint point) {
    if ( nHitTest == HTCAPTION ) {
        dll_layout->UpdateSystemMenu(static_cast<pELControlWindow>(this));
    }
    CDialog::OnNcRButtonDown(nHitTest, point);
}

void PlotControl::OnLButtonDown(UINT flags, CPoint point) {
    if ( !m_flags[component::window_visible] ) { return; }

    // Click was not inside a button, treat as the beginning of a drag
    m_mouse_drag_start_point = point;
    m_flags.set(component::mouse_dragging);

    // Update the cursor
    OnMouseMove(0, point);

    SetCapture();
}

void PlotControl::OnMouseMove(UINT flags, CPoint point) {
    if ( !m_flags[component::window_visible] ) { return; }

    if ( m_flags[component::mouse_dragging] ) {
        dll_host->SetLogCursor(TimeAtPoint(point.x));
        m_mouse_drag_start_point = point;
        return;
    }
}

void PlotControl::OnLButtonUp(UINT flags, CPoint point) {
    if ( !m_flags[component::window_visible] ) { return; }
    m_flags.reset(component::mouse_dragging);
    ReleaseCapture();
}

// OnEnterSizeMove
// Called when the window has started moving or resizing
LRESULT PlotControl::OnEnterSizeMove(WPARAM w, LPARAM l) {
    dll_layout->WindowMoveStart(static_cast<pELWindow>(this));
    return 0;
}

// OnExitSizeMove
// Called when the window has stopped moving or resizing
LRESULT PlotControl::OnExitSizeMove(WPARAM w, LPARAM l) {
    dll_layout->WindowMoveEnd(static_cast<pELWindow>(this));
    return 0;
}
void PlotControl::OnMoving(UINT side, LPRECT new_rect) { dll_layout->WindowMoving(static_cast<pELWindow>(this)); }
void PlotControl::OnSizing(UINT side, LPRECT new_rect) { dll_layout->WindowMoving(static_cast<pELWindow>(this)); }
