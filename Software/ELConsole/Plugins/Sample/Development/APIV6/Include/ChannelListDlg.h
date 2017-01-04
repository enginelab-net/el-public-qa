#pragma once

#include "helpers.h"

// Just displays the channel list based off the contents in the given list.
// Expected to be a modal dialog.
class ChannelListDlg : public CDialog {
    DECLARE_DYNAMIC(ChannelListDlg)
public:
    ChannelListDlg(pELItemList list, CWnd *parent = NULL);
    ChannelListDlg(TARGETINFO ti, pELGenericInfoList info, CWnd *parent = NULL);
    // Return the item, if any, that the user chose.
    pELItem PickedItem() { return m_item; }
    pELGenericInfo PickedInfo() { return m_info; }

    void DoDataExchange(CDataExchange *pDX);
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog();

    void OnOK();
private:
    pELItemList m_list;
    pELItem m_item;
    TARGETINFO m_info_type;
    pELGenericInfoList m_info_list;
    pELGenericInfo m_info;

    CListBox m_list_wnd;
};