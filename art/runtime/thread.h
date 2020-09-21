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

#ifndef ART_RUNTIME_THREAD_H_
#define ART_RUNTIME_THREAD_H_

#include <setjmp.h>

#include <bitset>
#include <deque>
#include <iosfwd>
#include <list>
#include <memory>
#include <string>

#include "arch/context.h"
#include "arch/instruction_set.h"
#include "base/atomic.h"
#include "base/enums.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "entrypoints/jni/jni_entrypoints.h"
#include "entrypoints/quick/quick_entrypoints.h"
#include "globals.h"
#include "handle_scope.h"
#include "instrumentation.h"
#include "jvalue.h"
#include "managed_stack.h"
#include "offsets.h"
#include "read_barrier_config.h"
#include "runtime_stats.h"
#include "suspend_reason.h"
#include "thread_state.h"

class BacktraceMap;

namespace art {

namespace gc {
namespace accounting {
template<class T> class AtomicStack;
}  // namespace accounting
namespace collector {
class SemiSpace;
}  // namespace collector
}  // namespace gc

namespace mirror {
class Array;
class Class;
class ClassLoader;
class Object;
template<class T> class ObjectArray;
template<class T> class PrimitiveArray;
typedef PrimitiveArray<int32_t> IntArray;
class StackTraceElement;
class String;
class Throwable;
}  // namespace mirror

namespace verifier {
class MethodVerifier;
class VerifierDeps;
}  // namespace verifier

class ArtMethod;
class BaseMutex;
class ClassLinker;
class Closure;
class Context;
struct DebugInvokeReq;
class DeoptimizationContextRecord;
class DexFile;
class FrameIdToShadowFrame;
class JavaVMExt;
class JNIEnvExt;
class Monitor;
class RootVisitor;
class ScopedObjectAccessAlreadyRunnable;
class ShadowFrame;
class SingleStepControl;
class StackedShadowFrameRecord;
class Thread;
class ThreadList;
enum VisitRootFlags : uint8_t;

// Thread priorities. These must match the Thread.MIN_PRIORITY,
// Thread.NORM_PRIORITY, and Thread.MAX_PRIORITY constants.
enum ThreadPriority {
  kMinThreadPriority = 1,
  kNormThreadPriority = 5,
  kMaxThreadPriority = 10,
};

enum ThreadFlag {
  kSuspendRequest   = 1,  // If set implies that suspend_count_ > 0 and the Thread should enter the
                          // safepoint handler.
  kCheckpointRequest = 2,  // Request that the thread do some checkpoint work and then continue.
  kEmptyCheckpointRequest = 4,  // Request that the thread do empty checkpoint and then continue.
  kActiveSuspendBarrier = 8,  // Register that at least 1 suspend barrier needs to be passed.
};

enum class StackedShadowFrameType {
  kShadowFrameUnderConstruction,
  kDeoptimizationShadowFrame,
};

// The type of method that triggers deoptimization. It contains info on whether
// the deoptimized method should advance dex_pc.
enum class DeoptimizationMethodType {
  kKeepDexPc,  // dex pc is required to be kept upon deoptimization.
  kDefault     // dex pc may or may not advance depending on other conditions.
};

// This should match RosAlloc::kNumThreadLocalSizeBrackets.
static constexpr size_t kNumRosAllocThreadLocalSizeBracketsInThread = 16;

// Thread's stack layout for implicit stack overflow checks:
//
//   +---------------------+  <- highest address of stack memory
//   |                     |
//   .                     .  <- SP
//   |                     |
//   |                     |
//   +---------------------+  <- stack_end
//   |                     |
//   |  Gap                |
//   |                     |
//   +---------------------+  <- stack_begin
//   |                     |
//   | Protected region    |
//   |                     |
//   +---------------------+  <- lowest address of stack memory
//
// The stack always grows down in memory.  At the lowest address is a region of memory
// that is set mprotect(PROT_NONE).  Any attempt to read/write to this region will
// result in a segmentation fault signal.  At any point, the thread's SP will be somewhere
// between the stack_end and the highest address in stack memory.  An implicit stack
// overflow check is a read of memory at a certain offset below the current SP (4K typically).
// If the thread's SP is below the stack_end address this will be a read into the protected
// region.  If the SP is above the stack_end address, the thread is guaranteed to have
// at least 4K of space.  Because stack overflow checks are only performed in generated code,
// if the thread makes a call out to a native function (through JNI), that native function
// might only have 4K of memory (if the SP is adjacent to stack_end).

class Thread {
 public:
  static const size_t kStackOverflowImplicitCheckSize;
  static constexpr bool kVerifyStack = kIsDebugBuild;

  // Creates a new native thread corresponding to the given managed peer.
  // Used to implement Thread.start.
  static void CreateNativeThread(JNIEnv* env, jobject peer, size_t stack_size, bool daemon);

  // Attaches the calling native thread to the runtime, returning the new native peer.
  // Used to implement JNI AttachCurrentThread and AttachCurrentThreadAsDaemon calls.
  static Thread* Attach(const char* thread_name, bool as_daemon, jobject thread_group,
                        bool create_peer);
  // Attaches the calling native thread to the runtime, returning the new native peer.
  static Thread* Attach(const char* thread_name, bool as_daemon, jobject thread_peer);

  // Reset internal state of child thread after fork.
  void InitAfterFork();

  // Get the currently executing thread, frequently referred to as 'self'. This call has reasonably
  // high cost and so we favor passing self around when possible.
  // TODO: mark as PURE so the compiler may coalesce and remove?
  static Thread* Current();

  // On a runnable thread, check for pending thread suspension request and handle if pending.
  void AllowThreadSuspension() REQUIRES_SHARED(Locks::mutator_lock_);

  // Process pending thread suspension request and handle if pending.
  void CheckSuspend() REQUIRES_SHARED(Locks::mutator_lock_);

  // Process a pending empty checkpoint if pending.
  void CheckEmptyCheckpointFromWeakRefAccess(BaseMutex* cond_var_mutex);
  void CheckEmptyCheckpointFromMutex();

  static Thread* FromManagedThread(const ScopedObjectAccessAlreadyRunnable& ts,
                                   ObjPtr<mirror::Object> thread_peer)
      REQUIRES(Locks::thread_list_lock_, !Locks::thread_suspend_count_lock_)
      REQUIRES_SHARED(Locks::mutator_lock_);
  static Thread* FromManagedThread(const ScopedObjectAccessAlreadyRunnable& ts, jobject thread)
      REQUIRES(Locks::thread_list_lock_, !Locks::thread_suspend_count_lock_)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Translates 172 to pAllocArrayFromCode and so on.
  template<PointerSize size_of_pointers>
  static void DumpThreadOffset(std::ostream& os, uint32_t offset);

  // Dumps a one-line summary of thread state (used for operator<<).
  void ShortDump(std::ostream& os) const;

  // Dumps the detailed thread state and the thread stack (used for SIGQUIT).
  void Dump(std::ostream& os,
            bool dump_native_stack = true,
            BacktraceMap* backtrace_map = nullptr,
            bool force_dump_stack = false) const
      REQUIRES(!Locks::thread_suspend_count_lock_)
      REQUIRES_SHARED(Locks::mutator_lock_);

  void DumpJavaStack(std::ostream& os,
                     bool check_suspended = true,
                     bool dump_locks = true) const
      REQUIRES(!Locks::thread_suspend_count_lock_)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Dumps the SIGQUIT per-thread header. 'thread' can be null for a non-attached thread, in which
  // case we use 'tid' to identify the thread, and we'll include as much information as we can.
  static void DumpState(std::ostream& os, const Thread* thread, pid_t tid)
      REQUIRES(!Locks::thread_suspend_count_lock_)
      REQUIRES_SHARED(Locks::mutator_lock_);

  ThreadState GetState() const {
    DCHECK_GE(tls32_.state_and_flags.as_struct.state, kTerminated);
    DCHECK_LE(tls32_.state_and_flags.as_struct.state, kSuspended);
    return static_cast<ThreadState>(tls32_.state_and_flags.as_struct.state);
  }

  ThreadState SetState(ThreadState new_state);

  int GetSuspendCount() const REQUIRES(Locks::thread_suspend_count_lock_) {
    return tls32_.suspend_count;
  }

  int GetUserCodeSuspendCount() const REQUIRES(Locks::thread_suspend_count_lock_,
                                               Locks::user_code_suspension_lock_) {
    return tls32_.user_code_suspend_count;
  }

  int GetDebugSuspendCount() const REQUIRES(Locks::thread_suspend_count_lock_) {
    return tls32_.debug_suspend_count;
  }

  bool IsSuspended() const {
    union StateAndFlags state_and_flags;
    state_and_flags.as_int = tls32_.state_and_flags.as_int;
    return state_and_flags.as_struct.state != kRunnable &&
        (state_and_flags.as_struct.flags & kSuspendRequest) != 0;
  }

  // If delta > 0 and (this != self or suspend_barrier is not null), this function may temporarily
  // release thread_suspend_count_lock_ internally.
  ALWAYS_INLINE
  bool ModifySuspendCount(Thread* self,
                          int delta,
                          AtomicInteger* suspend_barrier,
                          SuspendReason reason)
      WARN_UNUSED
      REQUIRES(Locks::thread_suspend_count_lock_);

  // Requests a checkpoint closure to run on another thread. The closure will be run when the thread
  // gets suspended. This will return true if the closure was added and will (eventually) be
  // executed. It returns false otherwise.
  //
  // Since multiple closures can be queued and some closures can delay other threads from running no
  // closure should attempt to suspend another thread while running.
  // TODO We should add some debug option that verifies this.
  bool RequestCheckpoint(Closure* function)
      REQUIRES(Locks::thread_suspend_count_lock_);

  // RequestSynchronousCheckpoint releases the thread_list_lock_ as a part of its execution. This is
  // due to the fact that Thread::Current() needs to go to sleep to allow the targeted thread to
  // execute the checkpoint for us if it is Runnable. The suspend_state is the state that the thread
  // will go into while it is awaiting the checkpoint to be run.
  // NB Passing ThreadState::kRunnable may cause the current thread to wait in a condition variable
  // while holding the mutator_lock_.  Callers should ensure that this will not cause any problems
  // for the closure or the rest of the system.
  // NB Since multiple closures can be queued and some closures can delay other threads from running
  // no closure should attempt to suspend another thread while running.
  bool RequestSynchronousCheckpoint(Closure* function,
                                    ThreadState suspend_state = ThreadState::kWaiting)
      REQUIRES_SHARED(Locks::mutator_lock_)
      RELEASE(Locks::thread_list_lock_)
      REQUIRES(!Locks::thread_suspend_count_lock_);

  bool RequestEmptyCheckpoint()
      REQUIRES(Locks::thread_suspend_count_lock_);

  void SetFlipFunction(Closure* function);
  Closure* GetFlipFunction();

  gc::accounting::AtomicStack<mirror::Object>* GetThreadLocalMarkStack() {
    CHECK(kUseReadBarrier);
    return tlsPtr_.thread_local_mark_stack;
  }
  void SetThreadLocalMarkStack(gc::accounting::AtomicStack<mirror::Object>* stack) {
    CHECK(kUseReadBarrier);
    tlsPtr_.thread_local_mark_stack = stack;
  }

  // Called when thread detected that the thread_suspend_count_ was non-zero. Gives up share of
  // mutator_lock_ and waits until it is resumed and thread_suspend_count_ is zero.
  void FullSuspendCheck()
      REQUIRES(!Locks::thread_suspend_count_lock_)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Transition from non-runnable to runnable state acquiring share on mutator_lock_.
  ALWAYS_INLINE ThreadState TransitionFromSuspendedToRunnable()
      REQUIRES(!Locks::thread_suspend_count_lock_)
      SHARED_LOCK_FUNCTION(Locks::mutator_lock_);

  // Transition from runnable into a state where mutator privileges are denied. Releases share of
  // mutator lock.
  ALWAYS_INLINE void TransitionFromRunnableToSuspended(ThreadState new_state)
      REQUIRES(!Locks::thread_suspend_count_lock_, !Roles::uninterruptible_)
      UNLOCK_FUNCTION(Locks::mutator_lock_);

  // Once called thread suspension will cause an assertion failure.
  const char* StartAssertNoThreadSuspension(const char* cause) ACQUIRE(Roles::uninterruptible_) {
    Roles::uninterruptible_.Acquire();  // No-op.
    if (kIsDebugBuild) {
      CHECK(cause != nullptr);
      const char* previous_cause = tlsPtr_.last_no_thread_suspension_cause;
      tls32_.no_thread_suspension++;
      tlsPtr_.last_no_thread_suspension_cause = cause;
      return previous_cause;
    } else {
      return nullptr;
    }
  }

  // End region where no thread suspension is expected.
  void EndAssertNoThreadSuspension(const char* old_cause) RELEASE(Roles::uninterruptible_) {
    if (kIsDebugBuild) {
      CHECK(old_cause != nullptr || tls32_.no_thread_suspension == 1);
      CHECK_GT(tls32_.no_thread_suspension, 0U);
      tls32_.no_thread_suspension--;
      tlsPtr_.last_no_thread_suspension_cause = old_cause;
    }
    Roles::uninterruptible_.Release();  // No-op.
  }

  void AssertThreadSuspensionIsAllowable(bool check_locks = true) const;

  // Return true if thread suspension is allowable.
  bool IsThreadSuspensionAllowable() const;

  bool IsDaemon() const {
    return tls32_.daemon;
  }

  size_t NumberOfHeldMutexes() const;

  bool HoldsLock(ObjPtr<mirror::Object> object) const REQUIRES_SHARED(Locks::mutator_lock_);

  /*
   * Changes the priority of this thread to match that of the java.lang.Thread object.
   *
   * We map a priority value from 1-10 to Linux "nice" values, where lower
   * numbers indicate higher priority.
   */
  void SetNativePriority(int newPriority);

  /*
   * Returns the thread priority for the current thread by querying the system.
   * This is useful when attaching a thread through JNI.
   *
   * Returns a value from 1 to 10 (compatible with java.lang.Thread values).
   */
  static int GetNativePriority();

  // Guaranteed to be non-zero.
  uint32_t GetThreadId() const {
    return tls32_.thin_lock_thread_id;
  }

  pid_t GetTid() const {
    return tls32_.tid;
  }

  // Returns the java.lang.Thread's name, or null if this Thread* doesn't have a peer.
  mirror::String* GetThreadName() const REQUIRES_SHARED(Locks::mutator_lock_);

  // Sets 'name' to the java.lang.Thread's name. This requires no transition to managed code,
  // allocation, or locking.
  void GetThreadName(std::string& name) const;

  // Sets the thread's name.
  void SetThreadName(const char* name) REQUIRES_SHARED(Locks::mutator_lock_);

  // Returns the thread-specific CPU-time clock in microseconds or -1 if unavailable.
  uint64_t GetCpuMicroTime() const;

  mirror::Object* GetPeer() const REQUIRES_SHARED(Locks::mutator_lock_) {
    DCHECK(Thread::Current() == this) << "Use GetPeerFromOtherThread instead";
    CHECK(tlsPtr_.jpeer == nullptr);
    return tlsPtr_.opeer;
  }
  // GetPeer is not safe if called on another thread in the middle of the CC thread flip and
  // the thread's stack may have not been flipped yet and peer may be a from-space (stale) ref.
  // This function will explicitly mark/forward it.
  mirror::Object* GetPeerFromOtherThread() const REQUIRES_SHARED(Locks::mutator_lock_);

  bool HasPeer() const {
    return tlsPtr_.jpeer != nullptr || tlsPtr_.opeer != nullptr;
  }

  RuntimeStats* GetStats() {
    return &tls64_.stats;
  }

  bool IsStillStarting() const;

  bool IsExceptionPending() const {
    return tlsPtr_.exception != nullptr;
  }

  bool IsAsyncExceptionPending() const {
    return tlsPtr_.async_exception != nullptr;
  }

  mirror::Throwable* GetException() const REQUIRES_SHARED(Locks::mutator_lock_) {
    return tlsPtr_.exception;
  }

  void AssertPendingException() const;
  void AssertPendingOOMException() const REQUIRES_SHARED(Locks::mutator_lock_);
  void AssertNoPendingException() const;
  void AssertNoPendingExceptionForNewException(const char* msg) const;

  void SetException(ObjPtr<mirror::Throwable> new_exception) REQUIRES_SHARED(Locks::mutator_lock_);

  // Set an exception that is asynchronously thrown from a different thread. This will be checked
  // periodically and might overwrite the current 'Exception'. This can only be called from a
  // checkpoint.
  //
  // The caller should also make sure that the thread has been deoptimized so that the exception
  // could be detected on back-edges.
  void SetAsyncException(ObjPtr<mirror::Throwable> new_exception)
      REQUIRES_SHARED(Locks::mutator_lock_);

  void ClearException() REQUIRES_SHARED(Locks::mutator_lock_) {
    tlsPtr_.exception = nullptr;
  }

  // Move the current async-exception to the main exception. This should be called when the current
  // thread is ready to deal with any async exceptions. Returns true if there is an async exception
  // that needs to be dealt with, false otherwise.
  bool ObserveAsyncException() REQUIRES_SHARED(Locks::mutator_lock_);

  // Find catch block and perform long jump to appropriate exception handle
  NO_RETURN void QuickDeliverException() REQUIRES_SHARED(Locks::mutator_lock_);

  Context* GetLongJumpContext();
  void ReleaseLongJumpContext(Context* context) {
    if (tlsPtr_.long_jump_context != nullptr) {
      // Each QuickExceptionHandler gets a long jump context and uses
      // it for doing the long jump, after finding catch blocks/doing deoptimization.
      // Both finding catch blocks and deoptimization can trigger another
      // exception such as a result of class loading. So there can be nested
      // cases of exception handling and multiple contexts being used.
      // ReleaseLongJumpContext tries to save the context in tlsPtr_.long_jump_context
      // for reuse so there is no need to always allocate a new one each time when
      // getting a context. Since we only keep one context for reuse, delete the
      // existing one since the passed in context is yet to be used for longjump.
      delete tlsPtr_.long_jump_context;
    }
    tlsPtr_.long_jump_context = context;
  }

  // Get the current method and dex pc. If there are errors in retrieving the dex pc, this will
  // abort the runtime iff abort_on_error is true.
  ArtMethod* GetCurrentMethod(uint32_t* dex_pc,
                              bool check_suspended = true,
                              bool abort_on_error = true) const
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Returns whether the given exception was thrown by the current Java method being executed
  // (Note that this includes native Java methods).
  bool IsExceptionThrownByCurrentMethod(ObjPtr<mirror::Throwable> exception) const
      REQUIRES_SHARED(Locks::mutator_lock_);

  void SetTopOfStack(ArtMethod** top_method) {
    tlsPtr_.managed_stack.SetTopQuickFrame(top_method);
  }

  void SetTopOfStackTagged(ArtMethod** top_method) {
    tlsPtr_.managed_stack.SetTopQuickFrameTagged(top_method);
  }

  void SetTopOfShadowStack(ShadowFrame* top) {
    tlsPtr_.managed_stack.SetTopShadowFrame(top);
  }

  bool HasManagedStack() const {
    return tlsPtr_.managed_stack.HasTopQuickFrame() || tlsPtr_.managed_stack.HasTopShadowFrame();
  }

  // If 'msg' is null, no detail message is set.
  void ThrowNewException(const char* exception_class_descriptor, const char* msg)
      REQUIRES_SHARED(Locks::mutator_lock_) REQUIRES(!Roles::uninterruptible_);

  // If 'msg' is null, no detail message is set. An exception must be pending, and will be
  // used as the new exception's cause.
  void ThrowNewWrappedException(const char* exception_class_descriptor, const char* msg)
      REQUIRES_SHARED(Locks::mutator_lock_) REQUIRES(!Roles::uninterruptible_);

  void ThrowNewExceptionF(const char* exception_class_descriptor, const char* fmt, ...)
      __attribute__((format(printf, 3, 4)))
      REQUIRES_SHARED(Locks::mutator_lock_) REQUIRES(!Roles::uninterruptible_);

  void ThrowNewExceptionV(const char* exception_class_descriptor, const char* fmt, va_list ap)
      REQUIRES_SHARED(Locks::mutator_lock_) REQUIRES(!Roles::uninterruptible_);

  // OutOfMemoryError is special, because we need to pre-allocate an instance.
  // Only the GC should call this.
  void ThrowOutOfMemoryError(const char* msg) REQUIRES_SHARED(Locks::mutator_lock_)
      REQUIRES(!Roles::uninterruptible_);

  static void Startup();
  static void FinishStartup();
  static void Shutdown();

  // Notify this thread's thread-group that this thread has started.
  // Note: the given thread-group is used as a fast path and verified in debug build. If the value
  //       is null, the thread's thread-group is loaded from the peer.
  void NotifyThreadGroup(ScopedObjectAccessAlreadyRunnable& soa, jobject thread_group = nullptr)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // JNI methods
  JNIEnvExt* GetJniEnv() const {
    return tlsPtr_.jni_env;
  }

  // Convert a jobject into a Object*
  ObjPtr<mirror::Object> DecodeJObject(jobject obj) const REQUIRES_SHARED(Locks::mutator_lock_);
  // Checks if the weak global ref has been cleared by the GC without decoding it.
  bool IsJWeakCleared(jweak obj) const REQUIRES_SHARED(Locks::mutator_lock_);

  mirror::Object* GetMonitorEnterObject() const REQUIRES_SHARED(Locks::mutator_lock_) {
    return tlsPtr_.monitor_enter_object;
  }

  void SetMonitorEnterObject(mirror::Object* obj) REQUIRES_SHARED(Locks::mutator_lock_) {
    tlsPtr_.monitor_enter_object = obj;
  }

  // Implements java.lang.Thread.interrupted.
  bool Interrupted();
  // Implements java.lang.Thread.isInterrupted.
  bool IsInterrupted();
  void Interrupt(Thread* self) REQUIRES(!*wait_mutex_);
  void SetInterrupted(bool i) {
    tls32_.interrupted.StoreSequentiallyConsistent(i);
  }
  void Notify() REQUIRES(!*wait_mutex_);

  ALWAYS_INLINE void PoisonObjectPointers() {
    ++poison_object_cookie_;
  }

  ALWAYS_INLINE static void PoisonObjectPointersIfDebug();

  ALWAYS_INLINE uintptr_t GetPoisonObjectCookie() const {
    return poison_object_cookie_;
  }

 private:
  void NotifyLocked(Thread* self) REQUIRES(wait_mutex_);

 public:
  Mutex* GetWaitMutex() const LOCK_RETURNED(wait_mutex_) {
    return wait_mutex_;
  }

  ConditionVariable* GetWaitConditionVariable() const REQUIRES(wait_mutex_) {
    return wait_cond_;
  }

  Monitor* GetWaitMonitor() const REQUIRES(wait_mutex_) {
    return wait_monitor_;
  }

  void SetWaitMonitor(Monitor* mon) REQUIRES(wait_mutex_) {
    wait_monitor_ = mon;
  }

  // Waiter link-list support.
  Thread* GetWaitNext() const {
    return tlsPtr_.wait_next;
  }

  void SetWaitNext(Thread* next) {
    tlsPtr_.wait_next = next;
  }

  jobject GetClassLoaderOverride() {
    return tlsPtr_.class_loader_override;
  }

  void SetClassLoaderOverride(jobject class_loader_override);

  // Create the internal representation of a stack trace, that is more time
  // and space efficient to compute than the StackTraceElement[].
  template<bool kTransactionActive>
  jobject CreateInternalStackTrace(const ScopedObjectAccessAlreadyRunnable& soa) const
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Convert an internal stack trace representation (returned by CreateInternalStackTrace) to a
  // StackTraceElement[]. If output_array is null, a new array is created, otherwise as many
  // frames as will fit are written into the given array. If stack_depth is non-null, it's updated
  // with the number of valid frames in the returned array.
  static jobjectArray InternalStackTraceToStackTraceElementArray(
      const ScopedObjectAccessAlreadyRunnable& soa, jobject internal,
      jobjectArray output_array = nullptr, int* stack_depth = nullptr)
      REQUIRES_SHARED(Locks::mutator_lock_);

  jobjectArray CreateAnnotatedStackTrace(const ScopedObjectAccessAlreadyRunnable& soa) const
      REQUIRES_SHARED(Locks::mutator_lock_);

  bool HasDebuggerShadowFrames() const {
    return tlsPtr_.frame_id_to_shadow_frame != nullptr;
  }

  void VisitRoots(RootVisitor* visitor, VisitRootFlags flags)
      REQUIRES_SHARED(Locks::mutator_lock_);

  void VerifyStack() REQUIRES_SHARED(Locks::mutator_lock_) {
    if (kVerifyStack) {
      VerifyStackImpl();
    }
  }

  //
  // Offsets of various members of native Thread class, used by compiled code.
  //

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> ThinLockIdOffset() {
    return ThreadOffset<pointer_size>(
        OFFSETOF_MEMBER(Thread, tls32_) +
        OFFSETOF_MEMBER(tls_32bit_sized_values, thin_lock_thread_id));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> InterruptedOffset() {
    return ThreadOffset<pointer_size>(
        OFFSETOF_MEMBER(Thread, tls32_) +
        OFFSETOF_MEMBER(tls_32bit_sized_values, interrupted));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> ThreadFlagsOffset() {
    return ThreadOffset<pointer_size>(
        OFFSETOF_MEMBER(Thread, tls32_) +
        OFFSETOF_MEMBER(tls_32bit_sized_values, state_and_flags));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> IsGcMarkingOffset() {
    return ThreadOffset<pointer_size>(
        OFFSETOF_MEMBER(Thread, tls32_) +
        OFFSETOF_MEMBER(tls_32bit_sized_values, is_gc_marking));
  }

  static constexpr size_t IsGcMarkingSize() {
    return sizeof(tls32_.is_gc_marking);
  }

  // Deoptimize the Java stack.
  void DeoptimizeWithDeoptimizationException(JValue* result) REQUIRES_SHARED(Locks::mutator_lock_);

 private:
  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> ThreadOffsetFromTlsPtr(size_t tls_ptr_offset) {
    size_t base = OFFSETOF_MEMBER(Thread, tlsPtr_);
    size_t scale;
    size_t shrink;
    if (pointer_size == kRuntimePointerSize) {
      scale = 1;
      shrink = 1;
    } else if (pointer_size > kRuntimePointerSize) {
      scale = static_cast<size_t>(pointer_size) / static_cast<size_t>(kRuntimePointerSize);
      shrink = 1;
    } else {
      DCHECK_GT(kRuntimePointerSize, pointer_size);
      scale = 1;
      shrink = static_cast<size_t>(kRuntimePointerSize) / static_cast<size_t>(pointer_size);
    }
    return ThreadOffset<pointer_size>(base + ((tls_ptr_offset * scale) / shrink));
  }

 public:
  static uint32_t QuickEntryPointOffsetWithSize(size_t quick_entrypoint_offset,
                                                PointerSize pointer_size) {
    if (pointer_size == PointerSize::k32) {
      return QuickEntryPointOffset<PointerSize::k32>(quick_entrypoint_offset).
          Uint32Value();
    } else {
      return QuickEntryPointOffset<PointerSize::k64>(quick_entrypoint_offset).
          Uint32Value();
    }
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> QuickEntryPointOffset(size_t quick_entrypoint_offset) {
    return ThreadOffsetFromTlsPtr<pointer_size>(
        OFFSETOF_MEMBER(tls_ptr_sized_values, quick_entrypoints) + quick_entrypoint_offset);
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> JniEntryPointOffset(size_t jni_entrypoint_offset) {
    return ThreadOffsetFromTlsPtr<pointer_size>(
        OFFSETOF_MEMBER(tls_ptr_sized_values, jni_entrypoints) + jni_entrypoint_offset);
  }

  // Return the entry point offset integer value for ReadBarrierMarkRegX, where X is `reg`.
  template <PointerSize pointer_size>
  static int32_t ReadBarrierMarkEntryPointsOffset(size_t reg) {
    // The entry point list defines 30 ReadBarrierMarkRegX entry points.
    DCHECK_LT(reg, 30u);
    // The ReadBarrierMarkRegX entry points are ordered by increasing
    // register number in Thread::tls_Ptr_.quick_entrypoints.
    return QUICK_ENTRYPOINT_OFFSET(pointer_size, pReadBarrierMarkReg00).Int32Value()
        + static_cast<size_t>(pointer_size) * reg;
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> SelfOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values, self));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> MterpCurrentIBaseOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(
        OFFSETOF_MEMBER(tls_ptr_sized_values, mterp_current_ibase));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> MterpDefaultIBaseOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(
        OFFSETOF_MEMBER(tls_ptr_sized_values, mterp_default_ibase));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> MterpAltIBaseOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(
        OFFSETOF_MEMBER(tls_ptr_sized_values, mterp_alt_ibase));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> ExceptionOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values, exception));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> PeerOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values, opeer));
  }


  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> CardTableOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values, card_table));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> ThreadSuspendTriggerOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(
        OFFSETOF_MEMBER(tls_ptr_sized_values, suspend_trigger));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> ThreadLocalPosOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values,
                                                                thread_local_pos));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> ThreadLocalEndOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values,
                                                                thread_local_end));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> ThreadLocalObjectsOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values,
                                                                thread_local_objects));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> RosAllocRunsOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values,
                                                                rosalloc_runs));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> ThreadLocalAllocStackTopOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values,
                                                                thread_local_alloc_stack_top));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> ThreadLocalAllocStackEndOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values,
                                                                thread_local_alloc_stack_end));
  }

  // Size of stack less any space reserved for stack overflow
  size_t GetStackSize() const {
    return tlsPtr_.stack_size - (tlsPtr_.stack_end - tlsPtr_.stack_begin);
  }

  uint8_t* GetStackEndForInterpreter(bool implicit_overflow_check) const {
    if (implicit_overflow_check) {
      // The interpreter needs the extra overflow bytes that stack_end does
      // not include.
      return tlsPtr_.stack_end + GetStackOverflowReservedBytes(kRuntimeISA);
    } else {
      return tlsPtr_.stack_end;
    }
  }

  uint8_t* GetStackEnd() const {
    return tlsPtr_.stack_end;
  }

  // Set the stack end to that to be used during a stack overflow
  void SetStackEndForStackOverflow() REQUIRES_SHARED(Locks::mutator_lock_);

  // Set the stack end to that to be used during regular execution
  void ResetDefaultStackEnd() {
    // Our stacks grow down, so we want stack_end_ to be near there, but reserving enough room
    // to throw a StackOverflowError.
    tlsPtr_.stack_end = tlsPtr_.stack_begin + GetStackOverflowReservedBytes(kRuntimeISA);
  }

  bool IsHandlingStackOverflow() const {
    return tlsPtr_.stack_end == tlsPtr_.stack_begin;
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> StackEndOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(
        OFFSETOF_MEMBER(tls_ptr_sized_values, stack_end));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> JniEnvOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(
        OFFSETOF_MEMBER(tls_ptr_sized_values, jni_env));
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> TopOfManagedStackOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(
        OFFSETOF_MEMBER(tls_ptr_sized_values, managed_stack) +
        ManagedStack::TaggedTopQuickFrameOffset());
  }

  const ManagedStack* GetManagedStack() const {
    return &tlsPtr_.managed_stack;
  }

  // Linked list recording fragments of managed stack.
  void PushManagedStackFragment(ManagedStack* fragment) {
    tlsPtr_.managed_stack.PushManagedStackFragment(fragment);
  }
  void PopManagedStackFragment(const ManagedStack& fragment) {
    tlsPtr_.managed_stack.PopManagedStackFragment(fragment);
  }

  ALWAYS_INLINE ShadowFrame* PushShadowFrame(ShadowFrame* new_top_frame);
  ALWAYS_INLINE ShadowFrame* PopShadowFrame();

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> TopShadowFrameOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(
        OFFSETOF_MEMBER(tls_ptr_sized_values, managed_stack) +
        ManagedStack::TopShadowFrameOffset());
  }

  // Is the given obj in this thread's stack indirect reference table?
  bool HandleScopeContains(jobject obj) const;

  void HandleScopeVisitRoots(RootVisitor* visitor, pid_t thread_id)
      REQUIRES_SHARED(Locks::mutator_lock_);

  BaseHandleScope* GetTopHandleScope() {
    return tlsPtr_.top_handle_scope;
  }

  void PushHandleScope(BaseHandleScope* handle_scope) {
    DCHECK_EQ(handle_scope->GetLink(), tlsPtr_.top_handle_scope);
    tlsPtr_.top_handle_scope = handle_scope;
  }

  BaseHandleScope* PopHandleScope() {
    BaseHandleScope* handle_scope = tlsPtr_.top_handle_scope;
    DCHECK(handle_scope != nullptr);
    tlsPtr_.top_handle_scope = tlsPtr_.top_handle_scope->GetLink();
    return handle_scope;
  }

  template<PointerSize pointer_size>
  static ThreadOffset<pointer_size> TopHandleScopeOffset() {
    return ThreadOffsetFromTlsPtr<pointer_size>(OFFSETOF_MEMBER(tls_ptr_sized_values,
                                                                top_handle_scope));
  }

  DebugInvokeReq* GetInvokeReq() const {
    return tlsPtr_.debug_invoke_req;
  }

  SingleStepControl* GetSingleStepControl() const {
    return tlsPtr_.single_step_control;
  }

  // Indicates whether this thread is ready to invoke a method for debugging. This
  // is only true if the thread has been suspended by a debug event.
  bool IsReadyForDebugInvoke() const {
    return tls32_.ready_for_debug_invoke;
  }

  void SetReadyForDebugInvoke(bool ready) {
    tls32_.ready_for_debug_invoke = ready;
  }

  bool IsDebugMethodEntry() const {
    return tls32_.debug_method_entry_;
  }

  void SetDebugMethodEntry() {
    tls32_.debug_method_entry_ = true;
  }

  void ClearDebugMethodEntry() {
    tls32_.debug_method_entry_ = false;
  }

  bool GetIsGcMarking() const {
    CHECK(kUseReadBarrier);
    return tls32_.is_gc_marking;
  }

  void SetIsGcMarkingAndUpdateEntrypoints(bool is_marking);

  bool GetWeakRefAccessEnabled() const {
    CHECK(kUseReadBarrier);
    return tls32_.weak_ref_access_enabled;
  }

  void SetWeakRefAccessEnabled(bool enabled) {
    CHECK(kUseReadBarrier);
    tls32_.weak_ref_access_enabled = enabled;
  }

  uint32_t GetDisableThreadFlipCount() const {
    CHECK(kUseReadBarrier);
    return tls32_.disable_thread_flip_count;
  }

  void IncrementDisableThreadFlipCount() {
    CHECK(kUseReadBarrier);
    ++tls32_.disable_thread_flip_count;
  }

  void DecrementDisableThreadFlipCount() {
    CHECK(kUseReadBarrier);
    DCHECK_GT(tls32_.disable_thread_flip_count, 0U);
    --tls32_.disable_thread_flip_count;
  }

  // Returns true if the thread is allowed to call into java.
  bool CanCallIntoJava() const {
    return can_call_into_java_;
  }

  void SetCanCallIntoJava(bool can_call_into_java) {
    can_call_into_java_ = can_call_into_java;
  }

  // Activates single step control for debugging. The thread takes the
  // ownership of the given SingleStepControl*. It is deleted by a call
  // to DeactivateSingleStepControl or upon thread destruction.
  void ActivateSingleStepControl(SingleStepControl* ssc);

  // Deactivates single step control for debugging.
  void DeactivateSingleStepControl();

  // Sets debug invoke request for debugging. When the thread is resumed,
  // it executes the method described by this request then sends the reply
  // before suspending itself. The thread takes the ownership of the given
  // DebugInvokeReq*. It is deleted by a call to ClearDebugInvokeReq.
  void SetDebugInvokeReq(DebugInvokeReq* req);

  // Clears debug invoke request for debugging. When the thread completes
  // method invocation, it deletes its debug invoke request and suspends
  // itself.
  void ClearDebugInvokeReq();

  // Returns the fake exception used to activate deoptimization.
  static mirror::Throwable* GetDeoptimizationException() {
    // Note that the mirror::Throwable must be aligned to kObjectAlignment or else it cannot be
    // represented by ObjPtr.
    return reinterpret_cast<mirror::Throwable*>(0x100);
  }

  // Currently deoptimization invokes verifier which can trigger class loading
  // and execute Java code, so there might be nested deoptimizations happening.
  // We need to save the ongoing deoptimization shadow frames and return
  // values on stacks.
  // 'from_code' denotes whether the deoptimization was explicitly made from
  // compiled code.
  // 'method_type' contains info on whether deoptimization should advance
  // dex_pc.
  void PushDeoptimizationContext(const JValue& return_value,
                                 bool is_reference,
                                 ObjPtr<mirror::Throwable> exception,
                                 bool from_code,
                                 DeoptimizationMethodType method_type)
      REQUIRES_SHARED(Locks::mutator_lock_);
  void PopDeoptimizationContext(JValue* result,
                                ObjPtr<mirror::Throwable>* exception,
                                bool* from_code,
                                DeoptimizationMethodType* method_type)
      REQUIRES_SHARED(Locks::mutator_lock_);
  void AssertHasDeoptimizationContext()
      REQUIRES_SHARED(Locks::mutator_lock_);
  void PushStackedShadowFrame(ShadowFrame* sf, StackedShadowFrameType type);
  ShadowFrame* PopStackedShadowFrame(StackedShadowFrameType type, bool must_be_present = true);

  // For debugger, find the shadow frame that corresponds to a frame id.
  // Or return null if there is none.
  ShadowFrame* FindDebuggerShadowFrame(size_t frame_id)
      REQUIRES_SHARED(Locks::mutator_lock_);
  // For debugger, find the bool array that keeps track of the updated vreg set
  // for a frame id.
  bool* GetUpdatedVRegFlags(size_t frame_id) REQUIRES_SHARED(Locks::mutator_lock_);
  // For debugger, find the shadow frame that corresponds to a frame id. If
  // one doesn't exist yet, create one and track it in frame_id_to_shadow_frame.
  ShadowFrame* FindOrCreateDebuggerShadowFrame(size_t frame_id,
                                               uint32_t num_vregs,
                                               ArtMethod* method,
                                               uint32_t dex_pc)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Delete the entry that maps from frame_id to shadow_frame.
  void RemoveDebuggerShadowFrameMapping(size_t frame_id)
      REQUIRES_SHARED(Locks::mutator_lock_);

  std::deque<instrumentation::InstrumentationStackFrame>* GetInstrumentationStack() {
    return tlsPtr_.instrumentation_stack;
  }

  std::vector<ArtMethod*>* GetStackTraceSample() const {
    DCHECK(!IsAotCompiler());
    return tlsPtr_.deps_or_stack_trace_sample.stack_trace_sample;
  }

  void SetStackTraceSample(std::vector<ArtMethod*>* sample) {
    DCHECK(!IsAotCompiler());
    tlsPtr_.deps_or_stack_trace_sample.stack_trace_sample = sample;
  }

  verifier::VerifierDeps* GetVerifierDeps() const {
    DCHECK(IsAotCompiler());
    return tlsPtr_.deps_or_stack_trace_sample.verifier_deps;
  }

  // It is the responsability of the caller to make sure the verifier_deps
  // entry in the thread is cleared before destruction of the actual VerifierDeps
  // object, or the thread.
  void SetVerifierDeps(verifier::VerifierDeps* verifier_deps) {
    DCHECK(IsAotCompiler());
    DCHECK(verifier_deps == nullptr || tlsPtr_.deps_or_stack_trace_sample.verifier_deps == nullptr);
    tlsPtr_.deps_or_stack_trace_sample.verifier_deps = verifier_deps;
  }

  uint64_t GetTraceClockBase() const {
    return tls64_.trace_clock_base;
  }

  void SetTraceClockBase(uint64_t clock_base) {
    tls64_.trace_clock_base = clock_base;
  }

  BaseMutex* GetHeldMutex(LockLevel level) const {
    return tlsPtr_.held_mutexes[level];
  }

  void SetHeldMutex(LockLevel level, BaseMutex* mutex) {
    tlsPtr_.held_mutexes[level] = mutex;
  }

  void ClearSuspendBarrier(AtomicInteger* target)
      REQUIRES(Locks::thread_suspend_count_lock_);

  bool ReadFlag(ThreadFlag flag) const {
    return (tls32_.state_and_flags.as_struct.flags & flag) != 0;
  }

  bool TestAllFlags() const {
    return (tls32_.state_and_flags.as_struct.flags != 0);
  }

  void AtomicSetFlag(ThreadFlag flag) {
    tls32_.state_and_flags.as_atomic_int.FetchAndBitwiseOrSequentiallyConsistent(flag);
  }

  void AtomicClearFlag(ThreadFlag flag) {
    tls32_.state_and_flags.as_atomic_int.FetchAndBitwiseAndSequentiallyConsistent(-1 ^ flag);
  }

  void ResetQuickAllocEntryPointsForThread(bool is_marking);

  // Returns the remaining space in the TLAB.
  size_t TlabSize() const {
    return tlsPtr_.thread_local_end - tlsPtr_.thread_local_pos;
  }

  // Returns the remaining space in the TLAB if we were to expand it to maximum capacity.
  size_t TlabRemainingCapacity() const {
    return tlsPtr_.thread_local_limit - tlsPtr_.thread_local_pos;
  }

  // Expand the TLAB by a fixed number of bytes. There must be enough capacity to do so.
  void ExpandTlab(size_t bytes) {
    tlsPtr_.thread_local_end += bytes;
    DCHECK_LE(tlsPtr_.thread_local_end, tlsPtr_.thread_local_limit);
  }

  // Doesn't check that there is room.
  mirror::Object* AllocTlab(size_t bytes);
  void SetTlab(uint8_t* start, uint8_t* end, uint8_t* limit);
  bool HasTlab() const;
  uint8_t* GetTlabStart() {
    return tlsPtr_.thread_local_start;
  }
  uint8_t* GetTlabPos() {
    return tlsPtr_.thread_local_pos;
  }

  // Remove the suspend trigger for this thread by making the suspend_trigger_ TLS value
  // equal to a valid pointer.
  // TODO: does this need to atomic?  I don't think so.
  void RemoveSuspendTrigger() {
    tlsPtr_.suspend_trigger = reinterpret_cast<uintptr_t*>(&tlsPtr_.suspend_trigger);
  }

  // Trigger a suspend check by making the suspend_trigger_ TLS value an invalid pointer.
  // The next time a suspend check is done, it will load from the value at this address
  // and trigger a SIGSEGV.
  void TriggerSuspend() {
    tlsPtr_.suspend_trigger = nullptr;
  }


  // Push an object onto the allocation stack.
  bool PushOnThreadLocalAllocationStack(mirror::Object* obj)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Set the thread local allocation pointers to the given pointers.
  void SetThreadLocalAllocationStack(StackReference<mirror::Object>* start,
                                     StackReference<mirror::Object>* end);

  // Resets the thread local allocation pointers.
  void RevokeThreadLocalAllocationStack();

  size_t GetThreadLocalBytesAllocated() const {
    return tlsPtr_.thread_local_end - tlsPtr_.thread_local_start;
  }

  size_t GetThreadLocalObjectsAllocated() const {
    return tlsPtr_.thread_local_objects;
  }

  void* GetRosAllocRun(size_t index) const {
    return tlsPtr_.rosalloc_runs[index];
  }

  void SetRosAllocRun(size_t index, void* run) {
    tlsPtr_.rosalloc_runs[index] = run;
  }

  bool ProtectStack(bool fatal_on_error = true);
  bool UnprotectStack();

  void SetMterpDefaultIBase(void* ibase) {
    tlsPtr_.mterp_default_ibase = ibase;
  }

  void SetMterpCurrentIBase(void* ibase) {
    tlsPtr_.mterp_current_ibase = ibase;
  }

  void SetMterpAltIBase(void* ibase) {
    tlsPtr_.mterp_alt_ibase = ibase;
  }

  const void* GetMterpDefaultIBase() const {
    return tlsPtr_.mterp_default_ibase;
  }

  const void* GetMterpCurrentIBase() const {
    return tlsPtr_.mterp_current_ibase;
  }

  const void* GetMterpAltIBase() const {
    return tlsPtr_.mterp_alt_ibase;
  }

  bool HandlingSignal() const {
    return tls32_.handling_signal_;
  }

  void SetHandlingSignal(bool handling_signal) {
    tls32_.handling_signal_ = handling_signal;
  }

  bool IsTransitioningToRunnable() const {
    return tls32_.is_transitioning_to_runnable;
  }

  void SetIsTransitioningToRunnable(bool value) {
    tls32_.is_transitioning_to_runnable = value;
  }

  void PushVerifier(verifier::MethodVerifier* verifier);
  void PopVerifier(verifier::MethodVerifier* verifier);

  void InitStringEntryPoints();

  void ModifyDebugDisallowReadBarrier(int8_t delta) {
    debug_disallow_read_barrier_ += delta;
  }

  uint8_t GetDebugDisallowReadBarrierCount() const {
    return debug_disallow_read_barrier_;
  }

  void* GetCustomTLS() const REQUIRES(Locks::thread_list_lock_) {
    return custom_tls_;
  }

  void SetCustomTLS(void* data) REQUIRES(Locks::thread_list_lock_) {
    custom_tls_ = data;
  }

  // Returns true if the current thread is the jit sensitive thread.
  bool IsJitSensitiveThread() const {
    return this == jit_sensitive_thread_;
  }

  // Returns true if StrictMode events are traced for the current thread.
  static bool IsSensitiveThread() {
    if (is_sensitive_thread_hook_ != nullptr) {
      return (*is_sensitive_thread_hook_)();
    }
    return false;
  }

  // Set to the read barrier marking entrypoints to be non-null.
  void SetReadBarrierEntrypoints();

  static jobject CreateCompileTimePeer(JNIEnv* env,
                                       const char* name,
                                       bool as_daemon,
                                       jobject thread_group)
      REQUIRES_SHARED(Locks::mutator_lock_);

 private:
  explicit Thread(bool daemon);
  ~Thread() REQUIRES(!Locks::mutator_lock_, !Locks::thread_suspend_count_lock_);
  void Destroy();

  // Attaches the calling native thread to the runtime, returning the new native peer.
  // Used to implement JNI AttachCurrentThread and AttachCurrentThreadAsDaemon calls.
  template <typename PeerAction>
  static Thread* Attach(const char* thread_name,
                        bool as_daemon,
                        PeerAction p);

  void CreatePeer(const char* name, bool as_daemon, jobject thread_group);

  template<bool kTransactionActive>
  static void InitPeer(ScopedObjectAccessAlreadyRunnable& soa,
                       ObjPtr<mirror::Object> peer,
                       jboolean thread_is_daemon,
                       jobject thread_group,
                       jobject thread_name,
                       jint thread_priority)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Avoid use, callers should use SetState. Used only by SignalCatcher::HandleSigQuit, ~Thread and
  // Dbg::ManageDeoptimization.
  ThreadState SetStateUnsafe(ThreadState new_state) {
    ThreadState old_state = GetState();
    if (old_state == kRunnable && new_state != kRunnable) {
      // Need to run pending checkpoint and suspend barriers. Run checkpoints in runnable state in
      // case they need to use a ScopedObjectAccess. If we are holding the mutator lock and a SOA
      // attempts to TransitionFromSuspendedToRunnable, it results in a deadlock.
      TransitionToSuspendedAndRunCheckpoints(new_state);
      // Since we transitioned to a suspended state, check the pass barrier requests.
      PassActiveSuspendBarriers();
    } else {
      tls32_.state_and_flags.as_struct.state = new_state;
    }
    return old_state;
  }

  void VerifyStackImpl() REQUIRES_SHARED(Locks::mutator_lock_);

  void DumpState(std::ostream& os) const REQUIRES_SHARED(Locks::mutator_lock_);
  void DumpStack(std::ostream& os,
                 bool dump_native_stack = true,
                 BacktraceMap* backtrace_map = nullptr,
                 bool force_dump_stack = false) const
      REQUIRES(!Locks::thread_suspend_count_lock_)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Out-of-line conveniences for debugging in gdb.
  static Thread* CurrentFromGdb();  // Like Thread::Current.
  // Like Thread::Dump(std::cerr).
  void DumpFromGdb() const REQUIRES_SHARED(Locks::mutator_lock_);

  static void* CreateCallback(void* arg);

  void HandleUncaughtExceptions(ScopedObjectAccessAlreadyRunnable& soa)
      REQUIRES_SHARED(Locks::mutator_lock_);
  void RemoveFromThreadGroup(ScopedObjectAccessAlreadyRunnable& soa)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Initialize a thread.
  //
  // The third parameter is not mandatory. If given, the thread will use this JNIEnvExt. In case
  // Init succeeds, this means the thread takes ownership of it. If Init fails, it is the caller's
  // responsibility to destroy the given JNIEnvExt. If the parameter is null, Init will try to
  // create a JNIEnvExt on its own (and potentially fail at that stage, indicated by a return value
  // of false).
  bool Init(ThreadList*, JavaVMExt*, JNIEnvExt* jni_env_ext = nullptr)
      REQUIRES(Locks::runtime_shutdown_lock_);
  void InitCardTable();
  void InitCpu();
  void CleanupCpu();
  void InitTlsEntryPoints();
  void InitTid();
  void InitPthreadKeySelf();
  bool InitStackHwm();

  void SetUpAlternateSignalStack();
  void TearDownAlternateSignalStack();

  ALWAYS_INLINE void TransitionToSuspendedAndRunCheckpoints(ThreadState new_state)
      REQUIRES(!Locks::thread_suspend_count_lock_, !Roles::uninterruptible_);

  ALWAYS_INLINE void PassActiveSuspendBarriers()
      REQUIRES(!Locks::thread_suspend_count_lock_, !Roles::uninterruptible_);

  // Registers the current thread as the jit sensitive thread. Should be called just once.
  static void SetJitSensitiveThread() {
    if (jit_sensitive_thread_ == nullptr) {
      jit_sensitive_thread_ = Thread::Current();
    } else {
      LOG(WARNING) << "Attempt to set the sensitive thread twice. Tid:"
          << Thread::Current()->GetTid();
    }
  }

  static void SetSensitiveThreadHook(bool (*is_sensitive_thread_hook)()) {
    is_sensitive_thread_hook_ = is_sensitive_thread_hook;
  }

  bool ModifySuspendCountInternal(Thread* self,
                                  int delta,
                                  AtomicInteger* suspend_barrier,
                                  SuspendReason reason)
      WARN_UNUSED
      REQUIRES(Locks::thread_suspend_count_lock_);

  // Runs a single checkpoint function. If there are no more pending checkpoint functions it will
  // clear the kCheckpointRequest flag. The caller is responsible for calling this in a loop until
  // the kCheckpointRequest flag is cleared.
  void RunCheckpointFunction();
  void RunEmptyCheckpoint();

  bool PassActiveSuspendBarriers(Thread* self)
      REQUIRES(!Locks::thread_suspend_count_lock_);

  // Install the protected region for implicit stack checks.
  void InstallImplicitProtection();

  template <bool kPrecise>
  void VisitRoots(RootVisitor* visitor) REQUIRES_SHARED(Locks::mutator_lock_);

  static bool IsAotCompiler();

  // 32 bits of atomically changed state and flags. Keeping as 32 bits allows and atomic CAS to
  // change from being Suspended to Runnable without a suspend request occurring.
  union PACKED(4) StateAndFlags {
    StateAndFlags() {}
    struct PACKED(4) {
      // Bitfield of flag values. Must be changed atomically so that flag values aren't lost. See
      // ThreadFlags for bit field meanings.
      volatile uint16_t flags;
      // Holds the ThreadState. May be changed non-atomically between Suspended (ie not Runnable)
      // transitions. Changing to Runnable requires that the suspend_request be part of the atomic
      // operation. If a thread is suspended and a suspend_request is present, a thread may not
      // change to Runnable as a GC or other operation is in progress.
      volatile uint16_t state;
    } as_struct;
    AtomicInteger as_atomic_int;
    volatile int32_t as_int;

   private:
    // gcc does not handle struct with volatile member assignments correctly.
    // See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=47409
    DISALLOW_COPY_AND_ASSIGN(StateAndFlags);
  };
  static_assert(sizeof(StateAndFlags) == sizeof(int32_t), "Weird state_and_flags size");

  static void ThreadExitCallback(void* arg);

  // Maximum number of suspend barriers.
  static constexpr uint32_t kMaxSuspendBarriers = 3;

  // Has Thread::Startup been called?
  static bool is_started_;

  // TLS key used to retrieve the Thread*.
  static pthread_key_t pthread_key_self_;

  // Used to notify threads that they should attempt to resume, they will suspend again if
  // their suspend count is > 0.
  static ConditionVariable* resume_cond_ GUARDED_BY(Locks::thread_suspend_count_lock_);

  // Hook passed by framework which returns true
  // when StrictMode events are traced for the current thread.
  static bool (*is_sensitive_thread_hook_)();
  // Stores the jit sensitive thread (which for now is the UI thread).
  static Thread* jit_sensitive_thread_;

  /***********************************************************************************************/
  // Thread local storage. Fields are grouped by size to enable 32 <-> 64 searching to account for
  // pointer size differences. To encourage shorter encoding, more frequently used values appear
  // first if possible.
  /***********************************************************************************************/

  struct PACKED(4) tls_32bit_sized_values {
    // We have no control over the size of 'bool', but want our boolean fields
    // to be 4-byte quantities.
    typedef uint32_t bool32_t;

    explicit tls_32bit_sized_values(bool is_daemon) :
      suspend_count(0), debug_suspend_count(0), thin_lock_thread_id(0), tid(0),
      daemon(is_daemon), throwing_OutOfMemoryError(false), no_thread_suspension(0),
      thread_exit_check_count(0), handling_signal_(false),
      is_transitioning_to_runnable(false), ready_for_debug_invoke(false),
      debug_method_entry_(false), is_gc_marking(false), weak_ref_access_enabled(true),
      disable_thread_flip_count(0), user_code_suspend_count(0) {
    }

    union StateAndFlags state_and_flags;
    static_assert(sizeof(union StateAndFlags) == sizeof(int32_t),
                  "Size of state_and_flags and int32 are different");

    // A non-zero value is used to tell the current thread to enter a safe point
    // at the next poll.
    int suspend_count GUARDED_BY(Locks::thread_suspend_count_lock_);

    // How much of 'suspend_count_' is by request of the debugger, used to set things right
    // when the debugger detaches. Must be <= suspend_count_.
    int debug_suspend_count GUARDED_BY(Locks::thread_suspend_count_lock_);

    // Thin lock thread id. This is a small integer used by the thin lock implementation.
    // This is not to be confused with the native thread's tid, nor is it the value returned
    // by java.lang.Thread.getId --- this is a distinct value, used only for locking. One
    // important difference between this id and the ids visible to managed code is that these
    // ones get reused (to ensure that they fit in the number of bits available).
    uint32_t thin_lock_thread_id;

    // System thread id.
    uint32_t tid;

    // Is the thread a daemon?
    const bool32_t daemon;

    // A boolean telling us whether we're recursively throwing OOME.
    bool32_t throwing_OutOfMemoryError;

    // A positive value implies we're in a region where thread suspension isn't expected.
    uint32_t no_thread_suspension;

    // How many times has our pthread key's destructor been called?
    uint32_t thread_exit_check_count;

    // True if signal is being handled by this thread.
    bool32_t handling_signal_;

    // True if the thread is in TransitionFromSuspendedToRunnable(). This is used to distinguish the
    // non-runnable threads (eg. kNative, kWaiting) that are about to transition to runnable from
    // the rest of them.
    bool32_t is_transitioning_to_runnable;

    // True if the thread has been suspended by a debugger event. This is
    // used to invoke method from the debugger which is only allowed when
    // the thread is suspended by an event.
    bool32_t ready_for_debug_invoke;

    // True if the thread enters a method. This is used to detect method entry
    // event for the debugger.
    bool32_t debug_method_entry_;

    // True if the GC is in the marking phase. This is used for the CC collector only. This is
    // thread local so that we can simplify the logic to check for the fast path of read barriers of
    // GC roots.
    bool32_t is_gc_marking;

    // Thread "interrupted" status; stays raised until queried or thrown.
    Atomic<bool32_t> interrupted;

    // True if the thread is allowed to access a weak ref (Reference::GetReferent() and system
    // weaks) and to potentially mark an object alive/gray. This is used for concurrent reference
    // processing of the CC collector only. This is thread local so that we can enable/disable weak
    // ref access by using a checkpoint and avoid a race around the time weak ref access gets
    // disabled and concurrent reference processing begins (if weak ref access is disabled during a
    // pause, this is not an issue.) Other collectors use Runtime::DisallowNewSystemWeaks() and
    // ReferenceProcessor::EnableSlowPath().
    bool32_t weak_ref_access_enabled;

    // A thread local version of Heap::disable_thread_flip_count_. This keeps track of how many
    // levels of (nested) JNI critical sections the thread is in and is used to detect a nested JNI
    // critical section enter.
    uint32_t disable_thread_flip_count;

    // How much of 'suspend_count_' is by request of user code, used to distinguish threads
    // suspended by the runtime from those suspended by user code.
    // This should have GUARDED_BY(Locks::user_code_suspension_lock_) but auto analysis cannot be
    // told that AssertHeld should be good enough.
    int user_code_suspend_count GUARDED_BY(Locks::thread_suspend_count_lock_);
  } tls32_;

  struct PACKED(8) tls_64bit_sized_values {
    tls_64bit_sized_values() : trace_clock_base(0) {
    }

    // The clock base used for tracing.
    uint64_t trace_clock_base;

    RuntimeStats stats;
  } tls64_;

  struct PACKED(sizeof(void*)) tls_ptr_sized_values {
      tls_ptr_sized_values() : card_table(nullptr), exception(nullptr), stack_end(nullptr),
      managed_stack(), suspend_trigger(nullptr), jni_env(nullptr), tmp_jni_env(nullptr),
      self(nullptr), opeer(nullptr), jpeer(nullptr), stack_begin(nullptr), stack_size(0),
      deps_or_stack_trace_sample(), wait_next(nullptr), monitor_enter_object(nullptr),
      top_handle_scope(nullptr), class_loader_override(nullptr), long_jump_context(nullptr),
      instrumentation_stack(nullptr), debug_invoke_req(nullptr), single_step_control(nullptr),
      stacked_shadow_frame_record(nullptr), deoptimization_context_stack(nullptr),
      frame_id_to_shadow_frame(nullptr), name(nullptr), pthread_self(0),
      last_no_thread_suspension_cause(nullptr), checkpoint_function(nullptr),
      thread_local_start(nullptr), thread_local_pos(nullptr), thread_local_end(nullptr),
      thread_local_limit(nullptr),
      thread_local_objects(0), mterp_current_ibase(nullptr), mterp_default_ibase(nullptr),
      mterp_alt_ibase(nullptr), thread_local_alloc_stack_top(nullptr),
      thread_local_alloc_stack_end(nullptr),
      flip_function(nullptr), method_verifier(nullptr), thread_local_mark_stack(nullptr),
      async_exception(nullptr) {
      std::fill(held_mutexes, held_mutexes + kLockLevelCount, nullptr);
    }

    // The biased card table, see CardTable for details.
    uint8_t* card_table;

    // The pending exception or null.
    mirror::Throwable* exception;

    // The end of this thread's stack. This is the lowest safely-addressable address on the stack.
    // We leave extra space so there's room for the code that throws StackOverflowError.
    uint8_t* stack_end;

    // The top of the managed stack often manipulated directly by compiler generated code.
    ManagedStack managed_stack;

    // In certain modes, setting this to 0 will trigger a SEGV and thus a suspend check.  It is
    // normally set to the address of itself.
    uintptr_t* suspend_trigger;

    // Every thread may have an associated JNI environment
    JNIEnvExt* jni_env;

    // Temporary storage to transfer a pre-allocated JNIEnvExt from the creating thread to the
    // created thread.
    JNIEnvExt* tmp_jni_env;

    // Initialized to "this". On certain architectures (such as x86) reading off of Thread::Current
    // is easy but getting the address of Thread::Current is hard. This field can be read off of
    // Thread::Current to give the address.
    Thread* self;

    // Our managed peer (an instance of java.lang.Thread). The jobject version is used during thread
    // start up, until the thread is registered and the local opeer_ is used.
    mirror::Object* opeer;
    jobject jpeer;

    // The "lowest addressable byte" of the stack.
    uint8_t* stack_begin;

    // Size of the stack.
    size_t stack_size;

    // Sampling profiler and AOT verification cannot happen on the same run, so we share
    // the same entry for the stack trace and the verifier deps.
    union DepsOrStackTraceSample {
      DepsOrStackTraceSample() {
        verifier_deps = nullptr;
        stack_trace_sample = nullptr;
      }
      // Pointer to previous stack trace captured by sampling profiler.
      std::vector<ArtMethod*>* stack_trace_sample;
      // When doing AOT verification, per-thread VerifierDeps.
      verifier::VerifierDeps* verifier_deps;
    } deps_or_stack_trace_sample;

    // The next thread in the wait set this thread is part of or null if not waiting.
    Thread* wait_next;

    // If we're blocked in MonitorEnter, this is the object we're trying to lock.
    mirror::Object* monitor_enter_object;

    // Top of linked list of handle scopes or null for none.
    BaseHandleScope* top_handle_scope;

    // Needed to get the right ClassLoader in JNI_OnLoad, but also
    // useful for testing.
    jobject class_loader_override;

    // Thread local, lazily allocated, long jump context. Used to deliver exceptions.
    Context* long_jump_context;

    // Additional stack used by method instrumentation to store method and return pc values.
    // Stored as a pointer since std::deque is not PACKED.
    std::deque<instrumentation::InstrumentationStackFrame>* instrumentation_stack;

    // JDWP invoke-during-breakpoint support.
    DebugInvokeReq* debug_invoke_req;

    // JDWP single-stepping support.
    SingleStepControl* single_step_control;

    // For gc purpose, a shadow frame record stack that keeps track of:
    // 1) shadow frames under construction.
    // 2) deoptimization shadow frames.
    StackedShadowFrameRecord* stacked_shadow_frame_record;

    // Deoptimization return value record stack.
    DeoptimizationContextRecord* deoptimization_context_stack;

    // For debugger, a linked list that keeps the mapping from frame_id to shadow frame.
    // Shadow frames may be created before deoptimization happens so that the debugger can
    // set local values there first.
    FrameIdToShadowFrame* frame_id_to_shadow_frame;

    // A cached copy of the java.lang.Thread's name.
    std::string* name;

    // A cached pthread_t for the pthread underlying this Thread*.
    pthread_t pthread_self;

    // If no_thread_suspension_ is > 0, what is causing that assertion.
    const char* last_no_thread_suspension_cause;

    // Pending checkpoint function or null if non-pending. If this checkpoint is set and someone\
    // requests another checkpoint, it goes to the checkpoint overflow list.
    Closure* checkpoint_function GUARDED_BY(Locks::thread_suspend_count_lock_);

    // Pending barriers that require passing or NULL if non-pending. Installation guarding by
    // Locks::thread_suspend_count_lock_.
    // They work effectively as art::Barrier, but implemented directly using AtomicInteger and futex
    // to avoid additional cost of a mutex and a condition variable, as used in art::Barrier.
    AtomicInteger* active_suspend_barriers[kMaxSuspendBarriers];

    // Thread-local allocation pointer. Moved here to force alignment for thread_local_pos on ARM.
    uint8_t* thread_local_start;

    // thread_local_pos and thread_local_end must be consecutive for ldrd and are 8 byte aligned for
    // potentially better performance.
    uint8_t* thread_local_pos;
    uint8_t* thread_local_end;

    // Thread local limit is how much we can expand the thread local buffer to, it is greater or
    // equal to thread_local_end.
    uint8_t* thread_local_limit;

    size_t thread_local_objects;

    // Entrypoint function pointers.
    // TODO: move this to more of a global offset table model to avoid per-thread duplication.
    JniEntryPoints jni_entrypoints;
    QuickEntryPoints quick_entrypoints;

    // Mterp jump table bases.
    void* mterp_current_ibase;
    void* mterp_default_ibase;
    void* mterp_alt_ibase;

    // There are RosAlloc::kNumThreadLocalSizeBrackets thread-local size brackets per thread.
    void* rosalloc_runs[kNumRosAllocThreadLocalSizeBracketsInThread];

    // Thread-local allocation stack data/routines.
    StackReference<mirror::Object>* thread_local_alloc_stack_top;
    StackReference<mirror::Object>* thread_local_alloc_stack_end;

    // Support for Mutex lock hierarchy bug detection.
    BaseMutex* held_mutexes[kLockLevelCount];

    // The function used for thread flip.
    Closure* flip_function;

    // Current method verifier, used for root marking.
    verifier::MethodVerifier* method_verifier;

    // Thread-local mark stack for the concurrent copying collector.
    gc::accounting::AtomicStack<mirror::Object>* thread_local_mark_stack;

    // The pending async-exception or null.
    mirror::Throwable* async_exception;
  } tlsPtr_;

  // Guards the 'wait_monitor_' members.
  Mutex* wait_mutex_ DEFAULT_MUTEX_ACQUIRED_AFTER;

  // Condition variable waited upon during a wait.
  ConditionVariable* wait_cond_ GUARDED_BY(wait_mutex_);
  // Pointer to the monitor lock we're currently waiting on or null if not waiting.
  Monitor* wait_monitor_ GUARDED_BY(wait_mutex_);

  // Debug disable read barrier count, only is checked for debug builds and only in the runtime.
  uint8_t debug_disallow_read_barrier_ = 0;

  // Note that it is not in the packed struct, may not be accessed for cross compilation.
  uintptr_t poison_object_cookie_ = 0;

  // Pending extra checkpoints if checkpoint_function_ is already used.
  std::list<Closure*> checkpoint_overflow_ GUARDED_BY(Locks::thread_suspend_count_lock_);

  // Custom TLS field that can be used by plugins.
  // TODO: Generalize once we have more plugins.
  void* custom_tls_;

  // True if the thread is allowed to call back into java (for e.g. during class resolution).
  // By default this is true.
  bool can_call_into_java_;

  friend class Dbg;  // For SetStateUnsafe.
  friend class gc::collector::SemiSpace;  // For getting stack traces.
  friend class Runtime;  // For CreatePeer.
  friend class QuickExceptionHandler;  // For dumping the stack.
  friend class ScopedThreadStateChange;
  friend class StubTest;  // For accessing entrypoints.
  friend class ThreadList;  // For ~Thread and Destroy.

  friend class EntrypointsOrderTest;  // To test the order of tls entries.

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

class SCOPED_CAPABILITY ScopedAssertNoThreadSuspension {
 public:
  ALWAYS_INLINE ScopedAssertNoThreadSuspension(const char* cause,
                                               bool enabled = true)
      ACQUIRE(Roles::uninterruptible_)
      : enabled_(enabled) {
    if (!enabled_) {
      return;
    }
    if (kIsDebugBuild) {
      self_ = Thread::Current();
      old_cause_ = self_->StartAssertNoThreadSuspension(cause);
    } else {
      Roles::uninterruptible_.Acquire();  // No-op.
    }
  }
  ALWAYS_INLINE ~ScopedAssertNoThreadSuspension() RELEASE(Roles::uninterruptible_) {
    if (!enabled_) {
      return;
    }
    if (kIsDebugBuild) {
      self_->EndAssertNoThreadSuspension(old_cause_);
    } else {
      Roles::uninterruptible_.Release();  // No-op.
    }
  }

 private:
  Thread* self_;
  const bool enabled_;
  const char* old_cause_;
};

class ScopedStackedShadowFramePusher {
 public:
  ScopedStackedShadowFramePusher(Thread* self, ShadowFrame* sf, StackedShadowFrameType type)
    : self_(self), type_(type) {
    self_->PushStackedShadowFrame(sf, type);
  }
  ~ScopedStackedShadowFramePusher() {
    self_->PopStackedShadowFrame(type_);
  }

 private:
  Thread* const self_;
  const StackedShadowFrameType type_;

  DISALLOW_COPY_AND_ASSIGN(ScopedStackedShadowFramePusher);
};

// Only works for debug builds.
class ScopedDebugDisallowReadBarriers {
 public:
  explicit ScopedDebugDisallowReadBarriers(Thread* self) : self_(self) {
    self_->ModifyDebugDisallowReadBarrier(1);
  }
  ~ScopedDebugDisallowReadBarriers() {
    self_->ModifyDebugDisallowReadBarrier(-1);
  }

 private:
  Thread* const self_;
};

class ScopedTransitioningToRunnable : public ValueObject {
 public:
  explicit ScopedTransitioningToRunnable(Thread* self)
      : self_(self) {
    DCHECK_EQ(self, Thread::Current());
    if (kUseReadBarrier) {
      self_->SetIsTransitioningToRunnable(true);
    }
  }

  ~ScopedTransitioningToRunnable() {
    if (kUseReadBarrier) {
      self_->SetIsTransitioningToRunnable(false);
    }
  }

 private:
  Thread* const self_;
};

class ThreadLifecycleCallback {
 public:
  virtual ~ThreadLifecycleCallback() {}

  virtual void ThreadStart(Thread* self) REQUIRES_SHARED(Locks::mutator_lock_) = 0;
  virtual void ThreadDeath(Thread* self) REQUIRES_SHARED(Locks::mutator_lock_) = 0;
};

std::ostream& operator<<(std::ostream& os, const Thread& thread);
std::ostream& operator<<(std::ostream& os, const StackedShadowFrameType& thread);

}  // namespace art

#endif  // ART_RUNTIME_THREAD_H_
