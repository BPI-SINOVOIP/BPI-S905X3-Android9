/******************************************************************************
 *
 *  Copyright 2015, The linux Foundation. All rights reserved.
 *
 *  Not a Contribution.
 *
 *  Copyright 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/************************************************************************************
 *
 *  Filename:      mcap_tool.cc
 *
 *  Description:   Fluoride MCAP Test Tool application
 *
 ***********************************************************************************/
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef OS_GENERIC
#include <sys/capability.h>
#endif
#include <sys/prctl.h>
#include <time.h>
#include <unistd.h>

#include <hardware/bluetooth.h>
#ifndef OS_GENERIC
#include <private/android_filesystem_config.h>
#endif
#include <base/logging.h>

#include "bt_types.h"
#include "l2c_api.h"
#include "mca_api.h"
#include "mca_defs.h"
#include "osi/include/compat.h"
#include "hal_util.h"
#include "mcap_test_app.h"
#include "mcap_test_mcl.h"
#include "mcap_test_mdep.h"
#include "mcap_test_mdl.h"

using SYSTEM_BT_TOOLS_MCAP_TOOL::McapTestApp;
using SYSTEM_BT_TOOLS_MCAP_TOOL::McapMcl;
using SYSTEM_BT_TOOLS_MCAP_TOOL::McapMdep;
using SYSTEM_BT_TOOLS_MCAP_TOOL::McapMdl;

/******************************************************************************
 *  Constants & Macros
 *****************************************************************************/
#define PID_FILE "/data/.bdt_pid"

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define CASE_RETURN_STR(const) \
  case const:                  \
    return #const;

#ifndef OS_GENERIC
/* Permission Groups */
static gid_t groups[] = {AID_NET_BT,    AID_INET, AID_NET_BT_ADMIN,
                         AID_SYSTEM,    AID_MISC, AID_SDCARD_RW,
                         AID_NET_ADMIN, AID_VPN};
#endif
/******************************************************************************
 *  Static variables
 *****************************************************************************/
/* Console loop states */
static bool global_main_done = false;
static bt_status_t global_status;
static bool global_strict_mode = false;

/* Device and Profile Interfaces */
const bt_interface_t* sBtInterface = nullptr;
static btmcap_test_interface_t* sMcapTestInterface = nullptr;
static McapTestApp* sMcapTestApp = nullptr;

/* Bluetooth stack states */
static bool global_bt_enabled = false;
static int global_adapter_state = BT_STATE_OFF;
static int global_pair_state = BT_BOND_STATE_NONE;
/************************************************************************************
**  Static functions
************************************************************************************/
static void process_cmd(char* p, bool is_job);

/*******************************************************************************
 ** Misc helper functions
 *******************************************************************************/
static const char* dump_bt_status(const bt_status_t status) {
  switch (status) {
    CASE_RETURN_STR(BT_STATUS_SUCCESS)
    CASE_RETURN_STR(BT_STATUS_FAIL)
    CASE_RETURN_STR(BT_STATUS_NOT_READY)
    CASE_RETURN_STR(BT_STATUS_NOMEM)
    CASE_RETURN_STR(BT_STATUS_BUSY)
    CASE_RETURN_STR(BT_STATUS_UNSUPPORTED)
    default:
      return "unknown status code";
  }
}

/************************************************************************************
**  MCAP Callbacks
************************************************************************************/
static void mcap_ctrl_callback(tMCA_HANDLE handle, tMCA_CL mcl, uint8_t event,
                               tMCA_CTRL* p_data) {
  sMcapTestApp->ControlCallback(handle, mcl, event, p_data);
}

static void mcap_data_cb(tMCA_DL mdl, BT_HDR* p_pkt) {
  printf("%s: mdl=%d, event=%d, len=%d, offset=%d, layer_specific=%d\n",
         __func__, mdl, p_pkt->event, p_pkt->len, p_pkt->offset,
         p_pkt->layer_specific);
  printf("%s: HEXDUMP OF DATA LENGTH %u:\n", __func__, p_pkt->len);
  printf("=========================Begin=========================\n");
  bool newline = false;
  for (int i = 0; i < p_pkt->len; ++i) {
    printf("%02x", p_pkt->data[i]);
    if (i > 0 && (i % 25) == 0) {
      printf("\n");
      newline = true;
    } else {
      printf(" ");
      newline = false;
    }
  }
  if (!newline) printf("\n");
  printf("=========================End===========================\n");
}

/************************************************************************************
**  Shutdown helper functions
************************************************************************************/

static void console_shutdown(void) {
  LOG(INFO) << __func__ << ": Shutdown Fluoride MCAP test app";
  global_main_done = true;
}

/*****************************************************************************
** Android's init.rc does not yet support applying linux capabilities
*****************************************************************************/

#ifndef OS_GENERIC
static void config_permissions(void) {
  struct __user_cap_header_struct header;
  struct __user_cap_data_struct cap[2];

  printf("set_aid_and_cap : pid %d, uid %d gid %d", getpid(), getuid(),
         getgid());

  header.pid = 0;

  prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);

  setuid(AID_BLUETOOTH);
  setgid(AID_BLUETOOTH);

  header.version = _LINUX_CAPABILITY_VERSION_3;

  cap[CAP_TO_INDEX(CAP_NET_RAW)].permitted |= CAP_TO_MASK(CAP_NET_RAW);
  cap[CAP_TO_INDEX(CAP_NET_ADMIN)].permitted |= CAP_TO_MASK(CAP_NET_ADMIN);
  cap[CAP_TO_INDEX(CAP_NET_BIND_SERVICE)].permitted |=
      CAP_TO_MASK(CAP_NET_BIND_SERVICE);
  cap[CAP_TO_INDEX(CAP_SYS_RAWIO)].permitted |= CAP_TO_MASK(CAP_SYS_RAWIO);
  cap[CAP_TO_INDEX(CAP_SYS_NICE)].permitted |= CAP_TO_MASK(CAP_SYS_NICE);
  cap[CAP_TO_INDEX(CAP_SETGID)].permitted |= CAP_TO_MASK(CAP_SETGID);
  cap[CAP_TO_INDEX(CAP_WAKE_ALARM)].permitted |= CAP_TO_MASK(CAP_WAKE_ALARM);

  cap[CAP_TO_INDEX(CAP_NET_RAW)].effective |= CAP_TO_MASK(CAP_NET_RAW);
  cap[CAP_TO_INDEX(CAP_NET_ADMIN)].effective |= CAP_TO_MASK(CAP_NET_ADMIN);
  cap[CAP_TO_INDEX(CAP_NET_BIND_SERVICE)].effective |=
      CAP_TO_MASK(CAP_NET_BIND_SERVICE);
  cap[CAP_TO_INDEX(CAP_SYS_RAWIO)].effective |= CAP_TO_MASK(CAP_SYS_RAWIO);
  cap[CAP_TO_INDEX(CAP_SYS_NICE)].effective |= CAP_TO_MASK(CAP_SYS_NICE);
  cap[CAP_TO_INDEX(CAP_SETGID)].effective |= CAP_TO_MASK(CAP_SETGID);
  cap[CAP_TO_INDEX(CAP_WAKE_ALARM)].effective |= CAP_TO_MASK(CAP_WAKE_ALARM);

  capset(&header, &cap[0]);
  setgroups(sizeof(groups) / sizeof(groups[0]), groups);
}
#endif

/*******************************************************************************
 ** Console helper functions
 *******************************************************************************/

void skip_blanks(char** p) {
  while (**p == ' ') (*p)++;
}

uint32_t get_int(char** p, int DefaultValue) {
  uint32_t Value = 0;
  unsigned char UseDefault;

  UseDefault = 1;
  skip_blanks(p);

  while (((**p) <= '9' && (**p) >= '0')) {
    Value = Value * 10 + (**p) - '0';
    UseDefault = 0;
    (*p)++;
  }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}

int get_signed_int(char** p, int DefaultValue) {
  int Value = 0;
  unsigned char UseDefault;
  unsigned char NegativeNum = 0;

  UseDefault = 1;
  skip_blanks(p);

  if ((**p) == '-') {
    NegativeNum = 1;
    (*p)++;
  }
  while (((**p) <= '9' && (**p) >= '0')) {
    Value = Value * 10 + (**p) - '0';
    UseDefault = 0;
    (*p)++;
  }

  if (UseDefault)
    return DefaultValue;
  else
    return ((NegativeNum == 0) ? Value : -Value);
}

void get_str(char** p, char* Buffer) {
  skip_blanks(p);

  while (**p != 0 && **p != ' ') {
    *Buffer = **p;
    (*p)++;
    Buffer++;
  }

  *Buffer = 0;
}

uint32_t get_hex_any(char** p, int DefaultValue, unsigned int NumOfNibble) {
  uint32_t Value = 0;
  unsigned char UseDefault;
  // unsigned char   NumOfNibble = 8;  //Since we are returning uint32, max
  // allowed is 4 bytes(8 nibbles).

  UseDefault = 1;
  skip_blanks(p);

  while ((NumOfNibble) &&
         (((**p) <= '9' && (**p) >= '0') || ((**p) <= 'f' && (**p) >= 'a') ||
          ((**p) <= 'F' && (**p) >= 'A'))) {
    if (**p >= 'a')
      Value = Value * 16 + (**p) - 'a' + 10;
    else if (**p >= 'A')
      Value = Value * 16 + (**p) - 'A' + 10;
    else
      Value = Value * 16 + (**p) - '0';
    UseDefault = 0;
    (*p)++;
    NumOfNibble--;
  }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}
uint32_t get_hex(char** p, int DefaultValue) {
  return get_hex_any(p, DefaultValue, 8);
}
uint8_t get_hex_byte(char** p, int DefaultValue) {
  return get_hex_any(p, DefaultValue, 2);
}

bool is_cmd(const char* cmd, const char* str) {
  return (strlen(str) == strlen(cmd)) && (strncmp(cmd, str, strlen(str)) == 0);
}

typedef void(console_cmd_handler_t)(char* p);

typedef struct {
  const char* name;
  console_cmd_handler_t* handler;
  const char* help;
  bool is_job;
} cmd_t;

extern const cmd_t console_cmd_list[];
static int console_cmd_maxlen = 0;

static void* cmdjob_handler(void* param) {
  char* job_cmd = (char*)param;
  LOG(INFO) << "cmdjob starting: " << job_cmd;
  process_cmd(job_cmd, true);
  LOG(INFO) << "cmdjob terminating";
  free(job_cmd);
  return nullptr;
}

static int create_cmdjob(char* cmd) {
  CHECK(cmd);
  char* job_cmd = (char*)calloc(1, strlen(cmd) + 1); /* freed in job handler */
  if (job_cmd) {
    strlcpy(job_cmd, cmd, strlen(job_cmd) + 1);
    pthread_t thread_id;
    int ret =
        pthread_create(&thread_id, nullptr, cmdjob_handler, (void*)job_cmd);
    LOG_IF(ERROR, ret != 0) << "Error during pthread_create";
  } else {
    LOG(INFO) << "Cannot Allocate memory for cmdjob: " << cmd;
  }
  return 0;
}

/*******************************************************************************
 ** Load stack lib
 *******************************************************************************/

int HAL_load(void) {
  LOG(INFO) << "Loading HAL library and extensions";
  bt_interface_t* interface;
  int err = hal_util_load_bt_library((const bt_interface_t**)&interface);
  if (err) {
    LOG(ERROR) << "Error loading HAL library: " << strerror(err);
    return err;
  }
  sBtInterface = interface;
  LOG(INFO) << "HAL library loaded";
  return err;
}

int HAL_unload(void) {
  int err = 0;
  LOG(INFO) << "Unloading HAL lib";
  sBtInterface = nullptr;
  LOG(INFO) << "HAL library unloaded, status: " << strerror(err);
  return err;
}

/*******************************************************************************
 ** HAL test functions & callbacks
 *******************************************************************************/

void setup_test_env(void) {
  int i = 0;
  while (console_cmd_list[i].name) {
    console_cmd_maxlen =
        MAX(console_cmd_maxlen, (int)strlen(console_cmd_list[i].name));
    i++;
  }
}

void check_return_status(bt_status_t status) {
  if (status != BT_STATUS_SUCCESS) {
    LOG(INFO) << "HAL REQUEST FAILED status : " << status << " ("
              << dump_bt_status(status) << ")";
  } else {
    LOG(INFO) << "HAL REQUEST SUCCESS";
  }
}

static void adapter_state_changed(bt_state_t state) {
  int V1 = 1000, V2 = 2;
  bt_property_t property = {BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT, 4, &V1};
  bt_property_t property1 = {BT_PROPERTY_ADAPTER_SCAN_MODE, 2, &V2};
  bt_property_t property2 = {BT_PROPERTY_BDNAME, 6, (void*)"Fluoride_Test"};

  global_adapter_state = state;

  if (state == BT_STATE_ON) {
    global_bt_enabled = true;
    global_status = (bt_status_t)sBtInterface->set_adapter_property(&property1);
    global_status = (bt_status_t)sBtInterface->set_adapter_property(&property);
    global_status = (bt_status_t)sBtInterface->set_adapter_property(&property2);
  } else {
    global_bt_enabled = false;
  }
}

static void adapter_properties_changed(bt_status_t status, int num_properties,
                                       bt_property_t* properties) {
  RawAddress bd_addr;
  if (!properties) {
    printf("properties is null\n");
    return;
  }
  switch (properties->type) {
    case BT_PROPERTY_BDADDR:
      memcpy(bd_addr.address, properties->val,
             MIN((size_t)properties->len, sizeof(bd_addr)));
      LOG(INFO) << "Local Bd Addr = " << bd_addr;
      break;
    default:
      break;
  }
  return;
}

static void discovery_state_changed(bt_discovery_state_t state) {
  LOG(INFO) << "Discovery State Updated: "
            << (state == BT_DISCOVERY_STOPPED ? "STOPPED" : "STARTED");
}

static void pin_request_cb(RawAddress* remote_bd_addr, bt_bdname_t* bd_name,
                           uint32_t cod, bool min_16_digit) {
  bt_pin_code_t pincode = {{0x31, 0x32, 0x33, 0x34}};

  if (BT_STATUS_SUCCESS !=
      sBtInterface->pin_reply(remote_bd_addr, true, 4, &pincode)) {
    LOG(INFO) << "Pin Reply failed";
  }
}

static void ssp_request_cb(RawAddress* remote_bd_addr, bt_bdname_t* bd_name,
                           uint32_t cod, bt_ssp_variant_t pairing_variant,
                           uint32_t pass_key) {
  LOG(INFO) << __func__ << ": device_name:" << bd_name->name
            << ", pairing_variant: " << (int)pairing_variant
            << ", passkey: " << unsigned(pass_key);
  if (BT_STATUS_SUCCESS !=
      sBtInterface->ssp_reply(remote_bd_addr, pairing_variant, true,
                              pass_key)) {
    LOG(ERROR) << "SSP Reply failed";
  }
}

static void bond_state_changed_cb(bt_status_t status,
                                  RawAddress* remote_bd_addr,
                                  bt_bond_state_t state) {
  LOG(INFO) << "Bond State Changed = " << state;
  global_pair_state = state;
}

static void acl_state_changed(bt_status_t status, RawAddress* remote_bd_addr,
                              bt_acl_state_t state) {
  LOG(INFO) << __func__ << ": remote_bd_addr=" << *remote_bd_addr
            << ", acl status=" << (state == BT_ACL_STATE_CONNECTED
                                       ? "ACL Connected"
                                       : "ACL Disconnected");
}

static void dut_mode_recv(uint16_t opcode, uint8_t* buf, uint8_t len) {
  LOG(INFO) << "DUT MODE RECV : NOT IMPLEMENTED";
}

static bt_callbacks_t bt_callbacks = {
    sizeof(bt_callbacks_t),
    adapter_state_changed,
    adapter_properties_changed, /*adapter_properties_cb */
    nullptr,                    /* remote_device_properties_cb */
    nullptr,                    /* device_found_cb */
    discovery_state_changed,    /* discovery_state_changed_cb */
    pin_request_cb,             /* pin_request_cb  */
    ssp_request_cb,             /* ssp_request_cb  */
    bond_state_changed_cb,      /*bond_state_changed_cb */
    acl_state_changed,          /* acl_state_changed_cb */
    nullptr,                    /* thread_evt_cb */
    dut_mode_recv,              /*dut_mode_recv_cb */
    nullptr,                    /* le_test_mode_cb */
    nullptr                     /* energy_info_cb */
};

static bool set_wake_alarm(uint64_t delay_millis, bool should_wake, alarm_cb cb,
                           void* data) {
  static timer_t timer;
  static bool timer_created;

  if (!timer_created) {
    struct sigevent sigevent;
    memset(&sigevent, 0, sizeof(sigevent));
    sigevent.sigev_notify = SIGEV_THREAD;
    sigevent.sigev_notify_function = (void (*)(union sigval))cb;
    sigevent.sigev_value.sival_ptr = data;
    timer_create(CLOCK_MONOTONIC, &sigevent, &timer);
    timer_created = true;
  }

  struct itimerspec new_value;
  new_value.it_value.tv_sec = delay_millis / 1000;
  new_value.it_value.tv_nsec = (delay_millis % 1000) * 1000 * 1000;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 0;
  timer_settime(timer, 0, &new_value, nullptr);

  return true;
}

static int acquire_wake_lock(const char* lock_name) {
  return BT_STATUS_SUCCESS;
}

static int release_wake_lock(const char* lock_name) {
  return BT_STATUS_SUCCESS;
}

static bt_os_callouts_t callouts = {
    sizeof(bt_os_callouts_t), set_wake_alarm, acquire_wake_lock,
    release_wake_lock,
};

void adapter_init(void) {
  LOG(INFO) << __func__;
  global_status = (bt_status_t)sBtInterface->init(&bt_callbacks);
  if (global_status == BT_STATUS_SUCCESS) {
    global_status = (bt_status_t)sBtInterface->set_os_callouts(&callouts);
  }
  check_return_status(global_status);
}

void adapter_enable(void) {
  LOG(INFO) << __func__;
  if (global_bt_enabled) {
    LOG(INFO) << __func__ << ": Bluetooth is already enabled";
    return;
  }
  global_status = (bt_status_t)sBtInterface->enable(global_strict_mode);
  check_return_status(global_status);
}

void adapter_disable(void) {
  LOG(INFO) << __func__;
  if (!global_bt_enabled) {
    LOG(INFO) << __func__ << ": Bluetooth is already disabled";
    return;
  }
  global_status = (bt_status_t)sBtInterface->disable();
  check_return_status(global_status);
}
void adapter_dut_mode_configure(char* p) {
  LOG(INFO) << __func__;
  if (!global_bt_enabled) {
    LOG(INFO) << __func__
              << ": Bluetooth must be enabled for test_mode to work.";
    return;
  }
  int32_t mode = get_signed_int(&p, -1);  // arg1
  if ((mode != 0) && (mode != 1)) {
    LOG(INFO) << __func__ << "Please specify mode: 1 to enter, 0 to exit";
    return;
  }
  global_status = (bt_status_t)sBtInterface->dut_mode_configure(mode);
  check_return_status(global_status);
}

void adapter_cleanup(void) {
  LOG(INFO) << __func__;
  sBtInterface->cleanup();
}

/*******************************************************************************
 ** Console commands
 *******************************************************************************/

void do_help(char* p) {
  int i = 0;
  char line[128];
  int pos = 0;

  while (console_cmd_list[i].name != nullptr) {
    pos = snprintf(line, sizeof(line), "%s", (char*)console_cmd_list[i].name);
    printf("%s %s\n", (char*)line, (char*)console_cmd_list[i].help);
    i++;
  }
}

void do_quit(char* p) { console_shutdown(); }

/*******************************************************************
 *
 *  BT TEST  CONSOLE COMMANDS
 *
 *  Parses argument lists and passes to API test function
 *
 */

void do_init(char* p) { adapter_init(); }

void do_enable(char* p) { adapter_enable(); }

void do_disable(char* p) { adapter_disable(); }

void do_cleanup(char* p) { adapter_cleanup(); }

/**
 * MCAP API commands
 */
void do_mcap_register(char* p) {
  uint16_t ctrl_psm = get_hex(&p, 0);  // arg1
  uint16_t data_psm = get_hex(&p, 0);  // arg2
  uint16_t sec_mask = get_int(&p, 0);  // arg3
  printf("%s: ctrl_psm=0x%04x, data_psm=0x%04x, sec_mask=0x%04x\n", __func__,
         ctrl_psm, data_psm, sec_mask);
  if (!ctrl_psm || !data_psm) {
    printf("%s: Invalid Parameters\n", __func__);
    return;
  }
  sMcapTestApp->Register(ctrl_psm, data_psm, sec_mask, mcap_ctrl_callback);
  printf("%s: mcap_handle=%d\n", __func__, sMcapTestApp->GetHandle());
}

void do_mcap_deregister(char* p) {
  printf("%s: mcap_handle=%d\n", __func__, sMcapTestApp->GetHandle());
  sMcapTestApp->Deregister();
  printf("%s: handle=%d\n", __func__, sMcapTestApp->GetHandle());
}

void do_mcap_create_mdep(char* p) {
  int type = get_int(&p, -1);  // arg1
  printf("%s: mcap_handle=%d, type=%d\n", __func__, sMcapTestApp->GetHandle(),
         type);
  bool ret = sMcapTestApp->CreateMdep(type, MCA_NUM_MDLS, mcap_data_cb);
  printf("%s: %s\n", __func__, ret ? "SUCCESS" : "FAIL");
}

static void do_mcap_delete_mdep(char* p) {
  uint8_t mdep_handle = get_int(&p, 0);
  printf("%s: mcap_handle=%d, mdep_handle=%d\n", __func__,
         sMcapTestApp->GetHandle(), mdep_handle);
  if (!mdep_handle) {
    printf("%s: Invalid Parameters\n", __func__);
    return;
  }
  McapMdep* mcap_mdep = sMcapTestApp->FindMdepByHandle(mdep_handle);
  if (!mcap_mdep) {
    LOG(ERROR) << "No MDEP for handle " << (int)mdep_handle;
    return;
  }
  bool ret = mcap_mdep->Delete();
  printf("%s: %s\n", __func__, ret ? "SUCCESS" : "FAIL");
}

static void do_mcap_connect_mcl(char* p) {
  char buf[64];
  get_str(&p, buf);  // arg1
  RawAddress bd_addr;
  bool valid_bd_addr = RawAddress::FromString(buf, bd_addr);
  uint16_t ctrl_psm = get_hex(&p, 0);  // arg2
  uint16_t sec_mask = get_int(&p, 0);  // arg3
  printf("%s: mcap_handle=%d, ctrl_psm=0x%04x, secMask=0x%04x, bd_addr=%s\n",
         __func__, sMcapTestApp->GetHandle(), ctrl_psm, sec_mask, buf);
  if (!ctrl_psm || !valid_bd_addr) {
    printf("%s: Invalid Parameters\n", __func__);
    return;
  }
  bool ret = sMcapTestApp->ConnectMcl(bd_addr, ctrl_psm, sec_mask);
  printf("%s: %s\n", __func__, ret ? "SUCCESS" : "FAIL");
}

static void do_mcap_disconnect_mcl(char* p) {
  char buf[64];
  get_str(&p, buf);  // arg1
  RawAddress bd_addr;
  bool valid_bd_addr = RawAddress::FromString(buf, bd_addr);
  printf("%s: bd_addr=%s\n", __func__, buf);
  if (!valid_bd_addr) {
    printf("%s: Invalid Parameters\n", __func__);
    return;
  }
  McapMcl* mcap_mcl = sMcapTestApp->FindMclByPeerAddress(bd_addr);
  if (!mcap_mcl) {
    LOG(ERROR) << "No MCL for bd_addr " << buf;
    return;
  }
  bool ret = mcap_mcl->Disconnect();
  printf("%s: %s\n", __func__, ret ? "SUCCESS" : "FAIL");
}

static void do_mcap_create_mdl(char* p) {
  char buf[64];
  get_str(&p, buf);  // arg1
  RawAddress bd_addr;
  bool valid_bd_addr = RawAddress::FromString(buf, bd_addr);
  uint16_t mdep_handle = get_int(&p, 0);  // arg2
  uint16_t data_psm = get_hex(&p, 0);     // arg3
  uint16_t mdl_id = get_int(&p, 0);       // arg4
  uint8_t peer_dep_id = get_int(&p, 0);   // arg5
  uint8_t cfg = get_hex(&p, 0);           // arg6
  int do_not_connect = get_int(&p, 0);    // arg7
  printf(
      "%s: bd_addr=%s, mdep_handle=%d, data_psm=0x%04x, mdl_id=%d,"
      " peer_dep_id=%d, cfg=0x%02x, do_not_connect=%d\n",
      __func__, buf, mdep_handle, data_psm, mdl_id, peer_dep_id, cfg,
      do_not_connect);
  if (!data_psm || !peer_dep_id || !valid_bd_addr || !mdep_handle) {
    printf("%s: Invalid Parameters\n", __func__);
    return;
  }
  McapMcl* mcap_mcl = sMcapTestApp->FindMclByPeerAddress(bd_addr);
  if (!mcap_mcl) {
    LOG(ERROR) << "No MCL for bd_addr " << buf;
    return;
  }
  bool ret = mcap_mcl->CreateMdl(mdep_handle, data_psm, mdl_id, peer_dep_id,
                                 cfg, !do_not_connect);
  printf("%s: %s\n", __func__, ret ? "SUCCESS" : "FAIL");
}

static void do_mcap_data_channel_config(char* p) {
  char buf[64];
  get_str(&p, buf);  // arg1
  RawAddress bd_addr;
  bool valid_bd_addr = RawAddress::FromString(buf, bd_addr);
  printf("%s: bd_addr=%s\n", __func__, buf);
  if (!valid_bd_addr) {
    printf("%s: Invalid Parameters\n", __func__);
    return;
  }
  McapMcl* mcap_mcl = sMcapTestApp->FindMclByPeerAddress(bd_addr);
  if (!mcap_mcl) {
    LOG(ERROR) << "No MCL for bd_addr " << buf;
    return;
  }
  bool ret = mcap_mcl->DataChannelConfig();
  printf("%s: %s\n", __func__, ret ? "SUCCESS" : "FAIL");
}

static void do_mcap_abort_mdl(char* p) {
  char buf[64];
  get_str(&p, buf);  // arg1
  RawAddress bd_addr;
  bool valid_bd_addr = RawAddress::FromString(buf, bd_addr);
  printf("%s: bd_addr=%s\n", __func__, buf);
  if (!valid_bd_addr) {
    printf("%s: Invalid Parameters\n", __func__);
    return;
  }
  McapMcl* mcap_mcl = sMcapTestApp->FindMclByPeerAddress(bd_addr);
  if (!mcap_mcl) {
    LOG(ERROR) << "No MCL for bd_addr " << buf;
    return;
  }
  bool ret = mcap_mcl->AbortMdl();
  printf("%s: %s\n", __func__, ret ? "SUCCESS" : "FAIL");
}

static void do_mcap_delete_mdl(char* p) {
  char buf[64];
  get_str(&p, buf);  // arg1
  RawAddress bd_addr;
  bool valid_bd_addr = RawAddress::FromString(buf, bd_addr);
  uint16_t mdl_id = get_int(&p, 0);  // arg2
  printf("%s: bd_addr=%s, mdl_id=%d\n", __func__, buf, mdl_id);
  if (!valid_bd_addr) {
    printf("%s: Invalid Parameters\n", __func__);
    return;
  }
  McapMcl* mcap_mcl = sMcapTestApp->FindMclByPeerAddress(bd_addr);
  if (!mcap_mcl) {
    LOG(ERROR) << "No MCL for bd_addr " << buf;
    return;
  }
  bool ret = mcap_mcl->DeleteMdl(mdl_id);
  printf("%s: %s\n", __func__, ret ? "SUCCESS" : "FAIL");
}

static void do_mcap_close_mdl(char* p) {
  char buf[64];
  get_str(&p, buf);  // arg1
  RawAddress bd_addr;
  bool valid_bd_addr = RawAddress::FromString(buf, bd_addr);
  uint16_t mdl_id = get_int(&p, 0);  // arg2
  printf("%s: bd_addr=%s, mdl_id=%d\n", __func__, buf, mdl_id);
  if (!valid_bd_addr || !mdl_id) {
    printf("%s: Invalid Parameters\n", __func__);
    return;
  }
  McapMcl* mcap_mcl = sMcapTestApp->FindMclByPeerAddress(bd_addr);
  if (!mcap_mcl) {
    LOG(ERROR) << "No MCL for bd_addr " << buf;
    return;
  }
  McapMdl* mcap_mdl = mcap_mcl->FindMdlById(mdl_id);
  if (!mcap_mdl) {
    LOG(ERROR) << "No MDL for ID " << (int)mdl_id;
    return;
  }
  bool ret = mcap_mdl->Close();
  printf("%s: %s\n", __func__, ret ? "SUCCESS" : "FAIL");
}

static void do_mcap_reconnect_mdl(char* p) {
  char buf[64];
  get_str(&p, buf);  // arg1
  RawAddress bd_addr;
  bool valid_bd_addr = RawAddress::FromString(buf, bd_addr);
  uint16_t data_psm = get_hex(&p, 0);  // arg1
  uint16_t mdl_id = get_int(&p, 0);    // arg2
  printf("%s: data_psm=0x%04x, mdl_id=%d\n", __func__, data_psm, mdl_id);
  if (!valid_bd_addr) {
    printf("%s: Invalid Parameters\n", __func__);
    return;
  }
  McapMcl* mcap_mcl = sMcapTestApp->FindMclByPeerAddress(bd_addr);
  if (!mcap_mcl) {
    LOG(ERROR) << "No MCL for bd_addr " << buf;
    return;
  }
  McapMdl* mcap_mdl = mcap_mcl->FindMdlById(mdl_id);
  if (!mcap_mdl) {
    LOG(ERROR) << "No MDL for ID " << (int)mdl_id;
    return;
  }
  bool ret = mcap_mdl->Reconnect(data_psm);
  printf("%s: %s\n", __func__, ret ? "SUCCESS" : "FAIL");
}

static void do_pairing(char* p) {
  RawAddress bd_addr;
  if (!RawAddress::FromString(p, bd_addr)) {
    LOG(ERROR) << "Invalid Bluetooth address " << p;
    return;
  }
  if (BT_STATUS_SUCCESS !=
      sBtInterface->create_bond(&bd_addr, BT_TRANSPORT_BR_EDR)) {
    LOG(ERROR) << "Failed to Initiate Pairing";
    return;
  }
}

/** CONSOLE COMMAND TABLE */

const cmd_t console_cmd_list[] = {
    /* INTERNAL */
    {"help", do_help, "", 0},
    {"quit", do_quit, "", 0},
    /* API CONSOLE COMMANDS */
    /* Init and Cleanup shall be called automatically */
    {"enable_bluetooth", do_enable, "", 0},
    {"disable_bluetooth", do_disable, "", 0},
    {"pair", do_pairing, "BD_ADDR<xx:xx:xx:xx:xx:xx>", 0},
    {"register", do_mcap_register,
     "ctrl_psm<hex> data_psm<hex> security_mask<0-10>", 0},
    {"deregister", do_mcap_deregister, "", 0},
    {"create_mdep", do_mcap_create_mdep, "type<0-Echo, 1-Normal>", 0},
    {"delete_mdep", do_mcap_delete_mdep, "mdep_handle<int>", 0},
    {"connect_mcl", do_mcap_connect_mcl,
     "BD_ADDR<xx:xx:xx:xx:xx:xx> ctrl_psm<hex> security_mask<0-10>", 0},
    {"disconnect_mcl", do_mcap_disconnect_mcl, "BD_ADDR<xx:xx:xx:xx:xx:xx>", 0},
    {"create_mdl", do_mcap_create_mdl,
     "BD_ADDR<xx:xx:xx:xx:xx:xx> mdep_handle<int> data_psm<hex> mdl_id<int> "
     "peer_dep_id<int> cfg<hex> "
     "do_not_connect<0-connect,1-wait_for_data_channel_config>",
     0},
    {"data_channel_config", do_mcap_data_channel_config,
     "BD_ADDR<xx:xx:xx:xx:xx:xx>", 0},
    {"abort_mdl", do_mcap_abort_mdl, "BD_ADDR<xx:xx:xx:xx:xx:xx>", 0},
    {"close_mdl", do_mcap_close_mdl, "BD_ADDR<xx:xx:xx:xx:xx:xx> mdl_id<int>",
     0},
    {"delete_mdl", do_mcap_delete_mdl, "BD_ADDR<xx:xx:xx:xx:xx:xx> mdl_id<int>",
     0},
    {"reconnect_mdl", do_mcap_reconnect_mdl,
     "BD_ADDR<xx:xx:xx:xx:xx:xx> data_psm<hex> mdl_id<int>", 0},
    /* last entry */
    {nullptr, nullptr, "", 0},
};

/** Main console command handler */

static void process_cmd(char* p, bool is_job) {
  char cmd[2048];
  int i = 0;
  char* p_saved = p;

  get_str(&p, cmd);  // arg1

  /* table commands */
  while (console_cmd_list[i].name != nullptr) {
    if (is_cmd(cmd, console_cmd_list[i].name)) {
      if (!is_job && console_cmd_list[i].is_job)
        create_cmdjob(p_saved);
      else {
        console_cmd_list[i].handler(p);
      }
      return;
    }
    i++;
  }
  LOG(ERROR) << "Unknown command: " << p_saved;
  do_help(nullptr);
}

int main(int argc, char* argv[]) {
  setbuf(stdout, NULL);
#if !defined(OS_GENERIC)
  config_permissions();
#endif
  LOG(INFO) << "Fluoride MCAP test app is starting";

  if (HAL_load() < 0) {
    fprintf(stderr, "%s: HAL failed to initialize, exit\n", __func__);
    unlink(PID_FILE);
    exit(0);
  }

  setup_test_env();

  /* Automatically perform the init */
  adapter_init();
  sleep(2);
  adapter_enable();
  sleep(2);
  sMcapTestInterface =
      (btmcap_test_interface_t*)sBtInterface->get_profile_interface(
          BT_TEST_INTERFACE_MCAP_ID);
  sMcapTestInterface->init();
  sMcapTestApp = new McapTestApp(sMcapTestInterface);

  /* Main loop */
  char line[2048];
  while (!global_main_done) {
    memset(line, '\0', sizeof(line));
    /* command prompt */
    printf(">");
    fflush(stdout);
    fgets(line, sizeof(line), stdin);
    if (line[0] != '\0') {
      /* Remove line feed */
      line[strlen(line) - 1] = 0;
      if (strlen(line) != 0) process_cmd(line, false);
    }
  }
  adapter_cleanup();
  HAL_unload();
  LOG(INFO) << "Fluoride MCAP test app is terminating";

  return 0;
}
