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

#ifndef ART_RUNTIME_METHOD_HANDLES_INL_H_
#define ART_RUNTIME_METHOD_HANDLES_INL_H_

#include "method_handles.h"

#include "common_throws.h"
#include "dex/dex_instruction.h"
#include "interpreter/interpreter_common.h"
#include "jvalue.h"
#include "mirror/class.h"
#include "mirror/method_type.h"
#include "mirror/object.h"
#include "reflection.h"
#include "stack.h"

namespace art {

inline bool ConvertArgumentValue(Handle<mirror::MethodType> callsite_type,
                                 Handle<mirror::MethodType> callee_type,
                                 ObjPtr<mirror::Class> from_class,
                                 ObjPtr<mirror::Class> to_class,
                                 JValue* value) REQUIRES_SHARED(Locks::mutator_lock_) {
  if (from_class == to_class) {
    return true;
  }

  // |value| may contain a bare heap pointer which is generally
  // |unsafe. ConvertJValueCommon() saves |value|, |from_class|, and
  // |to_class| to Handles where necessary to avoid issues if the heap
  // changes.
  if (ConvertJValueCommon(callsite_type, callee_type, from_class, to_class, value)) {
    DCHECK(!Thread::Current()->IsExceptionPending());
    return true;
  } else {
    DCHECK(Thread::Current()->IsExceptionPending());
    value->SetJ(0);
    return false;
  }
}

inline bool ConvertArgumentValue(Handle<mirror::MethodType> callsite_type,
                                 Handle<mirror::MethodType> callee_type,
                                 int index,
                                 JValue* value) REQUIRES_SHARED(Locks::mutator_lock_) {
  return ConvertArgumentValue(callsite_type,
                              callee_type,
                              callsite_type->GetPTypes()->GetWithoutChecks(index),
                              callee_type->GetPTypes()->GetWithoutChecks(index),
                              value);
}

inline bool ConvertReturnValue(Handle<mirror::MethodType> callsite_type,
                               Handle<mirror::MethodType> callee_type,
                               JValue* value)  REQUIRES_SHARED(Locks::mutator_lock_) {
  ObjPtr<mirror::Class> from_class(callee_type->GetRType());
  ObjPtr<mirror::Class> to_class(callsite_type->GetRType());
  if (to_class->GetPrimitiveType() == Primitive::kPrimVoid || from_class == to_class) {
    return true;
  }

  // |value| may contain a bare heap pointer which is generally
  // unsafe. ConvertJValueCommon() saves |value|, |from_class|, and
  // |to_class| to Handles where necessary to avoid issues if the heap
  // changes.
  if (ConvertJValueCommon(callsite_type, callee_type, from_class, to_class, value)) {
    DCHECK(!Thread::Current()->IsExceptionPending());
    return true;
  } else {
    DCHECK(Thread::Current()->IsExceptionPending());
    value->SetJ(0);
    return false;
  }
}

template <typename G, typename S>
bool PerformConversions(Thread* self,
                        Handle<mirror::MethodType> callsite_type,
                        Handle<mirror::MethodType> callee_type,
                        G* getter,
                        S* setter,
                        int32_t start_index,
                        int32_t end_index) REQUIRES_SHARED(Locks::mutator_lock_) {
  StackHandleScope<2> hs(self);
  Handle<mirror::ObjectArray<mirror::Class>> from_types(hs.NewHandle(callsite_type->GetPTypes()));
  Handle<mirror::ObjectArray<mirror::Class>> to_types(hs.NewHandle(callee_type->GetPTypes()));

  for (int32_t i = start_index; i < end_index; ++i) {
    ObjPtr<mirror::Class> from(from_types->GetWithoutChecks(i));
    ObjPtr<mirror::Class> to(to_types->GetWithoutChecks(i - start_index));
    const Primitive::Type from_type = from->GetPrimitiveType();
    const Primitive::Type to_type = to->GetPrimitiveType();
    if (from == to) {
      // Easy case - the types are identical. Nothing left to do except to pass
      // the arguments along verbatim.
      if (Primitive::Is64BitType(from_type)) {
        setter->SetLong(getter->GetLong());
      } else if (from_type == Primitive::kPrimNot) {
        setter->SetReference(getter->GetReference());
      } else {
        setter->Set(getter->Get());
      }
    } else {
      JValue value;
      if (Primitive::Is64BitType(from_type)) {
        value.SetJ(getter->GetLong());
      } else if (from_type == Primitive::kPrimNot) {
        value.SetL(getter->GetReference());
      } else {
        value.SetI(getter->Get());
      }
      // Caveat emptor - ObjPtr's not guaranteed valid after this call.
      if (!ConvertArgumentValue(callsite_type, callee_type, from, to, &value)) {
        DCHECK(self->IsExceptionPending());
        return false;
      }
      if (Primitive::Is64BitType(to_type)) {
        setter->SetLong(value.GetJ());
      } else if (to_type == Primitive::kPrimNot) {
        setter->SetReference(value.GetL());
      } else {
        setter->Set(value.GetI());
      }
    }
  }
  return true;
}

template <typename G, typename S>
bool PerformConversions(Thread* self,
                        Handle<mirror::MethodType> callsite_type,
                        Handle<mirror::MethodType> callee_type,
                        G* getter,
                        S* setter,
                        int32_t num_conversions)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  return PerformConversions(self, callsite_type, callee_type, getter, setter, 0, num_conversions);
}

template <typename G, typename S>
bool PerformConversions(Thread* self,
                        Handle<mirror::MethodType> callsite_type,
                        Handle<mirror::MethodType> callee_type,
                        G* getter,
                        S* setter)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  int32_t num_conversions = callee_type->GetPTypes()->GetLength();
  return PerformConversions(self, callsite_type, callee_type, getter, setter, 0, num_conversions);
}

}  // namespace art

#endif  // ART_RUNTIME_METHOD_HANDLES_INL_H_
