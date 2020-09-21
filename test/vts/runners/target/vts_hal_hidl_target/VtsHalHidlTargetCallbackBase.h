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

#ifndef __VTS_HAL_HIDL_TARGET_CALLBACK_BASE_H
#define __VTS_HAL_HIDL_TARGET_CALLBACK_BASE_H

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <utility>

using namespace ::std;
using namespace ::std::chrono;

constexpr char kVtsHalHidlTargetCallbackDefaultName[] =
    "VtsHalHidlTargetCallbackDefaultName";
constexpr milliseconds DEFAULT_CALLBACK_WAIT_TIMEOUT_INITIAL = minutes(1);

namespace testing {

/*
 * VTS target side test template for callback.
 *
 * Providing wait and notify for callback functionality.
 *
 * A typical usage looks like this:
 *
 * class CallbackArgs {
 *   ArgType1 arg1;
 *   ArgType2 arg2;
 * }
 *
 * class MyCallback
 *     : public ::testing::VtsHalHidlTargetCallbackBase<>,
 *       public CallbackInterface {
 *  public:
 *   CallbackApi1(ArgType1 arg1) {
 *     CallbackArgs data;
 *     data.arg1 = arg1;
 *     NotifyFromCallback("CallbackApi1", data);
 *   }
 *
 *   CallbackApi2(ArgType2 arg2) {
 *     CallbackArgs data;
 *     data.arg1 = arg1;
 *     NotifyFromCallback("CallbackApi2", data);
 *   }
 * }
 *
 * Test(MyTest) {
 *   CallApi1();
 *   CallApi2();
 *   auto result = cb_.WaitForCallback("CallbackApi1");
 *   // cb_ as an instance of MyCallback, result is an instance of
 *   // ::testing::VtsHalHidlTargetCallbackBase::WaitForCallbackResult
 *   EXPECT_TRUE(result.no_timeout); // Check wait did not time out
 *   EXPECT_TRUE(result.args); // Check CallbackArgs is received (not
 *                                  nullptr). This is optional.
 *   // Here check value of args using the pointer result.args;
 *   result = cb_.WaitForCallback("CallbackApi2");
 *   EXPECT_TRUE(result.no_timeout);
 *   // Here check value of args using the pointer result.args;
 *
 *   // Additionally. a test can wait for one of multiple callbacks.
 *   // In this case, wait will return when any of the callbacks in the provided
 *   // name list is called.
 *   result = cb_.WaitForCallbackAny(<vector_of_string>)
 *   // When vector_of_string is not provided, all callback functions will
 *   // be monitored. The name of callback function that was invoked
 *   // is stored in result.name
 * }
 *
 * Note type of CallbackArgsTemplateClass is same across the class, which means
 * all WaitForCallback method will return the same data type.
 */
template <class CallbackArgsTemplateClass>
class VtsHalHidlTargetCallbackBase {
 public:
  struct WaitForCallbackResult {
    WaitForCallbackResult()
        : no_timeout(false),
          args(shared_ptr<CallbackArgsTemplateClass>(nullptr)),
          name("") {}

    // Whether the wait timed out
    bool no_timeout;
    // Arguments data from callback functions. Defaults to nullptr.
    shared_ptr<CallbackArgsTemplateClass> args;
    // Name of the callback. Defaults to empty string.
    string name;
  };

  VtsHalHidlTargetCallbackBase()
      : cb_default_wait_timeout_(DEFAULT_CALLBACK_WAIT_TIMEOUT_INITIAL) {}

  virtual ~VtsHalHidlTargetCallbackBase() {
    for (auto it : cb_lock_map_) {
      delete it.second;
    }
  }

  /*
   * Wait for a callback function in a test.
   * Returns a WaitForCallbackResult object containing wait results.
   * If callback_function_name is not provided, a default name will be used.
   * Timeout defaults to -1 milliseconds. Negative timeout means use to
   * use the time out set for the callback or default callback wait time out.
   */
  WaitForCallbackResult WaitForCallback(
      const string& callback_function_name =
          kVtsHalHidlTargetCallbackDefaultName,
      milliseconds timeout = milliseconds(-1)) {
    return GetCallbackLock(callback_function_name)->WaitForCallback(timeout);
  }

  /*
   * Wait for any of the callback functions specified.
   * Returns a WaitForCallbackResult object containing wait results.
   * If callback_function_names is not provided, all callback functions will
   * be monitored, and the list of callback functions will be updated
   * dynamically during run time.
   * If timeout_any is not provided, the shortest timeout from the function
   * list will be used.
   */
  WaitForCallbackResult WaitForCallbackAny(
      const vector<string>& callback_function_names = vector<string>(),
      milliseconds timeout_any = milliseconds(-1)) {
    unique_lock<mutex> lock(cb_wait_any_mtx_);

    auto start_time = system_clock::now();

    WaitForCallbackResult res = PeekCallbackLocks(callback_function_names);
    while (!res.no_timeout) {
      auto expiration =
          GetWaitAnyTimeout(callback_function_names, start_time, timeout_any);
      auto status = cb_wait_any_cv_.wait_until(lock, expiration);
      if (status == cv_status::timeout) {
        cerr << "Timed out waiting for callback functions." << endl;
        break;
      }
      res = PeekCallbackLocks(callback_function_names);
    }
    return res;
  }

  /*
   * Notify a waiting test when a callback is invoked.
   * If callback_function_name is not provided, a default name will be used.
   */
  void NotifyFromCallback(const string& callback_function_name =
                              kVtsHalHidlTargetCallbackDefaultName) {
    unique_lock<mutex> lock(cb_wait_any_mtx_);
    GetCallbackLock(callback_function_name)->NotifyFromCallback();
    cb_wait_any_cv_.notify_one();
  }

  /*
   * Notify a waiting test with data when a callback is invoked.
   */
  void NotifyFromCallback(const CallbackArgsTemplateClass& data) {
    NotifyFromCallback(kVtsHalHidlTargetCallbackDefaultName, data);
  }

  /*
   * Notify a waiting test with data when a callback is invoked.
   * If callback_function_name is not provided, a default name will be used.
   */
  void NotifyFromCallback(const string& callback_function_name,
                          const CallbackArgsTemplateClass& data) {
    unique_lock<mutex> lock(cb_wait_any_mtx_);
    GetCallbackLock(callback_function_name)->NotifyFromCallback(data);
    cb_wait_any_cv_.notify_one();
  }

  /*
   * Clear lock and data for a callback function.
   * This function is optional.
   */
  void ClearForCallback(const string& callback_function_name =
                            kVtsHalHidlTargetCallbackDefaultName) {
    GetCallbackLock(callback_function_name, true);
  }

  /*
   * Get wait timeout for a specific callback function.
   * If callback_function_name is not provided, a default name will be used.
   */
  milliseconds GetWaitTimeout(const string& callback_function_name =
                                  kVtsHalHidlTargetCallbackDefaultName) {
    return GetCallbackLock(callback_function_name)->GetWaitTimeout();
  }

  /*
   * Set wait timeout for a specific callback function.
   * To set a default timeout (not for the default function name),
   * use SetWaitTimeoutDefault. default function name callback timeout will
   * also be set by SetWaitTimeoutDefault.
   */
  void SetWaitTimeout(const string& callback_function_name,
                      milliseconds timeout) {
    GetCallbackLock(callback_function_name)->SetWaitTimeout(timeout);
  }

  /*
   * Get default wait timeout for a callback function.
   * The default timeout is valid for all callback function names that
   * have not been specified a timeout value, including default function name.
   */
  milliseconds GetWaitTimeoutDefault() { return cb_default_wait_timeout_; }

  /*
   * Set default wait timeout for a callback function.
   * The default timeout is valid for all callback function names that
   * have not been specified a timeout value, including default function name.
   */
  void SetWaitTimeoutDefault(milliseconds timeout) {
    cb_default_wait_timeout_ = timeout;
  }

 private:
  /*
   * A utility class to store semaphore and data for a callback name.
   */
  class CallbackLock {
   public:
    CallbackLock(VtsHalHidlTargetCallbackBase& parent, const string& name)
        : wait_count_(0),
          parent_(parent),
          timeout_(milliseconds(-1)),
          name_(name) {}

    /*
     * Wait for represented callback function.
     * Timeout defaults to -1 milliseconds. Negative timeout means use to
     * use the time out set for the callback or default callback wait time out.
     */
    WaitForCallbackResult WaitForCallback(
        milliseconds timeout = milliseconds(-1),
        bool no_wait_blocking = false) {
      return Wait(timeout, no_wait_blocking);
    }

    /*
     * Wait for represented callback function.
     * Timeout defaults to -1 milliseconds. Negative timeout means use to
     * use the time out set for the callback or default callback wait time out.
     */
    WaitForCallbackResult WaitForCallback(bool no_wait_blocking) {
      return Wait(milliseconds(-1), no_wait_blocking);
    }

    /* Notify from represented callback function. */
    void NotifyFromCallback() {
      unique_lock<mutex> lock(wait_mtx_);
      Notify();
    }

    /* Notify from represented callback function with data. */
    void NotifyFromCallback(const CallbackArgsTemplateClass& data) {
      unique_lock<mutex> wait_lock(wait_mtx_);
      arg_data_.push(make_shared<CallbackArgsTemplateClass>(data));
      Notify();
    }

    /* Set wait timeout for represented callback function. */
    void SetWaitTimeout(milliseconds timeout) { timeout_ = timeout; }

    /* Get wait timeout for represented callback function. */
    milliseconds GetWaitTimeout() {
      if (timeout_ < milliseconds(0)) {
        return parent_.GetWaitTimeoutDefault();
      }
      return timeout_;
    }

   private:
    /*
     * Wait for represented callback function in a test.
     * Returns a WaitForCallbackResult object containing wait results.
     * Timeout defaults to -1 milliseconds. Negative timeout means use to
     * use the time out set for the callback or default callback wait time out.
     */
    WaitForCallbackResult Wait(milliseconds timeout, bool no_wait_blocking) {
      unique_lock<mutex> lock(wait_mtx_);
      WaitForCallbackResult res;
      res.name = name_;
      if (!no_wait_blocking) {
        if (timeout < milliseconds(0)) {
          timeout = GetWaitTimeout();
        }
        auto expiration = system_clock::now() + timeout;
        while (wait_count_ == 0) {
          auto status = wait_cv_.wait_until(lock, expiration);
          if (status == cv_status::timeout) {
            cerr << "Timed out waiting for callback" << endl;
            return res;
          }
        }
      } else if (!wait_count_) {
        return res;
      }

      wait_count_--;
      res.no_timeout = true;
      if (!arg_data_.empty()) {
        res.args = arg_data_.front();
        arg_data_.pop();
      }
      return res;
    }

    /* Notify from represented callback function. */
    void Notify() {
      wait_count_++;
      wait_cv_.notify_one();
    }

    // Mutex for protecting operations on wait count and conditional variable
    mutex wait_mtx_;
    // Conditional variable for callback wait and notify
    condition_variable wait_cv_;
    // Count for callback conditional variable
    unsigned int wait_count_;
    // A queue of callback arg data
    queue<shared_ptr<CallbackArgsTemplateClass>> arg_data_;
    // Pointer to parent class
    VtsHalHidlTargetCallbackBase& parent_;
    // Wait time out
    milliseconds timeout_;
    // Name of the represented callback function
    string name_;
  };

  /*
   * Get CallbackLock object using callback function name.
   * If callback_function_name is not provided, a default name will be used.
   * If callback_function_name does not exists in map yet, a new CallbackLock
   * object will be created.
   * If auto_clear is true, the old CallbackLock will be deleted.
   */
  CallbackLock* GetCallbackLock(const string& callback_function_name,
                                bool auto_clear = false) {
    unique_lock<mutex> lock(cb_lock_map_mtx_);
    auto found = cb_lock_map_.find(callback_function_name);
    if (found == cb_lock_map_.end()) {
      CallbackLock* result = new CallbackLock(*this, callback_function_name);
      cb_lock_map_.insert({callback_function_name, result});
      return result;
    } else {
      if (auto_clear) {
        delete (found->second);
        found->second = new CallbackLock(*this, callback_function_name);
      }
      return found->second;
    }
  }

  /*
   * Get wait timeout for a list of function names.
   * If timeout_any is not negative, start_time + timeout_any will be returned.
   * Otherwise, the shortest timeout from the list will be returned.
   */
  system_clock::time_point GetWaitAnyTimeout(
      const vector<string>& callback_function_names,
      system_clock::time_point start_time, milliseconds timeout_any) {
    if (timeout_any >= milliseconds(0)) {
      return start_time + timeout_any;
    }

    auto locks = GetWaitAnyCallbackLocks(callback_function_names);

    auto timeout_min = system_clock::duration::max();
    for (auto lock : locks) {
      auto timeout = lock->GetWaitTimeout();
      if (timeout < timeout_min) {
        timeout_min = timeout;
      }
    }

    return start_time + timeout_min;
  }

  /*
   * Get a list of CallbackLock pointers from provided function name list.
   */
  vector<CallbackLock*> GetWaitAnyCallbackLocks(
      const vector<string>& callback_function_names) {
    vector<CallbackLock*> res;
    if (callback_function_names.empty()) {
      for (auto const& it : cb_lock_map_) {
        res.push_back(it.second);
      }
    } else {
      for (auto const& name : callback_function_names) {
        res.push_back(GetCallbackLock(name));
      }
    }
    return res;
  }

  /*
   * Peek into the list of callback locks to check whether any of the
   * callback functions has been called.
   */
  WaitForCallbackResult PeekCallbackLocks(
      const vector<string>& callback_function_names) {
    auto locks = GetWaitAnyCallbackLocks(callback_function_names);
    for (auto lock : locks) {
      auto test = lock->WaitForCallback(true);
      if (test.no_timeout) {
        return test;
      }
    }
    WaitForCallbackResult res;
    return res;
  }

  // A map of function name and CallbackLock object pointers
  unordered_map<string, CallbackLock*> cb_lock_map_;
  // Mutex for protecting operations on lock map
  mutex cb_lock_map_mtx_;
  // Mutex for protecting waiting any callback
  mutex cb_wait_any_mtx_;
  // Default wait timeout
  milliseconds cb_default_wait_timeout_;
  // Conditional variable for any callback notify
  condition_variable cb_wait_any_cv_;
};

}  // namespace testing

#endif  // __VTS_HAL_HIDL_TARGET_CALLBACK_BASE_H
