/*
 * Copyright (c) 1999, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "vmDebug.h"

#include "JDWP.h"
#include "debugLoop.h"
#include "transport.h"
#include "util.h"

static _Atomic(jlong) lastDebuggerActivity = ATOMIC_VAR_INIT(0LL);
static _Atomic(jboolean) hasSeenDebuggerActivity = ATOMIC_VAR_INIT(JNI_FALSE);

// Reset the tracking variables.
void vmDebug_onDisconnect()
{
    atomic_store(&lastDebuggerActivity, 0LL);
    atomic_store(&hasSeenDebuggerActivity, JNI_FALSE);
}

// Mark us as having seen actual debugger activity (so isDebuggerConnected can return true) and that
// we are currently doing something as the debugger.
void vmDebug_notifyDebuggerActivityStart()
{
    atomic_store(&lastDebuggerActivity, 0LL);
    atomic_store(&hasSeenDebuggerActivity, JNI_TRUE);
}

// Update the timestamp for the last debugger activity.
void vmDebug_notifyDebuggerActivityEnd()
{
    atomic_store(&lastDebuggerActivity, milliTime());
}

// For backwards compatibility we are only considered 'connected' as far as VMDebug is concerned if
// we have gotten at least one non-ddms JDWP packet.
static jboolean
isDebuggerConnected()
{
    return transport_is_open() && atomic_load(&hasSeenDebuggerActivity);
}

static jboolean JNICALL
VMDebug_isDebuggerConnected(JNIEnv* env, jclass klass)
{
    return isDebuggerConnected();
}

static jboolean JNICALL
VMDebug_isDebuggingEnabled(JNIEnv* env, jclass klass)
{
    // We are running the debugger so debugging is definitely enabled.
    return JNI_TRUE;
}

static jlong JNICALL
VMDebug_lastDebuggerActivity(JNIEnv* env, jclass klass)
{
    if (!isDebuggerConnected()) {
        LOG_ERROR(("VMDebug.lastDebuggerActivity called without active debugger"));
        return -1;
    }
    jlong last_time = atomic_load(&lastDebuggerActivity);
    if (last_time == 0) {
        LOG_MISC(("debugger is performing an action"));
        return 0;
    }

    jlong cur_time = milliTime();

    if (cur_time < last_time) {
        LOG_ERROR(("Time seemed to go backwards: last was %lld, current is %lld",
                   last_time, cur_time));
        return 0;
    }
    jlong res = cur_time - last_time;
    LOG_MISC(("Debugger interval is %lld", res));
    return res;
}

void
vmDebug_initalize(JNIEnv* env)
{
    WITH_LOCAL_REFS(env, 1) {
        jclass vmdebug_class = JNI_FUNC_PTR(env,FindClass)(env, "dalvik/system/VMDebug");
        if (vmdebug_class == NULL) {
            // The VMDebug class isn't available. We don't need to do anything.
            LOG_MISC(("dalvik.system.VMDebug does not seem to be available on this runtime."));
            // Get rid of the ClassNotFoundException.
            JNI_FUNC_PTR(env,ExceptionClear)(env);
            goto finish;
        }

        JNINativeMethod methods[3];

        // Take over the implementation of these three functions.
        methods[0].name = "lastDebuggerActivity";
        methods[0].signature = "()J";
        methods[0].fnPtr = (void*)VMDebug_lastDebuggerActivity;

        methods[1].name = "isDebuggingEnabled";
        methods[1].signature = "()Z";
        methods[1].fnPtr = (void*)VMDebug_isDebuggingEnabled;

        methods[2].name = "isDebuggerConnected";
        methods[2].signature = "()Z";
        methods[2].fnPtr = (void*)VMDebug_isDebuggerConnected;

        jint res = JNI_FUNC_PTR(env,RegisterNatives)(env,
                                                     vmdebug_class,
                                                     methods,
                                                     sizeof(methods) / sizeof(JNINativeMethod));
        if (res != JNI_OK) {
            EXIT_ERROR(JVMTI_ERROR_INTERNAL,
                       "RegisterNatives returned failure for VMDebug class");
        }

        finish: ;
    } END_WITH_LOCAL_REFS(env);
}

