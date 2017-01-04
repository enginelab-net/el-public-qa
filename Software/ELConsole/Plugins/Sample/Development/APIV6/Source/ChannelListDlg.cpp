#include "stdafx.h"
#include "ChannelListDlg.h"
#include "Resource.h"


IMPLEMENT_DYNAMIC(ChannelListDlg, CDialog)

ChannelListDlg::ChannelListDlg(pELItemList list, CWnd *parent)
    : CDialog(IDD_CHANNEL_LIST, parent)
    , m_list(list)
    , m_item(NULL)
    , m_info_type(0)
    , m_info_list(NULL)
    , m_info(NULL)
{ }

ChannelListDlg::ChannelListDlg(TARGETINFO type, pELGenericInfoList list, CWnd *parent)
    : CDialog(IDD_CHANNEL_LIST, parent)
    , m_list(NULL)
    , m_item(NULL)
    , m_info_type(type)
    , m_info_list(list)
    , m_info(NULL)
{ }

void ChannelListDlg::DoDataExchange(CDataExchange *pDX) {
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CHANNEL_LIST_LISTBOX, m_list_wnd);
}

BEGIN_MESSAGE_MAP(ChannelListDlg, CDialog)
END_MESSAGE_MAP()

BOOL ChannelListDlg::OnInitDialog() {
    CDialog::OnInitDialog();

    CString str;
    string type_name;
    string name;
    if (m_list) {
        for (UINT i = 0; i < m_list->Count(); ++i) {
            pELItem tmp = m_list->Item(i);
            type_name = proper(tmp->Type()->Name());
            name = proper(tmp->Name());
            str.Format(_T("%04X - %s - %s"), tmp->Channel(), type_name.c_str(), name.c_str());
            m_list_wnd.AddString(str);
        }

        if (m_list->InterfaceVersion() >= 2) {
            ITEMFILTER raw_filter = m_list->Filter();
            ITEMFILTER filter = raw_filter & ITEMFILTER_FILTER_MASK;
            switch (filter) {
            case ITEMFILTER_NONE: str = _T("Channel List - [All]"); break;
            case ITEMFILTER_ALL_HW_PROCESSED: str = _T("Channel List - [Not hardware processed]"); break;
            case ITEMFILTER_ALL_OF_TYPE: str = _T("Channel List - [Not of type]"); break;
            case ITEMFILTER_ALL_BUT_PARENTS_OF_ITEM: str = _T("Channel List - [Parent of]"); break;
            case ITEMFILTER_ALL_BUT_CHILDREN_OF_ITEM: str = _T("Channel List - [Children of]"); break;
            case ITEMFILTER_ALL_BUT_HW_PROCESSED: str = _T("Channel List - [Hardware processed]"); break;
            case ITEMFILTER_ALL_BUT_OF_TYPE: str = _T("Channel List - [type]"); break;
            case ITEMFILTER_ALL:
            default: str = _T("Channel List - [Empty]"); break;
            }
        }
        else {
            str = _T("Channel List");
        }
    }
    else if (m_info_list) {
        for (UINT i = 0; i < m_info_list->Count(); ++i) {
            pELGenericInfo tmp = m_info_list->Info(i);
            name = proper(tmp->Name());
            str.Format(_T("%s"), name.c_str());
            m_list_wnd.AddString(str);
        }

        switch (m_info_type) {
        case TARGETINFO_TIMING_TYPES: str = _T("Info List - [Timing Types]"); break;
        case TARGETINFO_EDGE_TRIGGERS: str = _T("Info List - [Edge Triggers]"); break;
        case TARGETINFO_ENGINE_CYCLES: str = _T("Info List - [Engine Cycles]"); break;
        case TARGETINFO_INJECTOR_TYPES: str = _T("Info List - [Injector Types]"); break;
        case TARGETINFO_HARNESS_PIN_MODES: str = _T("Info List - [Harness Pin Modes]"); break;
        case TARGETINFO_CPU_PIN_MODES: str = _T("Info List - [CPU Pin Modes]"); break;
        case TARGETINFO_TIMER_MODES: str = _T("Info List - [Timer Modes]"); break;
        case TARGETINFO_UNIT_CONVERSIONS: str = _T("Info List - [Unit Conversions]"); break;
        case TARGETINFO_SYNC_RESPONSES: str = _T("Info List - [Sync Responses]"); break;
        case TARGETINFO_THERMISTOR_OUTPUT_TYPES: str = _T("Info List - [Thermistor Output Types]"); break;
        case TARGETINFO_FIXED_TABLE_TYPES: str = _T("Info List - [Fixed Table Types]"); break;
        case TARGETINFO_SIMPLE_CAN_FORMAT_TYPES: str = _T("Info List - [Simple CAN Format Types]"); break;
        case TARGETINFO_CAN_ENDIANESS: str = _T("Info List - [CAN Endianess]"); break;
        case TARGETINFO_CAN_ID_FORMATS: str = _T("Info List - [CAN ID Formats]"); break;
        case TARGETINFO_MATH_TOKENS: str = _T("Info List - [Math Tokens]"); break;
        case TARGETINFO_ITEM_TYPES: str = _T("Info List - [Item Types]"); break;
        case TARGETINFO_DATA_TYPES: str = _T("Info List - [Data Types]"); break;
        case TARGETINFO_CAN_PACKET_DATA_TYPES: str = _T("Info List - [CAN Packet Data Types]"); break;
        case TARGETINFO_SERIAL_STREAM_DATA_TYPES: str = _T("Info List - [Serial Stream Data Types]"); break;
        case TARGETINFO_COMMUNICATION_DATA_TYPES: str = _T("Info List - [Communication Data Types]"); break;
        case TARGETINFO_BUS_CHANNELS: str = _T("Info List - [Bus Channels]"); break;
        case TARGETINFO_BUS_DIRECTIONS: str = _T("Info List - [Bus Directions]"); break;
        case TARGETINFO_TX_MODES: str = _T("Info List - [TX Modes]"); break;
        case TARGETINFO_LATCH_EDGES: str = _T("Info List - [Latch Edges]"); break;
        case TARGETINFO_LATCH_START_STATES:
        default: str = _T("Info List - [Latch Start States]"); break;
        }
    }
    SetWindowText(str);
    return TRUE;
}

void ChannelListDlg::OnOK() {
    int sel = m_list_wnd.GetCurSel();
    if ( sel != LB_ERR ) {
        if (m_list) {
            m_item = m_list->Item((UINT)sel);
        }
        else if (m_info_list) {
            m_info = m_info_list->Info((UINT)sel);
        }
        CDialog::OnOK();
        return;
    }

    CDialog::OnCancel();
}
