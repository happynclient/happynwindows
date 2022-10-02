#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <objbase.h>
#include <netcon.h>
#include <windows.h>

#include "registry.h"
#include "netinterface.h"
#include "utils.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

#define MAX_TRIES 3
#define MAX_ADAPTER_NAME_LEN 1024
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
const ULONG WORKING_BUFFER_SIZE = 15000;
#define NPCAP_LOOPBACK_INTERFACE_NAME			TEXT("HAPPYNET")


static BOOL rename_netinterface_by_id(INetSharingManager *pNSM, wchar_t device_uuid[])
{   // add a port mapping to every firewalled or shared connection 
    BOOL is_found = FALSE;
    INetSharingEveryConnectionCollection * nsecc_ptr = NULL;
    HRESULT hr = pNSM->get_EnumEveryConnection(&nsecc_ptr);
    if (!nsecc_ptr)
        LogEvent(TEXT("failed to get EveryConnectionCollection!\r\n"));
    else {

        // enumerate connections
        IEnumVARIANT * ev_ptr = NULL;
        IUnknown * unk_ptr = NULL;
        hr = nsecc_ptr->get__NewEnum(&unk_ptr);
        if (unk_ptr) {
            hr = unk_ptr->QueryInterface(__uuidof(IEnumVARIANT),
                (void**)&ev_ptr);
            unk_ptr->Release();
        }
        if (ev_ptr) {
            VARIANT v;
            VariantInit(&v);

            while ((S_OK == ev_ptr->Next(1, &v, NULL)) && (is_found == FALSE)) {
                if (V_VT(&v) == VT_UNKNOWN) {
                    INetConnection * pNC = NULL;
                    V_UNKNOWN(&v)->QueryInterface(__uuidof(INetConnection),
                        (void**)&pNC);
                    if (pNC) {
                        NETCON_PROPERTIES *pNETCON_PROPERTIES;
                        pNC->GetProperties(&pNETCON_PROPERTIES);

                        wchar_t currentGUID[255];
                        GUID guid = pNETCON_PROPERTIES->guidId;
                        wsprintf(currentGUID, TEXT("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
                            guid.Data1, guid.Data2, guid.Data3,
                            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

                        if (wcscmp(currentGUID, device_uuid) == 0)
                        {
                            hr = pNC->Rename(NPCAP_LOOPBACK_INTERFACE_NAME);
                            is_found = TRUE;
                            if (hr != S_OK)
                            {
                                LogEvent(TEXT("failed to create rename NPCAP_LOOPBACK_INTERFACE_NAME\r\n"));
                            }
                        }

                        pNC->Release();
                    }
                }
                VariantClear(&v);
            }
            ev_ptr->Release();
        }
        nsecc_ptr->Release();
    }

    return is_found;
}

BOOL set_netinterface_name_by_id(WCHAR device_uuid[])
{
    BOOL ret = FALSE;
    /*	CoInitialize (NULL);*/

        // init security to enum RAS connections
    CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_PKT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);

    INetSharingManager * nsm_ptr = NULL;
    HRESULT hr = ::CoCreateInstance(__uuidof(NetSharingManager),
        NULL,
        CLSCTX_ALL,
        __uuidof(INetSharingManager),
        (void**)&nsm_ptr);
    if (!nsm_ptr)
    {
        LogEvent(TEXT("failed to create NetSharingManager object\r\n"));
        return ret;
    }
    else {

        // add a port mapping to every shared or firewalled connection.
        ret = rename_netinterface_by_id(nsm_ptr, device_uuid);

        nsm_ptr->Release();
    }

    /*	CoUninitialize ();*/

    return ret;
}


VOID get_mac_address(WCHAR* mac_address, WCHAR* guid)
{
	PIP_ADAPTER_ADDRESSES addresses = NULL;
	PIP_ADAPTER_ADDRESSES curr_addr;
	ULONG buf_len = 0;
	ULONG err = GetAdaptersAddresses(AF_INET, NULL, NULL, NULL, &buf_len);
	if (err == ERROR_BUFFER_OVERFLOW)
	{
		addresses = (IP_ADAPTER_ADDRESSES*)HeapAlloc(GetProcessHeap(), 0, buf_len);
		err = GetAdaptersAddresses(AF_INET, NULL, NULL, addresses, &buf_len);
		if (err == ERROR_SUCCESS)
		{
			curr_addr = addresses;
			CHAR* guid_ansi = NULL;
			guid_ansi = new char[wcslen(guid) + 1];
			UINT num_conv;
			wcstombs_s(&num_conv, guid_ansi, wcslen(guid) + 1, guid, _TRUNCATE);
			while (curr_addr != NULL)
			{
				if (_stricmp(curr_addr->AdapterName, guid_ansi))
				{
					curr_addr = curr_addr->Next;
					continue;
				}
				BYTE* a = curr_addr->PhysicalAddress;
				for (int i = 0; i < 5; i++)
				{
					wsprintf(mac_address, TEXT("%02X:%02X:%02X:%02X:%02X:%02X"),
                                a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
				}
				break;
			}
			delete [] guid_ansi;
		}
		HeapFree(GetProcessHeap(), 0, addresses);
	}
}

DWORD get_adapter_friendly_name(CHAR* adapter_name, WCHAR* friendly_name, ULONG max_name_length)
{
    ULONG out_buf_len = WORKING_BUFFER_SIZE;
    ULONG iterations = 0;
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    DWORD ret_value = 0;
    do {
        pAddresses = (IP_ADAPTER_ADDRESSES *)MALLOC(out_buf_len);
        if (pAddresses == NULL) {
            LogEvent(TEXT("Error:%s"), TEXT("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n"));
            return E_FAIL;
        }

        ret_value = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &out_buf_len);

        if (ret_value == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
            return E_FAIL;
        }
        iterations++;
    } while ((ret_value == ERROR_BUFFER_OVERFLOW) && (iterations < MAX_TRIES));

    PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
    while (pCurrAddresses) {
        if (!lstrcmpA(pCurrAddresses->AdapterName, adapter_name)) {
            UINT friendly_name_length = lstrlenW(pCurrAddresses->FriendlyName);
            if (friendly_name_length < max_name_length && friendly_name_length > 0) {
                lstrcpynW(friendly_name, pCurrAddresses->FriendlyName, friendly_name_length + 1);
                if (pAddresses != NULL) {
                    FREE(pAddresses);
                    pAddresses = NULL;
                }
                return NOERROR;
            }
        }
        pCurrAddresses = pCurrAddresses->Next;
    }
    if (pAddresses != NULL) {
        FREE(pAddresses);
        pAddresses = NULL;
    }
    return E_FAIL;
}


VOID get_addresses(WCHAR* ip_address, WCHAR* mac_address)
{
	HKEY hkey_adapters, hkey_adapter, hkey_ip, hkey_ipg;
	DWORD i = 0;
	WCHAR net_key[] = TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
	WCHAR tcpip_key[] = TEXT("SYSTEM\\CurrentControlSet\\services\\Tcpip\\Parameters\\Interfaces");
	WCHAR subkey_name[128];
	WCHAR component_id[128];
	WCHAR guid[39];
	DWORD subkey_size = 128;
	DWORD component_size = 128 * sizeof(WCHAR);
	DWORD guid_size = 39 * sizeof(WCHAR);
	BOOL adapter_found = FALSE;

	// Clear the addresses
	wsprintf(ip_address, TEXT("<unknown>"));
	wsprintf(mac_address, TEXT("<unknown>"));

    // check if user select adapter manually by ad_option dialog
    WCHAR tmp_buf[MAX_ADAPTER_NAME_LEN] = {0};
    DWORD buf_len = MAX_ADAPTER_NAME_LEN;
    WCHAR *strtok_buf = NULL, *adapter_id = NULL;
    const WCHAR s[4] = TEXT("_");
    HKEY hkey_adapter_id;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"),
                        NULL, KEY_READ, &hkey_adapter_id) == ERROR_SUCCESS)
    {
        // Community
        GetRegString(hkey_adapter_id, TEXT("adapter"), tmp_buf, buf_len);
        adapter_id = wcstok_s(tmp_buf, s, &strtok_buf);
        adapter_id = wcstok_s(NULL, s, &strtok_buf);
        RegCloseKey(hkey_adapter_id);
    }

	// First we need to find the TAP-Win32 adapter (identified by 'tap0901') and get its GUID
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, net_key, NULL, KEY_READ, &hkey_adapters);
    while (RegEnumKeyEx(hkey_adapters, i, subkey_name, &subkey_size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        RegOpenKeyEx(hkey_adapters, subkey_name, NULL, KEY_READ, &hkey_adapter);
        RegQueryValueEx(hkey_adapter, TEXT("ComponentId"), NULL, NULL, (LPBYTE)component_id, &component_size);
        RegQueryValueEx(hkey_adapter, TEXT("NetCfgInstanceId"), NULL, NULL, (LPBYTE)guid, &guid_size);
        RegCloseKey(hkey_adapter);
        if (
            // auto select adapter
            ((adapter_id == NULL) && (!wcscmp(component_id, TEXT("tap0901")) || !wcscmp(component_id, TEXT("TAP0901")))) ||
            // user select adapter manually by ad_option dialog
            ((adapter_id != NULL) && (!wcscmp(guid, adapter_id)))
            )
		{
			adapter_found = true;
			break;
		}
		i++;
		subkey_size = 128;
	}
	RegCloseKey(hkey_adapters);

	// If we have the guid, fetch the IP address from the registry
	if (adapter_found)
	{
		DWORD dhcp_enabled = 0;
		DWORD buf_size = sizeof(DWORD);
		DWORD ip_size = 16 * sizeof(WCHAR);
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, tcpip_key, NULL, KEY_READ, &hkey_ip);
		RegOpenKeyEx(hkey_ip, guid, NULL, KEY_READ, &hkey_ipg);
		RegQueryValueEx(hkey_ipg, TEXT("EnableDHCP"), NULL, NULL, (LPBYTE)&dhcp_enabled, &buf_size);
		if (dhcp_enabled)
		{
			RegQueryValueEx(hkey_ipg, TEXT("DhcpIPAddress"), NULL, NULL, (LPBYTE)ip_address, &ip_size);
		}
		else
		{
			RegQueryValueEx(hkey_ipg, TEXT("IPAddress"), NULL, NULL, (LPBYTE)ip_address, &ip_size);
		}
		RegCloseKey(hkey_ipg);
		RegCloseKey(hkey_ip);

		get_mac_address(mac_address, guid);
	}
}

BOOL validate_ipv4_address(WCHAR* ip_address)
{
	WCHAR c;
	WCHAR octet_val[4];
	octet_val[0] = 0;
	INT octets = 0;
	INT c_octet_len = 0;
	for (INT i = 0; i < (INT)wcslen(ip_address); i++)
	{
		c = ip_address[i];
		if ((c < '0' || c > '9') && c != '.' && c != '/') return FALSE;
		if (c == '.' || c == '/')
		{
			if (c_octet_len < 1 || c_octet_len > 3) return FALSE;
			octets++;
			octet_val[c_octet_len] = 0;
			INT int_octet_val = _wtoi(octet_val);
			if (int_octet_val < 0 || int_octet_val > 255) return FALSE;
			if (octets > 3 && c != '/') return FALSE;
			c_octet_len = 0;
			continue;
		}
		octet_val[c_octet_len] = c;
		c_octet_len++;
		if (c_octet_len > 3) return FALSE;
	}
	octets++;
	if (c_octet_len < 1 || c_octet_len > 3 || (octets != 4  && octets != 5)) return FALSE;
	return TRUE;
}

BOOL validate_mac_address(WCHAR* mac_address)
{
	if (wcslen(mac_address) != 17) return FALSE;
	WCHAR mac_addr_lower[18];
	wcscpy_s(mac_addr_lower, 18, mac_address);
	_wcslwr_s(mac_addr_lower, 18);
	WCHAR* w;
	for (INT i = 0; i < 6; i++)
	{
		w = mac_addr_lower + (i * 3);
		if ((w[0] < '0' || w[0] > '9') && (w[0] < 'a' || w[0] > 'f')) return FALSE;
		if ((w[1] < '0' || w[1] > '9') && (w[1] < 'a' || w[1] > 'f')) return FALSE;
		if (i != 5 && w[2] != ':') return FALSE;
	}
	return TRUE;
}

BOOL validate_number_range(WCHAR* num, INT min, INT max)
{
	INT v = _wtoi(num);
	if (min != -1 && v < min) return FALSE;
	if (max != -1 && v > max) return FALSE;
	return TRUE;
}