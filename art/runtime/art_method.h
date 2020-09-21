/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef ART_RUNTIME_ART_METHOD_H_
#define ART_RUNTIME_ART_METHOD_H_

#include <cstddef>

#include <android-base/logging.h>

#include "base/array_ref.h"
#include "base/bit_utils.h"
#include "base/casts.h"
#include "base/enums.h"
#include "base/iteration_range.h"
#include "base/macros.h"
#include "base/runtime_debug.h"
#include "dex/code_item_accessors.h"
#include "dex/dex_file.h"
#include "dex/dex_instruction_iterator.h"
#include "dex/modifiers.h"
#include "dex/primitive.h"
#include "gc_root.h"
#include "obj_ptr.h"
#include "offsets.h"
#include "read_barrier_option.h"

namespace art {

template<class T> class Handle;
class ImtConflictTable;
enum InvokeType : uint32_t;
union JValue;
class OatQuickMethodHeader;
class ProfilingInfo;
class ScopedObjectAccessAlreadyRunnable;
class StringPiece;
class ShadowFrame;

namespace mirror {
class Array;
class Class;
class ClassLoader;
class DexCache;
class IfTable;
class Object;
template <typename MirrorType> class ObjectArray;
class PointerArray;
class String;

template <typename T> struct NativeDexCachePair;
using MethodDexCachePair = NativeDexCachePair<ArtMethod>;
using MethodDexCacheType = std::atomic<MethodDexCachePair>;
}  // namespace mirror

class ArtMethod FINAL {
 public:
  // Should the class state be checked on sensitive operations?
  DECLARE_RUNTIME_DEBUG_FLAG(kCheckDeclaringClassState);

  // The runtime dex_method_index is kDexNoIndex. To lower dependencies, we use this
  // constexpr, and ensure that the value is correct in art_method.cc.
  static constexpr uint32_t kRuntimeMethodDexMethodIndex = 0xFFFFFFFF;

  ArtMethod() : access_flags_(0), dex_code_item_offset_(0), dex_method_index_(0),
      method_index_(0), hotness_count_(0) { }

  ArtMethod(ArtMethod* src, PointerSize image_pointer_size) {
    CopyFrom(src, image_pointer_size);
  }

  static ArtMethod* FromReflectedMethod(const ScopedObjectAccessAlreadyRunnable& soa,
                                        jobject jlr_method)
      REQUIRES_SHARED(Locks::mutator_lock_);

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE mirror::Class* GetDeclaringClass() REQUIRES_SHARED(Locks::mutator_lock_);

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE mirror::Class* GetDeclaringClassUnchecked()
      REQUIRES_SHARED(Locks::mutator_lock_);

  mirror::CompressedReference<mirror::Object>* GetDeclaringClassAddressWithoutBarrier() {
    return declaring_class_.AddressWithoutBarrier();
  }

  void SetDeclaringClass(ObjPtr<mirror::Class> new_declaring_class)
      REQUIRES_SHARED(Locks::mutator_lock_);

  bool CASDeclaringClass(mirror::Class* expected_class, mirror::Class* desired_class)
      REQUIRES_SHARED(Locks::mutator_lock_);

  static MemberOffset DeclaringClassOffset() {
    return MemberOffset(OFFSETOF_MEMBER(ArtMethod, declaring_class_));
  }

  // Note: GetAccessFlags acquires the mutator lock in debug mode to check that it is not called for
  // a proxy method.
  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  uint32_t GetAccessFlags() {
    if (kCheckDeclaringClassState) {
      GetAccessFlagsDCheck<kReadBarrierOption>();
    }
    return access_flags_.load(std::memory_order_relaxed);
  }

  // This version should only be called when it's certain there is no
  // concurrency so there is no need to guarantee atomicity. For example,
  // before the method is linked.
  void SetAccessFlags(uint32_t new_access_flags) {
    access_flags_.store(new_access_flags, std::memory_order_relaxed);
  }

  static MemberOffset AccessFlagsOffset() {
    return MemberOffset(OFFSETOF_MEMBER(ArtMethod, access_flags_));
  }

  // Approximate what kind of method call would be used for this method.
  InvokeType GetInvokeType() REQUIRES_SHARED(Locks::mutator_lock_);

  // Returns true if the method is declared public.
  bool IsPublic() {
    return (GetAccessFlags() & kAccPublic) != 0;
  }

  // Returns true if the method is declared private.
  bool IsPrivate() {
    return (GetAccessFlags() & kAccPrivate) != 0;
  }

  // Returns true if the method is declared static.
  bool IsStatic() {
    return (GetAccessFlags() & kAccStatic) != 0;
  }

  // Returns true if the method is a constructor according to access flags.
  bool IsConstructor() {
    return (GetAccessFlags() & kAccConstructor) != 0;
  }

  // Returns true if the method is a class initializer according to access flags.
  bool IsClassInitializer() {
    return IsConstructor() && IsStatic();
  }

  // Returns true if the method is static, private, or a constructor.
  bool IsDirect() {
    return IsDirect(GetAccessFlags());
  }

  static bool IsDirect(uint32_t access_flags) {
    constexpr uint32_t direct = kAccStatic | kAccPrivate | kAccConstructor;
    return (access_flags & direct) != 0;
  }

  // Returns true if the method is declared synchronized.
  bool IsSynchronized() {
    constexpr uint32_t synchonized = kAccSynchronized | kAccDeclaredSynchronized;
    return (GetAccessFlags() & synchonized) != 0;
  }

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  bool IsFinal() {
    return (GetAccessFlags<kReadBarrierOption>() & kAccFinal) != 0;
  }

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  bool IsIntrinsic() {
    return (GetAccessFlags<kReadBarrierOption>() & kAccIntrinsic) != 0;
  }

  ALWAYS_INLINE void SetIntrinsic(uint32_t intrinsic) REQUIRES_SHARED(Locks::mutator_lock_);

  uint32_t GetIntrinsic() {
    static const int kAccFlagsShift = CTZ(kAccIntrinsicBits);
    static_assert(IsPowerOfTwo((kAccIntrinsicBits >> kAccFlagsShift) + 1),
                  "kAccIntrinsicBits are not continuous");
    static_assert((kAccIntrinsic & kAccIntrinsicBits) == 0,
                  "kAccIntrinsic overlaps kAccIntrinsicBits");
    DCHECK(IsIntrinsic());
    return (GetAccessFlags() & kAccIntrinsicBits) >> kAccFlagsShift;
  }

  void SetNotIntrinsic() REQUIRES_SHARED(Locks::mutator_lock_);

  bool IsCopied() {
    static_assert((kAccCopied & (kAccIntrinsic | kAccIntrinsicBits)) == 0,
                  "kAccCopied conflicts with intrinsic modifier");
    const bool copied = (GetAccessFlags() & kAccCopied) != 0;
    // (IsMiranda() || IsDefaultConflicting()) implies copied
    DCHECK(!(IsMiranda() || IsDefaultConflicting()) || copied)
        << "Miranda or default-conflict methods must always be copied.";
    return copied;
  }

  bool IsMiranda() {
    // The kAccMiranda flag value is used with a different meaning for native methods,
    // so we need to check the kAccNative flag as well.
    return (GetAccessFlags() & (kAccNative | kAccMiranda)) == kAccMiranda;
  }

  // Returns true if invoking this method will not throw an AbstractMethodError or
  // IncompatibleClassChangeError.
  bool IsInvokable() {
    return !IsAbstract() && !IsDefaultConflicting();
  }

  bool IsCompilable() {
    if (IsIntrinsic()) {
      // kAccCompileDontBother overlaps with kAccIntrinsicBits.
      return true;
    }
    return (GetAccessFlags() & kAccCompileDontBother) == 0;
  }

  void SetDontCompile() {
    AddAccessFlags(kAccCompileDontBother);
  }

  // A default conflict method is a special sentinel method that stands for a conflict between
  // multiple default methods. It cannot be invoked, throwing an IncompatibleClassChangeError if one
  // attempts to do so.
  bool IsDefaultConflicting() {
    if (IsIntrinsic()) {
      return false;
    }
    return (GetAccessFlags() & kAccDefaultConflict) != 0u;
  }

  // This is set by the class linker.
  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  bool IsDefault() {
    static_assert((kAccDefault & (kAccIntrinsic | kAccIntrinsicBits)) == 0,
                  "kAccDefault conflicts with intrinsic modifier");
    return (GetAccessFlags<kReadBarrierOption>() & kAccDefault) != 0;
  }

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  bool IsObsolete() {
    return (GetAccessFlags<kReadBarrierOption>() & kAccObsoleteMethod) != 0;
  }

  void SetIsObsolete() {
    AddAccessFlags(kAccObsoleteMethod);
  }

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  bool IsNative() {
    return (GetAccessFlags<kReadBarrierOption>() & kAccNative) != 0;
  }

  // Checks to see if the method was annotated with @dalvik.annotation.optimization.FastNative.
  bool IsFastNative() {
    // The presence of the annotation is checked by ClassLinker and recorded in access flags.
    // The kAccFastNative flag value is used with a different meaning for non-native methods,
    // so we need to check the kAccNative flag as well.
    constexpr uint32_t mask = kAccFastNative | kAccNative;
    return (GetAccessFlags() & mask) == mask;
  }

  // Checks to see if the method was annotated with @dalvik.annotation.optimization.CriticalNative.
  bool IsCriticalNative() {
    // The presence of the annotation is checked by ClassLinker and recorded in access flags.
    // The kAccCriticalNative flag value is used with a different meaning for non-native methods,
    // so we need to check the kAccNative flag as well.
    constexpr uint32_t mask = kAccCriticalNative | kAccNative;
    return (GetAccessFlags() & mask) == mask;
  }

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  bool IsAbstract() {
    return (GetAccessFlags<kReadBarrierOption>() & kAccAbstract) != 0;
  }

  bool IsSynthetic() {
    return (GetAccessFlags() & kAccSynthetic) != 0;
  }

  bool IsVarargs() {
    return (GetAccessFlags() & kAccVarargs) != 0;
  }

  bool IsProxyMethod() REQUIRES_SHARED(Locks::mutator_lock_);

  bool IsPolymorphicSignature() REQUIRES_SHARED(Locks::mutator_lock_);

  bool SkipAccessChecks() {
    // The kAccSkipAccessChecks flag value is used with a different meaning for native methods,
    // so we need to check the kAccNative flag as well.
    return (GetAccessFlags() & (kAccSkipAccessChecks | kAccNative)) == kAccSkipAccessChecks;
  }

  void SetSkipAccessChecks() {
    // SkipAccessChecks() is applicable only to non-native methods.
    DCHECK(!IsNative<kWithoutReadBarrier>());
    AddAccessFlags(kAccSkipAccessChecks);
  }

  bool PreviouslyWarm() {
    if (IsIntrinsic()) {
      // kAccPreviouslyWarm overlaps with kAccIntrinsicBits.
      return true;
    }
    return (GetAccessFlags() & kAccPreviouslyWarm) != 0;
  }

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  void SetPreviouslyWarm() {
    if (IsIntrinsic<kReadBarrierOption>()) {
      // kAccPreviouslyWarm overlaps with kAccIntrinsicBits.
      return;
    }
    AddAccessFlags<kReadBarrierOption>(kAccPreviouslyWarm);
  }

  // Should this method be run in the interpreter and count locks (e.g., failed structured-
  // locking verification)?
  bool MustCountLocks() {
    if (IsIntrinsic()) {
      return false;
    }
    return (GetAccessFlags() & kAccMustCountLocks) != 0;
  }

  void SetMustCountLocks() {
    AddAccessFlags(kAccMustCountLocks);
  }

  HiddenApiAccessFlags::ApiList GetHiddenApiAccessFlags() REQUIRES_SHARED(Locks::mutator_lock_);

  // Returns true if this method could be overridden by a default method.
  bool IsOverridableByDefaultMethod() REQUIRES_SHARED(Locks::mutator_lock_);

  bool CheckIncompatibleClassChange(InvokeType type) REQUIRES_SHARED(Locks::mutator_lock_);

  // Throws the error that would result from trying to invoke this method (i.e.
  // IncompatibleClassChangeError or AbstractMethodError). Only call if !IsInvokable();
  void ThrowInvocationTimeError() REQUIRES_SHARED(Locks::mutator_lock_);

  uint16_t GetMethodIndex() REQUIRES_SHARED(Locks::mutator_lock_);

  // Doesn't do erroneous / unresolved class checks.
  uint16_t GetMethodIndexDuringLinking() REQUIRES_SHARED(Locks::mutator_lock_);

  size_t GetVtableIndex() REQUIRES_SHARED(Locks::mutator_lock_) {
    return GetMethodIndex();
  }

  void SetMethodIndex(uint16_t new_method_index) REQUIRES_SHARED(Locks::mutator_lock_) {
    // Not called within a transaction.
    method_index_ = new_method_index;
  }

  static MemberOffset DexMethodIndexOffset() {
    return MemberOffset(OFFSETOF_MEMBER(ArtMethod, dex_method_index_));
  }

  static MemberOffset MethodIndexOffset() {
    return MemberOffset(OFFSETOF_MEMBER(ArtMethod, method_index_));
  }

  uint32_t GetCodeItemOffset() {
    return dex_code_item_offset_;
  }

  void SetCodeItemOffset(uint32_t new_code_off) {
    // Not called within a transaction.
    dex_code_item_offset_ = new_code_off;
  }

  // Number of 32bit registers that would be required to hold all the arguments
  static size_t NumArgRegisters(const StringPiece& shorty);

  ALWAYS_INLINE uint32_t GetDexMethodIndexUnchecked() {
    return dex_method_index_;
  }
  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE uint32_t GetDexMethodIndex() REQUIRES_SHARED(Locks::mutator_lock_);

  void SetDexMethodIndex(uint32_t new_idx) {
    // Not called within a transaction.
    dex_method_index_ = new_idx;
  }

  // Lookup the Class* from the type index into this method's dex cache.
  ObjPtr<mirror::Class> LookupResolvedClassFromTypeIndex(dex::TypeIndex type_idx)
      REQUIRES_SHARED(Locks::mutator_lock_);
  // Resolve the Class* from the type index into this method's dex cache.
  ObjPtr<mirror::Class> ResolveClassFromTypeIndex(dex::TypeIndex type_idx)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Returns true if this method has the same name and signature of the other method.
  bool HasSameNameAndSignature(ArtMethod* other) REQUIRES_SHARED(Locks::mutator_lock_);

  // Find the method that this method overrides.
  ArtMethod* FindOverriddenMethod(PointerSize pointer_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Find the method index for this method within other_dexfile. If this method isn't present then
  // return dex::kDexNoIndex. The name_and_signature_idx MUST refer to a MethodId with the same
  // name and signature in the other_dexfile, such as the method index used to resolve this method
  // in the other_dexfile.
  uint32_t FindDexMethodIndexInOtherDexFile(const DexFile& other_dexfile,
                                            uint32_t name_and_signature_idx)
      REQUIRES_SHARED(Locks::mutator_lock_);

  void Invoke(Thread* self, uint32_t* args, uint32_t args_size, JValue* result, const char* shorty)
      REQUIRES_SHARED(Locks::mutator_lock_);

  const void* GetEntryPointFromQuickCompiledCode() {
    return GetEntryPointFromQuickCompiledCodePtrSize(kRuntimePointerSize);
  }
  ALWAYS_INLINE const void* GetEntryPointFromQuickCompiledCodePtrSize(PointerSize pointer_size) {
    return GetNativePointer<const void*>(
        EntryPointFromQuickCompiledCodeOffset(pointer_size), pointer_size);
  }

  void SetEntryPointFromQuickCompiledCode(const void* entry_point_from_quick_compiled_code) {
    SetEntryPointFromQuickCompiledCodePtrSize(entry_point_from_quick_compiled_code,
                                              kRuntimePointerSize);
  }
  ALWAYS_INLINE void SetEntryPointFromQuickCompiledCodePtrSize(
      const void* entry_point_from_quick_compiled_code, PointerSize pointer_size) {
    SetNativePointer(EntryPointFromQuickCompiledCodeOffset(pointer_size),
                     entry_point_from_quick_compiled_code,
                     pointer_size);
  }

  // Registers the native method and returns the new entry point. NB The returned entry point might
  // be different from the native_method argument if some MethodCallback modifies it.
  const void* RegisterNative(const void* native_method)
      REQUIRES_SHARED(Locks::mutator_lock_) WARN_UNUSED;

  void UnregisterNative() REQUIRES_SHARED(Locks::mutator_lock_);

  static MemberOffset DataOffset(PointerSize pointer_size) {
    return MemberOffset(PtrSizedFieldsOffset(pointer_size) + OFFSETOF_MEMBER(
        PtrSizedFields, data_) / sizeof(void*) * static_cast<size_t>(pointer_size));
  }

  static MemberOffset EntryPointFromJniOffset(PointerSize pointer_size) {
    return DataOffset(pointer_size);
  }

  static MemberOffset EntryPointFromQuickCompiledCodeOffset(PointerSize pointer_size) {
    return MemberOffset(PtrSizedFieldsOffset(pointer_size) + OFFSETOF_MEMBER(
        PtrSizedFields, entry_point_from_quick_compiled_code_) / sizeof(void*)
            * static_cast<size_t>(pointer_size));
  }

  ImtConflictTable* GetImtConflictTable(PointerSize pointer_size) {
    DCHECK(IsRuntimeMethod());
    return reinterpret_cast<ImtConflictTable*>(GetDataPtrSize(pointer_size));
  }

  ALWAYS_INLINE void SetImtConflictTable(ImtConflictTable* table, PointerSize pointer_size) {
    DCHECK(IsRuntimeMethod());
    SetDataPtrSize(table, pointer_size);
  }

  ProfilingInfo* GetProfilingInfo(PointerSize pointer_size) REQUIRES_SHARED(Locks::mutator_lock_) {
    // Don't do a read barrier in the DCHECK() inside GetAccessFlags() called by IsNative(),
    // as GetProfilingInfo is called in places where the declaring class is treated as a weak
    // reference (accessing it with a read barrier would either prevent unloading the class,
    // or crash the runtime if the GC wants to unload it).
    if (UNLIKELY(IsNative<kWithoutReadBarrier>()) || UNLIKELY(IsProxyMethod())) {
      return nullptr;
    }
    return reinterpret_cast<ProfilingInfo*>(GetDataPtrSize(pointer_size));
  }

  ALWAYS_INLINE void SetProfilingInfo(ProfilingInfo* info) {
    SetDataPtrSize(info, kRuntimePointerSize);
  }

  ALWAYS_INLINE void SetProfilingInfoPtrSize(ProfilingInfo* info, PointerSize pointer_size) {
    SetDataPtrSize(info, pointer_size);
  }

  static MemberOffset ProfilingInfoOffset() {
    DCHECK(IsImagePointerSize(kRuntimePointerSize));
    return DataOffset(kRuntimePointerSize);
  }

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE bool HasSingleImplementation() REQUIRES_SHARED(Locks::mutator_lock_);

  ALWAYS_INLINE void SetHasSingleImplementation(bool single_impl) {
    DCHECK(!IsIntrinsic()) << "conflict with intrinsic bits";
    if (single_impl) {
      AddAccessFlags(kAccSingleImplementation);
    } else {
      ClearAccessFlags(kAccSingleImplementation);
    }
  }

  // Takes a method and returns a 'canonical' one if the method is default (and therefore
  // potentially copied from some other class). For example, this ensures that the debugger does not
  // get confused as to which method we are in.
  ArtMethod* GetCanonicalMethod(PointerSize pointer_size = kRuntimePointerSize)
      REQUIRES_SHARED(Locks::mutator_lock_);

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ArtMethod* GetSingleImplementation(PointerSize pointer_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE void SetSingleImplementation(ArtMethod* method, PointerSize pointer_size) {
    DCHECK(!IsNative<kReadBarrierOption>());
    // Non-abstract method's single implementation is just itself.
    DCHECK(IsAbstract<kReadBarrierOption>());
    SetDataPtrSize(method, pointer_size);
  }

  void* GetEntryPointFromJni() {
    DCHECK(IsNative());
    return GetEntryPointFromJniPtrSize(kRuntimePointerSize);
  }

  ALWAYS_INLINE void* GetEntryPointFromJniPtrSize(PointerSize pointer_size) {
    return GetDataPtrSize(pointer_size);
  }

  void SetEntryPointFromJni(const void* entrypoint) {
    DCHECK(IsNative());
    SetEntryPointFromJniPtrSize(entrypoint, kRuntimePointerSize);
  }

  ALWAYS_INLINE void SetEntryPointFromJniPtrSize(const void* entrypoint, PointerSize pointer_size) {
    SetDataPtrSize(entrypoint, pointer_size);
  }

  ALWAYS_INLINE void* GetDataPtrSize(PointerSize pointer_size) {
    DCHECK(IsImagePointerSize(pointer_size));
    return GetNativePointer<void*>(DataOffset(pointer_size), pointer_size);
  }

  ALWAYS_INLINE void SetDataPtrSize(const void* data, PointerSize pointer_size) {
    DCHECK(IsImagePointerSize(pointer_size));
    SetNativePointer(DataOffset(pointer_size), data, pointer_size);
  }

  // Is this a CalleSaveMethod or ResolutionMethod and therefore doesn't adhere to normal
  // conventions for a method of managed code. Returns false for Proxy methods.
  ALWAYS_INLINE bool IsRuntimeMethod() {
    return dex_method_index_ == kRuntimeMethodDexMethodIndex;
  }

  // Is this a hand crafted method used for something like describing callee saves?
  bool IsCalleeSaveMethod() REQUIRES_SHARED(Locks::mutator_lock_);

  bool IsResolutionMethod() REQUIRES_SHARED(Locks::mutator_lock_);

  bool IsImtUnimplementedMethod() REQUIRES_SHARED(Locks::mutator_lock_);

  // Find the catch block for the given exception type and dex_pc. When a catch block is found,
  // indicates whether the found catch block is responsible for clearing the exception or whether
  // a move-exception instruction is present.
  uint32_t FindCatchBlock(Handle<mirror::Class> exception_type, uint32_t dex_pc,
                          bool* has_no_move_exception)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // NO_THREAD_SAFETY_ANALYSIS since we don't know what the callback requires.
  template<ReadBarrierOption kReadBarrierOption = kWithReadBarrier, typename RootVisitorType>
  void VisitRoots(RootVisitorType& visitor, PointerSize pointer_size) NO_THREAD_SAFETY_ANALYSIS;

  const DexFile* GetDexFile() REQUIRES_SHARED(Locks::mutator_lock_);

  const char* GetDeclaringClassDescriptor() REQUIRES_SHARED(Locks::mutator_lock_);

  ALWAYS_INLINE const char* GetShorty() REQUIRES_SHARED(Locks::mutator_lock_);

  const char* GetShorty(uint32_t* out_length) REQUIRES_SHARED(Locks::mutator_lock_);

  const Signature GetSignature() REQUIRES_SHARED(Locks::mutator_lock_);

  ALWAYS_INLINE const char* GetName() REQUIRES_SHARED(Locks::mutator_lock_);

  ObjPtr<mirror::String> GetNameAsString(Thread* self) REQUIRES_SHARED(Locks::mutator_lock_);

  const DexFile::CodeItem* GetCodeItem() REQUIRES_SHARED(Locks::mutator_lock_);

  bool IsResolvedTypeIdx(dex::TypeIndex type_idx) REQUIRES_SHARED(Locks::mutator_lock_);

  int32_t GetLineNumFromDexPC(uint32_t dex_pc) REQUIRES_SHARED(Locks::mutator_lock_);

  const DexFile::ProtoId& GetPrototype() REQUIRES_SHARED(Locks::mutator_lock_);

  const DexFile::TypeList* GetParameterTypeList() REQUIRES_SHARED(Locks::mutator_lock_);

  const char* GetDeclaringClassSourceFile() REQUIRES_SHARED(Locks::mutator_lock_);

  uint16_t GetClassDefIndex() REQUIRES_SHARED(Locks::mutator_lock_);

  const DexFile::ClassDef& GetClassDef() REQUIRES_SHARED(Locks::mutator_lock_);

  ALWAYS_INLINE size_t GetNumberOfParameters() REQUIRES_SHARED(Locks::mutator_lock_);

  const char* GetReturnTypeDescriptor() REQUIRES_SHARED(Locks::mutator_lock_);

  ALWAYS_INLINE Primitive::Type GetReturnTypePrimitive() REQUIRES_SHARED(Locks::mutator_lock_);

  const char* GetTypeDescriptorFromTypeIdx(dex::TypeIndex type_idx)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Lookup return type.
  ObjPtr<mirror::Class> LookupResolvedReturnType() REQUIRES_SHARED(Locks::mutator_lock_);
  // Resolve return type. May cause thread suspension due to GetClassFromTypeIdx
  // calling ResolveType this caused a large number of bugs at call sites.
  ObjPtr<mirror::Class> ResolveReturnType() REQUIRES_SHARED(Locks::mutator_lock_);

  mirror::ClassLoader* GetClassLoader() REQUIRES_SHARED(Locks::mutator_lock_);

  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  mirror::DexCache* GetDexCache() REQUIRES_SHARED(Locks::mutator_lock_);
  mirror::DexCache* GetObsoleteDexCache() REQUIRES_SHARED(Locks::mutator_lock_);

  ALWAYS_INLINE ArtMethod* GetInterfaceMethodForProxyUnchecked(PointerSize pointer_size)
      REQUIRES_SHARED(Locks::mutator_lock_);
  ALWAYS_INLINE ArtMethod* GetInterfaceMethodIfProxy(PointerSize pointer_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  ArtMethod* GetNonObsoleteMethod() REQUIRES_SHARED(Locks::mutator_lock_);

  // May cause thread suspension due to class resolution.
  bool EqualParameters(Handle<mirror::ObjectArray<mirror::Class>> params)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Size of an instance of this native class.
  static size_t Size(PointerSize pointer_size) {
    return PtrSizedFieldsOffset(pointer_size) +
        (sizeof(PtrSizedFields) / sizeof(void*)) * static_cast<size_t>(pointer_size);
  }

  // Alignment of an instance of this native class.
  static size_t Alignment(PointerSize pointer_size) {
    // The ArtMethod alignment is the same as image pointer size. This differs from
    // alignof(ArtMethod) if cross-compiling with pointer_size != sizeof(void*).
    return static_cast<size_t>(pointer_size);
  }

  void CopyFrom(ArtMethod* src, PointerSize image_pointer_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Note, hotness_counter_ updates are non-atomic but it doesn't need to be precise.  Also,
  // given that the counter is only 16 bits wide we can expect wrap-around in some
  // situations.  Consumers of hotness_count_ must be able to deal with that.
  uint16_t IncrementCounter() {
    return ++hotness_count_;
  }

  void ClearCounter() {
    hotness_count_ = 0;
  }

  void SetCounter(int16_t hotness_count) {
    hotness_count_ = hotness_count;
  }

  uint16_t GetCounter() const {
    return hotness_count_;
  }

  static MemberOffset HotnessCountOffset() {
    return MemberOffset(OFFSETOF_MEMBER(ArtMethod, hotness_count_));
  }

  ArrayRef<const uint8_t> GetQuickenedInfo() REQUIRES_SHARED(Locks::mutator_lock_);
  uint16_t GetIndexFromQuickening(uint32_t dex_pc) REQUIRES_SHARED(Locks::mutator_lock_);

  // Returns the method header for the compiled code containing 'pc'. Note that runtime
  // methods will return null for this method, as they are not oat based.
  const OatQuickMethodHeader* GetOatQuickMethodHeader(uintptr_t pc)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Get compiled code for the method, return null if no code exists.
  const void* GetOatMethodQuickCode(PointerSize pointer_size)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Returns whether the method has any compiled code, JIT or AOT.
  bool HasAnyCompiledCode() REQUIRES_SHARED(Locks::mutator_lock_);

  // Returns a human-readable signature for 'm'. Something like "a.b.C.m" or
  // "a.b.C.m(II)V" (depending on the value of 'with_signature').
  static std::string PrettyMethod(ArtMethod* m, bool with_signature = true)
      REQUIRES_SHARED(Locks::mutator_lock_);
  std::string PrettyMethod(bool with_signature = true)
      REQUIRES_SHARED(Locks::mutator_lock_);
  // Returns the JNI native function name for the non-overloaded method 'm'.
  std::string JniShortName()
      REQUIRES_SHARED(Locks::mutator_lock_);
  // Returns the JNI native function name for the overloaded method 'm'.
  std::string JniLongName()
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Update heap objects and non-entrypoint pointers by the passed in visitor for image relocation.
  // Does not use read barrier.
  template <typename Visitor>
  ALWAYS_INLINE void UpdateObjectsForImageRelocation(const Visitor& visitor)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Update entry points by passing them through the visitor.
  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier, typename Visitor>
  ALWAYS_INLINE void UpdateEntrypoints(const Visitor& visitor, PointerSize pointer_size);

  // Visit the individual members of an ArtMethod.  Used by imgdiag.
  // As imgdiag does not support mixing instruction sets or pointer sizes (e.g., using imgdiag32
  // to inspect 64-bit images, etc.), we can go beneath the accessors directly to the class members.
  template <typename VisitorFunc>
  void VisitMembers(VisitorFunc& visitor) {
    DCHECK(IsImagePointerSize(kRuntimePointerSize));
    visitor(this, &declaring_class_, "declaring_class_");
    visitor(this, &access_flags_, "access_flags_");
    visitor(this, &dex_code_item_offset_, "dex_code_item_offset_");
    visitor(this, &dex_method_index_, "dex_method_index_");
    visitor(this, &method_index_, "method_index_");
    visitor(this, &hotness_count_, "hotness_count_");
    visitor(this, &ptr_sized_fields_.data_, "ptr_sized_fields_.data_");
    visitor(this,
            &ptr_sized_fields_.entry_point_from_quick_compiled_code_,
            "ptr_sized_fields_.entry_point_from_quick_compiled_code_");
  }

  // Returns the dex instructions of the code item for the art method. Returns an empty array for
  // the null code item case.
  ALWAYS_INLINE CodeItemInstructionAccessor DexInstructions()
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Returns the dex code item data section of the DexFile for the art method.
  ALWAYS_INLINE CodeItemDataAccessor DexInstructionData()
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Returns the dex code item debug info section of the DexFile for the art method.
  ALWAYS_INLINE CodeItemDebugInfoAccessor DexInstructionDebugInfo()
      REQUIRES_SHARED(Locks::mutator_lock_);

 protected:
  // Field order required by test "ValidateFieldOrderOfJavaCppUnionClasses".
  // The class we are a part of.
  GcRoot<mirror::Class> declaring_class_;

  // Access flags; low 16 bits are defined by spec.
  // Getting and setting this flag needs to be atomic when concurrency is
  // possible, e.g. after this method's class is linked. Such as when setting
  // verifier flags and single-implementation flag.
  std::atomic<std::uint32_t> access_flags_;

  /* Dex file fields. The defining dex file is available via declaring_class_->dex_cache_ */

  // Offset to the CodeItem.
  uint32_t dex_code_item_offset_;

  // Index into method_ids of the dex file associated with this method.
  uint32_t dex_method_index_;

  /* End of dex file fields. */

  // Entry within a dispatch table for this method. For static/direct methods the index is into
  // the declaringClass.directMethods, for virtual methods the vtable and for interface methods the
  // ifTable.
  uint16_t method_index_;

  // The hotness we measure for this method. Not atomic, as we allow
  // missing increments: if the method is hot, we will see it eventually.
  uint16_t hotness_count_;

  // Fake padding field gets inserted here.

  // Must be the last fields in the method.
  struct PtrSizedFields {
    // Depending on the method type, the data is
    //   - native method: pointer to the JNI function registered to this method
    //                    or a function to resolve the JNI function,
    //   - conflict method: ImtConflictTable,
    //   - abstract/interface method: the single-implementation if any,
    //   - proxy method: the original interface method or constructor,
    //   - other methods: the profiling data.
    void* data_;

    // Method dispatch from quick compiled code invokes this pointer which may cause bridging into
    // the interpreter.
    void* entry_point_from_quick_compiled_code_;
  } ptr_sized_fields_;

 private:
  uint16_t FindObsoleteDexClassDefIndex() REQUIRES_SHARED(Locks::mutator_lock_);

  static constexpr size_t PtrSizedFieldsOffset(PointerSize pointer_size) {
    // Round up to pointer size for padding field. Tested in art_method.cc.
    return RoundUp(offsetof(ArtMethod, hotness_count_) + sizeof(hotness_count_),
                   static_cast<size_t>(pointer_size));
  }

  // Compare given pointer size to the image pointer size.
  static bool IsImagePointerSize(PointerSize pointer_size);

  dex::TypeIndex GetReturnTypeIndex() REQUIRES_SHARED(Locks::mutator_lock_);

  template<typename T>
  ALWAYS_INLINE T GetNativePointer(MemberOffset offset, PointerSize pointer_size) const {
    static_assert(std::is_pointer<T>::value, "T must be a pointer type");
    const auto addr = reinterpret_cast<uintptr_t>(this) + offset.Uint32Value();
    if (pointer_size == PointerSize::k32) {
      return reinterpret_cast<T>(*reinterpret_cast<const uint32_t*>(addr));
    } else {
      auto v = *reinterpret_cast<const uint64_t*>(addr);
      return reinterpret_cast<T>(dchecked_integral_cast<uintptr_t>(v));
    }
  }

  template<typename T>
  ALWAYS_INLINE void SetNativePointer(MemberOffset offset, T new_value, PointerSize pointer_size) {
    static_assert(std::is_pointer<T>::value, "T must be a pointer type");
    const auto addr = reinterpret_cast<uintptr_t>(this) + offset.Uint32Value();
    if (pointer_size == PointerSize::k32) {
      uintptr_t ptr = reinterpret_cast<uintptr_t>(new_value);
      *reinterpret_cast<uint32_t*>(addr) = dchecked_integral_cast<uint32_t>(ptr);
    } else {
      *reinterpret_cast<uint64_t*>(addr) = reinterpret_cast<uintptr_t>(new_value);
    }
  }

  template <ReadBarrierOption kReadBarrierOption> void GetAccessFlagsDCheck();

  static inline bool IsValidIntrinsicUpdate(uint32_t modifier) {
    return (((modifier & kAccIntrinsic) == kAccIntrinsic) &&
            (((modifier & ~(kAccIntrinsic | kAccIntrinsicBits)) == 0)));
  }

  static inline bool OverlapsIntrinsicBits(uint32_t modifier) {
    return (modifier & kAccIntrinsicBits) != 0;
  }

  // This setter guarantees atomicity.
  template <ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  void AddAccessFlags(uint32_t flag) {
    DCHECK(!IsIntrinsic<kReadBarrierOption>() ||
           !OverlapsIntrinsicBits(flag) ||
           IsValidIntrinsicUpdate(flag));
    uint32_t old_access_flags;
    uint32_t new_access_flags;
    do {
      old_access_flags = access_flags_.load(std::memory_order_relaxed);
      new_access_flags = old_access_flags | flag;
    } while (!access_flags_.compare_exchange_weak(old_access_flags, new_access_flags));
  }

  // This setter guarantees atomicity.
  void ClearAccessFlags(uint32_t flag) {
    DCHECK(!IsIntrinsic() || !OverlapsIntrinsicBits(flag) || IsValidIntrinsicUpdate(flag));
    uint32_t old_access_flags;
    uint32_t new_access_flags;
    do {
      old_access_flags = access_flags_.load(std::memory_order_relaxed);
      new_access_flags = old_access_flags & ~flag;
    } while (!access_flags_.compare_exchange_weak(old_access_flags, new_access_flags));
  }

  DISALLOW_COPY_AND_ASSIGN(ArtMethod);  // Need to use CopyFrom to deal with 32 vs 64 bits.
};

class MethodCallback {
 public:
  virtual ~MethodCallback() {}

  virtual void RegisterNativeMethod(ArtMethod* method,
                                    const void* original_implementation,
                                    /*out*/void** new_implementation)
      REQUIRES_SHARED(Locks::mutator_lock_) = 0;
};

}  // namespace art

#endif  // ART_RUNTIME_ART_METHOD_H_
