#ifndef _H_NET
#define _H_NET

void get_mac_address(WCHAR* mac_address, WCHAR* guid);
void get_addresses(WCHAR* ip_address, WCHAR* mac_address);
bool validate_ipv4_address(WCHAR* ip_address);
bool validate_mac_address(WCHAR* mac_address);
bool validate_number_range(WCHAR* num, int min, int max);

#endif