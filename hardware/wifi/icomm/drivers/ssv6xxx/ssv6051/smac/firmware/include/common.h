#ifndef _COMMON_H_
#define _COMMON_H_


#include <types.h>

#ifndef EAPOL_ETHER_TYPPE
#define EAPOL_ETHER_TYPPE	0x888E
#endif

typedef enum{
	SSV6XXX_SEC_NONE,
	SSV6XXX_SEC_WEP_40,			//5		ASCII
	SSV6XXX_SEC_WEP_104,		//13	ASCII
	SSV6XXX_SEC_WPA_PSK,		//8~63	ASCII
	SSV6XXX_SEC_WPA2_PSK,		//8~63	ASCII
	SSV6XXX_SEC_WPS,
}ssv6xxx_sec_type;

#define MAX_SSID_LEN 32
#define MAX_PASSWD_LEN 63

#define MAX_WEP_PASSWD_LEN (13+1)

//------------------------------------------------

/**
 *  struct cfg_sta_info - STA structure description
 *
 */
struct cfg_sta_info {
    ETHER_ADDR      addr;
#if 1
		u32 			bit_rates; /* The first eight rates are the basic rate set */
	
		u8				listen_interval;
	
		u8				key_id;
		u8				key[16];
		u8				mic_key[8];
	
		/* TKIP IV */
		u16 			iv16;
		u32 			iv32;
#endif

} ;//__attribute__ ((packed));




/**
 *  struct cfg_bss_info - BSS/IBSS structure description
 *
 */
struct cfg_bss_info {
    ETHER_ADDR          bssid;

};// __attribute__ ((packed));



#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))


#endif /* _COMMON_H_ */

