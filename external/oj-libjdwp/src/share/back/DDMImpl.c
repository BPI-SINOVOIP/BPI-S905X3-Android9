/*
 * Copyright (c) 1999, 2005, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include <stdatomic.h>

#include "util.h"
#include "DDMImpl.h"
#include "inStream.h"
#include "outStream.h"

static _Atomic(jboolean) ddmIsActive = ATOMIC_VAR_INIT(JNI_FALSE);

static void
SendDdmBroadcast(JNIEnv* env, jboolean connected)
{
  WITH_LOCAL_REFS(env, 2) {
    jclass ddm_class = JNI_FUNC_PTR(env,FindClass)(env, "org/apache/harmony/dalvik/ddmc/DdmServer");
    if (JNI_FUNC_PTR(env,ExceptionCheck)(env)) {
      JNI_FUNC_PTR(env,ExceptionClear)(env);
      goto end;
    }
    jmethodID broadcast = JNI_FUNC_PTR(env,GetStaticMethodID)(env, ddm_class, "broadcast", "(I)V");
    if (broadcast == NULL) {
      ERROR_MESSAGE(("JDWP Cannot find DdmServer.broadcast(I)V method!"));
      JNI_FUNC_PTR(env,ExceptionDescribe)(env);
      JNI_FUNC_PTR(env,ExceptionClear)(env);
      goto end;
    }
    jint event =
        connected == JNI_TRUE ? 1 /* DdmServer.CONNECTED */ : 2 /*DdmServer.DISCONNECTED */;
    JNI_FUNC_PTR(env,CallStaticVoidMethod)(env, ddm_class, broadcast, event);
    if (JNI_FUNC_PTR(env,ExceptionCheck)(env)) {
      LOG_ERROR(("DdmServer.broadcast %d failed", event));
      JNI_FUNC_PTR(env,ExceptionDescribe)(env);
      JNI_FUNC_PTR(env,ExceptionClear)(env);
    }
    if (!connected) {
      // If we are disconnecting we also need to call DdmVmInternal.threadNotify(false)
      jclass ddm_vm_internal =
          JNI_FUNC_PTR(env,FindClass)(env, "org/apache/harmony/dalvik/ddmc/DdmVmInternal");
      jmethodID thread_notify =
          JNI_FUNC_PTR(env,GetStaticMethodID)(env, ddm_vm_internal, "threadNotify", "(Z)V");
      if (thread_notify == NULL) {
        ERROR_MESSAGE(("JDWP Cannot find DdmVmInternal.threadNotify(Z)V method!"));
        JNI_FUNC_PTR(env,ExceptionDescribe)(env);
        JNI_FUNC_PTR(env,ExceptionClear)(env);
        goto end;
      }
      JNI_FUNC_PTR(env,CallStaticVoidMethod)(env, ddm_vm_internal, thread_notify, JNI_FALSE);
      if (JNI_FUNC_PTR(env,ExceptionCheck)(env)) {
        LOG_ERROR(("DdmVmInternal.threadNotify(false) failed"));
        JNI_FUNC_PTR(env,ExceptionDescribe)(env);
        JNI_FUNC_PTR(env,ExceptionClear)(env);
      }
    }
end: ;
  } END_WITH_LOCAL_REFS(env);
}

static jboolean
chunk(PacketInputStream *in, PacketOutputStream *out)
{
    int i;
    jint type_in;
    jint type_out;
    jint len_in;
    jint len_out;
    jbyte* data_in;
    jbyte* data_out;
    jvmtiError error;

    type_in = inStream_readInt(in);
    len_in = inStream_readInt(in);
    data_in = inStream_readBytes(in, len_in, (jbyte*)jvmtiAllocate(len_in));

    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    if (gdata->ddm_process_chunk == NULL) {
        jvmtiDeallocate(data_in);
        outStream_setError(out, JDWP_ERROR(NOT_IMPLEMENTED));
        return JNI_TRUE;
    }

    jboolean ddm_newly_active = !atomic_exchange(&ddmIsActive, JNI_TRUE);
    if (ddm_newly_active) {
      SendDdmBroadcast(getEnv(), /* connected */JNI_TRUE);
    }

    LOG_JVMTI(("com.android.art.internal.ddm.process_chunk()"));
    error = gdata->ddm_process_chunk(gdata->jvmti,
                                     type_in,
                                     len_in,
                                     data_in,
                                     &type_out,
                                     &len_out,
                                     &data_out);

    jvmtiDeallocate(data_in);

    if (error != JVMTI_ERROR_NONE) {
        // For backwards-compatibility we do not actually return any error or any data at all
        // here.
        LOG_MISC(("Suppressing error from com.android.art.internal.ddm.process_chunk for backwards "
                  "compatibility. Error was %s (%d)", jvmtiErrorText(error), error));
        return JNI_TRUE;
    }

    outStream_writeInt(out, type_out);
    outStream_writeByteArray(out, len_out, data_out);
    jvmtiDeallocate(data_out);

    return JNI_TRUE;
}

void DDM_initialize(void)
{
  atomic_exchange(&ddmIsActive, gdata->ddmInitiallyActive);
}

void DDM_onDisconnect(void)
{
  jboolean was_active = atomic_exchange(&ddmIsActive, JNI_FALSE);
  if (was_active) {
    JNIEnv* env = getEnv();
    SendDdmBroadcast(getEnv(), /*connected*/ JNI_FALSE);
  }
}

void *DDM_Cmds[] = { (void *)1
    ,(void *)chunk
};
