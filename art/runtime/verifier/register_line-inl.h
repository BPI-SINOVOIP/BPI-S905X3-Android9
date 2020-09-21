/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef ART_RUNTIME_VERIFIER_REGISTER_LINE_INL_H_
#define ART_RUNTIME_VERIFIER_REGISTER_LINE_INL_H_

#include "register_line.h"

#include "base/logging.h"  // For VLOG.
#include "debug_print.h"
#include "method_verifier.h"
#include "reg_type_cache-inl.h"

namespace art {
namespace verifier {

// Should we dump a warning on failures to verify balanced locking? That would be an indication to
// developers that their code will be slow.
static constexpr bool kDumpLockFailures = true;

inline const RegType& RegisterLine::GetRegisterType(MethodVerifier* verifier, uint32_t vsrc) const {
  // The register index was validated during the static pass, so we don't need to check it here.
  DCHECK_LT(vsrc, num_regs_);
  return verifier->GetRegTypeCache()->GetFromId(line_[vsrc]);
}

template <LockOp kLockOp>
inline bool RegisterLine::SetRegisterType(MethodVerifier* verifier, uint32_t vdst,
                                          const RegType& new_type) {
  DCHECK_LT(vdst, num_regs_);
  if (new_type.IsLowHalf() || new_type.IsHighHalf()) {
    verifier->Fail(VERIFY_ERROR_BAD_CLASS_HARD) << "Expected category1 register type not '"
        << new_type << "'";
    return false;
  } else {
    // Note: previously we failed when asked to set a conflict. However, conflicts are OK as long
    //       as they are not accessed, and our backends can handle this nowadays.
    line_[vdst] = new_type.GetId();
  }
  switch (kLockOp) {
    case LockOp::kClear:
      // Clear the monitor entry bits for this register.
      ClearAllRegToLockDepths(vdst);
      break;
    case LockOp::kKeep:
      // Should only be doing this with reference types.
      DCHECK(new_type.IsReferenceTypes());
      break;
  }
  return true;
}

inline bool RegisterLine::SetRegisterTypeWide(MethodVerifier* verifier, uint32_t vdst,
                                              const RegType& new_type1,
                                              const RegType& new_type2) {
  DCHECK_LT(vdst + 1, num_regs_);
  if (!new_type1.CheckWidePair(new_type2)) {
    verifier->Fail(VERIFY_ERROR_BAD_CLASS_SOFT) << "Invalid wide pair '"
        << new_type1 << "' '" << new_type2 << "'";
    return false;
  } else {
    line_[vdst] = new_type1.GetId();
    line_[vdst + 1] = new_type2.GetId();
  }
  // Clear the monitor entry bits for this register.
  ClearAllRegToLockDepths(vdst);
  ClearAllRegToLockDepths(vdst + 1);
  return true;
}

inline void RegisterLine::SetResultTypeToUnknown(MethodVerifier* verifier) {
  result_[0] = verifier->GetRegTypeCache()->Undefined().GetId();
  result_[1] = result_[0];
}

inline void RegisterLine::SetResultRegisterType(MethodVerifier* verifier, const RegType& new_type) {
  DCHECK(!new_type.IsLowHalf());
  DCHECK(!new_type.IsHighHalf());
  result_[0] = new_type.GetId();
  result_[1] = verifier->GetRegTypeCache()->Undefined().GetId();
}

inline void RegisterLine::SetResultRegisterTypeWide(const RegType& new_type1,
                                                    const RegType& new_type2) {
  DCHECK(new_type1.CheckWidePair(new_type2));
  result_[0] = new_type1.GetId();
  result_[1] = new_type2.GetId();
}

inline void RegisterLine::CopyRegister1(MethodVerifier* verifier, uint32_t vdst, uint32_t vsrc,
                                 TypeCategory cat) {
  DCHECK(cat == kTypeCategory1nr || cat == kTypeCategoryRef);
  const RegType& type = GetRegisterType(verifier, vsrc);
  if (!SetRegisterType<LockOp::kClear>(verifier, vdst, type)) {
    return;
  }
  if (!type.IsConflict() &&                                  // Allow conflicts to be copied around.
      ((cat == kTypeCategory1nr && !type.IsCategory1Types()) ||
       (cat == kTypeCategoryRef && !type.IsReferenceTypes()))) {
    verifier->Fail(VERIFY_ERROR_BAD_CLASS_HARD) << "copy1 v" << vdst << "<-v" << vsrc << " type=" << type
                                                 << " cat=" << static_cast<int>(cat);
  } else if (cat == kTypeCategoryRef) {
    CopyRegToLockDepth(vdst, vsrc);
  }
}

inline void RegisterLine::CopyRegister2(MethodVerifier* verifier, uint32_t vdst, uint32_t vsrc) {
  const RegType& type_l = GetRegisterType(verifier, vsrc);
  const RegType& type_h = GetRegisterType(verifier, vsrc + 1);

  if (!type_l.CheckWidePair(type_h)) {
    verifier->Fail(VERIFY_ERROR_BAD_CLASS_HARD) << "copy2 v" << vdst << "<-v" << vsrc
                                                 << " type=" << type_l << "/" << type_h;
  } else {
    SetRegisterTypeWide(verifier, vdst, type_l, type_h);
  }
}

inline bool RegisterLine::VerifyRegisterType(MethodVerifier* verifier, uint32_t vsrc,
                                             const RegType& check_type) {
  // Verify the src register type against the check type refining the type of the register
  const RegType& src_type = GetRegisterType(verifier, vsrc);
  if (UNLIKELY(!check_type.IsAssignableFrom(src_type, verifier))) {
    enum VerifyError fail_type;
    if (!check_type.IsNonZeroReferenceTypes() || !src_type.IsNonZeroReferenceTypes()) {
      // Hard fail if one of the types is primitive, since they are concretely known.
      fail_type = VERIFY_ERROR_BAD_CLASS_HARD;
    } else if (check_type.IsUninitializedTypes() || src_type.IsUninitializedTypes()) {
      // Hard fail for uninitialized types, which don't match anything but themselves.
      fail_type = VERIFY_ERROR_BAD_CLASS_HARD;
    } else if (check_type.IsUnresolvedTypes() || src_type.IsUnresolvedTypes()) {
      fail_type = VERIFY_ERROR_NO_CLASS;
    } else {
      fail_type = VERIFY_ERROR_BAD_CLASS_SOFT;
    }
    verifier->Fail(fail_type) << "register v" << vsrc << " has type "
                               << src_type << " but expected " << check_type;
    if (check_type.IsNonZeroReferenceTypes() &&
        !check_type.IsUnresolvedTypes() &&
        check_type.HasClass() &&
        src_type.IsNonZeroReferenceTypes() &&
        !src_type.IsUnresolvedTypes() &&
        src_type.HasClass()) {
      DumpB77342775DebugData(check_type.GetClass(), src_type.GetClass());
    }
    return false;
  }
  if (check_type.IsLowHalf()) {
    const RegType& src_type_h = GetRegisterType(verifier, vsrc + 1);
    if (UNLIKELY(!src_type.CheckWidePair(src_type_h))) {
      verifier->Fail(VERIFY_ERROR_BAD_CLASS_HARD) << "wide register v" << vsrc << " has type "
                                                   << src_type << "/" << src_type_h;
      return false;
    }
  }
  // The register at vsrc has a defined type, we know the lower-upper-bound, but this is less
  // precise than the subtype in vsrc so leave it for reference types. For primitive types
  // if they are a defined type then they are as precise as we can get, however, for constant
  // types we may wish to refine them. Unfortunately constant propagation has rendered this useless.
  return true;
}

inline void RegisterLine::VerifyMonitorStackEmpty(MethodVerifier* verifier) const {
  if (MonitorStackDepth() != 0) {
    verifier->Fail(VERIFY_ERROR_LOCKING);
    if (kDumpLockFailures) {
      VLOG(verifier) << "expected empty monitor stack in "
                     << verifier->GetMethodReference().PrettyMethod();
    }
  }
}

inline size_t RegisterLine::ComputeSize(size_t num_regs) {
  return OFFSETOF_MEMBER(RegisterLine, line_) + num_regs * sizeof(uint16_t);
}

inline RegisterLine* RegisterLine::Create(size_t num_regs, MethodVerifier* verifier) {
  void* memory = verifier->GetScopedAllocator().Alloc(ComputeSize(num_regs));
  return new (memory) RegisterLine(num_regs, verifier);
}

inline RegisterLine::RegisterLine(size_t num_regs, MethodVerifier* verifier)
    : num_regs_(num_regs),
      monitors_(verifier->GetScopedAllocator().Adapter(kArenaAllocVerifier)),
      reg_to_lock_depths_(std::less<uint32_t>(),
                          verifier->GetScopedAllocator().Adapter(kArenaAllocVerifier)),
      this_initialized_(false) {
  std::uninitialized_fill_n(line_, num_regs_, 0u);
  SetResultTypeToUnknown(verifier);
}

inline void RegisterLine::ClearRegToLockDepth(size_t reg, size_t depth) {
  CHECK_LT(depth, 32u);
  DCHECK(IsSetLockDepth(reg, depth));
  auto it = reg_to_lock_depths_.find(reg);
  DCHECK(it != reg_to_lock_depths_.end());
  uint32_t depths = it->second ^ (1 << depth);
  if (depths != 0) {
    it->second = depths;
  } else {
    reg_to_lock_depths_.erase(it);
  }
  // Need to unlock every register at the same lock depth. These are aliased locks.
  uint32_t mask = 1 << depth;
  for (auto& pair : reg_to_lock_depths_) {
    if ((pair.second & mask) != 0) {
      VLOG(verifier) << "Also unlocking " << pair.first;
      pair.second ^= mask;
    }
  }
}

inline void RegisterLineArenaDelete::operator()(RegisterLine* ptr) const {
  if (ptr != nullptr) {
    ptr->~RegisterLine();
    ProtectMemory(ptr, RegisterLine::ComputeSize(ptr->NumRegs()));
  }
}

}  // namespace verifier
}  // namespace art

#endif  // ART_RUNTIME_VERIFIER_REGISTER_LINE_INL_H_
