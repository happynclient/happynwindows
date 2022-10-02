#ifndef _H_NET_INTERFACE
#define _H_NET_INTERfACE


BOOL SetNetinterfaceNameById(WCHAR device_uuid[]);
DWORD GetAdapterFriendlyName(CHAR* adapter_name, WCHAR* friendly_name, ULONG max_name_length);
VOID GetMacAddress(WCHAR* mac_address, WCHAR* guid);
VOID GetIpMacAddresses(WCHAR* ip_address, WCHAR* mac_address);
BOOL ValidateIpv4Address(WCHAR* ip_address);
BOOL ValidateMacAddress(WCHAR* mac_address);
BOOL ValidateNumberRange(WCHAR* num, INT min, INT max);

#endif