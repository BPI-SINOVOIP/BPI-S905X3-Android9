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

#ifndef ART_RUNTIME_ENTRYPOINTS_ENTRYPOINT_UTILS_INL_H_
#define ART_RUNTIME_ENTRYPOINTS_ENTRYPOINT_UTILS_INL_H_

#include "entrypoint_utils.h"

#include "art_field-inl.h"
#include "art_method-inl.h"
#include "base/enums.h"
#include "class_linker-inl.h"
#include "common_throws.h"
#include "dex/dex_file.h"
#include "dex/invoke_type.h"
#include "entrypoints/quick/callee_save_frame.h"
#include "handle_scope-inl.h"
#include "imt_conflict_table.h"
#include "imtable-inl.h"
#include "indirect_reference_table.h"
#include "jni_internal.h"
#include "mirror/array.h"
#include "mirror/class-inl.h"
#include "mirror/object-inl.h"
#include "mirror/throwable.h"
#include "nth_caller_visitor.h"
#include "runtime.h"
#include "stack_map.h"
#include "thread.h"
#include "well_known_classes.h"

namespace art {

inline ArtMethod* GetResolvedMethod(ArtMethod* outer_method,
                                    const MethodInfo& method_info,
                                    const InlineInfo& inline_info,
                                    const InlineInfoEncoding& encoding,
                                    uint8_t inlining_depth)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  DCHECK(!outer_method->IsObsolete());

  // This method is being used by artQuickResolutionTrampoline, before it sets up
  // the passed parameters in a GC friendly way. Therefore we must never be
  // suspended while executing it.
  ScopedAssertNoThreadSuspension sants(__FUNCTION__);

  if (inline_info.EncodesArtMethodAtDepth(encoding, inlining_depth)) {
    return inline_info.GetArtMethodAtDepth(encoding, inlining_depth);
  }

  uint32_t method_index = inline_info.GetMethodIndexAtDepth(encoding, method_info, inlining_depth);
  if (inline_info.GetDexPcAtDepth(encoding, inlining_depth) == static_cast<uint32_t>(-1)) {
    // "charAt" special case. It is the only non-leaf method we inline across dex files.
    ArtMethod* inlined_method = jni::DecodeArtMethod(WellKnownClasses::java_lang_String_charAt);
    DCHECK_EQ(inlined_method->GetDexMethodIndex(), method_index);
    return inlined_method;
  }

  // Find which method did the call in the inlining hierarchy.
  ClassLinker* class_linker = Runtime::Current()->GetClassLinker();
  ArtMethod* method = outer_method;
  for (uint32_t depth = 0, end = inlining_depth + 1u; depth != end; ++depth) {
    DCHECK(!inline_info.EncodesArtMethodAtDepth(encoding, depth));
    DCHECK_NE(inline_info.GetDexPcAtDepth(encoding, depth), static_cast<uint32_t>(-1));
    method_index = inline_info.GetMethodIndexAtDepth(encoding, method_info, depth);
    ArtMethod* inlined_method = class_linker->LookupResolvedMethod(method_index,
                                                                   method->GetDexCache(),
                                                                   method->GetClassLoader());
    if (UNLIKELY(inlined_method == nullptr)) {
      LOG(FATAL) << "Could not find an inlined method from an .oat file: "
                 << method->GetDexFile()->PrettyMethod(method_index) << " . "
                 << "This must be due to duplicate classes or playing wrongly with class loaders";
      UNREACHABLE();
    }
    DCHECK(!inlined_method->IsRuntimeMethod());
    if (UNLIKELY(inlined_method->GetDexFile() != method->GetDexFile())) {
      // TODO: We could permit inlining within a multi-dex oat file and the boot image,
      // even going back from boot image methods to the same oat file. However, this is
      // not currently implemented in the compiler. Therefore crossing dex file boundary
      // indicates that the inlined definition is not the same as the one used at runtime.
      LOG(FATAL) << "Inlined method resolution crossed dex file boundary: from "
                 << method->PrettyMethod()
                 << " in " << method->GetDexFile()->GetLocation() << "/"
                 << static_cast<const void*>(method->GetDexFile())
                 << " to " << inlined_method->PrettyMethod()
                 << " in " << inlined_method->GetDexFile()->GetLocation() << "/"
                 << static_cast<const void*>(inlined_method->GetDexFile()) << ". "
                 << "This must be due to duplicate classes or playing wrongly with class loaders";
      UNREACHABLE();
    }
    method = inlined_method;
  }

  return method;
}

ALWAYS_INLINE inline mirror::Class* CheckObjectAlloc(mirror::Class* klass,
                                                     Thread* self,
                                                     bool* slow_path)
    REQUIRES_SHARED(Locks::mutator_lock_)
    REQUIRES(!Roles::uninterruptible_) {
  if (UNLIKELY(!klass->IsInstantiable())) {
    self->ThrowNewException("Ljava/lang/InstantiationError;", klass->PrettyDescriptor().c_str());
    *slow_path = true;
    return nullptr;  // Failure
  }
  if (UNLIKELY(klass->IsClassClass())) {
    ThrowIllegalAccessError(nullptr, "Class %s is inaccessible",
                            klass->PrettyDescriptor().c_str());
    *slow_path = true;
    return nullptr;  // Failure
  }
  if (UNLIKELY(!klass->IsInitialized())) {
    StackHandleScope<1> hs(self);
    Handle<mirror::Class> h_klass(hs.NewHandle(klass));
    // EnsureInitialized (the class initializer) might cause a GC.
    // may cause us to suspend meaning that another thread may try to
    // change the allocator while we are stuck in the entrypoints of
    // an old allocator. Also, the class initialization may fail. To
    // handle these cases we mark the slow path boolean as true so
    // that the caller knows to check the allocator type to see if it
    // has changed and to null-check the return value in case the
    // initialization fails.
    *slow_path = true;
    if (!Runtime::Current()->GetClassLinker()->EnsureInitialized(self, h_klass, true, true)) {
      DCHECK(self->IsExceptionPending());
      return nullptr;  // Failure
    } else {
      DCHECK(!self->IsExceptionPending());
    }
    return h_klass.Get();
  }
  return klass;
}

ALWAYS_INLINE
inline mirror::Class* CheckClassInitializedForObjectAlloc(mirror::Class* klass,
                                                          Thread* self,
                                                          bool* slow_path)
    REQUIRES_SHARED(Locks::mutator_lock_)
    REQUIRES(!Roles::uninterruptible_) {
  if (UNLIKELY(!klass->IsInitialized())) {
    StackHandleScope<1> hs(self);
    Handle<mirror::Class> h_class(hs.NewHandle(klass));
    // EnsureInitialized (the class initializer) might cause a GC.
    // may cause us to suspend meaning that another thread may try to
    // change the allocator while we are stuck in the entrypoints of
    // an old allocator. Also, the class initialization may fail. To
    // handle these cases we mark the slow path boolean as true so
    // that the caller knows to check the allocator type to see if it
    // has changed and to null-check the return value in case the
    // initialization fails.
    *slow_path = true;
    if (!Runtime::Current()->GetClassLinker()->EnsureInitialized(self, h_class, true, true)) {
      DCHECK(self->IsExceptionPending());
      return nullptr;  // Failure
    }
    return h_class.Get();
  }
  return klass;
}

// Allocate an instance of klass. Throws InstantationError if klass is not instantiable,
// or IllegalAccessError if klass is j.l.Class. Performs a clinit check too.
template <bool kInstrumented>
ALWAYS_INLINE
inline mirror::Object* AllocObjectFromCode(mirror::Class* klass,
                                           Thread* self,
                                           gc::AllocatorType allocator_type) {
  bool slow_path = false;
  klass = CheckObjectAlloc(klass, self, &slow_path);
  if (UNLIKELY(slow_path)) {
    if (klass == nullptr) {
      return nullptr;
    }
    // CheckObjectAlloc can cause thread suspension which means we may now be instrumented.
    return klass->Alloc</*kInstrumented*/true>(
        self,
        Runtime::Current()->GetHeap()->GetCurrentAllocator()).Ptr();
  }
  DCHECK(klass != nullptr);
  return klass->Alloc<kInstrumented>(self, allocator_type).Ptr();
}

// Given the context of a calling Method and a resolved class, create an instance.
template <bool kInstrumented>
ALWAYS_INLINE
inline mirror::Object* AllocObjectFromCodeResolved(mirror::Class* klass,
                                                   Thread* self,
                                                   gc::AllocatorType allocator_type) {
  DCHECK(klass != nullptr);
  bool slow_path = false;
  klass = CheckClassInitializedForObjectAlloc(klass, self, &slow_path);
  if (UNLIKELY(slow_path)) {
    if (klass == nullptr) {
      return nullptr;
    }
    gc::Heap* heap = Runtime::Current()->GetHeap();
    // Pass in false since the object cannot be finalizable.
    // CheckClassInitializedForObjectAlloc can cause thread suspension which means we may now be
    // instrumented.
    return klass->Alloc</*kInstrumented*/true, false>(self, heap->GetCurrentAllocator()).Ptr();
  }
  // Pass in false since the object cannot be finalizable.
  return klass->Alloc<kInstrumented, false>(self, allocator_type).Ptr();
}

// Given the context of a calling Method and an initialized class, create an instance.
template <bool kInstrumented>
ALWAYS_INLINE
inline mirror::Object* AllocObjectFromCodeInitialized(mirror::Class* klass,
                                                      Thread* self,
                                                      gc::AllocatorType allocator_type) {
  DCHECK(klass != nullptr);
  // Pass in false since the object cannot be finalizable.
  return klass->Alloc<kInstrumented, false>(self, allocator_type).Ptr();
}


template <bool kAccessCheck>
ALWAYS_INLINE
inline mirror::Class* CheckArrayAlloc(dex::TypeIndex type_idx,
                                      int32_t component_count,
                                      ArtMethod* method,
                                      bool* slow_path) {
  if (UNLIKELY(component_count < 0)) {
    ThrowNegativeArraySizeException(component_count);
    *slow_path = true;
    return nullptr;  // Failure
  }
  ObjPtr<mirror::Class> klass = method->GetDexCache()->GetResolvedType(type_idx);
  if (UNLIKELY(klass == nullptr)) {  // Not in dex cache so try to resolve
    ClassLinker* class_linker = Runtime::Current()->GetClassLinker();
    klass = class_linker->ResolveType(type_idx, method);
    *slow_path = true;
    if (klass == nullptr) {  // Error
      DCHECK(Thread::Current()->IsExceptionPending());
      return nullptr;  // Failure
    }
    CHECK(klass->IsArrayClass()) << klass->PrettyClass();
  }
  if (kAccessCheck) {
    mirror::Class* referrer = method->GetDeclaringClass();
    if (UNLIKELY(!referrer->CanAccess(klass))) {
      ThrowIllegalAccessErrorClass(referrer, klass);
      *slow_path = true;
      return nullptr;  // Failure
    }
  }
  return klass.Ptr();
}

// Given the context of a calling Method, use its DexCache to resolve a type to an array Class. If
// it cannot be resolved, throw an error. If it can, use it to create an array.
// When verification/compiler hasn't been able to verify access, optionally perform an access
// check.
template <bool kAccessCheck, bool kInstrumented>
ALWAYS_INLINE
inline mirror::Array* AllocArrayFromCode(dex::TypeIndex type_idx,
                                         int32_t component_count,
                                         ArtMethod* method,
                                         Thread* self,
                                         gc::AllocatorType allocator_type) {
  bool slow_path = false;
  mirror::Class* klass = CheckArrayAlloc<kAccessCheck>(type_idx, component_count, method,
                                                       &slow_path);
  if (UNLIKELY(slow_path)) {
    if (klass == nullptr) {
      return nullptr;
    }
    gc::Heap* heap = Runtime::Current()->GetHeap();
    // CheckArrayAlloc can cause thread suspension which means we may now be instrumented.
    return mirror::Array::Alloc</*kInstrumented*/true>(self,
                                                       klass,
                                                       component_count,
                                                       klass->GetComponentSizeShift(),
                                                       heap->GetCurrentAllocator());
  }
  return mirror::Array::Alloc<kInstrumented>(self, klass, component_count,
                                             klass->GetComponentSizeShift(), allocator_type);
}

template <bool kInstrumented>
ALWAYS_INLINE
inline mirror::Array* AllocArrayFromCodeResolved(mirror::Class* klass,
                                                 int32_t component_count,
                                                 Thread* self,
                                                 gc::AllocatorType allocator_type) {
  DCHECK(klass != nullptr);
  if (UNLIKELY(component_count < 0)) {
    ThrowNegativeArraySizeException(component_count);
    return nullptr;  // Failure
  }
  // No need to retry a slow-path allocation as the above code won't cause a GC or thread
  // suspension.
  return mirror::Array::Alloc<kInstrumented>(self, klass, component_count,
                                             klass->GetComponentSizeShift(), allocator_type);
}

template<FindFieldType type, bool access_check>
inline ArtField* FindFieldFromCode(uint32_t field_idx,
                                   ArtMethod* referrer,
                                   Thread* self,
                                   size_t expected_size) {
  bool is_primitive;
  bool is_set;
  bool is_static;
  switch (type) {
    case InstanceObjectRead:     is_primitive = false; is_set = false; is_static = false; break;
    case InstanceObjectWrite:    is_primitive = false; is_set = true;  is_static = false; break;
    case InstancePrimitiveRead:  is_primitive = true;  is_set = false; is_static = false; break;
    case InstancePrimitiveWrite: is_primitive = true;  is_set = true;  is_static = false; break;
    case StaticObjectRead:       is_primitive = false; is_set = false; is_static = true;  break;
    case StaticObjectWrite:      is_primitive = false; is_set = true;  is_static = true;  break;
    case StaticPrimitiveRead:    is_primitive = true;  is_set = false; is_static = true;  break;
    case StaticPrimitiveWrite:   // Keep GCC happy by having a default handler, fall-through.
    default:                     is_primitive = true;  is_set = true;  is_static = true;  break;
  }
  ClassLinker* class_linker = Runtime::Current()->GetClassLinker();

  ArtField* resolved_field;
  if (access_check) {
    // Slow path: According to JLS 13.4.8, a linkage error may occur if a compile-time
    // qualifying type of a field and the resolved run-time qualifying type of a field differed
    // in their static-ness.
    //
    // In particular, don't assume the dex instruction already correctly knows if the
    // real field is static or not. The resolution must not be aware of this.
    ArtMethod* method = referrer->GetInterfaceMethodIfProxy(kRuntimePointerSize);

    StackHandleScope<2> hs(self);
    Handle<mirror::DexCache> h_dex_cache(hs.NewHandle(method->GetDexCache()));
    Handle<mirror::ClassLoader> h_class_loader(hs.NewHandle(method->GetClassLoader()));

    resolved_field = class_linker->ResolveFieldJLS(field_idx,
                                                   h_dex_cache,
                                                   h_class_loader);
  } else {
    // Fast path: Verifier already would've called ResolveFieldJLS and we wouldn't
    // be executing here if there was a static/non-static mismatch.
    resolved_field = class_linker->ResolveField(field_idx, referrer, is_static);
  }

  if (UNLIKELY(resolved_field == nullptr)) {
    DCHECK(self->IsExceptionPending());  // Throw exception and unwind.
    return nullptr;  // Failure.
  }
  ObjPtr<mirror::Class> fields_class = resolved_field->GetDeclaringClass();
  if (access_check) {
    if (UNLIKELY(resolved_field->IsStatic() != is_static)) {
      ThrowIncompatibleClassChangeErrorField(resolved_field, is_static, referrer);
      return nullptr;
    }
    mirror::Class* referring_class = referrer->GetDeclaringClass();
    if (UNLIKELY(!referring_class->CheckResolvedFieldAccess(fields_class,
                                                            resolved_field,
                                                            referrer->GetDexCache(),
                                                            field_idx))) {
      DCHECK(self->IsExceptionPending());  // Throw exception and unwind.
      return nullptr;  // Failure.
    }
    if (UNLIKELY(is_set && resolved_field->IsFinal() && (fields_class != referring_class))) {
      ThrowIllegalAccessErrorFinalField(referrer, resolved_field);
      return nullptr;  // Failure.
    } else {
      if (UNLIKELY(resolved_field->IsPrimitiveType() != is_primitive ||
                   resolved_field->FieldSize() != expected_size)) {
        self->ThrowNewExceptionF("Ljava/lang/NoSuchFieldError;",
                                 "Attempted read of %zd-bit %s on field '%s'",
                                 expected_size * (32 / sizeof(int32_t)),
                                 is_primitive ? "primitive" : "non-primitive",
                                 resolved_field->PrettyField(true).c_str());
        return nullptr;  // Failure.
      }
    }
  }
  if (!is_static) {
    // instance fields must be being accessed on an initialized class
    return resolved_field;
  } else {
    // If the class is initialized we're done.
    if (LIKELY(fields_class->IsInitialized())) {
      return resolved_field;
    } else {
      StackHandleScope<1> hs(self);
      if (LIKELY(class_linker->EnsureInitialized(self, hs.NewHandle(fields_class), true, true))) {
        // Otherwise let's ensure the class is initialized before resolving the field.
        return resolved_field;
      }
      DCHECK(self->IsExceptionPending());  // Throw exception and unwind
      return nullptr;  // Failure.
    }
  }
}

// Explicit template declarations of FindFieldFromCode for all field access types.
#define EXPLICIT_FIND_FIELD_FROM_CODE_TEMPLATE_DECL(_type, _access_check) \
template REQUIRES_SHARED(Locks::mutator_lock_) ALWAYS_INLINE \
ArtField* FindFieldFromCode<_type, _access_check>(uint32_t field_idx, \
                                                  ArtMethod* referrer, \
                                                  Thread* self, size_t expected_size) \

#define EXPLICIT_FIND_FIELD_FROM_CODE_TYPED_TEMPLATE_DECL(_type) \
    EXPLICIT_FIND_FIELD_FROM_CODE_TEMPLATE_DECL(_type, false); \
    EXPLICIT_FIND_FIELD_FROM_CODE_TEMPLATE_DECL(_type, true)

EXPLICIT_FIND_FIELD_FROM_CODE_TYPED_TEMPLATE_DECL(InstanceObjectRead);
EXPLICIT_FIND_FIELD_FROM_CODE_TYPED_TEMPLATE_DECL(InstanceObjectWrite);
EXPLICIT_FIND_FIELD_FROM_CODE_TYPED_TEMPLATE_DECL(InstancePrimitiveRead);
EXPLICIT_FIND_FIELD_FROM_CODE_TYPED_TEMPLATE_DECL(InstancePrimitiveWrite);
EXPLICIT_FIND_FIELD_FROM_CODE_TYPED_TEMPLATE_DECL(StaticObjectRead);
EXPLICIT_FIND_FIELD_FROM_CODE_TYPED_TEMPLATE_DECL(StaticObjectWrite);
EXPLICIT_FIND_FIELD_FROM_CODE_TYPED_TEMPLATE_DECL(StaticPrimitiveRead);
EXPLICIT_FIND_FIELD_FROM_CODE_TYPED_TEMPLATE_DECL(StaticPrimitiveWrite);

#undef EXPLICIT_FIND_FIELD_FROM_CODE_TYPED_TEMPLATE_DECL
#undef EXPLICIT_FIND_FIELD_FROM_CODE_TEMPLATE_DECL

template<InvokeType type, bool access_check>
inline ArtMethod* FindMethodFromCode(uint32_t method_idx,
                                     ObjPtr<mirror::Object>* this_object,
                                     ArtMethod* referrer,
                                     Thread* self) {
  ClassLinker* const class_linker = Runtime::Current()->GetClassLinker();
  constexpr ClassLinker::ResolveMode resolve_mode =
      access_check ? ClassLinker::ResolveMode::kCheckICCEAndIAE
                   : ClassLinker::ResolveMode::kNoChecks;
  ArtMethod* resolved_method;
  if (type == kStatic) {
    resolved_method = class_linker->ResolveMethod<resolve_mode>(self, method_idx, referrer, type);
  } else {
    StackHandleScope<1> hs(self);
    HandleWrapperObjPtr<mirror::Object> h_this(hs.NewHandleWrapper(this_object));
    resolved_method = class_linker->ResolveMethod<resolve_mode>(self, method_idx, referrer, type);
  }
  if (UNLIKELY(resolved_method == nullptr)) {
    DCHECK(self->IsExceptionPending());  // Throw exception and unwind.
    return nullptr;  // Failure.
  }
  // Next, null pointer check.
  if (UNLIKELY(*this_object == nullptr && type != kStatic)) {
    if (UNLIKELY(resolved_method->GetDeclaringClass()->IsStringClass() &&
                 resolved_method->IsConstructor())) {
      // Hack for String init:
      //
      // We assume that the input of String.<init> in verified code is always
      // an unitialized reference. If it is a null constant, it must have been
      // optimized out by the compiler. Do not throw NullPointerException.
    } else {
      // Maintain interpreter-like semantics where NullPointerException is thrown
      // after potential NoSuchMethodError from class linker.
      ThrowNullPointerExceptionForMethodAccess(method_idx, type);
      return nullptr;  // Failure.
    }
  }
  switch (type) {
    case kStatic:
    case kDirect:
      return resolved_method;
    case kVirtual: {
      ObjPtr<mirror::Class> klass = (*this_object)->GetClass();
      uint16_t vtable_index = resolved_method->GetMethodIndex();
      if (access_check &&
          (!klass->HasVTable() ||
           vtable_index >= static_cast<uint32_t>(klass->GetVTableLength()))) {
        // Behavior to agree with that of the verifier.
        ThrowNoSuchMethodError(type, resolved_method->GetDeclaringClass(),
                               resolved_method->GetName(), resolved_method->GetSignature());
        return nullptr;  // Failure.
      }
      DCHECK(klass->HasVTable()) << klass->PrettyClass();
      return klass->GetVTableEntry(vtable_index, class_linker->GetImagePointerSize());
    }
    case kSuper: {
      // TODO This lookup is quite slow.
      // NB This is actually quite tricky to do any other way. We cannot use GetDeclaringClass since
      //    that will actually not be what we want in some cases where there are miranda methods or
      //    defaults. What we actually need is a GetContainingClass that says which classes virtuals
      //    this method is coming from.
      StackHandleScope<2> hs2(self);
      HandleWrapperObjPtr<mirror::Object> h_this(hs2.NewHandleWrapper(this_object));
      Handle<mirror::Class> h_referring_class(hs2.NewHandle(referrer->GetDeclaringClass()));
      const dex::TypeIndex method_type_idx =
          referrer->GetDexFile()->GetMethodId(method_idx).class_idx_;
      ObjPtr<mirror::Class> method_reference_class =
          class_linker->ResolveType(method_type_idx, referrer);
      if (UNLIKELY(method_reference_class == nullptr)) {
        // Bad type idx.
        CHECK(self->IsExceptionPending());
        return nullptr;
      } else if (!method_reference_class->IsInterface()) {
        // It is not an interface. If the referring class is in the class hierarchy of the
        // referenced class in the bytecode, we use its super class. Otherwise, we throw
        // a NoSuchMethodError.
        ObjPtr<mirror::Class> super_class = nullptr;
        if (method_reference_class->IsAssignableFrom(h_referring_class.Get())) {
          super_class = h_referring_class->GetSuperClass();
        }
        uint16_t vtable_index = resolved_method->GetMethodIndex();
        if (access_check) {
          // Check existence of super class.
          if (super_class == nullptr ||
              !super_class->HasVTable() ||
              vtable_index >= static_cast<uint32_t>(super_class->GetVTableLength())) {
            // Behavior to agree with that of the verifier.
            ThrowNoSuchMethodError(type, resolved_method->GetDeclaringClass(),
                                   resolved_method->GetName(), resolved_method->GetSignature());
            return nullptr;  // Failure.
          }
        }
        DCHECK(super_class != nullptr);
        DCHECK(super_class->HasVTable());
        return super_class->GetVTableEntry(vtable_index, class_linker->GetImagePointerSize());
      } else {
        // It is an interface.
        if (access_check) {
          if (!method_reference_class->IsAssignableFrom(h_this->GetClass())) {
            ThrowIncompatibleClassChangeErrorClassForInterfaceSuper(resolved_method,
                                                                    method_reference_class,
                                                                    h_this.Get(),
                                                                    referrer);
            return nullptr;  // Failure.
          }
        }
        // TODO We can do better than this for a (compiled) fastpath.
        ArtMethod* result = method_reference_class->FindVirtualMethodForInterfaceSuper(
            resolved_method, class_linker->GetImagePointerSize());
        // Throw an NSME if nullptr;
        if (result == nullptr) {
          ThrowNoSuchMethodError(type, resolved_method->GetDeclaringClass(),
                                 resolved_method->GetName(), resolved_method->GetSignature());
        }
        return result;
      }
      UNREACHABLE();
    }
    case kInterface: {
      uint32_t imt_index = ImTable::GetImtIndex(resolved_method);
      PointerSize pointer_size = class_linker->GetImagePointerSize();
      ObjPtr<mirror::Class> klass = (*this_object)->GetClass();
      ArtMethod* imt_method = klass->GetImt(pointer_size)->Get(imt_index, pointer_size);
      if (!imt_method->IsRuntimeMethod()) {
        if (kIsDebugBuild) {
          ArtMethod* method = klass->FindVirtualMethodForInterface(
              resolved_method, class_linker->GetImagePointerSize());
          CHECK_EQ(imt_method, method) << ArtMethod::PrettyMethod(resolved_method) << " / "
                                       << imt_method->PrettyMethod() << " / "
                                       << ArtMethod::PrettyMethod(method) << " / "
                                       << klass->PrettyClass();
        }
        return imt_method;
      } else {
        ArtMethod* interface_method = klass->FindVirtualMethodForInterface(
            resolved_method, class_linker->GetImagePointerSize());
        if (UNLIKELY(interface_method == nullptr)) {
          ThrowIncompatibleClassChangeErrorClassForInterfaceDispatch(resolved_method,
                                                                     *this_object, referrer);
          return nullptr;  // Failure.
        }
        return interface_method;
      }
    }
    default:
      LOG(FATAL) << "Unknown invoke type " << type;
      return nullptr;  // Failure.
  }
}

// Explicit template declarations of FindMethodFromCode for all invoke types.
#define EXPLICIT_FIND_METHOD_FROM_CODE_TEMPLATE_DECL(_type, _access_check)                 \
  template REQUIRES_SHARED(Locks::mutator_lock_) ALWAYS_INLINE                       \
  ArtMethod* FindMethodFromCode<_type, _access_check>(uint32_t method_idx,         \
                                                      ObjPtr<mirror::Object>* this_object, \
                                                      ArtMethod* referrer, \
                                                      Thread* self)
#define EXPLICIT_FIND_METHOD_FROM_CODE_TYPED_TEMPLATE_DECL(_type) \
    EXPLICIT_FIND_METHOD_FROM_CODE_TEMPLATE_DECL(_type, false);   \
    EXPLICIT_FIND_METHOD_FROM_CODE_TEMPLATE_DECL(_type, true)

EXPLICIT_FIND_METHOD_FROM_CODE_TYPED_TEMPLATE_DECL(kStatic);
EXPLICIT_FIND_METHOD_FROM_CODE_TYPED_TEMPLATE_DECL(kDirect);
EXPLICIT_FIND_METHOD_FROM_CODE_TYPED_TEMPLATE_DECL(kVirtual);
EXPLICIT_FIND_METHOD_FROM_CODE_TYPED_TEMPLATE_DECL(kSuper);
EXPLICIT_FIND_METHOD_FROM_CODE_TYPED_TEMPLATE_DECL(kInterface);

#undef EXPLICIT_FIND_METHOD_FROM_CODE_TYPED_TEMPLATE_DECL
#undef EXPLICIT_FIND_METHOD_FROM_CODE_TEMPLATE_DECL

// Fast path field resolution that can't initialize classes or throw exceptions.
inline ArtField* FindFieldFast(uint32_t field_idx, ArtMethod* referrer, FindFieldType type,
                               size_t expected_size) {
  ScopedAssertNoThreadSuspension ants(__FUNCTION__);
  ArtField* resolved_field =
      referrer->GetDexCache()->GetResolvedField(field_idx, kRuntimePointerSize);
  if (UNLIKELY(resolved_field == nullptr)) {
    return nullptr;
  }
  // Check for incompatible class change.
  bool is_primitive;
  bool is_set;
  bool is_static;
  switch (type) {
    case InstanceObjectRead:     is_primitive = false; is_set = false; is_static = false; break;
    case InstanceObjectWrite:    is_primitive = false; is_set = true;  is_static = false; break;
    case InstancePrimitiveRead:  is_primitive = true;  is_set = false; is_static = false; break;
    case InstancePrimitiveWrite: is_primitive = true;  is_set = true;  is_static = false; break;
    case StaticObjectRead:       is_primitive = false; is_set = false; is_static = true;  break;
    case StaticObjectWrite:      is_primitive = false; is_set = true;  is_static = true;  break;
    case StaticPrimitiveRead:    is_primitive = true;  is_set = false; is_static = true;  break;
    case StaticPrimitiveWrite:   is_primitive = true;  is_set = true;  is_static = true;  break;
    default:
      LOG(FATAL) << "UNREACHABLE";
      UNREACHABLE();
  }
  if (UNLIKELY(resolved_field->IsStatic() != is_static)) {
    // Incompatible class change.
    return nullptr;
  }
  ObjPtr<mirror::Class> fields_class = resolved_field->GetDeclaringClass();
  if (is_static) {
    // Check class is initialized else fail so that we can contend to initialize the class with
    // other threads that may be racing to do this.
    if (UNLIKELY(!fields_class->IsInitialized())) {
      return nullptr;
    }
  }
  ObjPtr<mirror::Class> referring_class = referrer->GetDeclaringClass();
  if (UNLIKELY(!referring_class->CanAccess(fields_class) ||
               !referring_class->CanAccessMember(fields_class, resolved_field->GetAccessFlags()) ||
               (is_set && resolved_field->IsFinal() && (fields_class != referring_class)))) {
    // Illegal access.
    return nullptr;
  }
  if (UNLIKELY(resolved_field->IsPrimitiveType() != is_primitive ||
               resolved_field->FieldSize() != expected_size)) {
    return nullptr;
  }
  return resolved_field;
}

// Fast path method resolution that can't throw exceptions.
template <InvokeType type, bool access_check>
inline ArtMethod* FindMethodFast(uint32_t method_idx,
                                 ObjPtr<mirror::Object> this_object,
                                 ArtMethod* referrer) {
  ScopedAssertNoThreadSuspension ants(__FUNCTION__);
  if (UNLIKELY(this_object == nullptr && type != kStatic)) {
    return nullptr;
  }
  ObjPtr<mirror::Class> referring_class = referrer->GetDeclaringClass();
  ObjPtr<mirror::DexCache> dex_cache = referrer->GetDexCache();
  constexpr ClassLinker::ResolveMode resolve_mode = access_check
      ? ClassLinker::ResolveMode::kCheckICCEAndIAE
      : ClassLinker::ResolveMode::kNoChecks;
  ClassLinker* linker = Runtime::Current()->GetClassLinker();
  ArtMethod* resolved_method = linker->GetResolvedMethod<type, resolve_mode>(method_idx, referrer);
  if (UNLIKELY(resolved_method == nullptr)) {
    return nullptr;
  }
  if (type == kInterface) {  // Most common form of slow path dispatch.
    return this_object->GetClass()->FindVirtualMethodForInterface(resolved_method,
                                                                  kRuntimePointerSize);
  } else if (type == kStatic || type == kDirect) {
    return resolved_method;
  } else if (type == kSuper) {
    // TODO This lookup is rather slow.
    dex::TypeIndex method_type_idx = dex_cache->GetDexFile()->GetMethodId(method_idx).class_idx_;
    ObjPtr<mirror::Class> method_reference_class = linker->LookupResolvedType(
        method_type_idx, dex_cache, referrer->GetClassLoader());
    if (method_reference_class == nullptr) {
      // Need to do full type resolution...
      return nullptr;
    } else if (!method_reference_class->IsInterface()) {
      // It is not an interface. If the referring class is in the class hierarchy of the
      // referenced class in the bytecode, we use its super class. Otherwise, we cannot
      // resolve the method.
      if (!method_reference_class->IsAssignableFrom(referring_class)) {
        return nullptr;
      }
      ObjPtr<mirror::Class> super_class = referring_class->GetSuperClass();
      if (resolved_method->GetMethodIndex() >= super_class->GetVTableLength()) {
        // The super class does not have the method.
        return nullptr;
      }
      return super_class->GetVTableEntry(resolved_method->GetMethodIndex(), kRuntimePointerSize);
    } else {
      return method_reference_class->FindVirtualMethodForInterfaceSuper(
          resolved_method, kRuntimePointerSize);
    }
  } else {
    DCHECK(type == kVirtual);
    return this_object->GetClass()->GetVTableEntry(
        resolved_method->GetMethodIndex(), kRuntimePointerSize);
  }
}

inline ObjPtr<mirror::Class> ResolveVerifyAndClinit(dex::TypeIndex type_idx,
                                                    ArtMethod* referrer,
                                                    Thread* self,
                                                    bool can_run_clinit,
                                                    bool verify_access) {
  ClassLinker* class_linker = Runtime::Current()->GetClassLinker();
  ObjPtr<mirror::Class> klass = class_linker->ResolveType(type_idx, referrer);
  if (UNLIKELY(klass == nullptr)) {
    CHECK(self->IsExceptionPending());
    return nullptr;  // Failure - Indicate to caller to deliver exception
  }
  // Perform access check if necessary.
  mirror::Class* referring_class = referrer->GetDeclaringClass();
  if (verify_access && UNLIKELY(!referring_class->CanAccess(klass))) {
    ThrowIllegalAccessErrorClass(referring_class, klass);
    return nullptr;  // Failure - Indicate to caller to deliver exception
  }
  // If we're just implementing const-class, we shouldn't call <clinit>.
  if (!can_run_clinit) {
    return klass;
  }
  // If we are the <clinit> of this class, just return our storage.
  //
  // Do not set the DexCache InitializedStaticStorage, since that implies <clinit> has finished
  // running.
  if (klass == referring_class && referrer->IsConstructor() && referrer->IsStatic()) {
    return klass;
  }
  StackHandleScope<1> hs(self);
  Handle<mirror::Class> h_class(hs.NewHandle(klass));
  if (!class_linker->EnsureInitialized(self, h_class, true, true)) {
    CHECK(self->IsExceptionPending());
    return nullptr;  // Failure - Indicate to caller to deliver exception
  }
  return h_class.Get();
}

static inline ObjPtr<mirror::String> ResolveString(ClassLinker* class_linker,
                                                   dex::StringIndex string_idx,
                                                   ArtMethod* referrer)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  Thread::PoisonObjectPointersIfDebug();
  ObjPtr<mirror::String> string = referrer->GetDexCache()->GetResolvedString(string_idx);
  if (UNLIKELY(string == nullptr)) {
    StackHandleScope<1> hs(Thread::Current());
    Handle<mirror::DexCache> dex_cache(hs.NewHandle(referrer->GetDexCache()));
    string = class_linker->ResolveString(string_idx, dex_cache);
  }
  return string;
}

inline ObjPtr<mirror::String> ResolveStringFromCode(ArtMethod* referrer,
                                                    dex::StringIndex string_idx) {
  Thread::PoisonObjectPointersIfDebug();
  ObjPtr<mirror::String> string = referrer->GetDexCache()->GetResolvedString(string_idx);
  if (UNLIKELY(string == nullptr)) {
    StackHandleScope<1> hs(Thread::Current());
    Handle<mirror::DexCache> dex_cache(hs.NewHandle(referrer->GetDexCache()));
    ClassLinker* class_linker = Runtime::Current()->GetClassLinker();
    string = class_linker->ResolveString(string_idx, dex_cache);
  }
  return string;
}

inline void UnlockJniSynchronizedMethod(jobject locked, Thread* self) {
  // Save any pending exception over monitor exit call.
  mirror::Throwable* saved_exception = nullptr;
  if (UNLIKELY(self->IsExceptionPending())) {
    saved_exception = self->GetException();
    self->ClearException();
  }
  // Decode locked object and unlock, before popping local references.
  self->DecodeJObject(locked)->MonitorExit(self);
  if (UNLIKELY(self->IsExceptionPending())) {
    LOG(FATAL) << "Synchronized JNI code returning with an exception:\n"
        << saved_exception->Dump()
        << "\nEncountered second exception during implicit MonitorExit:\n"
        << self->GetException()->Dump();
  }
  // Restore pending exception.
  if (saved_exception != nullptr) {
    self->SetException(saved_exception);
  }
}

template <typename INT_TYPE, typename FLOAT_TYPE>
inline INT_TYPE art_float_to_integral(FLOAT_TYPE f) {
  const INT_TYPE kMaxInt = static_cast<INT_TYPE>(std::numeric_limits<INT_TYPE>::max());
  const INT_TYPE kMinInt = static_cast<INT_TYPE>(std::numeric_limits<INT_TYPE>::min());
  const FLOAT_TYPE kMaxIntAsFloat = static_cast<FLOAT_TYPE>(kMaxInt);
  const FLOAT_TYPE kMinIntAsFloat = static_cast<FLOAT_TYPE>(kMinInt);
  if (LIKELY(f > kMinIntAsFloat)) {
     if (LIKELY(f < kMaxIntAsFloat)) {
       return static_cast<INT_TYPE>(f);
     } else {
       return kMaxInt;
     }
  } else {
    return (f != f) ? 0 : kMinInt;  // f != f implies NaN
  }
}

}  // namespace art

#endif  // ART_RUNTIME_ENTRYPOINTS_ENTRYPOINT_UTILS_INL_H_
