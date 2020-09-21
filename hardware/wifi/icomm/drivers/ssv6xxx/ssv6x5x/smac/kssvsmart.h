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

#ifndef _SSVSMART_CONFIG_H
#define _SSVSMART_CONFIG_H 
#ifdef SSV_MAC80211
#include "ssv_mac80211.h"
#else
#include <net/mac80211.h>
#endif
#include <ssv6200.h>
#include <smac/dev.h>
#ifdef CONFIG_SSV_SMARTLINK
enum {
 SI_ST_NG,
 SI_ST_OK,
 SI_ST_PROCESSING,
 SI_ST_MAX
};
void ssv6xxx_process_si_event(struct ssv_softc *sc, struct sk_buff *skb);
int ssv6xxx_send_si_cmd(u32 smart_icomm_cmd);
inline void set_si_status(u32 st);
int get_si_status(char *input);
int get_si_ssid(char *input);
int get_si_pass(char *input);
#endif
#endif
