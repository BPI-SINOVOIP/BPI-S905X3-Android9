#ifndef _DRV_MAC_H_
#define _DRV_MAC_H_

#include <regs.h>
#include <hw_regs_api.h>

#define drv_mac_get_pbuf_offset()           GET_PB_OFFSET

s32 drv_mac_init(void);

#endif /* _DRV_MAC_H_ */
s32 drv_mac_set_rx_flow_data(const u32 *cflow);
s32 drv_mac_get_rx_flow_data(u32 *cflow);
s32 drv_mac_set_rx_flow_mgmt(const u32 *cflow);
s32 drv_mac_get_rx_flow_mgmt(u32 *cflow);
s32 drv_mac_set_rx_flow_ctrl(const u32 *cflow);
s32 drv_mac_get_rx_flow_ctrl(u32 *cflow);

s32 drv_mac_get_wsid_peer_mac (s32 index, ETHER_ADDR *mac);
s32 drv_mac_get_sta_mac(u8 *mac);
s32 drv_mac_get_bssid(u8 *mac);
void block_all_traffic (void);
void restore_all_traffic (void);
void drv_set_GPIO(u32 value);

void drv_mac_check_debug_int (const char *file, int line);

#define CHECK_DEBUG_INT() \
    do { \
        drv_mac_check_debug_int(__FILE__, __LINE__); \
    } while (0)



