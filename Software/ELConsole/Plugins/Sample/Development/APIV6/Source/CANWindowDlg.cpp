#include "stdafx.h"
#include "CANWindowDlg.h"
#include "Resource.h"

#include <iomanip>

IMPLEMENT_DYNAMIC(CANWindowDlg, CDialog)

CANWindowDlg::CANWindowDlg() {
    _window._data = this;
    m_cana_counter = 0;
    m_canb_counter = 0;
    m_resize_initialized = false;
}

CANWindowDlg::~CANWindowDlg() {
}

BOOL CANWindowDlg::Create(HWND parent) { return CDialog::Create(IDD_CAN_WINDOW, CWnd::FromHandle(parent)); }

void CANWindowDlg::DoDataExchange(CDataExchange *pDX) {
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CAN_STARTSTOP_BTN, m_start_stop_btn);
    DDX_Control(pDX, IDC_CAN_A_LIST, m_cana_list);
    DDX_Control(pDX, IDC_CAN_B_LIST, m_canb_list);
    DDX_Control(pDX, IDC_CANA_STATIC, m_cana_static);
    DDX_Control(pDX, IDC_CANB_STATIC, m_canb_static);
}

BEGIN_MESSAGE_MAP(CANWindowDlg, CDialog)
ON_WM_CLOSE()
ON_WM_PAINT()
ON_WM_ERASEBKGND()

ON_BN_CLICKED(IDC_CAN_STARTSTOP_BTN, OnStartStopClicked)
ON_WM_TIMER()

ON_WM_SIZE()
ON_MESSAGE(WM_ENTERSIZEMOVE, OnEnterSizeMove)
ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
ON_WM_MOVING()
ON_WM_SIZING()
END_MESSAGE_MAP()

BOOL CANWindowDlg::OnInitDialog() {
    CDialog::OnInitDialog();

    // As we are manually created, register ourself with the layout.  This allows for docking and possible saving.
    dll_layout->RegisterWindow(&_window);

    SetWindowText(_T("CAN Window"));
    SetTimer(1, 1000, NULL); // We want to refresh our display every 1 second.

    m_cana_list.GetWindowRect(&m_CANWindowDlg_ResizeData_public.cana_list_Rect);              ScreenToClient(&m_CANWindowDlg_ResizeData_public.cana_list_Rect);
    m_canb_list.GetWindowRect(&m_CANWindowDlg_ResizeData_public.canb_list_Rect);              ScreenToClient(&m_CANWindowDlg_ResizeData_public.canb_list_Rect);
    m_start_stop_btn.GetWindowRect(&m_CANWindowDlg_ResizeData_public.start_stop_button_Rect); ScreenToClient(&m_CANWindowDlg_ResizeData_public.start_stop_button_Rect);
    m_cana_static.GetWindowRect(&m_CANWindowDlg_ResizeData_public.cana_static_Rect);          ScreenToClient(&m_CANWindowDlg_ResizeData_public.cana_static_Rect);
    m_canb_static.GetWindowRect(&m_CANWindowDlg_ResizeData_public.canb_static_Rect);          ScreenToClient(&m_CANWindowDlg_ResizeData_public.canb_static_Rect);

    GetClientRect(&m_CANWindowDlg_ResizeData_public.dialog_Rect_client);
    GetWindowRect(&m_CANWindowDlg_ResizeData_public.dialog_Rect);

    int cana_list_width = m_CANWindowDlg_ResizeData_public.cana_list_Rect.right - m_CANWindowDlg_ResizeData_public.cana_list_Rect.left;
    int cana_list_height = m_CANWindowDlg_ResizeData_public.cana_list_Rect.bottom - m_CANWindowDlg_ResizeData_public.cana_list_Rect.top;
    int canb_list_width = m_CANWindowDlg_ResizeData_public.canb_list_Rect.right - m_CANWindowDlg_ResizeData_public.canb_list_Rect.left;
    int canb_list_height = m_CANWindowDlg_ResizeData_public.canb_list_Rect.bottom - m_CANWindowDlg_ResizeData_public.canb_list_Rect.top;

    CRect cana_list_Rect;
    m_cana_list.GetWindowRect(&cana_list_Rect);
    int margin_width = cana_list_Rect.left - m_CANWindowDlg_ResizeData_public.dialog_Rect.left;
    m_CANWindowDlg_ResizeData_public.dialog_Rect.right = m_CANWindowDlg_ResizeData_public.dialog_Rect.left + (2 * margin_width) + cana_list_width;
    int cana_list_top_margin = cana_list_Rect.top - m_CANWindowDlg_ResizeData_public.dialog_Rect.top;
    int cana_list_bottom_margin = m_CANWindowDlg_ResizeData_public.dialog_Rect.bottom - cana_list_Rect.bottom;
    m_CANWindowDlg_ResizeData_public.dialog_Rect.bottom = m_CANWindowDlg_ResizeData_public.dialog_Rect.top + cana_list_top_margin + cana_list_height + cana_list_bottom_margin;
    MoveWindow(&m_CANWindowDlg_ResizeData_public.dialog_Rect);

    m_resize_initialized = true;

    return TRUE;
}

// Just format the data to display nicely in our windows.
string CANWindowDlg::FormatString(int counter, const CANSniffer_TS& ts) {
    // Defined in stdafx.h depending on _UNICODE status.
    _oss.str(_T(""));
    _oss << std::dec << counter << _T(" ");

    // Tx or Rx from/to the target.
    if (ts.rsvd_0 & 1) {
        _oss << _T("TX ");
    } else {
        _oss << _T("RX ");
    }

    if (ts.rsvd_0 & (0x1F << 1)) {
        if (ts.rsvd_0 & (0x1 << 6)) {
            _oss << _T("Target(") << std::dec << ((ts.rsvd_0 >> 1) & 0x1F) << _T(") ");
        }else{
            _oss << _T("Sniffer(") << std::dec << ((ts.rsvd_0 >> 1) & 0x1F) << _T(") ");
        }
    } else {
        _oss << _T("Sniffer(0)");
    }

    // Is Extended Format?
    if (ts.identifier & 0x80000000) {
        _oss << _T("Ext. Id: ") << hex_upper(8) << (ts.identifier & 0x7FFFFFFF);
    } else {
        _oss << _T("Std. Id: ") << hex_upper(8) << ts.identifier;
    }
    _oss << _T(" Timestamp: ") << hex_upper(8) << ts.timestamp_h << hex_upper(8) << ts.timestamp_l << " "
       << (double)((ts.timestamp_l * 0.0000001) + (ts.timestamp_h * 429.4967296))
       << _T(" Data Length: ") << hex_upper(2) << (DWORD)ts.data_length << _T(" Data: [");
    for (int i = 0; i < ts.data_length; ++i) {
        _oss << hex_upper(2) << (DWORD)ts.msg_data[i];
        if (i != 7) {_oss << " "; }
    }
    _oss << _T("]");
    return _oss.str();
}

void CANWindowDlg::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == 1 && IsWindowVisible()) {
        // Only bother to update a display if new data was retrieved in the last second.
        if (_can_a.new_data()) {
            std::vector<CANSniffer_TS> & tmp = _can_a.retrieve();
            _oss.str(_T(""));
            _oss << _T("Retrieved ") << std::dec << tmp.size() << _T(" transactions");
            m_cana_list.AddString(_oss.str().c_str());
            int top_index = 0;
            for (std::vector<CANSniffer_TS>::iterator iter = tmp.begin(); iter != tmp.end(); ++iter) {
                top_index = m_cana_list.AddString(FormatString(m_cana_counter++, *iter).c_str());
                if (top_index > 300) {top_index = m_cana_list.DeleteString(0); }
            }
            m_cana_list.SetTopIndex(top_index);
        }

        if (_can_b.new_data()) {
            std::vector<CANSniffer_TS> &tmp = _can_b.retrieve();
            _oss.str(_T(""));
            _oss << _T("Retrieved ") << std::dec << tmp.size() << _T(" transactions");
            m_canb_list.AddString(_oss.str().c_str());
            int top_index = 0;
            for (std::vector<CANSniffer_TS>::iterator iter = tmp.begin(); iter != tmp.end(); ++iter) {
                top_index = m_canb_list.AddString(FormatString(m_canb_counter++, *iter).c_str());
                if (top_index > 300) {top_index = m_canb_list.DeleteString(0); }
            }
            m_canb_list.SetTopIndex(top_index);
        }
    }
}

void CANWindowDlg::OnStartStopClicked() {
    if (_can_a._registered || _can_b._registered) {
        // Be sure to notify the host that we no longer want can data.  This allows the host to clear any buffers if we
        // were the only entity wanting such information.
        dll_cana->UnregisterBuffer(&_can_a);
        _can_a._registered = false;
        dll_canb->UnregisterBuffer(&_can_b);
        _can_b._registered = false;
        m_start_stop_btn.SetWindowText(_T("Start"));
    } else {

        _can_a.clear();
        m_cana_list.ResetContent();
        m_cana_counter = 0;
        _can_b.clear();
        m_canb_list.ResetContent();
        m_canb_counter = 0;

        // Let the host know that we want can data on both channels.  This allows the host time to set up buffers, and start
        // queing up data for us to retrieve.
        _can_a._registered = dll_cana->RegisterBuffer(&_can_a) == DS_SUCCESS;
        _can_b._registered = dll_canb->RegisterBuffer(&_can_b) == DS_SUCCESS;
        if (_can_a._registered || _can_b._registered) {
            m_start_stop_btn.SetWindowText(_T("Stop"));
        }
    }
}

void CANWindowDlg::OnPaint() {
    CPaintDC dc(this);

    RECT r;
    GetClientRect(&r);

    dc.FillSolidRect(&r, dll_theme->background_color);
}

BOOL CANWindowDlg::OnEraseBkgnd(CDC *pDC) { return TRUE; }

extern CANWindowDlg *g_can_window_dlg;
void CANWindowDlg::PostNcDestroy() {
    dll_cana->UnregisterBuffer(&_can_a);
    dll_canb->UnregisterBuffer(&_can_b);
    CDialog::PostNcDestroy();
    delete this;
    g_can_window_dlg = NULL;
}

void CANWindowDlg::ReSizeControls(int cx, int cy) {
    if (m_resize_initialized) {
        int top_margin = m_CANWindowDlg_ResizeData_public.cana_list_Rect.top;
        int bottom_margin = m_CANWindowDlg_ResizeData_public.dialog_Rect_client.bottom - m_CANWindowDlg_ResizeData_public.canb_list_Rect.bottom;
        int can_a_top_b_bottom_spacing = m_CANWindowDlg_ResizeData_public.canb_list_Rect.top - m_CANWindowDlg_ResizeData_public.cana_list_Rect.bottom;
        int start_stop_bottom_margin = m_CANWindowDlg_ResizeData_public.dialog_Rect_client.bottom - m_CANWindowDlg_ResizeData_public.start_stop_button_Rect.bottom;
        int can_b_static_spacing = m_CANWindowDlg_ResizeData_public.canb_list_Rect.top - m_CANWindowDlg_ResizeData_public.canb_static_Rect.top; 

        // Get the dialog Rect.
        GetClientRect(&m_CANWindowDlg_ResizeData_public.dialog_Rect_client);

        //********************************************************************************************
        int can_a_list_top = m_CANWindowDlg_ResizeData_public.cana_list_Rect.top;
        int can_a_list_left = m_CANWindowDlg_ResizeData_public.cana_list_Rect.left;
        int can_a_list_width = cx - (2 * m_CANWindowDlg_ResizeData_public.cana_list_Rect.left);
        int can_a_list_height = (cy - top_margin - bottom_margin - can_a_top_b_bottom_spacing) / 2;

        m_cana_list.MoveWindow(can_a_list_left,
                               can_a_list_top,
                               can_a_list_width,
                               can_a_list_height);
        m_cana_list.GetWindowRect(&m_CANWindowDlg_ResizeData_public.cana_list_Rect); ScreenToClient(&m_CANWindowDlg_ResizeData_public.cana_list_Rect);
        //********************************************************************************************

        int can_b_list_top_margin = (m_CANWindowDlg_ResizeData_public.cana_list_Rect.bottom + can_a_top_b_bottom_spacing) - m_CANWindowDlg_ResizeData_public.dialog_Rect_client.top;
        //int can_b_list_bottom_margin = m_CANWindowDlg_ResizeData_public.dialog_Rect_client.bottom - m_CANWindowDlg_ResizeData_public.canb_list_Rect.bottom;

        //********************************************************************************************
        int can_b_list_top = m_CANWindowDlg_ResizeData_public.cana_list_Rect.bottom + can_a_top_b_bottom_spacing; 
        int can_b_list_left = m_CANWindowDlg_ResizeData_public.canb_list_Rect.left;
        int can_b_list_width = cx - (2 * m_CANWindowDlg_ResizeData_public.canb_list_Rect.left);
        int can_b_list_height = cy - top_margin - bottom_margin - can_a_top_b_bottom_spacing - can_a_list_height;

        m_canb_list.MoveWindow(can_b_list_left,
                               can_b_list_top,
                               can_b_list_width,
                               can_b_list_height);
        m_canb_list.GetWindowRect(&m_CANWindowDlg_ResizeData_public.canb_list_Rect); ScreenToClient(&m_CANWindowDlg_ResizeData_public.canb_list_Rect);
        //********************************************************************************************

        //********************************************************************************************
        int start_stop_top = cy - start_stop_bottom_margin -
           (m_CANWindowDlg_ResizeData_public.start_stop_button_Rect.bottom - m_CANWindowDlg_ResizeData_public.start_stop_button_Rect.top);
        int start_stop_left = m_CANWindowDlg_ResizeData_public.cana_list_Rect.right -
           (m_CANWindowDlg_ResizeData_public.start_stop_button_Rect.right - m_CANWindowDlg_ResizeData_public.start_stop_button_Rect.left);
        int start_stop_width = m_CANWindowDlg_ResizeData_public.start_stop_button_Rect.right - m_CANWindowDlg_ResizeData_public.start_stop_button_Rect.left;
        int start_stop_height = m_CANWindowDlg_ResizeData_public.start_stop_button_Rect.bottom - m_CANWindowDlg_ResizeData_public.start_stop_button_Rect.top;

        m_start_stop_btn.MoveWindow(start_stop_left,
                                    start_stop_top,
                                    start_stop_width,
                                    start_stop_height);
        m_start_stop_btn.GetWindowRect(&m_CANWindowDlg_ResizeData_public.start_stop_button_Rect); ScreenToClient(&m_CANWindowDlg_ResizeData_public.start_stop_button_Rect);
        //********************************************************************************************

        //********************************************************************************************
        int canb_static_top = can_b_list_top - can_b_static_spacing;
        int canb_static_left = m_CANWindowDlg_ResizeData_public.canb_static_Rect.left;
        int canb_static_width = m_CANWindowDlg_ResizeData_public.canb_static_Rect.right - m_CANWindowDlg_ResizeData_public.canb_static_Rect.left;
        int canb_static_height = m_CANWindowDlg_ResizeData_public.canb_static_Rect.bottom - m_CANWindowDlg_ResizeData_public.canb_static_Rect.top;

        m_canb_static.MoveWindow(canb_static_left,
                                 canb_static_top,
                                 canb_static_width,
                                 canb_static_height);
        m_canb_static.GetWindowRect(&m_CANWindowDlg_ResizeData_public.canb_static_Rect); ScreenToClient(&m_CANWindowDlg_ResizeData_public.canb_static_Rect);
        //********************************************************************************************
    }
}

void CANWindowDlg::OnSize(UINT type, int clientw, int clienth) {
    if (clientw < 100 || clienth < 100) {return; }
    CDialog::OnSize(type, clientw, clienth);

    ReSizeControls(clientw, clienth);

    Invalidate();
}

// Always inform the layout when we want to move, or change.  This allows the layout to keep tabs of open spaces for docking.  Also, by allowing
// the host to handle these notifications, such resize can be stopped without further notifications.
LRESULT CANWindowDlg::OnEnterSizeMove(WPARAM w, LPARAM l) {
    dll_layout->WindowMoveStart(&_window);
    return 0;
}
LRESULT CANWindowDlg::OnExitSizeMove(WPARAM w, LPARAM l) {
    dll_layout->WindowMoveEnd(&_window);
    Invalidate();
    return 0;
}
void CANWindowDlg::OnMoving(UINT side, LPRECT new_rect) { dll_layout->WindowMoving(&_window); }
void CANWindowDlg::OnSizing(UINT side, LPRECT new_rect) { dll_layout->WindowMoving(&_window); }
