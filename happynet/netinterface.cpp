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


static BOOL RenameNetinterfaceById(INetSharingManager *pNSM, WCHAR* pszDeviceUUID)
{   // add a port mapping to every firewalled or shared connection 
    BOOL bFound = FALSE;
    INetSharingEveryConnectionCollection * pINetNsecc = NULL;
    HRESULT hr = pNSM->get_EnumEveryConnection(&pINetNsecc);
    if (!pINetNsecc) {
        LogEvent(TEXT("failed to get EveryConnectionCollection!\r\n"));
    }else {
        // enumerate connections
        IEnumVARIANT * pEnumValue = NULL;
        IUnknown * pUnknown = NULL;
        hr = pINetNsecc->get__NewEnum(&pUnknown);
        if (pUnknown) {
            hr = pUnknown->QueryInterface(__uuidof(IEnumVARIANT),
                (void**)&pEnumValue);
            pUnknown->Release();
        }
        if (pEnumValue) {
            VARIANT v;
            VariantInit(&v);

            while ((S_OK == pEnumValue->Next(1, &v, NULL)) && (bFound == FALSE)) {
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

                        if (wcscmp(currentGUID, pszDeviceUUID) == 0)
                        {
                            hr = pNC->Rename(NPCAP_LOOPBACK_INTERFACE_NAME);
                            bFound = TRUE;
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
            pEnumValue->Release();
        }
        pINetNsecc->Release();
    }

    return bFound;
}

BOOL SetNetinterfaceNameById(WCHAR* pszDeviceUUID)
{
    BOOL bSuccess = FALSE;
    /*	CoInitialize (NULL);*/

        // init security to enum RAS connections
    CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_PKT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);

    INetSharingManager * pINetNsm = NULL;
    HRESULT hr = ::CoCreateInstance(__uuidof(NetSharingManager),
        NULL,
        CLSCTX_ALL,
        __uuidof(INetSharingManager),
        (void**)&pINetNsm);
    if (!pINetNsm) {
        LogEvent(TEXT("failed to create NetSharingManager object\r\n"));
        return bSuccess;
    }
    else {

        // add a port mapping to every shared or firewalled connection.
        bSuccess = RenameNetinterfaceById(pINetNsm, pszDeviceUUID);

        pINetNsm->Release();
    }

    /*	CoUninitialize ();*/
    return bSuccess;
}


VOID GetMacAddress(WCHAR* pszMacAddress, WCHAR* pszGuid)
{
	PIP_ADAPTER_ADDRESSES sAddresses = NULL;
	PIP_ADAPTER_ADDRESSES sCurrAddr;
	ULONG nBufLen = 0;
	ULONG nError = GetAdaptersAddresses(AF_INET, NULL, NULL, NULL, &nBufLen);
	if (nError == ERROR_BUFFER_OVERFLOW)
	{
		sAddresses = (IP_ADAPTER_ADDRESSES*)HeapAlloc(GetProcessHeap(), 0, nBufLen);
		nError = GetAdaptersAddresses(AF_INET, NULL, NULL, sAddresses, &nBufLen);
		if (nError == ERROR_SUCCESS)
		{
			sCurrAddr = sAddresses;
			CHAR* pszGuidAnsi = NULL;
			pszGuidAnsi = new CHAR[wcslen(pszGuid) + 1];
			UINT nConv = 0;
			wcstombs_s(&nConv, pszGuidAnsi, wcslen(pszGuid) + 1, pszGuid, _TRUNCATE);
			while (sCurrAddr != NULL)
			{
				if (_stricmp(sCurrAddr->AdapterName, pszGuidAnsi))
				{
					sCurrAddr = sCurrAddr->Next;
					continue;
				}
				BYTE* a = sCurrAddr->PhysicalAddress;
				for (int i = 0; i < 5; i++)
				{
					wsprintf(pszMacAddress, TEXT("%02X:%02X:%02X:%02X:%02X:%02X"),
                                a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
				}
				break;
			}
			delete []pszGuidAnsi;
		}
		HeapFree(GetProcessHeap(), 0, sAddresses);
	}
}

DWORD GetAdapterFriendlyName(CHAR* pszAdapterName, WCHAR* pszFriendlyName, ULONG nMaxNameLength)
{
    ULONG nOutBufLen = WORKING_BUFFER_SIZE;
    ULONG nIerations = 0;
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    DWORD dwRet = 0;
    do {
        pAddresses = (IP_ADAPTER_ADDRESSES *)MALLOC(nOutBufLen);
        if (pAddresses == NULL) {
            LogEvent(TEXT("Error:%s"), TEXT("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n"));
            return E_FAIL;
        }

        dwRet = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &nOutBufLen);

        if (dwRet == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
            return E_FAIL;
        }
        nIerations++;
    } while ((dwRet == ERROR_BUFFER_OVERFLOW) && (nIerations < MAX_TRIES));

    PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
    while (pCurrAddresses) {
        if (!lstrcmpA(pCurrAddresses->AdapterName, pszAdapterName)) {
            UINT friendly_name_length = lstrlenW(pCurrAddresses->FriendlyName);
            if (friendly_name_length < nMaxNameLength && friendly_name_length > 0) {
                lstrcpynW(pszFriendlyName, pCurrAddresses->FriendlyName, friendly_name_length + 1);
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


VOID GetIpMacAddresses(WCHAR* pszIpAddress, WCHAR* pszMacAddress)
{
	HKEY hkeyAdapters, hkeyAdapter, hkeyIp, hkeyIpg;
	DWORD i = 0;
	WCHAR arrcNetKey[] = TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
	WCHAR arrcTcpipKey[] = TEXT("SYSTEM\\CurrentControlSet\\services\\Tcpip\\Parameters\\Interfaces");
	WCHAR arrcSubkeyName[128];
	WCHAR arrcComponentId[128];
	WCHAR arrcGuid[39];
	DWORD dwSubkeySize = 128;
	DWORD dwComponentSize = 128 * sizeof(WCHAR);
	DWORD dwGuidSize = 39 * sizeof(WCHAR);
	BOOL bIsAdapterFound = FALSE;

	// Clear the addresses
	wsprintf(pszIpAddress, TEXT("<unknown>"));
	wsprintf(pszMacAddress, TEXT("<unknown>"));

    // check if user select adapter manually by ad_option dialog
    WCHAR arrcTmpBuf[MAX_ADAPTER_NAME_LEN] = {0};
    DWORD nBufLen = MAX_ADAPTER_NAME_LEN;
    WCHAR *pszStrtokBuf = NULL, *pAdapterId = NULL;
    const WCHAR s[4] = TEXT("_");
    HKEY hkeyAdapterId;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"),
                        NULL, KEY_READ, &hkeyAdapterId) == ERROR_SUCCESS)
    {
        // Community
        GetRegString(hkeyAdapterId, TEXT("adapter"), arrcTmpBuf, nBufLen);
        pAdapterId = wcstok_s(arrcTmpBuf, s, &pszStrtokBuf);
        pAdapterId = wcstok_s(NULL, s, &pszStrtokBuf);
        RegCloseKey(hkeyAdapterId);
    }

	// First we need to find the TAP-Win32 adapter (identified by 'tap0901') and get its GUID
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, arrcNetKey, NULL, KEY_READ, &hkeyAdapters);
    while (RegEnumKeyEx(hkeyAdapters, i, arrcSubkeyName, &dwSubkeySize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        RegOpenKeyEx(hkeyAdapters, arrcSubkeyName, NULL, KEY_READ, &hkeyAdapter);
        RegQueryValueEx(hkeyAdapter, TEXT("ComponentId"), NULL, NULL, (LPBYTE)arrcComponentId, &dwComponentSize);
        RegQueryValueEx(hkeyAdapter, TEXT("NetCfgInstanceId"), NULL, NULL, (LPBYTE)arrcGuid, &dwGuidSize);
        RegCloseKey(hkeyAdapter);
        if (
            // auto select adapter
            ((pAdapterId == NULL) && (!wcscmp(arrcComponentId, TEXT("tap0901")) || !wcscmp(arrcComponentId, TEXT("TAP0901")))) ||
            // user select adapter manually by ad_option dialog
            ((pAdapterId != NULL) && (!wcscmp(arrcGuid, pAdapterId)))
            )
		{
			bIsAdapterFound = true;
			break;
		}
		i++;
		dwSubkeySize = 128;
	}
	RegCloseKey(hkeyAdapters);

	// If we have the guid, fetch the IP address from the registry
	if (bIsAdapterFound)
	{
		DWORD dwDhcpEnabled = 0;
		DWORD nBufSize = sizeof(DWORD);
		DWORD nIpSize = 16 * sizeof(WCHAR);
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, arrcTcpipKey, NULL, KEY_READ, &hkeyIp);
		RegOpenKeyEx(hkeyIp, arrcGuid, NULL, KEY_READ, &hkeyIpg);
		RegQueryValueEx(hkeyIpg, TEXT("EnableDHCP"), NULL, NULL, (LPBYTE)&dwDhcpEnabled, &nBufSize);
		if (dwDhcpEnabled)
		{
			RegQueryValueEx(hkeyIpg, TEXT("DhcpIPAddress"), NULL, NULL, (LPBYTE)pszIpAddress, &nIpSize);
		}
		else
		{
			RegQueryValueEx(hkeyIpg, TEXT("IPAddress"), NULL, NULL, (LPBYTE)pszIpAddress, &nIpSize);
		}
		RegCloseKey(hkeyIpg);
		RegCloseKey(hkeyIp);

		GetMacAddress(pszMacAddress, arrcGuid);
	}
}

BOOL ValidateIpv4Address(WCHAR* pszIpAddress)
{
	WCHAR c;
	WCHAR arrcOctetValue[4];
	arrcOctetValue[0] = 0;
	INT nOctets = 0;
	INT nOctetLen = 0;
	for (INT i = 0; i < (INT)wcslen(pszIpAddress); i++)
	{
		c = pszIpAddress[i];
		if ((c < '0' || c > '9') && c != '.' && c != '/') return FALSE;
		if (c == '.' || c == '/')
		{
			if (nOctetLen < 1 || nOctetLen > 3) return FALSE;
			nOctets++;
			arrcOctetValue[nOctetLen] = 0;
			INT nOctetValue = _wtoi(arrcOctetValue);
			if (nOctetValue < 0 || nOctetValue > 255) return FALSE;
			if (nOctets > 3 && c != '/') return FALSE;
			nOctetLen = 0;
			continue;
		}
		arrcOctetValue[nOctetLen] = c;
		nOctetLen++;
		if (nOctetLen > 3) return FALSE;
	}
	nOctets++;
	if (nOctetLen < 1 || nOctetLen > 3 || (nOctets != 4  && nOctets != 5)) return FALSE;
	return TRUE;
}

BOOL ValidateMacAddress(WCHAR* pszMacAddress)
{
	if (wcslen(pszMacAddress) != 17) return FALSE;
	WCHAR arrcMacAddressLower[18];
	wcscpy_s(arrcMacAddressLower, 18, pszMacAddress);
	_wcslwr_s(arrcMacAddressLower, 18);
	WCHAR* w;
	for (INT i = 0; i < 6; i++)
	{
		w = arrcMacAddressLower + (i * 3);
		if ((w[0] < '0' || w[0] > '9') && (w[0] < 'a' || w[0] > 'f')) return FALSE;
		if ((w[1] < '0' || w[1] > '9') && (w[1] < 'a' || w[1] > 'f')) return FALSE;
		if (i != 5 && w[2] != ':') return FALSE;
	}
	return TRUE;
}

BOOL ValidateNumberRange(WCHAR* pNum, INT nMin, INT nMax)
{
	INT v = _wtoi(pNum);
	if (nMin != -1 && v < nMin) return FALSE;
	if (nMax != -1 && v > nMax) return FALSE;
	return TRUE;
}