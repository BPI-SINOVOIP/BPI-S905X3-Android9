/******************************************************************************
 *
 *  Copyright 1999-2012 Broadcom Corporation
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

#define LOG_TAG "bt_btu_task"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <base/bind.h>
#include <base/logging.h>
#include <base/run_loop.h>
#include <base/threading/thread.h>

#include "bta/sys/bta_sys.h"
#include "btcore/include/module.h"
#include "bte.h"
#include "btif/include/btif_common.h"
#include "osi/include/osi.h"
#include "osi/include/thread.h"
#include "stack/btm/btm_int.h"
#include "stack/include/btu.h"
#include "stack/l2cap/l2c_int.h"

static const int THREAD_RT_PRIORITY = 1;

/* Define BTU storage area */
uint8_t btu_trace_level = HCI_INITIAL_TRACE_LEVEL;

extern thread_t* bt_workqueue_thread;

static base::MessageLoop* message_loop_ = NULL;
static base::RunLoop* run_loop_ = NULL;
static thread_t* message_loop_thread_;

void btu_hci_msg_process(BT_HDR* p_msg) {
  /* Determine the input message type. */
  switch (p_msg->event & BT_EVT_MASK) {
    case BT_EVT_TO_BTU_HCI_ACL:
      /* All Acl Data goes to L2CAP */
      l2c_rcv_acl_data(p_msg);
      break;

    case BT_EVT_TO_BTU_L2C_SEG_XMIT:
      /* L2CAP segment transmit complete */
      l2c_link_segments_xmitted(p_msg);
      break;

    case BT_EVT_TO_BTU_HCI_SCO:
#if (BTM_SCO_INCLUDED == TRUE)
      btm_route_sco_data(p_msg);
      break;
#endif

    case BT_EVT_TO_BTU_HCI_EVT:
      btu_hcif_process_event((uint8_t)(p_msg->event & BT_SUB_EVT_MASK), p_msg);
      osi_free(p_msg);
      break;

    case BT_EVT_TO_BTU_HCI_CMD:
      btu_hcif_send_cmd((uint8_t)(p_msg->event & BT_SUB_EVT_MASK), p_msg);
      break;

    default:
      osi_free(p_msg);
      break;
  }
}

base::MessageLoop* get_message_loop() { return message_loop_; }

void btu_message_loop_run(UNUSED_ATTR void* context) {
  message_loop_ = new base::MessageLoop();
  run_loop_ = new base::RunLoop();

  // Inform the bt jni thread initialization is ok.
  message_loop_->task_runner()->PostTask(
      FROM_HERE, base::Bind(base::IgnoreResult(&btif_transfer_context),
                            btif_init_ok, 0, nullptr, 0, nullptr));

  run_loop_->Run();

  delete message_loop_;
  message_loop_ = NULL;

  delete run_loop_;
  run_loop_ = NULL;
}

void btu_task_start_up(UNUSED_ATTR void* context) {
  LOG(INFO) << "Bluetooth chip preload is complete";

  /* Initialize the mandatory core stack control blocks
     (BTU, BTM, L2CAP, and SDP)
   */
  btu_init_core();

  /* Initialize any optional stack components */
  BTE_InitStack();

  bta_sys_init();

  /* Initialise platform trace levels at this point as BTE_InitStack() and
   * bta_sys_init()
   * reset the control blocks and preset the trace level with
   * XXX_INITIAL_TRACE_LEVEL
   */
  module_init(get_module(BTE_LOGMSG_MODULE));

  message_loop_thread_ = thread_new("btu message loop");
  if (!message_loop_thread_) {
    LOG(FATAL) << __func__ << " unable to create btu message loop thread.";
  }

  thread_set_rt_priority(message_loop_thread_, THREAD_RT_PRIORITY);
  thread_post(message_loop_thread_, btu_message_loop_run, nullptr);

}

void btu_task_shut_down(UNUSED_ATTR void* context) {
  // Shutdown message loop on task completed
  if (run_loop_ && message_loop_) {
    message_loop_->task_runner()->PostTask(FROM_HERE, run_loop_->QuitClosure());
  }

  module_clean_up(get_module(BTE_LOGMSG_MODULE));

  bta_sys_free();
  btu_free_core();
}
