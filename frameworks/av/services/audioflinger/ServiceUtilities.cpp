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

#include <binder/AppOpsManager.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/PermissionCache.h>
#include <private/android_filesystem_config.h>
#include "ServiceUtilities.h"

/* When performing permission checks we do not use permission cache for
 * runtime permissions (protection level dangerous) as they may change at
 * runtime. All other permissions (protection level normal and dangerous)
 * can be cached as they never change. Of course all permission checked
 * here are platform defined.
 */

namespace android {

static const String16 sAndroidPermissionRecordAudio("android.permission.RECORD_AUDIO");

// Not valid until initialized by AudioFlinger constructor.  It would have to be
// re-initialized if the process containing AudioFlinger service forks (which it doesn't).
// This is often used to validate binder interface calls within audioserver
// (e.g. AudioPolicyManager to AudioFlinger).
pid_t getpid_cached;

// A trusted calling UID may specify the client UID as part of a binder interface call.
// otherwise the calling UID must be equal to the client UID.
bool isTrustedCallingUid(uid_t uid) {
    switch (uid) {
    case AID_MEDIA:
    case AID_AUDIOSERVER:
        return true;
    default:
        return false;
    }
}

static String16 resolveCallingPackage(PermissionController& permissionController,
        const String16& opPackageName, uid_t uid) {
    if (opPackageName.size() > 0) {
        return opPackageName;
    }
    // In some cases the calling code has no access to the package it runs under.
    // For example, code using the wilhelm framework's OpenSL-ES APIs. In this
    // case we will get the packages for the calling UID and pick the first one
    // for attributing the app op. This will work correctly for runtime permissions
    // as for legacy apps we will toggle the app op for all packages in the UID.
    // The caveat is that the operation may be attributed to the wrong package and
    // stats based on app ops may be slightly off.
    Vector<String16> packages;
    permissionController.getPackagesForUid(uid, packages);
    if (packages.isEmpty()) {
        ALOGE("No packages for uid %d", uid);
        return opPackageName; // empty string
    }
    return packages[0];
}

static inline bool isAudioServerOrRoot(uid_t uid) {
    // AID_ROOT is OK for command-line tests.  Native unforked audioserver always OK.
    return uid == AID_ROOT || uid == AID_AUDIOSERVER ;
}

static bool checkRecordingInternal(const String16& opPackageName, pid_t pid,
        uid_t uid, bool start) {
    // Okay to not track in app ops as audio server is us and if
    // device is rooted security model is considered compromised.
    if (isAudioServerOrRoot(uid)) return true;

    // We specify a pid and uid here as mediaserver (aka MediaRecorder or StageFrightRecorder)
    // may open a record track on behalf of a client.  Note that pid may be a tid.
    // IMPORTANT: DON'T USE PermissionCache - RUNTIME PERMISSIONS CHANGE.
    PermissionController permissionController;
    const bool ok = permissionController.checkPermission(sAndroidPermissionRecordAudio, pid, uid);
    if (!ok) {
        ALOGE("Request requires %s", String8(sAndroidPermissionRecordAudio).c_str());
        return false;
    }

    String16 resolvedOpPackageName = resolveCallingPackage(
            permissionController, opPackageName, uid);
    if (resolvedOpPackageName.size() == 0) {
        return false;
    }

    AppOpsManager appOps;
    const int32_t op = appOps.permissionToOpCode(sAndroidPermissionRecordAudio);
    if (start) {
        if (appOps.startOpNoThrow(op, uid, resolvedOpPackageName, /*startIfModeDefault*/ false)
                != AppOpsManager::MODE_ALLOWED) {
            ALOGE("Request denied by app op: %d", op);
            return false;
        }
    } else {
        if (appOps.noteOp(op, uid, resolvedOpPackageName) != AppOpsManager::MODE_ALLOWED) {
            ALOGE("Request denied by app op: %d", op);
            return false;
        }
    }

    return true;
}

bool recordingAllowed(const String16& opPackageName, pid_t pid, uid_t uid) {
    return checkRecordingInternal(opPackageName, pid, uid, /*start*/ false);
}

bool startRecording(const String16& opPackageName, pid_t pid, uid_t uid) {
     return checkRecordingInternal(opPackageName, pid, uid, /*start*/ true);
}

void finishRecording(const String16& opPackageName, uid_t uid) {
    // Okay to not track in app ops as audio server is us and if
    // device is rooted security model is considered compromised.
    if (isAudioServerOrRoot(uid)) return;

    PermissionController permissionController;
    String16 resolvedOpPackageName = resolveCallingPackage(
            permissionController, opPackageName, uid);
    if (resolvedOpPackageName.size() == 0) {
        return;
    }

    AppOpsManager appOps;
    const int32_t op = appOps.permissionToOpCode(sAndroidPermissionRecordAudio);
    appOps.finishOp(op, uid, resolvedOpPackageName);
}

bool captureAudioOutputAllowed(pid_t pid, uid_t uid) {
    if (getpid_cached == IPCThreadState::self()->getCallingPid()) return true;
    static const String16 sCaptureAudioOutput("android.permission.CAPTURE_AUDIO_OUTPUT");
    bool ok = PermissionCache::checkPermission(sCaptureAudioOutput, pid, uid);
    if (!ok) ALOGE("Request requires android.permission.CAPTURE_AUDIO_OUTPUT");
    return ok;
}

bool captureHotwordAllowed(pid_t pid, uid_t uid) {
    // CAPTURE_AUDIO_HOTWORD permission implies RECORD_AUDIO permission
    bool ok = recordingAllowed(String16(""), pid, uid);

    if (ok) {
        static const String16 sCaptureHotwordAllowed("android.permission.CAPTURE_AUDIO_HOTWORD");
        // IMPORTANT: Use PermissionCache - not a runtime permission and may not change.
        ok = PermissionCache::checkCallingPermission(sCaptureHotwordAllowed);
    }
    if (!ok) ALOGE("android.permission.CAPTURE_AUDIO_HOTWORD");
    return ok;
}

bool settingsAllowed() {
    if (getpid_cached == IPCThreadState::self()->getCallingPid()) return true;
    static const String16 sAudioSettings("android.permission.MODIFY_AUDIO_SETTINGS");
    // IMPORTANT: Use PermissionCache - not a runtime permission and may not change.
    bool ok = PermissionCache::checkCallingPermission(sAudioSettings);
    if (!ok) ALOGE("Request requires android.permission.MODIFY_AUDIO_SETTINGS");
    return ok;
}

bool modifyAudioRoutingAllowed() {
    static const String16 sModifyAudioRoutingAllowed("android.permission.MODIFY_AUDIO_ROUTING");
    // IMPORTANT: Use PermissionCache - not a runtime permission and may not change.
    bool ok = PermissionCache::checkCallingPermission(sModifyAudioRoutingAllowed);
    if (!ok) ALOGE("android.permission.MODIFY_AUDIO_ROUTING");
    return ok;
}

bool dumpAllowed() {
    // don't optimize for same pid, since mediaserver never dumps itself
    static const String16 sDump("android.permission.DUMP");
    // IMPORTANT: Use PermissionCache - not a runtime permission and may not change.
    bool ok = PermissionCache::checkCallingPermission(sDump);
    // convention is for caller to dump an error message to fd instead of logging here
    //if (!ok) ALOGE("Request requires android.permission.DUMP");
    return ok;
}

bool modifyPhoneStateAllowed(pid_t pid, uid_t uid) {
    static const String16 sModifyPhoneState("android.permission.MODIFY_PHONE_STATE");
    bool ok = PermissionCache::checkPermission(sModifyPhoneState, pid, uid);
    if (!ok) ALOGE("Request requires android.permission.MODIFY_PHONE_STATE");
    return ok;
}

} // namespace android
