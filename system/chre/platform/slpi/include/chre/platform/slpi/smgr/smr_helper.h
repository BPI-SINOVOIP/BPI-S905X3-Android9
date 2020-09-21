/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef CHRE_PLATFORM_SLPI_SMGR_SMR_HELPER_H_
#define CHRE_PLATFORM_SLPI_SMGR_SMR_HELPER_H_

#include <type_traits>

extern "C" {

#include "qurt.h"
#include "sns_usmr.h"

}

#include "chre/platform/condition_variable.h"
#include "chre/platform/mutex.h"
#include "chre/util/non_copyable.h"
#include "chre/util/time.h"
#include "chre/util/unique_ptr.h"

namespace chre {

//! Default timeout for sendReqSync
constexpr Nanoseconds kDefaultSmrTimeout = Seconds(2);

//! Default timeout for waitForService. Have a longer timeout since there may be
//! external dependencies blocking SMGR initialization.
constexpr Nanoseconds kDefaultSmrWaitTimeout = Seconds(5);

/**
 * A helper class for making synchronous requests to SMR (Sensors Message
 * Router). Not safe to use from multiple threads.
 */
class SmrHelper : public NonCopyable {
 public:
  /**
   * Wrapper to convert the async smr_client_release() to a synchronous call.
   *
   * @param clientHandle SMR handle to release
   * @param timeout How long to wait for the response before abandoning it
   *
   * @return Result code returned by smr_client_release(), or SMR_TIMEOUT_ERR if
   *         the timeout was reached
   */
  smr_err releaseSync(smr_client_hndl clientHandle,
                      Nanoseconds timeout = kDefaultSmrTimeout);

  /**
   * Wrapper to convert the async smr_client_send_req() to a synchronous call.
   *
   * Only one request can be pending at a time per instance of SmrHelper.
   *
   * @param ReqStruct QMI IDL-generated request structure
   * @param RespStruct QMI IDL-generated response structure
   * @param clientHandle SMR handle previously given by smr_client_init()
   * @param msgId QMI message ID of the request to send
   * @param req Pointer to populated request structure
   * @param resp Pointer to structure to receive the response
   * @param timeout How long to wait for the response before abandoning it
   *
   * @return Result code returned by smr_client_send_req(), or SMR_TIMEOUT_ERR
   *         if the supplied timeout was reached
   */
  template<typename ReqStruct, typename RespStruct>
  smr_err sendReqSync(
      smr_client_hndl clientHandle, unsigned int msgId,
      UniquePtr<ReqStruct> *req, UniquePtr<RespStruct> *resp,
      Nanoseconds timeout = kDefaultSmrTimeout) {
    // Try to catch copy/paste errors at compile time - QMI always has a
    // different struct definition for request and response
    static_assert(!std::is_same<ReqStruct, RespStruct>::value,
                  "Request and response structures must be different");

    smr_err result;
    bool timedOut = !sendReqSyncUntyped(
        clientHandle, msgId, req->get(), sizeof(ReqStruct),
        resp->get(), sizeof(RespStruct), timeout, &result);

    // Unlike QMI, SMR does not support canceling an in-flight transaction.
    // SMR's internal request structure maintains a pointer to the client
    // request and response buffers, so in the event of a timeout, it is unsafe
    // for us to free the memory because the service may try to send the
    // response later on - we'll try to free it if that ever happens, but
    // otherwise we need to leave the memory allocation open.
    if (timedOut) {
      req->release();
      resp->release();
    }

    return result;
  }

  /**
   * Wrapper to convert the async smr_client_check_ext() to a synchronous call.
   * Waits for an SMR service to become available.
   *
   * @param serviceObj The SMR service object to wait for.
   * @param timeout The wait timeout in microseconds.
   *
   * @return Result code returned by smr_client_check_ext, or SMR_TIMEOUT_ERR if
   *         the timeout was reached
   */
  smr_err waitForService(qmi_idl_service_object_type serviceObj,
                         Microseconds timeout = kDefaultSmrWaitTimeout);

 private:
  /**
   * Used to track asynchronous SMR requests from sendReqSyncUntyped() to
   * smrRespCb()
   */
  struct SmrTransaction {
    //! Value of SmrHelper::mCurrentTransactionId when this instance was
    //! created - if it does not match at the time the transaction is given in
    //! the callback, this transaction is invalid (it has timed out)
    uint32_t transactionId;

    //! Pointer to the SmrHelper instance that created this transaction
    SmrHelper *parent;

    // SMR request and response buffers given by the client; only used to free
    // memory in the event of a late (post-timeout) callback
    void *reqBuf;
    void *rspBuf;
  };

  /**
   * Implements sendReqSync(), but with accepting untyped (void*) buffers.
   * snake_case parameters exactly match those given to smr_client_send_req().
   *
   * @param timeout How long to wait for the response before abandoning it
   * @param result If smr_client_send_req() returns an error, then that error
   *        code, otherwise the transport error code given in the SMR response
   *        callback (assuming there was no timeout)
   * @return false on timeout, otherwise true (includes the case where
   *         smr_client_send_req() returns an immediate error)
   *
   * @see sendReqSync()
   * @see smr_client_send_req()
   */
  bool sendReqSyncUntyped(
      smr_client_hndl client_handle, unsigned int msg_id,
      void *req_c_struct, unsigned int req_c_struct_len,
      void *resp_c_struct, unsigned int resp_c_struct_len,
      Nanoseconds timeout, smr_err *result);

  /**
   * Processes an SMR response callback
   *
   * @see smr_client_resp_cb
   */
  void handleResp(smr_client_hndl client_handle, unsigned int msg_id,
                  void *resp_c_struct, unsigned int resp_c_struct_len,
                  smr_err transp_err, SmrTransaction *txn);

  /**
   * Sets mWaiting to true in advance of calling an async SMR function.
   * Preconditions: mMutex not held, mWaiting false
   */
  void prepareForWait();

  /**
   * SMR release complete callback used with releaseSync()
   *
   * @see smr_client_release_cb
   */
  static void smrReleaseCb(void *release_cb_data);

  /**
   * Extracts "this" from resp_cb_data and calls through to handleResp()
   *
   * @see smr_client_resp_cb
   */
  static void smrRespCb(smr_client_hndl client_handle, unsigned int msg_id,
                        void *resp_c_struct, unsigned int resp_c_struct_len,
                        void *resp_cb_data, smr_err transp_err);

  /**
   * SMR wait for service callback used with waitForService()
   *
   * @see smr_client_init_ext_cb
   */
  static void smrWaitForServiceCb(qmi_idl_service_object_type service_obj,
                                  qmi_service_instance instance_id,
                                  bool timeout_expired,
                                  void *wait_for_service_cb_data);

  //! Used to synchronize responses
  ConditionVariable mCond;

  //! Used with mCond, and to protect access to member variables from other
  //! threads
  Mutex mMutex;

  //! true if we are waiting on an async response
  bool mWaiting = false;

  //! The transaction ID we're expecting in the next response callback
  uint32_t mCurrentTransactionId = 0;

  //! true if timed out while waiting for a service to become available
  bool mServiceTimedOut = false;

  //! The (transport) error code given in the response callback
  smr_err mTranspErr;
};

}  // namespace chre

#endif  // CHRE_PLATFORM_SLPI_SMGR_SMR_HELPER_H_
