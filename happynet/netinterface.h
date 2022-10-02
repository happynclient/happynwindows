#ifndef _H_NET_INTERFACE
#define _H_NET_INTERfACE


BOOL SetNetinterfaceNameById(WCHAR* pszDeviceUUID);
DWORD GetAdapterFriendlyName(CHAR* pszAdapterName, WCHAR* pszFriendlyName, ULONG nMaxNameLength);
VOID GetMacAddress(WCHAR* mac_address, WCHAR* guid);
VOID GetIpMacAddresses(WCHAR* ip_address, WCHAR* mac_address);
BOOL ValidateIpv4Address(WCHAR* ip_address);
BOOL ValidateMacAddress(WCHAR* mac_address);
BOOL ValidateNumberRange(WCHAR* num, INT min, INT max);

#endif