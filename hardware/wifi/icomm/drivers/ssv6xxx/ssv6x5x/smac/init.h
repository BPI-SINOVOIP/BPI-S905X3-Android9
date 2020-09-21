/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _INIT_H_
#define _INIT_H_ 
int tu_ssv6xxx_init_mac(struct ssv_hw *sh);
void ssv6xxx_deinit_mac(struct ssv_softc *sc);
void ssv6xxx_restart_hw(struct work_struct *work);
void ssv6xxx_umac_hci_start(char *driver_name, char *device_name);
void ssv6xxx_umac_test(char *driver_name, char *device_name, struct sk_buff *skb, int size);
void ssv6xxx_umac_reg_read(char *driver_name, char *device_name, u32 addr, u32 *regval);
void ssv6xxx_umac_reg_write(char *driver_name, char *device_name, u32 addr, u32 value);
void ssv6xxx_umac_tx_frame(char *driver_name, char *device_name, struct sk_buff *skb);
int ssv6xxx_umac_attach(char *driver_name, char *device_name, struct ssv6xxx_umac_ops *umac_ops);
int ssv6xxx_umac_deattach(char *driver_name, char *device_name);
#if defined(CONFIG_SMARTLINK) || defined(CONFIG_SSV_SUPPORT_ANDROID)
struct ssv_softc *ssv6xxx_driver_attach(char *driver_name);
#endif
#ifdef SSV_SUPPORT_HAL
    #define SSV_NEED_SW_CIPHER(_sh) HAL_NEED_SW_CIPHER(_sh)
    #define SSV_DO_IQ_CALIB(_sh,_pcfg) HAL_DO_IQ_CAL(_sh,_pcfg)
    #define SSV_SET_RF_ENABLE(_sc) HAL_SET_RF_ENABLE(_sc, true)
    #define SSV_SET_RF_DISABLE(_sc) HAL_SET_RF_ENABLE(_sc, false)
    #define SSV_SET_ON3_ENABLE(_sh, _val) HAL_SET_ON3_ENABLE(_sh, _val)
    #define SSV_GET_FW_NAME(_sh, _name) HAL_GET_FW_NAME(_sh, _name)
    #define SSV_FLASH_READ_ALL_MAP(_sh) HAL_FLASH_READ_ALL_MAP(_sh)
#else
void ssv6xxx_flash_read_all_map(struct ssv_hw *sh);
    #define SSV_NEED_SW_CIPHER(_sh) true
    #ifdef CONFIG_SSV_CABRIO_E
        int ssv6xxx_do_iq_calib(struct ssv_hw *sh, struct ssv6xxx_iqk_cfg *p_cfg);
        #define SSV_DO_IQ_CALIB(_sh,_pcfg) ssv6xxx_do_iq_calib(_sh,_pcfg)
    #else
        #define SSV_DO_IQ_CALIB(_sh,_pcfg)
    #endif
    #define SSV_SET_RF_ENABLE(_sc) ssv6xxx_rf_enable(_sc)
    #define SSV_SET_RF_DISABLE(_sc) ssv6xxx_rf_disable(_sc)
    #define SSV_SET_ON3_ENABLE(_sh, _val) ssv6xxx_set_on3_enable(_sh, _val)
    #define SSV_GET_FW_NAME(_sc, _name) strcpy(_name, "ssv6051-sw.bin")
    #define SSV_FLASH_READ_ALL_MAP(_sh) ssv6xxx_flash_read_all_map(_sh)
#endif
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
int tu_ssv6xxx_init(void);
void tu_ssv6xxx_exit(void);
#endif
#endif
