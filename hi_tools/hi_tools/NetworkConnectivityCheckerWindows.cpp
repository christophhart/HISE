/*  ===========================================================================
*
*   Network connectivity detection for Windows.
*   Based on code from SquarePine Modules (https://gitlab.com/squarepine/squarepinemodules)
*   Used under the MIT licence.
*
*   ===========================================================================
*/

#if JUCE_WINDOWS

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wlanapi.lib")

namespace hise {

namespace winConnectivityHelpers
{
    // @see https://msdn.microsoft.com/en-us/library/windows/desktop/aa814491(v=vs.85).aspx
    // "PhysicalMediumType" for a full list
    [[nodiscard]] Array<MIB_IF_ROW2> getEnabledNetworkDevices (NDIS_PHYSICAL_MEDIUM type)
    {
        Array<MIB_IF_ROW2> devices;

        PMIB_IF_TABLE2 table = nullptr;
        if (GetIfTable2Ex (MibIfTableRaw, &table) == NOERROR && table != nullptr)
        {
            devices.ensureStorageAllocated ((int) table->NumEntries);

            for (decltype (table->NumEntries) i = 0; i < table->NumEntries; ++i)
            {
                MIB_IF_ROW2 row;
                zerostruct (row);

                row.InterfaceIndex = static_cast<decltype (row.InterfaceIndex)> (i);

                if (GetIfEntry2 (&row) == NOERROR
                    && row.PhysicalMediumType == type
                    && row.TunnelType == TUNNEL_TYPE_NONE
                    && row.AccessType != NET_IF_ACCESS_LOOPBACK
                    && row.InterfaceAndOperStatusFlags.HardwareInterface == TRUE)
                {
                    devices.add (row);
                }
            }

            FreeMibTable (table);
        }

        devices.minimiseStorageOverheads();
        return devices;
    }

    [[nodiscard]] Array<MIB_IF_ROW2> getAllEnabledNetworkDevices()
    {
        Array<MIB_IF_ROW2> devices;
        for (int i = 0; i < NdisPhysicalMediumMax; ++i)
            devices.addArray (getEnabledNetworkDevices ((NDIS_PHYSICAL_MEDIUM) i));

        return devices;
    }
}

NetworkConnectivityChecker::NetworkType getCurrentSystemNetworkType()
{
    using namespace winConnectivityHelpers;

    for (const auto& possibleNetworkDevice : getAllEnabledNetworkDevices())
    {
        if (possibleNetworkDevice.MediaConnectState == MediaConnectStateConnected)
        {
            switch (possibleNetworkDevice.Type)
            {
                case IF_TYPE_IEEE80211:
                case 281 /* IF_TYPE_XBOX_WIRELESS */:
                    return NetworkConnectivityChecker::NetworkType::wifi;

                case IF_TYPE_ETHERNET_CSMACD:
                case IF_TYPE_ETHERNET_3MBIT:
                case IF_TYPE_FASTETHER:
                case IF_TYPE_FASTETHER_FX:
                case IF_TYPE_GIGABITETHERNET:
                    return NetworkConnectivityChecker::NetworkType::wired;

                case IF_TYPE_WWANPP:
                case IF_TYPE_WWANPP2:
                    return NetworkConnectivityChecker::NetworkType::mobile;

                default:
                    return NetworkConnectivityChecker::NetworkType::other;
            };
        }
    }

    return NetworkConnectivityChecker::NetworkType::none;
}

double getCurrentSystemRSSI()
{
    using namespace winConnectivityHelpers;

    DWORD maxClient = 2;
    DWORD currentVersion = 0;
    HANDLE clientHandle = INVALID_HANDLE_VALUE;
    if (WlanOpenHandle (maxClient, nullptr, &currentVersion, &clientHandle) != ERROR_SUCCESS)
        return 0.0;

    PWLAN_INTERFACE_INFO_LIST interfaceInfoList = nullptr;
    if (WlanEnumInterfaces (clientHandle, nullptr, &interfaceInfoList) != ERROR_SUCCESS)
        return 0.0;

    PWLAN_INTERFACE_INFO interfaceInfo = nullptr;
    for (size_t i = 0; i < (size_t) interfaceInfoList->dwNumberOfItems; ++i)
    {
        auto& info = interfaceInfoList->InterfaceInfo[i];
        if (info.isState == wlan_interface_state_connected)
        {
            interfaceInfo = &info;
            break;
        }
    }

    if (interfaceInfo == nullptr)
        return 0.0;

    auto opCode = wlan_opcode_value_type_invalid;
    PWLAN_CONNECTION_ATTRIBUTES connectionAttributes = nullptr;
    auto connectInfoSize = (DWORD) sizeof (WLAN_CONNECTION_ATTRIBUTES);

    if (WlanQueryInterface (clientHandle, &interfaceInfo->InterfaceGuid,
                            wlan_intf_opcode_current_connection,
                            nullptr, &connectInfoSize,
                            (PVOID*) &connectionAttributes, &opCode) != ERROR_SUCCESS)
    {
        return 0.0;
    }

    if (WlanScan (clientHandle, &interfaceInfo->InterfaceGuid,
                  &connectionAttributes->wlanAssociationAttributes.dot11Ssid,
                  nullptr, nullptr) != ERROR_SUCCESS)
    {
        return 0.0;
    }

    PWLAN_BSS_LIST wlanBssList = nullptr;
    if (WlanGetNetworkBssList (clientHandle, &interfaceInfo->InterfaceGuid,
                               &connectionAttributes->wlanAssociationAttributes.dot11Ssid,
                               dot11_BSS_type_infrastructure, TRUE,
                               nullptr, &wlanBssList) != ERROR_SUCCESS)
    {
        return 0.0;
    }

    // Link quality is more reliable than raw RSSI on Windows -- see:
    // https://www.syatech.com/blog/2017/12/05/windows-wifi-madness/
    return static_cast<double> (wlanBssList->wlanBssEntries[0].uLinkQuality) / 100.0;
}

} // namespace hise

#endif // JUCE_WINDOWS
