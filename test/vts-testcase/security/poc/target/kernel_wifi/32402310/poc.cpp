/*
 * Copyright 2016 The Android Open Source Project
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
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h> /* See NOTES */
#include <sys/un.h>
#include <unistd.h>

#include <netlink/msg.h>
#include <linux/wireless.h>

#include <linux/nl80211.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>

#define MAX_MSG_SIZE 4096
#define TEST_CNT 64

#define AID_INET 3003    /* can create AF_INET and AF_INET6 sockets */
#define AID_NET_RAW 3004 /* can create raw INET sockets */
#define AID_NET_ADMIN 3005

#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define NLA_DATA(na) ((void *)((char *)(na) + NLA_HDRLEN))

#define NL80211_ATTR_MAC 6
#define ETH_ALEN 6
#define NL80211_ATTR_IFINDEX 3
#define QCA_NL80211_VENDOR_ID 0x001374

#define QCA_NL80211_VENDOR_SUBCMD_ROAM 64
#define QCA_WLAN_VENDOR_ATTR_ROAMING_SUBCMD 1
#define QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_BSSID_PREFS 4
#define QCA_WLAN_VENDOR_ATTR_ROAMING_REQ_ID 2
#define QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PREFS 14
#define QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_NUM_BSSID 15
#define QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_BSSID 16
#define QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_RSSI_MODIFIER 17

typedef char tSirMacAddr[ETH_ALEN];
struct nl_sock *nl_sk;

int send_testmode(const char *ifname, u_int16_t nlmsg_type, u_int32_t nlmsg_pid,
                  u_int8_t genl_cmd, u_int8_t genl_version) {
  struct nl_msg *msg;
  int ret = POC_TEST_PASS;
  int i;
  unsigned char dst[ETH_ALEN];
  struct nlattr *rret;
  struct nlattr *rret2;
  struct nlattr *rret3;
  unsigned char oper_classes[253];
  tSirMacAddr mac_in;
  unsigned char hb_params[512];

  struct nl80211_sta_flag_update flags;

  msg = nlmsg_alloc();
  int if_index = if_nametoindex(ifname);

  genlmsg_put(msg, nlmsg_pid, 0, nlmsg_type, 0, 0, genl_cmd, genl_version);
  nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);
  nla_put_u32(msg, NL80211_ATTR_VENDOR_ID, QCA_NL80211_VENDOR_ID);
  nla_put_u32(msg, NL80211_ATTR_VENDOR_SUBCMD, QCA_NL80211_VENDOR_SUBCMD_ROAM);

  rret = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);

  if (!rret) {
    perror("nla_nest_start");
    ret = POC_TEST_FAIL;
    goto exit;
  }
  nla_put_u32(msg, QCA_WLAN_VENDOR_ATTR_ROAMING_SUBCMD,
              QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_BSSID_PREFS);

  nla_put_u32(msg, QCA_WLAN_VENDOR_ATTR_ROAMING_REQ_ID, 123);
  nla_put_u32(msg, QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_NUM_BSSID,
              0xffffffff);

  rret2 =
      nla_nest_start(msg, QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PREFS);
  if (!rret2) {
    perror("nla_nest_start2");
    ret = POC_TEST_FAIL;
    goto exit;
  }

  for (i = 0; i < TEST_CNT; i++) {
    rret3 =
        nla_nest_start(msg, QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PREFS);
    if (!rret3) {
      perror("nla_nest_start3");
      ret = POC_TEST_FAIL;
      goto exit;
    }

    nla_put(msg, QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_BSSID,
            sizeof(mac_in), &mac_in);
    nla_put_u32(msg,
                QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_RSSI_MODIFIER,
                0xdeadbeed);
    nla_nest_end(msg, rret3);
  }

  nla_nest_end(msg, rret2);

  nla_nest_end(msg, rret);

  nl_send_auto_complete(nl_sk, msg);
exit:
  nlmsg_free(msg);
  return ret;
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
  gid_t gid_groups[] = {AID_INET, AID_NET_ADMIN};

  if (getuid() != 0) {
    printf("need root privilege\n");
    return POC_TEST_FAIL;
  }

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
