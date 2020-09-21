/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_LIST_H_
#define ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_LIST_H_

// All quick entrypoints. Format is name, return type, argument types.

#define QUICK_ENTRYPOINT_LIST(V) \
  V(AllocArrayResolved, void*, mirror::Class*, int32_t) \
  V(AllocArrayResolved8, void*, mirror::Class*, int32_t) \
  V(AllocArrayResolved16, void*, mirror::Class*, int32_t) \
  V(AllocArrayResolved32, void*, mirror::Class*, int32_t) \
  V(AllocArrayResolved64, void*, mirror::Class*, int32_t) \
  V(AllocObjectResolved, void*, mirror::Class*) \
  V(AllocObjectInitialized, void*, mirror::Class*) \
  V(AllocObjectWithChecks, void*, mirror::Class*) \
  V(AllocStringFromBytes, void*, void*, int32_t, int32_t, int32_t) \
  V(AllocStringFromChars, void*, int32_t, int32_t, void*) \
  V(AllocStringFromString, void*, void*) \
\
  V(InstanceofNonTrivial, size_t, mirror::Object*, mirror::Class*) \
  V(CheckInstanceOf, void, mirror::Object*, mirror::Class*) \
\
  V(InitializeStaticStorage, void*, uint32_t) \
  V(InitializeTypeAndVerifyAccess, void*, uint32_t) \
  V(InitializeType, void*, uint32_t) \
  V(ResolveString, void*, uint32_t) \
\
  V(Set8Instance, int, uint32_t, void*, int8_t) \
  V(Set8Static, int, uint32_t, int8_t) \
  V(Set16Instance, int, uint32_t, void*, int16_t) \
  V(Set16Static, int, uint32_t, int16_t) \
  V(Set32Instance, int, uint32_t, void*, int32_t) \
  V(Set32Static, int, uint32_t, int32_t) \
  V(Set64Instance, int, uint32_t, void*, int64_t) \
  V(Set64Static, int, uint32_t, int64_t) \
  V(SetObjInstance, int, uint32_t, void*, void*) \
  V(SetObjStatic, int, uint32_t, void*) \
  V(GetByteInstance, ssize_t, uint32_t, void*) \
  V(GetBooleanInstance, size_t, uint32_t, void*) \
  V(GetByteStatic, ssize_t, uint32_t) \
  V(GetBooleanStatic, size_t, uint32_t) \
  V(GetShortInstance, ssize_t, uint32_t, void*) \
  V(GetCharInstance, size_t, uint32_t, void*) \
  V(GetShortStatic, ssize_t, uint32_t) \
  V(GetCharStatic, size_t, uint32_t) \
  V(Get32Instance, ssize_t, uint32_t, void*) \
  V(Get32Static, ssize_t, uint32_t) \
  V(Get64Instance, int64_t, uint32_t, void*) \
  V(Get64Static, int64_t, uint32_t) \
  V(GetObjInstance, void*, uint32_t, void*) \
  V(GetObjStatic, void*, uint32_t) \
\
  V(AputObject, void, mirror::Array*, int32_t, mirror::Object*) \
\
  V(JniMethodStart, uint32_t, Thread*) \
  V(JniMethodFastStart, uint32_t, Thread*) \
  V(JniMethodStartSynchronized, uint32_t, jobject, Thread*) \
  V(JniMethodEnd, void, uint32_t, Thread*) \
  V(JniMethodFastEnd, void, uint32_t, Thread*) \
  V(JniMethodEndSynchronized, void, uint32_t, jobject, Thread*) \
  V(JniMethodEndWithReference, mirror::Object*, jobject, uint32_t, Thread*) \
  V(JniMethodFastEndWithReference, mirror::Object*, jobject, uint32_t, Thread*) \
  V(JniMethodEndWithReferenceSynchronized, mirror::Object*, jobject, uint32_t, jobject, Thread*) \
  V(QuickGenericJniTrampoline, void, ArtMethod*) \
\
  V(LockObject, void, mirror::Object*) \
  V(UnlockObject, void, mirror::Object*) \
\
  V(CmpgDouble, int32_t, double, double) \
  V(CmpgFloat, int32_t, float, float) \
  V(CmplDouble, int32_t, double, double) \
  V(CmplFloat, int32_t, float, float) \
  V(Cos, double, double) \
  V(Sin, double, double) \
  V(Acos, double, double) \
  V(Asin, double, double) \
  V(Atan, double, double) \
  V(Atan2, double, double, double) \
  V(Pow, double, double, double) \
  V(Cbrt, double, double) \
  V(Cosh, double, double) \
  V(Exp, double, double) \
  V(Expm1, double, double) \
  V(Hypot, double, double, double) \
  V(Log, double, double) \
  V(Log10, double, double) \
  V(NextAfter, double, double, double) \
  V(Sinh, double, double) \
  V(Tan, double, double) \
  V(Tanh, double, double) \
  V(Fmod, double, double, double) \
  V(L2d, double, int64_t) \
  V(Fmodf, float, float, float) \
  V(L2f, float, int64_t) \
  V(D2iz, int32_t, double) \
  V(F2iz, int32_t, float) \
  V(Idivmod, int32_t, int32_t, int32_t) \
  V(D2l, int64_t, double) \
  V(F2l, int64_t, float) \
  V(Ldiv, int64_t, int64_t, int64_t) \
  V(Lmod, int64_t, int64_t, int64_t) \
  V(Lmul, int64_t, int64_t, int64_t) \
  V(ShlLong, uint64_t, uint64_t, uint32_t) \
  V(ShrLong, uint64_t, uint64_t, uint32_t) \
  V(UshrLong, uint64_t, uint64_t, uint32_t) \
\
  V(IndexOf, int32_t, void*, uint32_t, uint32_t) \
  V(StringCompareTo, int32_t, void*, void*) \
  V(Memcpy, void*, void*, const void*, size_t) \
\
  V(QuickImtConflictTrampoline, void, ArtMethod*) \
  V(QuickResolutionTrampoline, void, ArtMethod*) \
  V(QuickToInterpreterBridge, void, ArtMethod*) \
  V(InvokeDirectTrampolineWithAccessCheck, void, uint32_t, void*) \
  V(InvokeInterfaceTrampolineWithAccessCheck, void, uint32_t, void*) \
  V(InvokeStaticTrampolineWithAccessCheck, void, uint32_t, void*) \
  V(InvokeSuperTrampolineWithAccessCheck, void, uint32_t, void*) \
  V(InvokeVirtualTrampolineWithAccessCheck, void, uint32_t, void*) \
  V(InvokePolymorphic, void, uint32_t, void*) \
\
  V(TestSuspend, void, void) \
\
  V(DeliverException, void, mirror::Object*) \
  V(ThrowArrayBounds, void, int32_t, int32_t) \
  V(ThrowDivZero, void, void) \
  V(ThrowNullPointer, void, void) \
  V(ThrowStackOverflow, void, void*) \
  V(ThrowStringBounds, void, int32_t, int32_t) \
  V(Deoptimize, void, DeoptimizationKind) \
\
  V(A64Load, int64_t, volatile const int64_t *) \
  V(A64Store, void, volatile int64_t *, int64_t) \
\
  V(NewEmptyString, void, void) \
  V(NewStringFromBytes_B, void, void) \
  V(NewStringFromBytes_BI, void, void) \
  V(NewStringFromBytes_BII, void, void) \
  V(NewStringFromBytes_BIII, void, void) \
  V(NewStringFromBytes_BIIString, void, void) \
  V(NewStringFromBytes_BString, void, void) \
  V(NewStringFromBytes_BIICharset, void, void) \
  V(NewStringFromBytes_BCharset, void, void) \
  V(NewStringFromChars_C, void, void) \
  V(NewStringFromChars_CII, void, void) \
  V(NewStringFromChars_IIC, void, void) \
  V(NewStringFromCodePoints, void, void) \
  V(NewStringFromString, void, void) \
  V(NewStringFromStringBuffer, void, void) \
  V(NewStringFromStringBuilder, void, void) \
\
  V(ReadBarrierJni, void, mirror::CompressedReference<mirror::Object>*, Thread*) \
  V(ReadBarrierMarkReg00, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg01, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg02, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg03, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg04, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg05, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg06, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg07, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg08, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg09, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg10, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg11, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg12, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg13, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg14, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg15, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg16, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg17, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg18, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg19, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg20, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg21, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg22, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg23, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg24, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg25, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg26, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg27, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg28, mirror::Object*, mirror::Object*) \
  V(ReadBarrierMarkReg29, mirror::Object*, mirror::Object*) \
  V(ReadBarrierSlow, mirror::Object*, mirror::Object*, mirror::Object*, uint32_t) \
  V(ReadBarrierForRootSlow, mirror::Object*, GcRoot<mirror::Object>*) \
\

#endif  // ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_LIST_H_
#undef ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_LIST_H_   // #define is only for lint.
