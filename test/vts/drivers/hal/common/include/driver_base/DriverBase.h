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

#ifndef __VTS_SYSFUZZER_COMMON_FUZZER_BASE_H__
#define __VTS_SYSFUZZER_COMMON_FUZZER_BASE_H__

#include "component_loader/DllLoader.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

class DriverBase {
 public:
  DriverBase(int target_class);
  virtual ~DriverBase();

  // Loads a target component where the argument is the file path.
  // Returns true iff successful.
  bool LoadTargetComponent(const char* target_dll_path);

  // Gets the HIDL service.
  // Returns true iff successful.
  virtual bool GetService(bool /*get_stub*/, const char* /*service_name*/) {
    return false;
  };

  // Fuzz tests the loaded component using the provided interface specification.
  // Returns true iff the testing is conducted completely.
  bool Fuzz(vts::ComponentSpecificationMessage* message, void** result);

  // Actual implementation of routines to test a specific function using the
  // provided function interface specification message.
  // Returns true iff the testing is conducted completely.
  virtual bool Fuzz(vts::FunctionSpecificationMessage* /*func_msg*/,
                    void** /*result*/, const string& /*callback_socket_name*/) {
    return false;
  };

  virtual bool CallFunction(
      const vts::FunctionSpecificationMessage& /*func_msg*/,
      const string& /*callback_socket_name*/,
      vts::FunctionSpecificationMessage* /*result_msg*/) {
    return false;
  };

  virtual bool VerifyResults(
      const vts::FunctionSpecificationMessage& /*expected_result_msg*/,
      const vts::FunctionSpecificationMessage& /*actual_result_msg*/) {
    return false;
  };

  virtual bool GetAttribute(vts::FunctionSpecificationMessage* /*func_msg*/,
                            void** /*result*/) {
    return false;
  }

  // Called before calling a target function.
  void FunctionCallBegin();

  // Called after calling a target function. Fills in the code coverage info.
  bool FunctionCallEnd(FunctionSpecificationMessage* msg);

  // Scans all GCDA files under a given dir and adds to the message.
  bool ScanAllGcdaFiles(const string& basepath,
                        FunctionSpecificationMessage* msg);

 protected:
  bool ReadGcdaFile(const string& basepath, const string& filename,
                    FunctionSpecificationMessage* msg);

  // a pointer to a HAL data structure of the loaded component.
  struct hw_device_t* device_;

  // DLL Loader class.
  DllLoader target_loader_;

  // a pointer to the HAL_MODULE_INFO_SYM data structure of the loaded
  // component.
  struct hw_module_t* hmi_;

 private:
  // a pointer to the string which contains the loaded component.
  char* target_dll_path_;

  // function name prefix.
  const char* function_name_prefix_;

  // target class
  const int target_class_;

  // target component file name (without extension)
  char* component_filename_;

  // path to store the gcov output files.
  char* gcov_output_basepath_;
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_SYSFUZZER_COMMON_FUZZER_BASE_H__
