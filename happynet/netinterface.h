#ifndef _H_NET_INTERFACE
#define _H_NET_INTERfACE


BOOL SetNetinterfaceNameById(WCHAR* pszDeviceUUID);
DWORD GetAdapterFriendlyName(CHAR* pszAdapterName, WCHAR* pszFriendlyName, ULONG nMaxNameLength);
VOID GetMacAddress(WCHAR* pszMacAddress, WCHAR* pszGuid);
VOID GetIpMacAddresses(WCHAR* pszIpAddress, WCHAR* pszMacAddress);
BOOL ValidateIpv4Address(WCHAR* pszIpAddress);
BOOL ValidateMacAddress(WCHAR* pszMacAddress);
BOOL ValidateNumberRange(WCHAR* pNum, INT nMin, INT nMax);

#endif