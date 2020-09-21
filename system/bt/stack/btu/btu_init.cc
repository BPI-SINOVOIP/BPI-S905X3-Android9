/******************************************************************************
 *
 *  Copyright 2000-2012 Broadcom Corporation
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

#define LOG_TAG "bt_task"

#include <base/logging.h>
#include <pthread.h>
#include <string.h>

#include "bt_target.h"
#include "btm_int.h"
#include "btu.h"
#include "device/include/controller.h"
#include "gatt_api.h"
#include "gatt_int.h"
#include "l2c_int.h"
#include "osi/include/alarm.h"
#include "osi/include/fixed_queue.h"
#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "sdpint.h"
#include "smp_int.h"

// RT priority for audio-related tasks
#define BTU_TASK_RT_PRIORITY 1

// Communication queue from hci thread to bt_workqueue.
extern fixed_queue_t* btu_hci_msg_queue;

thread_t* bt_workqueue_thread;
static const char* BT_WORKQUEUE_NAME = "bt_workqueue";

extern void PLATFORM_DisableHciTransport(uint8_t bDisable);

void btu_task_start_up(void* context);
void btu_task_shut_down(void* context);

/*****************************************************************************
 *
 * Function         btu_init_core
 *
 * Description      Initialize control block memory for each core component.
 *
 *
 * Returns          void
 *
 *****************************************************************************/
void btu_init_core(void) {
  /* Initialize the mandatory core stack components */
  btm_init();

  l2c_init();

  sdp_init();

  gatt_init();

  SMP_Init();

  btm_ble_init();
}

/*****************************************************************************
 *
 * Function         btu_free_core
 *
 * Description      Releases control block memory for each core component.
 *
 *
 * Returns          void
 *
 *****************************************************************************/
void btu_free_core(void) {
  /* Free the mandatory core stack components */
  gatt_free();

  l2c_free();

  sdp_free();

  btm_free();
}

/*****************************************************************************
 *
 * Function         BTU_StartUp
 *
 * Description      Initializes the BTU control block.
 *
 *                  NOTE: Must be called before creating any tasks
 *                      (RPC, BTU, HCIT, APPL, etc.)
 *
 * Returns          void
 *
 *****************************************************************************/
void BTU_StartUp(void) {
  btu_trace_level = HCI_INITIAL_TRACE_LEVEL;

  bt_workqueue_thread = thread_new(BT_WORKQUEUE_NAME);
  if (bt_workqueue_thread == NULL) goto error_exit;

  thread_set_rt_priority(bt_workqueue_thread, BTU_TASK_RT_PRIORITY);

  // Continue startup on bt workqueue thread.
  thread_post(bt_workqueue_thread, btu_task_start_up, NULL);
  return;

error_exit:;
  LOG_ERROR(LOG_TAG, "%s Unable to allocate resources for bt_workqueue",
            __func__);
  BTU_ShutDown();
}

void BTU_ShutDown(void) {
  btu_task_shut_down(NULL);


  thread_free(bt_workqueue_thread);

  bt_workqueue_thread = NULL;
}
