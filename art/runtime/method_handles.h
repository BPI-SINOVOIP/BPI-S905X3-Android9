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

#ifndef ART_RUNTIME_METHOD_HANDLES_H_
#define ART_RUNTIME_METHOD_HANDLES_H_

#include <ostream>

#include "dex/dex_instruction.h"
#include "handle.h"
#include "interpreter/shadow_frame.h"
#include "jvalue.h"
#include "mirror/class.h"

namespace art {

namespace mirror {
class MethodHandle;
class MethodType;
}  // namespace mirror

// Returns true if there is a possible conversion from |from| to |to|
// for a MethodHandle parameter.
bool IsParameterTypeConvertible(ObjPtr<mirror::Class> from,
                                ObjPtr<mirror::Class> to);

// Returns true if there is a possible conversion from |from| to |to|
// for the return type of a MethodHandle.
bool IsReturnTypeConvertible(ObjPtr<mirror::Class> from,
                             ObjPtr<mirror::Class> to);

// Performs a conversion from type |from| to a distinct type |to| as
// part of conversion of |caller_type| to |callee_type|. The value to
// be converted is in |value|. Returns true on success and updates
// |value| with the converted value, false otherwise.
bool ConvertJValueCommon(Handle<mirror::MethodType> callsite_type,
                         Handle<mirror::MethodType> callee_type,
                         ObjPtr<mirror::Class> from,
                         ObjPtr<mirror::Class> to,
                         JValue* value)
    REQUIRES_SHARED(Locks::mutator_lock_);

// Converts the value of the argument at position |index| from type
// expected by |callee_type| to type used by |callsite_type|. |value|
// represents the value to be converted. Returns true on success and
// updates |value|, false otherwise.
ALWAYS_INLINE bool ConvertArgumentValue(Handle<mirror::MethodType> callsite_type,
                                        Handle<mirror::MethodType> callee_type,
                                        int index,
                                        JValue* value)
    REQUIRES_SHARED(Locks::mutator_lock_);

// Converts the return value from return type yielded by
// |callee_type| to the return type yielded by
// |callsite_type|. |value| represents the value to be
// converted. Returns true on success and updates |value|, false
// otherwise.
ALWAYS_INLINE bool ConvertReturnValue(Handle<mirror::MethodType> callsite_type,
                                      Handle<mirror::MethodType> callee_type,
                                      JValue* value)
    REQUIRES_SHARED(Locks::mutator_lock_);

// Perform argument conversions between |callsite_type| (the type of the
// incoming arguments) and |callee_type| (the type of the method being
// invoked). These include widening and narrowing conversions as well as
// boxing and unboxing. Returns true on success, on false on failure. A
// pending exception will always be set on failure.
//
// The values to be converted are read from an input source (of type G)
// that provides three methods :
//
// class G {
//   // Used to read the next boolean/short/int or float value from the
//   // source.
//   uint32_t Get();
//
//   // Used to the read the next reference value from the source.
//   ObjPtr<mirror::Object> GetReference();
//
//   // Used to read the next double or long value from the source.
//   int64_t GetLong();
// }
//
// After conversion, the values are written to an output sink (of type S)
// that provides three methods :
//
// class S {
//   void Set(uint32_t);
//   void SetReference(ObjPtr<mirror::Object>)
//   void SetLong(int64_t);
// }
//
// The semantics and usage of the Set methods are analagous to the getter
// class.
//
// This method is instantiated in three different scenarions :
// - <S = ShadowFrameSetter, G = ShadowFrameGetter> : copying from shadow
//   frame to shadow frame, used in a regular polymorphic non-exact invoke.
// - <S = EmulatedShadowFrameAccessor, G = ShadowFrameGetter> : entering into
//   a transformer method from a polymorphic invoke.
// - <S = ShadowFrameStter, G = EmulatedStackFrameAccessor> : entering into
//   a regular poly morphic invoke from a transformer method.
//
// TODO(narayan): If we find that the instantiations of this function take
// up too much space, we can make G / S abstract base classes that are
// overridden by concrete classes.
template <typename G, typename S>
bool PerformConversions(Thread* self,
                        Handle<mirror::MethodType> callsite_type,
                        Handle<mirror::MethodType> callee_type,
                        G* getter,
                        S* setter,
                        int32_t start_index,
                        int32_t end_index) REQUIRES_SHARED(Locks::mutator_lock_);

// A convenience class that allows for iteration through a list of
// input argument registers. This is used to iterate over input
// arguments while performing standard argument conversions.
class ShadowFrameGetter {
 public:
  ShadowFrameGetter(const ShadowFrame& shadow_frame,
                    const InstructionOperands* const operands,
                    size_t operand_index = 0u)
      : shadow_frame_(shadow_frame), operands_(operands), operand_index_(operand_index)  {}

  ALWAYS_INLINE uint32_t Get() REQUIRES_SHARED(Locks::mutator_lock_) {
    return shadow_frame_.GetVReg(Next());
  }

  ALWAYS_INLINE int64_t GetLong() REQUIRES_SHARED(Locks::mutator_lock_) {
    return shadow_frame_.GetVRegLong(NextLong());
  }

  ALWAYS_INLINE ObjPtr<mirror::Object> GetReference() REQUIRES_SHARED(Locks::mutator_lock_) {
    return shadow_frame_.GetVRegReference(Next());
  }

 private:
  uint32_t Next() {
    const uint32_t next = operands_->GetOperand(operand_index_);
    operand_index_ += 1;
    return next;
  }

  uint32_t NextLong() {
    const uint32_t next = operands_->GetOperand(operand_index_);
    operand_index_ += 2;
    return next;
  }

  const ShadowFrame& shadow_frame_;
  const InstructionOperands* const operands_;  // the set of register operands to read
  size_t operand_index_;  // the next register operand to read from frame
};

// A convenience class that allows values to be written to a given shadow frame,
// starting at location |first_dst_reg|.
class ShadowFrameSetter {
 public:
  ShadowFrameSetter(ShadowFrame* shadow_frame, size_t first_dst_reg)
      : shadow_frame_(shadow_frame), arg_index_(first_dst_reg) {}

  ALWAYS_INLINE void Set(uint32_t value) REQUIRES_SHARED(Locks::mutator_lock_) {
    shadow_frame_->SetVReg(arg_index_++, value);
  }

  ALWAYS_INLINE void SetReference(ObjPtr<mirror::Object> value)
      REQUIRES_SHARED(Locks::mutator_lock_) {
    shadow_frame_->SetVRegReference(arg_index_++, value.Ptr());
  }

  ALWAYS_INLINE void SetLong(int64_t value) REQUIRES_SHARED(Locks::mutator_lock_) {
    shadow_frame_->SetVRegLong(arg_index_, value);
    arg_index_ += 2;
  }

 private:
  ShadowFrame* shadow_frame_;
  size_t arg_index_;
};

bool MethodHandleInvoke(Thread* self,
                        ShadowFrame& shadow_frame,
                        Handle<mirror::MethodHandle> method_handle,
                        Handle<mirror::MethodType> callsite_type,
                        const InstructionOperands* const args,
                        JValue* result)
    REQUIRES_SHARED(Locks::mutator_lock_);

bool MethodHandleInvokeExact(Thread* self,
                             ShadowFrame& shadow_frame,
                             Handle<mirror::MethodHandle> method_handle,
                             Handle<mirror::MethodType> callsite_type,
                             const InstructionOperands* const args,
                             JValue* result)
    REQUIRES_SHARED(Locks::mutator_lock_);

}  // namespace art

#endif  // ART_RUNTIME_METHOD_HANDLES_H_
