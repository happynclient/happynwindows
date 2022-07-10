#ifndef _ADAPTER_INFO_H____
#define _ADAPTER_INFO_H____
#include <windows.h>
#include <tchar.h>
#include <iphlpapi.h>
#include <ipifcons.h>
#include <vector>

using namespace std;

class CNetworkAdapter;

#ifndef tstring
#ifdef _UNICODE
#define tstring			std::wstring
#else
#define tstring			std::string		
#endif
#endif

// import the internet protocol helper libarary
#pragma comment( lib, "iphlpapi.lib" )

#define DEFAULT_GATEWAY_ADDR	0

struct _IPINFO {
    std::string sIp;
    std::string sSubnet;
};

class CIpInfoArray : public vector< _IPINFO > {};
class StringArray : public vector< std::string > {};

/////////////////////////////////////////////
// Function Prototypes
DWORD EnumNetworkAdapters(CNetworkAdapter* lpBuffer, ULONG ulSzBuf, LPDWORD lpdwOutSzBuf);

//////////////////////////////////////////////////////////////////////////////////////////
//	Desc:
//		Class wrapper for a single network adapter.  A listing of these adapters
//		can be built using the EnumNetworkAdapters(...) function prototyped
//		above.
//////////////////////////////////////////////////////////////////////////////////////////
class CNetworkAdapter {
public:
    CNetworkAdapter();
    ~CNetworkAdapter();
    BOOL SetupAdapterInfo(IP_ADAPTER_INFO* pAdaptInfo);

    // information about the adapters name for the users
    // and its name to the system
    tstring GetAdapterName() const;
    tstring GetAdapterDescription() const;

    // dhcp lease access functions
    time_t	GetLeaseObtained() const;
    time_t	GetLeaseExpired() const;

    // access to lists of various server's ip address
    SIZE_T	GetNumIpAddrs() const;
    SIZE_T	GetNumDnsAddrs() const;
    std::string	GetIpAddr(int nIp = 0) const;
    std::string GetSubnetForIpAddr(int nIp = 0) const;
    std::string	GetDnsAddr(int nDns = 0) const;
    std::string GetCurrentIpAddress() const;

    // dhcp function
    BOOL	IsDhcpUsed() const;
    tstring	GetDchpAddr() const;

    // wins function
    BOOL	IsWinsUsed() const;
    tstring GetPrimaryWinsServer() const;
    tstring GetSecondaryWinsServer() const;

    std::string	GetGatewayAddr(int nGateway = DEFAULT_GATEWAY_ADDR) const;
    SIZE_T	GetNumGatewayAddrs() const;

    static	tstring GetAdapterTypeString(UINT nType);
    UINT	GetAdapterType() const;

    DWORD	GetAdapterIndex() const;
    BOOL	ReleaseAddress();
    BOOL	RenewAddress();

protected:
    std::string	GetStringFromArray(const StringArray* pPtr, int nIndex) const;
    BOOL	DoRenewRelease(DWORD(__stdcall *func)(PIP_ADAPTER_INDEX_MAP AdapterInfo));

private:
    tstring			m_sName;		// adapter name with the computer.  For human readable name use m_sDesc.
    tstring			m_sDesc;
    tstring			m_sPriWins;
    tstring			m_sSecWins;
    tstring			m_sDefGateway;
    tstring			m_sDhcpAddr;
    _IPINFO			m_sCurIpAddr;	// this is also in the ip address list but this is the address currently active.
    DWORD			m_dwIndex;		// machine index of the adapter.
    UINT			m_nAdapterType;
    BOOL			m_bDhcpUsed;
    BOOL			m_bWinsUsed;
    StringArray		m_DnsAddresses;
    CIpInfoArray	m_IpAddresses;
    StringArray		m_GatewayList;
    time_t			m_tLeaseObtained;
    time_t			m_tLeaseExpires;

    struct UNNAMED {
        BYTE	ucAddress[MAX_ADAPTER_ADDRESS_LENGTH];
        UINT	nLen;
    } m_ucAddress;
};

#endif //_ADAPTER_INFO_H____