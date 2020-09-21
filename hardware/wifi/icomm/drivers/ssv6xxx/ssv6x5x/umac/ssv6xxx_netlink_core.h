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

#ifndef _SSV6XXX_NETLINK_CORE_H_
#define _SSV6XXX_NETLINK_CORE_H_ 
struct ssv_wireless_register {
 u32 address;
 u32 value;
};
enum {
 SSV_WIRELESS_ATTR_UNSPEC,
 SSV_WIRELESS_ATTR_CONFIG,
 SSV_WIRELESS_ATTR_REGISTER,
 SSV_WIRELESS_ATTR_TXFRAME,
 SSV_WIRELESS_ATTR_RXFRAME,
 SSV_WIRELESS_ATTR_START_HCI,
 SSV_WIRELESS_ATTR_TEST,
 __SSV_WIRELESS_ATTR_MAX,
};
#define SSV_WIRELESS_ATTR_MAX (__SSV_WIRELESS_ATTR_MAX - 1)
enum {
 SSV_WIRELESS_CMD_UNSPEC,
 SSV_WIRELESS_CMD_CONFIG,
 SSV_WIRELESS_CMD_READ_REG,
 SSV_WIRELESS_CMD_WRITE_REG,
 SSV_WIRELESS_CMD_TXFRAME,
 SSV_WIRELESS_CMD_RXFRAME,
 SSV_WIRELESS_CMD_START_HCI,
 SSV_WIRELESS_CMD_TEST,
 __SSV_WIRELESS_CMD_MAX,
};
#define SSV_WIRELESS_CMD_MAX (__SSV_WIRELESS_CMD_MAX - 1)
#endif
