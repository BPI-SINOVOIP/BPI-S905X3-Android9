/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ART_RUNTIME_INTERPRETER_INTERPRETER_H_
#define ART_RUNTIME_INTERPRETER_INTERPRETER_H_

#include "base/mutex.h"
#include "dex/dex_file.h"
#include "obj_ptr.h"

namespace art {
namespace mirror {
class Object;
}  // namespace mirror

class ArtMethod;
class CodeItemDataAccessor;
union JValue;
class ShadowFrame;
class Thread;
enum class DeoptimizationMethodType;

namespace interpreter {

// Called by ArtMethod::Invoke, shadow frames arguments are taken from the args array.
// The optional stay_in_interpreter parameter (false by default) can be used by clients to
// explicitly force interpretation in the remaining path that implements method invocation.
extern void EnterInterpreterFromInvoke(Thread* self, ArtMethod* method,
                                       ObjPtr<mirror::Object> receiver,
                                       uint32_t* args,
                                       JValue* result,
                                       bool stay_in_interpreter = false)
    REQUIRES_SHARED(Locks::mutator_lock_);

// 'from_code' denotes whether the deoptimization was explicitly triggered by compiled code.
extern void EnterInterpreterFromDeoptimize(Thread* self,
                                           ShadowFrame* shadow_frame,
                                           JValue* ret_val,
                                           bool from_code,
                                           DeoptimizationMethodType method_type)
    REQUIRES_SHARED(Locks::mutator_lock_);

extern JValue EnterInterpreterFromEntryPoint(Thread* self,
                                             const CodeItemDataAccessor& accessor,
                                             ShadowFrame* shadow_frame)
    REQUIRES_SHARED(Locks::mutator_lock_);

void ArtInterpreterToInterpreterBridge(Thread* self,
                                       const CodeItemDataAccessor& accessor,
                                       ShadowFrame* shadow_frame,
                                       JValue* result)
    REQUIRES_SHARED(Locks::mutator_lock_);

// One-time sanity check.
void CheckInterpreterAsmConstants();

void InitInterpreterTls(Thread* self);

}  // namespace interpreter

}  // namespace art

#endif  // ART_RUNTIME_INTERPRETER_INTERPRETER_H_
