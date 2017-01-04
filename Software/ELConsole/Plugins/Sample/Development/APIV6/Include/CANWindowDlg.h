#pragma once

#include "helpers.h"
#include <string>
#include <vector>

// Defined in the project.

// can_buffer:
//  This helper struct wraps up a generic buffer, used to store/retrieve can data from the host/target.
struct can_buffer : public ELGenericBuffer {
    typedef std::vector<CANSniffer_TS>::iterator iterator;
    can_buffer() : _new_data(false), _registered(false), _vector_to_fill(&_elements1) { }
    // NewData: Called by the host if this buffer is registered, and the can channel has data that was not previously sent.
    STDMETHODIMP_(void) NewData(BYTE *buffer, UINT size) {
        UINT count = size / sizeof(CANSniffer_TS);
        if (count) {
            std::lock_guard<std::recursive_mutex> lg(_mutex);
            size_t cur_size = _vector_to_fill->size();
            _vector_to_fill->resize(cur_size + count);
            memcpy(&_vector_to_fill->at(cur_size), buffer, size);
            _new_data = true;
        }
    }

    void clear() {
        std::lock_guard<std::recursive_mutex> lg(_mutex);
        _elements1.clear();
        _elements2.clear();
        _vector_to_fill = &_elements1;
        _new_data = false;
    }

    bool new_data() const { return _new_data; }

    std::vector<CANSniffer_TS> &retrieve() {
        std::lock_guard<std::recursive_mutex> lg(_mutex);
        std::vector<CANSniffer_TS> &ret = _vector_to_fill == &_elements1 ? _elements1 : _elements2;
        // Flip the vector to fill.
        _vector_to_fill = _vector_to_fill == &_elements1 ? &_elements2 : &_elements1;
        _vector_to_fill->clear();
        _new_data = false;
        return ret;
    }

    volatile bool _new_data;
    volatile bool _registered;
    std::vector<CANSniffer_TS> _elements1;
    std::vector<CANSniffer_TS> _elements2;
    std::vector<CANSniffer_TS> *_vector_to_fill;
    std::recursive_mutex _mutex;
};

// Helper struct for allowing resizing of the window.
typedef struct tag_CANWindowDlg_ResizeData
{
    CRect dialog_Rect;         
    CRect dialog_Rect_client;         
    CRect cana_list_Rect;         
    CRect canb_list_Rect;         
    CRect start_stop_button_Rect;
    CRect cana_static_Rect;
    CRect canb_static_Rect;
}CANWindowDlg_ResizeData, pCANWindowDlg_ResizeData; 

class CANWindowDlg : public CDialog {
    DECLARE_DYNAMIC(CANWindowDlg)
public:
    // A normal window, taking advantage of API c++ wrapper, stops us from having to STDMETHODIMP_ every function.
    // And any normal defaults are already handled...
    my_window<CANWindowDlg> _window;

    CANWindowDlg();
    ~CANWindowDlg();

    BOOL Create(HWND parent);

    pELHost Host() { return dll_host; }
    ELWINDOWTYPE Type() { return ELWINDOWTYPE_NORMAL; }
    void Destroy() {
        dll_layout->WindowClosed(&_window);
        DestroyWindow();
    }
    HWND GetHWND() { return GetSafeHwnd(); }
    BOOL CanResize(RECT *new_rect) { return TRUE; }
    BOOL IsSavable() { return TRUE; }
    void BeginModal() { BeginModalState(); }
    void EndModal() { EndModalState(); }
    const char *TypeName() { return "CAN Window"; }
    void LoadSettingsFromString(const char *s) { }
    ELString *SaveSettingsToString() { return NULL; }
    void GetStrings(pELStringArray *strings, BOOL *release) { }
    void SetStrings(pELStringArray strings) { }
    void ThemeChanged(const pELThemeColors theme) { Invalidate(); }
    void HostStateChanged(const pELHostInfo host, HOSTSTATE states_changed) { }
    void ApplicationOptionsChanged() { }


    string FormatString(int counter, const CANSniffer_TS &ts);

    void DoDataExchange(CDataExchange *pDX);
    DECLARE_MESSAGE_MAP()
    BOOL PreTranslateMessage(MSG *pMsg) {
        if (dll_host && dll_host->HandleKeyPressed(pMsg)) { return TRUE; }
        return CDialog::PreTranslateMessage(pMsg);
    }
    BOOL OnInitDialog();
    void PostNcDestroy();
    bool OnExit() {
        dll_layout->WindowClosed(&_window);
        DestroyWindow();
        return false;
    }

    void OnTimer(UINT_PTR nIDEvent);
    void OnStartStopClicked();

    void OnPaint();
    BOOL OnEraseBkgnd(CDC *pDC);
    void OnSize(UINT type, int client_width, int client_height);
    LRESULT OnEnterSizeMove(WPARAM w, LPARAM l);
    LRESULT OnExitSizeMove(WPARAM w, LPARAM l);
    void OnMoving(UINT side, LPRECT new_rect);
    void OnSizing(UINT side, LPRECT new_rect);

    CANWindowDlg_ResizeData m_CANWindowDlg_ResizeData_public;
    bool m_resize_initialized;
    void ReSizeControls(int cx, int cy);

protected:
    can_buffer _can_a;
    can_buffer _can_b;

    // Used as a temporary when FormatString is called.
    ostringstream _oss;

    CButton m_start_stop_btn;
    CListBox m_cana_list;
    int m_cana_counter;
    CListBox m_canb_list;
    int m_canb_counter;
    CStatic m_cana_static;
    CStatic m_canb_static;

private:
    bool m_received_WM_CLOSE;

    // These overrides prevent ENTER and ESC from closing the window, but still allow normal closing via ALT + F4, etc...
    // Perform any additional cleanup in OnExit()
    void OnOK() {}
    void OnCancel() { if (m_received_WM_CLOSE && OnExit()) { CDialog::OnCancel(); } }
    void OnClose() { m_received_WM_CLOSE = true; CDialog::OnClose(); }
};
