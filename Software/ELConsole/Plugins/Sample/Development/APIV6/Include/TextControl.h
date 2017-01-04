#pragma once
/*
 TextControl
 This control is built to mimic the Hosts version of the text control.

*/
#include "helpers.h"

#include <vector>
#include <algorithm>
#include <sstream>
#include <bitset>

// Color is always nice.
#include "ColoredStatic.h"

pELControlWindow CreateText();

// Our system menu starts at 0x1000 as everything else is either Windows or Host owned.
#define SC_INITIAL_VALUE 0x1000

// Multiple inheritance to allow for easy conversions between types.
class TextControl : public CDialog, public ELControlWindow, public ELControl {
    DECLARE_DYNAMIC(TextControl)
public:
    TextControl() {
        m_active_channel = -1;

        _opt_value = NULL;
        // We allow users to select a channel, set formatting options, and set color options.
        _sysmenu = SYSMENUSUPPORT_SELECT_CHANNEL | SYSMENUSUPPORT_NUMBER_OPTIONS | SYSMENUSUPPORT_COLOR_OPTIONS;
        // As we display only a value, we allow all types of channels.
        _info.types_allowed = CHANNELTYPE_ALL;
        // However, we only display one channel at a time.
        _info.min_allowed = 1;
        _info.max_allowed = 1;
        // We do not allow multiple selection, so no flags needed.
        _info.flags = SELECTFLAGS_NONE;

        // Make sure we get the control window font the host has selected.  We want to look like everyone else.
        if ( dll_host->OptionValue("control_window_font_name", &_opt_value) ) {
            m_control_window_font_name = _opt_value->str;
        }

        // Clear all our helper flags.
        m_flags.reset();
    }
    virtual ~TextControl() {
        // As we use a single ELString for just about everything, clear its memory.
        dll_gen->FreeString(&_opt_value);
    }

public: // Method required for ELControlWindow
    // Host: Return the host we use.  Generally not needed, but nice to have.
    STDMETHODIMP_(pELHost) Host(void) { return dll_host; }
    // Type: We are a control window, so we follow the rules of a control.
    STDMETHODIMP_(ELWINDOWTYPE) Type() { return ELWINDOWTYPE_CONTROL; }
    // Destroy: The host actually wants us to be gone, so dissappear we do.
    STDMETHODIMP_(void) Destroy() {
        dll_layout->WindowClosed(static_cast<pELWindow>(this));
        if ( ::IsWindow(m_hWnd) ) { DestroyWindow(); } // DestroyWindow eventually causes us to be fully destroyed, GUI and all.
        else { delete this; }
    }
    // GetHWND: Return our handle.
    STDMETHODIMP_(HWND) GetHWND(void) { return GetSafeHwnd(); }
    // CanResize: Return if we would like to fit in the given rect.
    STDMETHODIMP_(BOOL) CanResize(RECT *new_rect) {
        return new_rect->right - new_rect->left >= EL_MINIMUM_CONTOL_WINDOW_WIDTH && new_rect->bottom - new_rect->top >= EL_MINIMUM_CONTOL_WINDOW_HEIGHT;
    }
    // IsSavable: Required to be a control as we allow to be saved to a layout file.
    STDMETHODIMP_(BOOL) IsSavable(void) { return TRUE; }
    // BeginModal: Called by the host when some modal window is called.  This stops us from receiving messages that the host does not want us to handle yet.
    STDMETHODIMP_(void) BeginModal(void) { BeginModalState(); }
    // EndModal: The host is done blocking us, continue onward happily.
    STDMETHODIMP_(void) EndModal(void) { EndModalState(); }
    // TypeName: Return our name.  This name is what is stored in layout files.  Should match the name used in SupportedControls of plugin.
	STDMETHODIMP_(const char *) TypeName(void) { return "Text"; }
    // LoadSettingsFromString:
    // A function only for backwards compatibility.  Allows us to handle older layout files without issue.
    // Load up any settings that we save to a layout file.
	STDMETHODIMP_(void) LoadSettingsFromString(const char *settings) {
        std::istringstream iss(settings);
        bool tmp = false;
        iss >> std::boolalpha >> tmp;
        m_flags.set(component::initial_value, tmp);
    }
    // SaveSettingsToString:
    // A function to save our settings to a layout file.  No longer used, but is a companion to LoadSettingsFromString so...
    STDMETHODIMP_(pELString) SaveSettingsToString(void) { return NULL; }
    // GetStrings:
    // The proper way to save our settings to a layout file.
    // sa - The array of strings we are setting.  The array is name then value, so shall always be a length divisible by 2.
    STDMETHODIMP_(void) GetStrings(pELStringArray *sa, BOOL *release) {
        *release = FALSE;
        if ( dll_gen->AllocateStringArray(2, TRUE, sa) != DS_SUCCESS ) { return; }
        (*sa)->count = 0;
        try {
            if ( dll_gen->SetString((*sa)->elements[0], "initial_value", (UINT)-1) != DS_SUCCESS ) { return; }
            std::ostringstream oss;
            oss << std::boolalpha << m_flags[component::initial_value];
            if ( dll_gen->SetString((*sa)->elements[1], oss.str().c_str(), (UINT)-1) != DS_SUCCESS ) { return; }
            (*sa)->count = 2;
        }
        catch (...) { } // Exceptions are not allowed across the dll boundary.  Bad juju.
    }
    // SetStrings:
    // A way to load our saved settings.  The given array shall always be supplied by the host to be length divisible by 2.
    // The array follows the same format as GetStrings, a name then value.
    STDMETHODIMP_(void) SetStrings( pELStringArray sa) {
        for (size_t i = 0; i < sa->count; i += 2) {
            pELString name = sa->elements[i];
            pELString value = sa->elements[i + 1];
            if ( !name || !value || name->length == 0 ) { continue; }
            std::istringstream iss(value->str);
            if ( strcmp(name->str, "initial_value") == 0 ) {
                bool init = false;
                iss >> std::boolalpha >> init;
                if ( init ) { m_flags.set(component::initial_value); }
                else { m_flags.reset(component::initial_value); }
            }
        }
    }
    // ThemeChanged:
    // Called by the host to notify us that we may need to change our colors.
    STDMETHODIMP_(void) ThemeChanged(const pELThemeColors theme) {
        if ( !m_flags[component::window_initialized] ) {
            // We are not visible according to the host, so just flag that the change is needed.
            m_flags.set(component::theme_changed);
            return;
        }
        m_flags.reset(component::theme_changed);
        // Visible and happy, adjust our colors.
        _invalid_bg = GetSysColor(COLOR_WINDOW);
        _invalid_txt = brightness(_invalid_bg) < 0.5f ? theme->text_light_color : theme->text_dark_color;
        ChannelsChanged();
    }
    // HostStateChanged:
    // We do not care about the state as much, but redo our display incase no longer connected or logging started or something.
    STDMETHODIMP_(void) HostStateChanged( const pELHostInfo host, HOSTSTATE states_changed) {
        ValidateWindow();
        // On next render, handle any further changes.
        m_flags.set(component::channels_changed);
    }
    // ApplicationOptionsChanged:
    // We only care if the font changed, we want to stay looking like other controls.
    STDMETHODIMP_(void) ApplicationOptionsChanged(void) {
        const char *opt = NULL;
        if ( (opt = dll_host->OptionChanged("control_window_font_name")) != NULL ) {
            m_control_window_font_name = opt;
            // May need to resize, make it happen on next render.
            m_flags.set(component::reposition_controls);
        }
    }
    // Control:  Return our ELControl being used.
    STDMETHODIMP_(pELControl) Control() { return static_cast<pELControl>(this); }
    // DestroyHWND: The host is hidding us, to save Windows resources, destroy only our GUI.
    STDMETHODIMP_(void) DestroyHWND() {
        if ( !::IsWindow(m_hWnd) ) { return; }
        m_flags.set(component::keep_elwindow); // make sure we stay around, just not our display.
        DestroyWindow();
    }
    // CreateHWND: The host wants us visible, recreate our GUI only.
    STDMETHODIMP_(HWND) CreateHWND( HWND parent);

    // Methods required for ELControl
    STDMETHODIMP_(void) Update() { } // Do nothing, we do not process in the background.
    // AddChannel:
    // The host is giving us another channel to display, we only display one channel but accept it anyways.
    STDMETHODIMP_(UINT) AddChannel(pELChannelInfo channel) {
        try {
            m_channels.push_back(channel);
            if ( m_channels.size() == 1 ) {
                m_active_channel = 0;
            }
            return (UINT)m_channels.size() - 1;
        }
        catch (...) { }
        return (UINT)-1;
    }
    // RemoveChannel:
    // The host wants us to remove a channel, we only ever display the first channel so delete the channel and do nothing much else.
    STDMETHODIMP_(void) RemoveChannel(pELChannelInfo channel) {
        m_channels.erase(std::find(m_channels.begin(), m_channels.end(), channel));
        if ( m_channels.empty() ) {
            m_active_channel = -1;
        }
    }
    // ChannelCount:
    // The number of channels we are managing.  Not the number we display.
    STDMETHODIMP_(UINT) ChannelCount() { return (UINT)m_channels.size(); }
    // Channel:
    // Yield the channel at the given index.
    STDMETHODIMP_(pELChannelInfo) Channel( UINT index) {
        if ( index < m_channels.size() ) { return m_channels[index]; }
        return NULL;
    }
    // ChannelIndex:
    // Yields the index of the channel, if any.
    STDMETHODIMP_(BOOL) ChannelIndex(pELChannelInfo channel, UINT *out_index) {
        for (size_t i = 0, end = m_channels.size(); i < end; ++i) {
            if ( m_channels[i] == channel ) {
                *out_index = (UINT)i;
                return TRUE;
            }
        }

        return FALSE;
    }
    // ChannelsChanged:
    // The channels we have... have changed.  Make sure we adjust our display accordingly.
    STDMETHODIMP_(void) ChannelsChanged() {
        if ( !m_flags[component::window_initialized] ) {
            // Bah we are not visible, so just set up to do the work on next render.
            m_flags.set(component::channels_changed);
            return;
        }
        m_flags.reset(component::channels_changed);

        if ( !ValidateWindow() ) { return; }

        pELChannelInfo info = ActiveChannel();

        m_label.ShowWindow(SW_SHOWNORMAL);
        m_flags.reset(component::no_data);
        if ( !flags(info, CHANNELFLAGS_VALID) ) {
            m_label.SetWindowText(_T("Invalid Channel"));
            m_label.SetBkColor(_invalid_bg);
            m_label.SetTextColor(_invalid_txt);
        }
        else if ( !live(dll_log) && !in_log(*info) && !logged(info) ) {
            m_flags.set(component::no_data);
            m_label.SetWindowText(_T("No Data"));
            m_label.SetBkColor(_invalid_bg);
            m_label.SetTextColor(_invalid_txt);
        }

        std::string tmp;
        adjust_name(*info, INFOSELECT_PRIMARY, _opt_value, tmp, false);
        move(info->primary, m_color_gradient);

        if ( m_flags[component::initial_value] ) {
            tmp += " [initial value]";
            SetWindowText(proper(tmp).c_str());
        }
        else {
            SetWindowText(proper(tmp).c_str());
        }
    }
    // ChannelModified:
    // Something of the channel has been changed.  As we do most of the work on a render, we only flag if certain channel information has changed.
    STDMETHODIMP_(void) ChannelModified(MODIFYOPTION option, pELChannelInfo channel, INFOSELECT which) {
        switch (option) {
        case MODIFYOPTION_SELECTION: // Fall through
        case MODIFYOPTION_FOLLOWECU: // Fall through
        case MODIFYOPTION_FORMATTING: // Fall through
        case MODIFYOPTION_COLOR: break;

        case MODIFYOPTION_DATA: // Fall through
        case MODIFYOPTION_VALIDITY:
        case MODIFYOPTION_UNIT:
        case MODIFYOPTION_LOGGED:
            if ( channel == ActiveChannel() ) {
                m_flags.set(component::channels_changed);
                if ( !dll_host_info->connected ) { ChannelsChanged(); }
            }
            break;
        }
    }
    // Info:
    // Return our nicely setup info from the constructor.
    STDMETHODIMP_(const pELChannelSelectInfo) Info() { return &_info; }
    // Window:
    // Return our ELWindow.
    STDMETHODIMP_(pELWindow) Window() { return this; }
    // Render:
    // This is called periodically from the host.  The host attempts to ensure that all data is accurate at the time of this call.
    // The host shall never call Render on more than one control at a time, so sharing resources is safe if the control is handled by
    // only the host.
    STDMETHODIMP_(void) Render() {
        if ( !m_flags[component::window_initialized] ) { return; }

        if ( m_flags[component::theme_changed] ) { ThemeChanged((const pELThemeColors)dll_theme); }
        if ( m_flags[component::validate_window] || m_flags[component::channels_changed] ) { ChannelsChanged(); }
        if ( !m_flags[component::window_visible] ) { return; }
        pELChannelInfo info = ActiveChannel();
        if ( !info || !type(info, _info.types_allowed) ) { return; }

        if ( (m_flags[component::no_data] && (live(dll_log) || in_log(*info))) ||
            (!m_flags[component::no_data] && !live(dll_log) && !in_log(*info)) ) {
            ChannelsChanged();
        }

        CRect win;
        GetClientRect(&win);
        if ( m_flags[component::reposition_controls] ) { RepositionControls(win.Width(), win.Height()); }

        if ( m_edit.IsWindowVisible() ) { return; }

        if ( !flags(info, CHANNELFLAGS_VALID) || (!live(dll_log) && !in_log(*info)) ) {
            m_label.SetWindowText(_T("No Data"));
            m_label.SetBkColor(_invalid_bg);
            m_label.SetTextColor(_invalid_txt);
            return;
        }

        float f = m_flags[component::initial_value] ? info->initial_value : *info->primary.value;
        COLORREF background = color_at(info->primary.info, m_color_gradient, f);

        m_label.SetBkColor(background);
        m_label.SetTextColor(brightness(background) < 0.5f ? dll_theme->text_light_color : dll_theme->text_dark_color);
        dll_gen->Print(&info->primary.info.number_format, f, &_opt_value);
        m_label.SetWindowText(proper(_opt_value->str).c_str());
    }
    // SysMenuSupport: Return our requirements for the system menu.
    STDMETHODIMP_(SYSMENUSUPPORT) SysMenuSupport() { return _sysmenu; }
    // ActiveChannel: Returns the channel we are currently displaying, this control only displays one channel but it can be
    // changed to display one channel per tab or something similar.  Controls that display multiple channels do not use this function.
    STDMETHODIMP_(pELChannelInfo) ActiveChannel() {
        if ( m_active_channel >= 0 && (size_t)m_active_channel < m_channels.size() ) { return m_channels[m_active_channel]; }
        return NULL;
    }
    // UserMathExpression:  Added for legacy reasons, generally used by controls that display table information or handle math expressions.
    STDMETHODIMP_(void) UserMathExpression( const char * expression) { }
    // ShowSelectChannelDlg: The user wants to change the channel (maybe), so call the host with our information and allow picking.
    STDMETHODIMP_(BOOL) ShowSelectChannelDlg() {
        if ( dll_dialogs->DisplayChannelList(static_cast<pELControl>(this), INFOSELECT_PRIMARY, 0, NULL) == DS_SUCCESS ) { return TRUE; }
        return FALSE;
    }
    // ShowPropertiesDlg: The host wants us to display our properties, we do not have properties so ignore-ish.
    // Return TRUE or creation will fail if created via the hosts control menu.
    STDMETHODIMP_(BOOL) ShowPropertiesDlg() { return TRUE; }
    // The following two functions can be used to store channel specific information in a layout file.  Not used by this control.
    STDMETHODIMP_(void) GetChannelOptions(pELChannelInfo channel, pELStringArray * options, BOOL * release) { *options = NULL;  *release = FALSE; }
    STDMETHODIMP_(void) SetChannelOptions(pELChannelInfo channel, pELStringArray options) { }

    // Helper function to determine if/when hotkeys are allowed.  Generally when editing we want all keys to be entered as text so
    // hot keys are disabled for that moment.
    bool EnableHotKeys() { return !(::IsWindow(m_edit.GetSafeHwnd()) && m_edit.IsWindowVisible()); }

    bool ValidateWindow();

    DECLARE_MESSAGE_MAP()
    BOOL PreTranslateMessage(MSG *pMsg);
    BOOL OnInitDialog();
    // The final message received by windows to any gui, use this to clear our memory.
    void PostNcDestroy() {
        CDialog::PostNcDestroy();
        bool keep = m_flags[component::keep_elwindow];
        m_flags.reset(component::keep_elwindow);
        m_flags.reset(component::window_initialized);
        if ( !keep ) {
            // This looks ugly, but this is the recommended place to delete the object
            delete this;
        }
    }
    bool OnExit() {
        dll_layout->WindowClosed(static_cast<pELWindow>(this));
        DestroyWindow();
        return false;
    }

    void OnPaint();
    void OnLButtonDblClk(UINT flags, CPoint point);
    void OnEditKillFocus();
    BOOL OnEraseBkgnd(CDC *pDC);

    void RepositionControls(int client_width, int client_height);

    void OnSize(UINT type, int client_width, int client_height) {
        CDialog::OnSize(type, client_width, client_height);
        // OnSize can be called before the window is actually valid
        m_flags.set(component::reposition_controls);
        Render();
    }
    void OnSysCommand(UINT nID, LPARAM lParam);
    LRESULT OnEnterSizeMove(WPARAM w, LPARAM l) { dll_layout->WindowMoveStart(static_cast<pELWindow>(this)); return 0; }
    LRESULT OnExitSizeMove(WPARAM w, LPARAM l) { dll_layout->WindowMoveEnd(static_cast<pELWindow>(this)); return 0; }
    void OnMoving(UINT side, LPRECT new_rect) { dll_layout->WindowMoving(static_cast<pELWindow>(this)); }
    void OnSizing(UINT side, LPRECT new_rect) { dll_layout->WindowMoving(static_cast<pELWindow>(this)); }

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

            no_data,
            _count
        };
    };
    std::bitset<component::_count> m_flags;

private:
    ELChannelSelectInfo _info;
    SYSMENUSUPPORT _sysmenu;
    std::vector<pELChannelInfo> m_channels;
    std::string m_control_window_font_name;
    int m_active_channel;
    HGDIOBJ m_font_handle;
    HGDIOBJ m_control_font_handle;
    LONG m_font_height;

    COLORREF _invalid_bg;
    COLORREF _invalid_txt;
    ELColorGradient m_color_gradient;

    pELString _opt_value;

    CColoredStatic m_label;
    CEdit m_edit;

    bool m_received_WM_CLOSE;

    // These overrides prevent ENTER and ESC from closing the window, but still allow normal closing via ALT + F4, etc...
    // Perform any additional cleanup in OnExit()
    void OnOK() {}
    void OnCancel() { if (m_received_WM_CLOSE && OnExit()) { CDialog::OnCancel(); } }
    void OnClose() { m_received_WM_CLOSE = true; CDialog::OnClose(); }
};
