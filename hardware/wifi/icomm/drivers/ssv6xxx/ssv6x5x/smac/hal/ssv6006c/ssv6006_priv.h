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

#ifndef _SSV6006_PRIV_H_
#define _SSV6006_PRIV_H_ 
#include <smac/ssv_rc_minstrel.h>
#define COMMON_FOR_SMAC 
#define REG32(_addr) REG32_R(_addr)
#define REG32_W(_addr,_value) do { SMAC_REG_WRITE(sh, _addr, _value); } while (0)
static void inline print_null(const char *fmt, ...)
{
}
#define MSLEEP(_val) msleep(_val)
#define MDELAY(_val) mdelay(_val)
#define UDELAY(_val) udelay(_val)
#define PRINT printk
#define PRINT_ERR printk
#define PRINT_INFO printk
void ssv_attach_ssv6006_common(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6051_phy(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6051_cabrioA_BBRF(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6006_phy(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6006c_phy(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6006c_mac(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6006_mac(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6006_cabrioA_BBRF(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6006_geminiA_BBRF(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6006_turismoA_BBRF(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6006_turismoB_BBRF(struct ssv_hal_ops *hal_ops);
void ssv_attach_ssv6006_turismoC_BBRF(struct ssv_hal_ops *hal_ops);
bool ssv6006_rc_get_previous_ampdu_rpt(struct ssv_minstrel_ht_sta *mhs, int pkt_no, int *rpt_idx);
void ssv6006_rc_add_ampdu_rpt_to_list(struct ssv_softc *sc, struct ssv_minstrel_ht_sta *mhs, void *rate_rpt,
   int pkt_no, int ampdu_len, int ampdu_ack_len);
int ssv6006_get_pa_band(int ch);
void ssv6006c_reset_mib_mac(struct ssv_hw *sh);
#endif
