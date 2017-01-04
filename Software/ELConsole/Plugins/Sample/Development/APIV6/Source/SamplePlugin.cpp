// SamplePlugin.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "helpers.h"
#include <string>
#include "NormalPluginDlg.h"
#include "WindowOnlyDlg.h"
#include "TextControl.h"
#include "PlotControl.h"
#include "CANWindowDlg.h"
#include "ChannelListDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Just some basic MFC DLL stuffs.
class CSamplePluginApp : public CWinApp {
public:
    CSamplePluginApp();

public:
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};

extern CSamplePluginApp theApp;

BEGIN_MESSAGE_MAP(CSamplePluginApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CSamplePluginApp::CSamplePluginApp() {
#if defined(AFX_RESTART_MANAGER_SUPPORT_RESTART)
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
#endif
}

CSamplePluginApp theApp;

BOOL CSamplePluginApp::InitInstance() {
    CWinApp::InitInstance();

    return TRUE;
}


/* Functions required for being a plugin */
char dll_name[] = "APIV6 - SamplePlugin";

extern "C" __declspec(dllexport) void GetPlugInName(char **name, unsigned int *name_size) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    *name = dll_name;
    *name_size = sizeof(dll_name);
}

/***
 InitializeDockablePlugIn
 Name is off for legacy reasons.
 This is the initial call from the host to the plugin.  Versions are checked by the host is order to allow release of plugin if unsupported.
 The version checks are honor based, so the cheat below is allowed but may cause issues if using the same plugin on a higher version API host...

 Parameters:
 versions - The versions as known by the host, this struct is not up to date with all interfaces.  Best to check individual interfaces manually if
            a specific version is needed by this plugin.
 host     - The host interface, this holds the main communication between a plugin and the host.  The majority of supported interfaces are retrieved from this interface.
 generic_functions - A non-interface based function table used for lower overhead calls, mainly when displaying certain data or doing allocations for generic data.

 Return:
 A pointer to the struct of versions supported/expected by this plugin.  Quick cheat: Just return the versions given by the host. Cheap, easy, but kind of wrong...
***/
extern "C" __declspec(dllexport) const pELVersions InitializeDockablePlugIn(const pELVersions versions, pELHost host, const pELGenericFunctionTable generic_functions) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // Set up and initialize all our interfaces.  From those given in host.
    dll_gen = (const ELGenericFunctionTable *)generic_functions;
    dll_host = host;
    dll_host_info = host->HostInfo();
    dll_layout = host->Layout();
    dll_log = host->LogInfo();
    dll_uc = host->UnitConverter();
    dll_dialogs = host->Dialogs();
    dll_theme = (const ELThemeColors *)dll_layout->ThemeColors();
    dll_target = host->Target(0);
    // This plugin wants can sniffer support, the interfaces and register calls only exist in ELTarget interface version 2 or higher.
    if ( dll_target->InterfaceVersion() >= 2 ) {
        dll_target->CANSniffer(CANCHANNEL_A, &dll_cana);
        dll_target->CANSniffer(CANCHANNEL_B, &dll_canb);
    }
    // This plugin wants to be a model builder, or have direct access to the underlying model.
    // ELModel interface and support only exists in version 3 or higher of a target.
    // NOTE: Note all targets allow modeling, check for success when using this method.
    if ( dll_target->InterfaceVersion() >= 3 ) {
        if ( dll_target->Modeler(&dll_model) != DS_SUCCESS ) { dll_model = NULL; }
    }
    // Terrible way of doing d3d, mainly used for internal plugins, and exists due to legacy.  Either way, get the struct pointer.
    dll_3d = host->D3DData();
    return versions;
}

/***
 InitializeDockablePlugInFinished
 Again, name is off for legacy reasons.
 This function is called by the host when the plugin has been loaded.  This function is called regardless of success or failure.
 This allows the plugin a chance to clean up memory if initialization failed, or to do some final initialization knowing that the host
 has accepted the plugin.

 Parameters:
 status - The status of the host regarding this plugin.  A success means that the plugin is accepted, failure otherwise.
 plugin_ids - The ids of the menu options in the plugin.  The ids in this list are the numbers as supplied by the plugin in the menu
              definition (see below).
 menu_ids - The menu ids given by the host.  The ids in this list are the id number as supplied by the host, not the ids given in 
            the menu definition (see below).
 id_count - The number of ids in each list, the count of both plugin_ids and menu_ids are to be the same.
***/
extern "C" __declspec(dllexport) void InitializeDockablePlugInFinished(PLUGINSTATUS status, UINT * plugin_ids, UINT * menu_ids, UINT id_count) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (status != PLUGINSTATUS_SUCCESS) { return; } // Bad JuJu, host does not like us as a plugin.

    for (UINT i = 0; i < id_count; ++i) {
        if ( plugin_ids[i] == 22 ) {
            // Register a hotkey that calls our Text Control creation.
            // NOTE: This call will fail if the hotkey is already owned by the host.
            dll_host->RegisterHotkey("ALT + SHIFT + T", menu_ids[i]);
            break;
        }
    }
}

/*
dll_menu
This will hold the string syntax on how to define a menu.
The resulting menu will be displayed in the hosts PlugIns menu.
*/
char dll_menu[] =
"Menu {                          "
"  \"Normal PlugIn Window\"=0;   "// Option for generating a simple single instance window.
"  \"Multiple\"=1;               "// Main name for, possibly, many options.
"  SubMenu {                     "
"    \"Window Only\"=20;         "// The actual option of generating a multiple instance window.
"  }                             "
"  \"Controls\"=2;               "
"  SubMenu {                     "
"    \"Text Control\"=22;        " // Creating the Text Control as defined by this plugin.
"  }                             "
"  \"CAN Window\"=3;             "// Display the CAN Sniffer example.
"  \"Create CAN Model\"=4;       "// Create a generic CAN model on the target. (Required Modeling Support)
"  \"Delete Channel\" = 5;       "// Delete a channel example (requires Modeling Support).
"  \"Channel List\"=6;           "// Display the Channel List example.
"  SubMenu {                     "
"   \"None\"=30;                 "// Display channel list with all items displayed.
"   \"All HW Processed\"=31;     "// Display channel list with all but hw processed items displayed.
"   \"All Of Type\"=32;          "// Display channel list with all but item type items displayed.
"   \"All But Parents\"=33;      "// Display channel list with only an items parents displayed.
"   \"All But Children\"=34;     "// Display channel list with only an items children displayed.
"   \"All But HW Processed\"=35; "// Display channel list with only hardware processed items displayed.
"   \"All But Of Type\"=36;      "// Display channel list with only item type items displayed.
"   \"All\"=37;                  "// Display empty channel list, all items filtered.
"  }                             "
"  \"Info Lists\"=7;             "// Display the Info List example.
"  SubMenu {                     "
"   \"Timing Types\"=40;         "
"   \"Edge Triggers\"=41;        "
"   \"Engine Cycles\"=42;        "
"   \"Injector Types\"=43;       "
"   \"Harness Pin Modes\"=44;    "
"   \"CPU Pin Modes\"=45;        "
"   \"Timer Modes\"=46;          "
"   \"Unit Conversions\"=47;     "
"   \"Sync Responses\"=48;       "
"   \"Thermistor Types\"=49;     "
"   \"Fixed Table Types\"=50;    "
"   \"Simple CAN Formats\"=51;   "
"   \"CAN Endianess\"=52;        "
"   \"CAN ID Formats\"=53;       "
"   \"Item Types\"=55;           "
"   \"Data Types\"=56;           "
"   \"CAN Packet Data Types\"=57;"
"   \"Serial Stream Data Types\"=58;"
"   \"Communication Data Types\"=59;"
"   \"Bus Channels\"=60;         "
"   \"Bus Directions\"=61;       "
"   \"TX Modes\"=62;             "
"   \"Latch Edges\"=63;          "
"   \"Latch Start States\"=64;   "
"  }                             "
"  \"Close\"=8;                  "// Close the plugin?  Useful if the plugin is one main window.
"}                               ";
/***
 GetPlugInMenuString
 The way the host will request the menu definition from this plugin.
 Quick way, make use of compiler and set like example.

 A menu cannot be changed after this call and the host will only call this function once per plugin.
 An error in the menu syntax will cause the host to unload the plugin and consider it to be invalid.

 The memory of the menu_string is expected to last until InitializeDockablePlugInFinished is called
 by the host.

 Parameters:
 menu_string - The pointer to set to our menu definition.  Set to NULL if not defined menu is expected.
 menu_size - The length in bytes of the menu definition.
***/
extern "C" __declspec(dllexport) void GetPlugInMenuString(char **menu_string, UINT *menu_size) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // Set the pointer to our defined menu.
    *menu_string = dll_menu;
    // Let the host know our definition 'length'.
    *menu_size = sizeof(dll_menu);
}

// Just some helper data used for single instance window.
NormalPluginDlg *g_normal_plugin_dlg = NULL;
int current_text_window = 0;
int current_plot_window = 0;

/***
 CreateNormalPlugIn
 A helper function used only by this plugin.  Will create the single instance window (if it does not already exist) and
 return the host ready interface.

 Parameters:
 parent - The HWND of the main host window.  This is useful for generating children of the window so that minimize and close
          are handled automcatically by Windows.

 Returns:
 NULL - If a failure occurred.
 pELWindow - If the window exists, or if creation succeeded.
***/
pELWindow CreateNormalPlugIn(HWND parent) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if ( !g_normal_plugin_dlg ) {
        // Single instance, only want to call this if the window does not already exist.
        g_normal_plugin_dlg = new NormalPluginDlg();
        if ( g_normal_plugin_dlg == NULL ) { return NULL; }
        if ( g_normal_plugin_dlg->Create(parent) != TRUE ) {
            delete g_normal_plugin_dlg;
            g_normal_plugin_dlg = NULL;
            return NULL;
        }
    }
    return &g_normal_plugin_dlg->_window;
}

// The CAN Sniffer window is also single instance.
CANWindowDlg *g_can_window_dlg = NULL;

/***
 CreateCANWindow
 This helper function is only used by this plugin.  Creates an instance of the CANSniffer IFF the window does not already exist.

 Parameters:
 parent - The HWND of the main host window.  This is useful for generating children of the window so that minimize and close
          are handled automcatically by Windows.

 Returns:
 NULL - If a failure occurred.
 pELWindow - If the window exists, or if creation succeeded.
***/
pELWindow CreateCANWindow(HWND parent) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (!g_can_window_dlg) {
        g_can_window_dlg = new CANWindowDlg();
        if ( !g_can_window_dlg ) { return NULL; }
        if ( g_can_window_dlg->Create(parent) != TRUE ) { return NULL; }
    }
    return &g_can_window_dlg->_window;
}

/***
 CreateWindowOnly
 This function is a helper for this plugin.  Used to generate the multiple instance example of the plugin.

 Parameters:
 parent - The HWND of the main host window.  This is useful for generating children of the window so that minimize and close
          are handled automcatically by Windows.

 Returns:
 NULL - If a failure occurred.
 pELWindow - If a new window was created successfully.
***/
pELWindow CreateWindowOnly(HWND parent) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    WindowOnlyDlg *dlg = NULL;
    {
        dlg = new WindowOnlyDlg;
        if ( !dlg ) { return NULL; }
        if ( dlg->Create(parent) != TRUE ) { return NULL; }
    }
    return static_cast<ELWindow *>(dlg);
}

/***
 HostReady
 This function is called by the host when it is ready to receive window creations (window meaning ELWindow types
 as they are maintained by the host.)  The host will only call this function if initialization of the plugin was
 successfull.

 Parameters:
 host - The same host as given in Initialize stage.
 layout - Just a helper point to the layout of the host.
***/
extern "C" __declspec(dllexport) void HostReady(pELHost host, pELLayout layout) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}

/***
 MenuClicked
 This function is called whenever a menu option as define by this plugin is clicked.
 Gives the plugin a way to handle the above defined menu.

 Parameters:
 menu_id - The number of the option as supplied in the menu definition above.
***/
extern "C" __declspec(dllexport) void MenuClicked(WORD menu_id) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    bool display_channel_list = false;
    ITEMFILTER cl_filter = 0;
    bool display_info_list = false;
    TARGETINFO il_targetinfo = 0;
    switch (menu_id) {
        case 0: /* Normal PlugIn Window */
            {
                pELWindow w = CreateNormalPlugIn(dll_host->GetHWND());
                UINT page = 0;
                if ( dll_layout->PageIndex(w, &page) == DS_SUCCESS ) {
                    dll_layout->GotoPage(page);
                }
                g_normal_plugin_dlg->ShowWindow(SW_SHOWNORMAL);
                g_normal_plugin_dlg->SetFocus();
            }
            break;

        case 1: /* Multiple */
            // Fall through to nothing, as the sub menu will handle the creation if picked.
        case 2: /* Controls */
            // Fall through to nothing, as the sub menu will handle the creation if picked.
        case 6: /* Channel List */
        case 8: /* Close */
            // Fall through as this plugin does not really have a close...
            break;

        case 3: /* CAN Window */
            {
                // Only handle this option if it was determined that the target/host supports can sniffing.
                if (!dll_cana || !dll_canb) {
                    ::MessageBox(dll_host->GetHWND(), _T("Not supported with current host version."), _T("Error"), MB_OK);
                }
                else {
                    pELWindow w = CreateCANWindow(dll_host->GetHWND());
                    if ( w ) {
                        UINT page = 0;
                        // Incase the single instance already exists, tell the layout to switch to the page where the window resides.
                        // If the window is not docked to a page, this will be ignored.
                        if (dll_layout->PageIndex(w, &page) == DS_SUCCESS) {
                            dll_layout->GotoPage(page);
                        }
                        // And of course, make sure the window is visible and has focus.
                        g_can_window_dlg->ShowWindow(SW_SHOWNORMAL);
                        g_can_window_dlg->SetFocus();
                    }
                }
            }
            break;

        case 4: /* Create CAN Model */
            {
                // Only handled if the target handles modeling.
                if ( !dll_model ) {
                    ::MessageBox(dll_host->GetHWND(), _T("Not supported with current host version."), _T("Error"), MB_OK);
                }
                else {
                    pELGenericInfoList item_types = NULL;
                    pELGenericInfoList bus_channels = NULL;
                    pELGenericInfoList bus_directions = NULL;
                    pELGenericInfoList tx_modes = NULL;
                    pELGenericInfoList data_types = NULL;
                    // Need to make sure the target understands what we need.
                    if ( dll_target->InfoList(TARGETINFO_ITEM_TYPES, &item_types) != DS_SUCCESS ||
                         dll_target->InfoList(TARGETINFO_BUS_CHANNELS, &bus_channels) != DS_SUCCESS ||
                         dll_target->InfoList(TARGETINFO_BUS_DIRECTIONS, &bus_directions) != DS_SUCCESS ||
                         dll_target->InfoList(TARGETINFO_TX_MODES, &tx_modes) != DS_SUCCESS ||
                         dll_target->InfoList(TARGETINFO_CAN_PACKET_DATA_TYPES, &data_types) != DS_SUCCESS
                        ) {
                        ::MessageBox(dll_host->GetHWND(), _T("Could not obtain lists"), _T("Error"), MB_OK);
                        break;
                    }

                    // First generate the no_ops.
                    WORD noop1_ch = 0;
                    WORD noop2_ch = 0;
                    WORD noop3_ch = 0;
                    pELNoOperationItem noop = NULL;
                    if ( dll_model->NewItem((pELItemType)item_types->Info(ITEMTYPE_NONE), "AG_NoOp1",(pELItem*)&noop) != DS_SUCCESS ) { break; }
                    if ( dll_model->CreateItem(noop, NULL) != DS_SUCCESS ) { noop->Release(); break; }
                    noop1_ch = noop->Channel();
                    noop->Header()->SetName("AG_NoOp2");
                    // Need to adjust the channel as well.
                    noop->Header()->SetChannel(dll_model->NextAvailableChannelNumber());
                    if ( dll_model->CreateItem(noop, NULL) != DS_SUCCESS ) { noop->Release(); break; }
                    noop2_ch = noop->Channel();
                    noop->Header()->SetName("AG_NoOP3");
                    noop->Header()->SetChannel(dll_model->NextAvailableChannelNumber());
                    if ( dll_model->CreateItem(noop, NULL) != DS_SUCCESS ) { noop->Release(); break; }
                    noop3_ch = noop->Channel();
                    noop->Release();

                    // Create the CANPacket Item.
                    pELCANPacketItem can = NULL;
                    if ( dll_model->NewItem((pELItemType)item_types->Info(ITEMTYPE_CANPACKET), "AG_CanPacket Tx", (pELItem*)&can) != DS_SUCCESS ) { break; }
                    // Only wanting this item to take at max 10 entries. (Only for examples sake);
                    can->SetNumberOfEntries(10);
                    can->ChannelBusOverride()->SetChannel((pELBusChannel)bus_channels->Info(BUSCHANNEL_CHANNELA));
                    can->ChannelBusOverride()->SetDirection((pELBusDirection)bus_directions->Info(BUSDIRECTION_TRANSMIT));
                    can->ChannelBusOverride()->SetModeTx((pELBusTxMode)tx_modes->Info(TXMODE_PERIODIC));
                    if ( dll_model->CreateItem(can, NULL) != DS_SUCCESS ) { can->Release(); break; }
                    can->Release();

                    BYTE buffer[0x1000] = {0};
                    pConfigurationCANPacketItem_modifiable_TS item = (pConfigurationCANPacketItem_modifiable_TS)buffer;
                    // Can channel created.  Have to use older style for updating (currently).
                    if ( dll_target->GetModifiableItem("AG_CanPacket Tx", (pConfigurationItemHeader_modifiable_TS)item) == DS_SUCCESS ) {
                        item->addresses.var.dword_data[0] = 0x00000102;
                        // The last entry for any given packet must flag the end of the packet with a PACKET_END
                        item->addresses.var.dword_data[1] = 0x00000102 | 0x40000000; //TS_CANPACKET_ITEM_MODIFIABLE_PACKET_END;
                        item->suggested_iteration_count.var.byte_data[0] = 0;
                        item->suggested_iteration_count.var.byte_data[1] = 0;
                        item->internal_inputs.var.word_data[0] = noop1_ch;
                        item->internal_input_data_pointers.var.dword_data[0] = 0;
                        item->internal_inputs.var.word_data[1] = noop2_ch;
                        item->internal_input_data_pointers.var.dword_data[1] = 0;
                        item->data_bits.var.byte_data[0] = 0x20;
                        item->data_shift.var.byte_data[0] = 0x00;
                        item->data_type.var.byte_data[0] = (BYTE)data_types->Info(DATATYPE_FLOAT_2_FLOAT)->Value();
                        item->data_min_value.var.float_data[0] = 0.0f;
                        item->data_max_value.var.float_data[0] = 1.0f;
                        // The second entry eats up the other 4 bytes of the packet.
                        item->data_bits.var.byte_data[1] = 0x20;
                        item->data_shift.var.byte_data[1] = 0x20;
                        item->data_type.var.byte_data[1] = (BYTE)data_types->Info(DATATYPE_FLOAT_2_FLOAT)->Value();
                        item->data_min_value.var.float_data[1] = 0.0f;
                        item->data_max_value.var.float_data[1] = 1.0f;

                        // For good measure, set the remaining addresses to unused.
                        for (int i = 2; i < 10; ++i) {
                            item->addresses.var.dword_data[i] = 0xFFFFFFFF; // TS_CANPACKET_ITEM_MODIFIABLE_ADDRESS_UNUSED
                        }

                        // Should now be ready, send update to target.
                        if ( dll_target->SetModifiableItem("AG_CanPacket Tx", (pConfigurationItemHeader_modifiable_TS)item) != DS_SUCCESS ) { 
                            ::MessageBox(dll_host->GetHWND(), _T("Could not modify the CAN item"), _T("Error"), MB_OK);
                        }
                    }
                }
            }
            break;

        case 5: /* Delete Channel */
            // Handle this only if the target handles modeling and supports info lists.
            if ( dll_target->InterfaceVersion() < 3 || !dll_model ) {
                ::MessageBox(dll_host->GetHWND(), _T("Not supported with current host version."), _T("Error"), MB_OK);
            }
            else {
                pELItemList list = NULL;
                // Do not display predefined, as they cannot be deleted anyways.
                // Essentially, display only those items that would come up the color blue in the hosts channel list.
                DOCKABLE_STATUS ds = dll_target->ItemList(ITEMFILTER_ALL_OF_TYPE | (ITEMTYPE_PREDEF << ITEMFILTER_TYPE_SHIFT), &list);
                if ( ds == DS_NOT_IMPLEMENTED ) {
                    // Sadly... This is possible.
                    ::MessageBox(dll_host->GetHWND(), _T("Not supported with current host version."), _T("Error"), MB_OK);
                    break;
                }

                if ( ds != DS_SUCCESS ) {
                    ::MessageBox(dll_host->GetHWND(), _T("Could not retrieve item list"), _T("Error"), MB_OK);
                    break;
                }
                
                // Display delete dialog.  Modal.
                ChannelListDlg dlg(list);
                if ( dlg.DoModal() == IDOK ) {
                    if ( dlg.PickedItem() ) {
                        // We want to delete the item, but make sure the name exists for easy replacement if/when needed.
                        dll_model->DeleteItem(dlg.PickedItem(), DELETEITEMOPTION_GENERATE_EXTERNS); // <-- There, the externs allow for easier recreation.
                    }
                }

                // And, always free host memory.
                list->Release();
            }
            break;

        case 30: /* Channel List -> None */
            display_channel_list = dll_target->InterfaceVersion() >= 3;
            cl_filter = ITEMFILTER_NONE;
            break;

        case 31: /* Channel List -> All HW Processed */
            display_channel_list = dll_target->InterfaceVersion() >= 3;
            cl_filter = ITEMFILTER_ALL_HW_PROCESSED;
            break;

        case 32: /* Channel List -> All Of Type */
            if ( dll_target->InterfaceVersion() >= 3 ) {
                pELGenericInfoList list = NULL;
                if (dll_target->InfoList(TARGETINFO_ITEM_TYPES, &list) == DS_SUCCESS) {
                    ChannelListDlg dlg(TARGETINFO_ITEM_TYPES, list);
                    if (dlg.DoModal() == IDOK && dlg.PickedInfo()) {
                        display_channel_list = true;
                        cl_filter = ITEMFILTER_ALL_OF_TYPE | (dlg.PickedInfo()->Value() << ITEMFILTER_TYPE_SHIFT);
                    }
                }
            }
            break;

        case 33: /* Channel List -> All But Parents */
            if ( dll_target->InterfaceVersion() >= 3 ) {
                pELItemList list = NULL;
                if (dll_target->ItemList(ITEMFILTER_NONE, &list) == DS_SUCCESS) {
                    ChannelListDlg dlg(list);
                    if (dlg.DoModal() == IDOK && dlg.PickedItem()) {
                        display_channel_list = true;
                        cl_filter = ITEMFILTER_ALL_BUT_PARENTS_OF_ITEM | (dlg.PickedItem()->Channel() << ITEMFILTER_CHANNEL_SHIFT);
                    }
                }
            }
            break;

        case 34: /* Channel List -> All But Children */
            if ( dll_target->InterfaceVersion() >= 3 ) {
                pELItemList list = NULL;
                if (dll_target->ItemList(ITEMFILTER_NONE, &list) == DS_SUCCESS) {
                    ChannelListDlg dlg(list);
                    if (dlg.DoModal() == IDOK && dlg.PickedItem()) {
                        display_channel_list = true;
                        cl_filter = ITEMFILTER_ALL_BUT_CHILDREN_OF_ITEM | (dlg.PickedItem()->Channel() << ITEMFILTER_CHANNEL_SHIFT);
                    }
                }
            }
            break;

        case 35: /* Channel List -> All But HW Processed */
            display_channel_list = dll_target->InterfaceVersion() >= 3;
            cl_filter = ITEMFILTER_ALL_BUT_HW_PROCESSED;
            break;

        case 36: /* Channel List -> All But Of Type */
            if (dll_target->InterfaceVersion() >= 3) {
                pELGenericInfoList list = NULL;
                if (dll_target->InfoList(TARGETINFO_ITEM_TYPES, &list) == DS_SUCCESS) {
                    ChannelListDlg dlg(TARGETINFO_ITEM_TYPES, list);
                    if (dlg.DoModal() == IDOK && dlg.PickedInfo()) {
                        display_channel_list = true;
                        cl_filter = ITEMFILTER_ALL_BUT_OF_TYPE | (dlg.PickedInfo()->Value() << ITEMFILTER_TYPE_SHIFT);
                    }
                }
            }
            break;

        case 37: /* Channel List -> All */
            display_channel_list = dll_target->InterfaceVersion() >= 3;
            cl_filter = ITEMFILTER_ALL;
            break;

        case 20: /* Window Only */
            {
                // This casting only works because we know WindowOnlyDlg is also an ELWindow...
                WindowOnlyDlg *dlg = static_cast<WindowOnlyDlg*>(CreateWindowOnly(dll_host->GetHWND()));
                if ( dlg ) {
                    dlg->ShowWindow(SW_SHOWNORMAL);
                    dlg->SetFocus();
                }
            }
            break;

        case 22: /* Text Control */
            {
                // This is awkward, our Text Control actually replaces the Hosts base Text Control
                // As such, some of those things that are auto handled by the host needs to be manually done when our menu option is clicked.
                ELControlWindow *tmp = CreateText();
                if ( tmp ) {
                    // Controls in the host do not create the window until it needs to be visible
                    // As we are creating the control now, create the window so the user can see.
                    if ( tmp->CreateHWND(dll_host->GetHWND()) == NULL ) {
                        tmp->Destroy();
                        break;
                    }
                    // And showing the window helps.
                    ::ShowWindow(tmp->GetHWND(), SW_SHOW);

                    // The control is created, make sure the host knows about it.
                    // Hand off control to the host.
                    if ( dll_layout->RegisterWindow(tmp) != DS_SUCCESS ) {
                        tmp->Destroy();
                        break;
                    }

                    // The system menu is the menu that appears when right clicking in the upper left (usually) of the window.
                    // This gives the host time to add those standard menu options.
                    dll_layout->UpdateSystemMenu(tmp);
                    // We are telling our new text to redo everything, to allow it to retrieve/display proper information.
                    tmp->HostStateChanged(dll_host->HostInfo(), HOSTSTATE_ALL);

                    // And now, let our new control grab the colors needed to mesh well with the users current host theme.
                    tmp->ThemeChanged(dll_layout->ThemeColors());

                    // As the host usually handles allowing the user to pick a channel, we need to manually do the same thing.
                    pELStringArray psa = NULL;
                    if ( dll_gen->AllocateStringArray(1, TRUE, &psa) == DS_SUCCESS ) {
                        // Being cheap, just set our own channel.
                        if ( dll_gen->SetString(psa->elements[0], "NoOp_Channel_1", (UINT)-1) == DS_SUCCESS ) {
                            psa->count = 1;
                            // Inform the host of our change/selection.
                            dll_host->SetChannelsEx(tmp->Control(), psa);
                        }
                    }
                    dll_gen->FreeStringArray(&psa);
                }
            }
            break;

        default: break;
    }

    if (menu_id >= 40 && menu_id <= 64) {
        menu_id -= 40;
        il_targetinfo = menu_id;
        display_info_list = true;
    }

    if ( display_channel_list ) {
        pELItemList list = NULL;
        if (dll_target->ItemList(cl_filter, &list) == DS_SUCCESS) {
            ChannelListDlg dlg(list);
            dlg.DoModal();
            list->Release();
        }
    }

    if (display_info_list) {
        pELGenericInfoList list = NULL;
        if (dll_target->InfoList(il_targetinfo, &list) == DS_SUCCESS) {
            ChannelListDlg dlg(il_targetinfo, list);
            dlg.DoModal();
        }
    }
}

/***
 TargetConnected
 This function is called by the host when a target has been connected/disconnected.
 Some hosts allow multiple targets to be connected at once, as such the target given is the
 target that caused this notification.

 Parameters:
 target - The target that was connected.
 target - The target that was disconnected.
***/
extern "C" __declspec(dllexport) void TargetConnected(pELTarget target) { }
/***
 TargetDisconnected
 This function is called by the host when a target has been connected/disconnected.
 Some hosts allow multiple targets to be connected at once, as such the target given is the
 target that caused this notification.

 Parameters:
 target - The target that was disconnected.
***/
extern "C" __declspec(dllexport) void TargetDisconnected(pELTarget target) { }

/***
 dll_controls
 This list holds those items that follow the rules of a Control for the host.  The names are needed
 as controls can and will be saved to layouts, the name allows for loading a layout to succeed if this plugin
 exists.
***/
ELControlInfo dll_controls[] = {
    {"Text", CreateText}, // Can be created by the host, or manually with our own options.  This will overwrite the hosts base Text control.
    {"Plot GDI", CreatePlotGDI}, // For simplicity; only allow the host to create this.
    {"Plot D3D", CreatePlotD3D}, // For simplicity; only allow the host to create this.
    {NULL, NULL} // Host expects the last entry to be invalid.
};
/***
 SupportedControls
 Return the list of controls this plugin supports.  This list does not mean the control will be used, as the user can decide not to use the
 control or another plugin loaded later will overwrite our controls.  The name is the key used for controls.
***/
extern "C" __declspec(dllexport) pELControlInfo SupportedControls() { return dll_controls; }

ELWindowInfo dll_windows[] = {
    {"NormalPlugIn_v6", CreateNormalPlugIn},
    {"WindowOnly", CreateWindowOnly},
    {NULL, NULL}
};
/***
 SavableWindows
 This function is called so that the host can get the list of normal windows that are supported in the layout.
 If a layout window is loaded with the name, our create function will be called.

 This holds true only if no other plugin overwrites our names with their own functions.
***/
extern "C" __declspec(dllexport) pELWindowInfo SavableWindows() { return dll_windows; }