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

#ifndef ART_RUNTIME_VERIFIER_REG_TYPE_INL_H_
#define ART_RUNTIME_VERIFIER_REG_TYPE_INL_H_

#include "reg_type.h"

#include "base/casts.h"
#include "base/scoped_arena_allocator.h"
#include "method_verifier.h"
#include "mirror/class.h"
#include "verifier_deps.h"

namespace art {
namespace verifier {

inline bool RegType::CanAccess(const RegType& other) const {
  DCHECK(IsReferenceTypes());
  DCHECK(!IsNull());
  if (Equals(other)) {
    return true;  // Trivial accessibility.
  } else {
    bool this_unresolved = IsUnresolvedTypes();
    bool other_unresolved = other.IsUnresolvedTypes();
    if (!this_unresolved && !other_unresolved) {
      return GetClass()->CanAccess(other.GetClass());
    } else if (!other_unresolved) {
      return other.GetClass()->IsPublic();  // Be conservative, only allow if other is public.
    } else {
      return false;  // More complicated test not possible on unresolved types, be conservative.
    }
  }
}

inline bool RegType::CanAccessMember(ObjPtr<mirror::Class> klass, uint32_t access_flags) const {
  DCHECK(IsReferenceTypes());
  if (IsNull()) {
    return true;
  }
  if (!IsUnresolvedTypes()) {
    return GetClass()->CanAccessMember(klass, access_flags);
  } else {
    return false;  // More complicated test not possible on unresolved types, be conservative.
  }
}

inline bool RegType::IsConstantBoolean() const {
  if (!IsConstant()) {
    return false;
  } else {
    const ConstantType* const_val = down_cast<const ConstantType*>(this);
    return const_val->ConstantValue() >= 0 && const_val->ConstantValue() <= 1;
  }
}

inline bool RegType::AssignableFrom(const RegType& lhs,
                                    const RegType& rhs,
                                    bool strict,
                                    MethodVerifier* verifier) {
  if (lhs.Equals(rhs)) {
    return true;
  } else {
    switch (lhs.GetAssignmentType()) {
      case AssignmentType::kBoolean:
        return rhs.IsBooleanTypes();
      case AssignmentType::kByte:
        return rhs.IsByteTypes();
      case AssignmentType::kShort:
        return rhs.IsShortTypes();
      case AssignmentType::kChar:
        return rhs.IsCharTypes();
      case AssignmentType::kInteger:
        return rhs.IsIntegralTypes();
      case AssignmentType::kFloat:
        return rhs.IsFloatTypes();
      case AssignmentType::kLongLo:
        return rhs.IsLongTypes();
      case AssignmentType::kDoubleLo:
        return rhs.IsDoubleTypes();
      case AssignmentType::kConflict:
        LOG(WARNING) << "RegType::AssignableFrom lhs is Conflict!";
        return false;
      case AssignmentType::kReference:
        if (rhs.IsZeroOrNull()) {
          return true;  // All reference types can be assigned null.
        } else if (!rhs.IsReferenceTypes()) {
          return false;  // Expect rhs to be a reference type.
        } else if (lhs.IsUninitializedTypes() || rhs.IsUninitializedTypes()) {
          // Uninitialized types are only allowed to be assigned to themselves.
          // TODO: Once we have a proper "reference" super type, this needs to be extended.
          return false;
        } else if (lhs.IsJavaLangObject()) {
          return true;  // All reference types can be assigned to Object.
        } else if (!strict && !lhs.IsUnresolvedTypes() && lhs.GetClass()->IsInterface()) {
          // If we're not strict allow assignment to any interface, see comment in ClassJoin.
          return true;
        } else if (lhs.IsJavaLangObjectArray()) {
          return rhs.IsObjectArrayTypes();  // All reference arrays may be assigned to Object[]
        } else if (lhs.HasClass() && rhs.HasClass()) {
          // Test assignability from the Class point-of-view.
          bool result = lhs.GetClass()->IsAssignableFrom(rhs.GetClass());
          // Record assignability dependency. The `verifier` is null during unit tests and
          // VerifiedMethod::GenerateSafeCastSet.
          if (verifier != nullptr) {
            VerifierDeps::MaybeRecordAssignability(
                verifier->GetDexFile(), lhs.GetClass(), rhs.GetClass(), strict, result);
          }
          return result;
        } else {
          // Unresolved types are only assignable for null and equality.
          // Null cannot be the left-hand side.
          return false;
        }
      case AssignmentType::kNotAssignable:
        break;
    }
    LOG(FATAL) << "Unexpected register type in IsAssignableFrom: '"
               << lhs << "' := '" << rhs << "'";
    UNREACHABLE();
  }
}

inline bool RegType::IsAssignableFrom(const RegType& src, MethodVerifier* verifier) const {
  return AssignableFrom(*this, src, false, verifier);
}

inline bool RegType::IsStrictlyAssignableFrom(const RegType& src, MethodVerifier* verifier) const {
  return AssignableFrom(*this, src, true, verifier);
}

inline const DoubleHiType* DoubleHiType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline const DoubleLoType* DoubleLoType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline const LongHiType* LongHiType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline const LongLoType* LongLoType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline const FloatType* FloatType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline const CharType* CharType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline const ShortType* ShortType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline const ByteType* ByteType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}


inline const IntegerType* IntegerType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline const BooleanType* BooleanType::GetInstance() {
  DCHECK(BooleanType::instance_ != nullptr);
  return BooleanType::instance_;
}

inline const ConflictType* ConflictType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline const UndefinedType* UndefinedType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline const NullType* NullType::GetInstance() {
  DCHECK(instance_ != nullptr);
  return instance_;
}

inline void* RegType::operator new(size_t size, ScopedArenaAllocator* allocator) {
  return allocator->Alloc(size, kArenaAllocMisc);
}

}  // namespace verifier
}  // namespace art

#endif  // ART_RUNTIME_VERIFIER_REG_TYPE_INL_H_
