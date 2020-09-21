/*
 * Copyright (C) 2009 The Android Open Source Project
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

#define LOG_TAG "AudioPolicyService"
//#define LOG_NDEBUG 0

#include "Configuration.h"
#undef __STRICT_ANSI__
#define __STDINT_LIMITS
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <sys/time.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <cutils/multiuser.h>
#include <cutils/properties.h>
#include <binder/IPCThreadState.h>
#include <binder/ActivityManager.h>
#include <binder/PermissionController.h>
#include <binder/IResultReceiver.h>
#include <utils/String16.h>
#include <utils/threads.h>
#include "AudioPolicyService.h"
#include "ServiceUtilities.h"
#include <hardware_legacy/power.h>
#include <media/AudioEffect.h>
#include <media/AudioParameter.h>

#include <system/audio.h>
#include <system/audio_policy.h>

#include <private/android_filesystem_config.h>

namespace android {

static const char kDeadlockedString[] = "AudioPolicyService may be deadlocked\n";
static const char kCmdDeadlockedString[] = "AudioPolicyService command thread may be deadlocked\n";

static const int kDumpLockRetries = 50;
static const int kDumpLockSleepUs = 20000;

static const nsecs_t kAudioCommandTimeoutNs = seconds(3); // 3 seconds

static const String16 sManageAudioPolicyPermission("android.permission.MANAGE_AUDIO_POLICY");

// ----------------------------------------------------------------------------

AudioPolicyService::AudioPolicyService()
    : BnAudioPolicyService(), mpAudioPolicyDev(NULL), mpAudioPolicy(NULL),
      mAudioPolicyManager(NULL), mAudioPolicyClient(NULL), mPhoneState(AUDIO_MODE_INVALID)
{
}

void AudioPolicyService::onFirstRef()
{
    {
        Mutex::Autolock _l(mLock);

        // start tone playback thread
        mTonePlaybackThread = new AudioCommandThread(String8("ApmTone"), this);
        // start audio commands thread
        mAudioCommandThread = new AudioCommandThread(String8("ApmAudio"), this);
        // start output activity command thread
        mOutputCommandThread = new AudioCommandThread(String8("ApmOutput"), this);

        mAudioPolicyClient = new AudioPolicyClient(this);
        mAudioPolicyManager = createAudioPolicyManager(mAudioPolicyClient);
    }
    // load audio processing modules
    sp<AudioPolicyEffects>audioPolicyEffects = new AudioPolicyEffects();
    {
        Mutex::Autolock _l(mLock);
        mAudioPolicyEffects = audioPolicyEffects;
    }

    mUidPolicy = new UidPolicy(this);
    mUidPolicy->registerSelf();
}

AudioPolicyService::~AudioPolicyService()
{
    mTonePlaybackThread->exit();
    mAudioCommandThread->exit();
    mOutputCommandThread->exit();

    destroyAudioPolicyManager(mAudioPolicyManager);
    delete mAudioPolicyClient;

    mNotificationClients.clear();
    mAudioPolicyEffects.clear();

    mUidPolicy->unregisterSelf();
    mUidPolicy.clear();
}

// A notification client is always registered by AudioSystem when the client process
// connects to AudioPolicyService.
void AudioPolicyService::registerClient(const sp<IAudioPolicyServiceClient>& client)
{
    if (client == 0) {
        ALOGW("%s got NULL client", __FUNCTION__);
        return;
    }
    Mutex::Autolock _l(mNotificationClientsLock);

    uid_t uid = IPCThreadState::self()->getCallingUid();
    if (mNotificationClients.indexOfKey(uid) < 0) {
        sp<NotificationClient> notificationClient = new NotificationClient(this,
                                                                           client,
                                                                           uid);
        ALOGV("registerClient() client %p, uid %d", client.get(), uid);

        mNotificationClients.add(uid, notificationClient);

        sp<IBinder> binder = IInterface::asBinder(client);
        binder->linkToDeath(notificationClient);
    }
}

void AudioPolicyService::setAudioPortCallbacksEnabled(bool enabled)
{
    Mutex::Autolock _l(mNotificationClientsLock);

    uid_t uid = IPCThreadState::self()->getCallingUid();
    if (mNotificationClients.indexOfKey(uid) < 0) {
        return;
    }
    mNotificationClients.valueFor(uid)->setAudioPortCallbacksEnabled(enabled);
}

// removeNotificationClient() is called when the client process dies.
void AudioPolicyService::removeNotificationClient(uid_t uid)
{
    {
        Mutex::Autolock _l(mNotificationClientsLock);
        mNotificationClients.removeItem(uid);
    }
    {
        Mutex::Autolock _l(mLock);
        if (mAudioPolicyManager) {
            // called from binder death notification: no need to clear caller identity
            mAudioPolicyManager->releaseResourcesForUid(uid);
        }
    }
}

void AudioPolicyService::onAudioPortListUpdate()
{
    mOutputCommandThread->updateAudioPortListCommand();
}

void AudioPolicyService::doOnAudioPortListUpdate()
{
    Mutex::Autolock _l(mNotificationClientsLock);
    for (size_t i = 0; i < mNotificationClients.size(); i++) {
        mNotificationClients.valueAt(i)->onAudioPortListUpdate();
    }
}

void AudioPolicyService::onAudioPatchListUpdate()
{
    mOutputCommandThread->updateAudioPatchListCommand();
}

void AudioPolicyService::doOnAudioPatchListUpdate()
{
    Mutex::Autolock _l(mNotificationClientsLock);
    for (size_t i = 0; i < mNotificationClients.size(); i++) {
        mNotificationClients.valueAt(i)->onAudioPatchListUpdate();
    }
}

void AudioPolicyService::onDynamicPolicyMixStateUpdate(const String8& regId, int32_t state)
{
    ALOGV("AudioPolicyService::onDynamicPolicyMixStateUpdate(%s, %d)",
            regId.string(), state);
    mOutputCommandThread->dynamicPolicyMixStateUpdateCommand(regId, state);
}

void AudioPolicyService::doOnDynamicPolicyMixStateUpdate(const String8& regId, int32_t state)
{
    Mutex::Autolock _l(mNotificationClientsLock);
    for (size_t i = 0; i < mNotificationClients.size(); i++) {
        mNotificationClients.valueAt(i)->onDynamicPolicyMixStateUpdate(regId, state);
    }
}

void AudioPolicyService::onRecordingConfigurationUpdate(int event,
        const record_client_info_t *clientInfo, const audio_config_base_t *clientConfig,
        const audio_config_base_t *deviceConfig, audio_patch_handle_t patchHandle)
{
    mOutputCommandThread->recordingConfigurationUpdateCommand(event, clientInfo,
            clientConfig, deviceConfig, patchHandle);
}

void AudioPolicyService::doOnRecordingConfigurationUpdate(int event,
        const record_client_info_t *clientInfo, const audio_config_base_t *clientConfig,
        const audio_config_base_t *deviceConfig, audio_patch_handle_t patchHandle)
{
    Mutex::Autolock _l(mNotificationClientsLock);
    for (size_t i = 0; i < mNotificationClients.size(); i++) {
        mNotificationClients.valueAt(i)->onRecordingConfigurationUpdate(event, clientInfo,
                clientConfig, deviceConfig, patchHandle);
    }
}

status_t AudioPolicyService::clientCreateAudioPatch(const struct audio_patch *patch,
                                                audio_patch_handle_t *handle,
                                                int delayMs)
{
    return mAudioCommandThread->createAudioPatchCommand(patch, handle, delayMs);
}

status_t AudioPolicyService::clientReleaseAudioPatch(audio_patch_handle_t handle,
                                                 int delayMs)
{
    return mAudioCommandThread->releaseAudioPatchCommand(handle, delayMs);
}

status_t AudioPolicyService::clientSetAudioPortConfig(const struct audio_port_config *config,
                                                      int delayMs)
{
    return mAudioCommandThread->setAudioPortConfigCommand(config, delayMs);
}

AudioPolicyService::NotificationClient::NotificationClient(const sp<AudioPolicyService>& service,
                                                     const sp<IAudioPolicyServiceClient>& client,
                                                     uid_t uid)
    : mService(service), mUid(uid), mAudioPolicyServiceClient(client),
      mAudioPortCallbacksEnabled(false)
{
}

AudioPolicyService::NotificationClient::~NotificationClient()
{
}

void AudioPolicyService::NotificationClient::binderDied(const wp<IBinder>& who __unused)
{
    sp<NotificationClient> keep(this);
    sp<AudioPolicyService> service = mService.promote();
    if (service != 0) {
        service->removeNotificationClient(mUid);
    }
}

void AudioPolicyService::NotificationClient::onAudioPortListUpdate()
{
    if (mAudioPolicyServiceClient != 0 && mAudioPortCallbacksEnabled) {
        mAudioPolicyServiceClient->onAudioPortListUpdate();
    }
}

void AudioPolicyService::NotificationClient::onAudioPatchListUpdate()
{
    if (mAudioPolicyServiceClient != 0 && mAudioPortCallbacksEnabled) {
        mAudioPolicyServiceClient->onAudioPatchListUpdate();
    }
}

void AudioPolicyService::NotificationClient::onDynamicPolicyMixStateUpdate(
        const String8& regId, int32_t state)
{
    if (mAudioPolicyServiceClient != 0 && multiuser_get_app_id(mUid) < AID_APP_START) {
        mAudioPolicyServiceClient->onDynamicPolicyMixStateUpdate(regId, state);
    }
}

void AudioPolicyService::NotificationClient::onRecordingConfigurationUpdate(
        int event, const record_client_info_t *clientInfo,
        const audio_config_base_t *clientConfig, const audio_config_base_t *deviceConfig,
        audio_patch_handle_t patchHandle)
{
    if (mAudioPolicyServiceClient != 0 && multiuser_get_app_id(mUid) < AID_APP_START) {
        mAudioPolicyServiceClient->onRecordingConfigurationUpdate(event, clientInfo,
                clientConfig, deviceConfig, patchHandle);
    }
}

void AudioPolicyService::NotificationClient::setAudioPortCallbacksEnabled(bool enabled)
{
    mAudioPortCallbacksEnabled = enabled;
}


void AudioPolicyService::binderDied(const wp<IBinder>& who) {
    ALOGW("binderDied() %p, calling pid %d", who.unsafe_get(),
            IPCThreadState::self()->getCallingPid());
}

static bool tryLock(Mutex& mutex)
{
    bool locked = false;
    for (int i = 0; i < kDumpLockRetries; ++i) {
        if (mutex.tryLock() == NO_ERROR) {
            locked = true;
            break;
        }
        usleep(kDumpLockSleepUs);
    }
    return locked;
}

status_t AudioPolicyService::dumpInternals(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, "AudioPolicyManager: %p\n", mAudioPolicyManager);
    result.append(buffer);
    snprintf(buffer, SIZE, "Command Thread: %p\n", mAudioCommandThread.get());
    result.append(buffer);
    snprintf(buffer, SIZE, "Tones Thread: %p\n", mTonePlaybackThread.get());
    result.append(buffer);

    write(fd, result.string(), result.size());
    return NO_ERROR;
}

void AudioPolicyService::setRecordSilenced(uid_t uid, bool silenced)
{
    {
        Mutex::Autolock _l(mLock);
        if (mAudioPolicyManager) {
            AutoCallerClear acc;
            mAudioPolicyManager->setRecordSilenced(uid, silenced);
        }
    }
    sp<IAudioFlinger> af = AudioSystem::get_audio_flinger();
    if (af) {
        af->setRecordSilenced(uid, silenced);
    }
}

status_t AudioPolicyService::dump(int fd, const Vector<String16>& args __unused)
{
    if (!dumpAllowed()) {
        dumpPermissionDenial(fd);
    } else {
        bool locked = tryLock(mLock);
        if (!locked) {
            String8 result(kDeadlockedString);
            write(fd, result.string(), result.size());
        }

        dumpInternals(fd);
        if (mAudioCommandThread != 0) {
            mAudioCommandThread->dump(fd);
        }
        if (mTonePlaybackThread != 0) {
            mTonePlaybackThread->dump(fd);
        }

        if (mAudioPolicyManager) {
            mAudioPolicyManager->dump(fd);
        }

        if (locked) mLock.unlock();
    }
    return NO_ERROR;
}

status_t AudioPolicyService::dumpPermissionDenial(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    snprintf(buffer, SIZE, "Permission Denial: "
            "can't dump AudioPolicyService from pid=%d, uid=%d\n",
            IPCThreadState::self()->getCallingPid(),
            IPCThreadState::self()->getCallingUid());
    result.append(buffer);
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

status_t AudioPolicyService::onTransact(
        uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
    switch (code) {
        case SHELL_COMMAND_TRANSACTION: {
            int in = data.readFileDescriptor();
            int out = data.readFileDescriptor();
            int err = data.readFileDescriptor();
            int argc = data.readInt32();
            Vector<String16> args;
            for (int i = 0; i < argc && data.dataAvail() > 0; i++) {
               args.add(data.readString16());
            }
            sp<IBinder> unusedCallback;
            sp<IResultReceiver> resultReceiver;
            status_t status;
            if ((status = data.readNullableStrongBinder(&unusedCallback)) != NO_ERROR) {
                return status;
            }
            if ((status = data.readNullableStrongBinder(&resultReceiver)) != NO_ERROR) {
                return status;
            }
            status = shellCommand(in, out, err, args);
            if (resultReceiver != nullptr) {
                resultReceiver->send(status);
            }
            return NO_ERROR;
        }
    }

    return BnAudioPolicyService::onTransact(code, data, reply, flags);
}

// ------------------- Shell command implementation -------------------

// NOTE: This is a remote API - make sure all args are validated
status_t AudioPolicyService::shellCommand(int in, int out, int err, Vector<String16>& args) {
    if (!checkCallingPermission(sManageAudioPolicyPermission, nullptr, nullptr)) {
        return PERMISSION_DENIED;
    }
    if (in == BAD_TYPE || out == BAD_TYPE || err == BAD_TYPE) {
        return BAD_VALUE;
    }
    if (args.size() == 3 && args[0] == String16("set-uid-state")) {
        return handleSetUidState(args, err);
    } else if (args.size() == 2 && args[0] == String16("reset-uid-state")) {
        return handleResetUidState(args, err);
    } else if (args.size() == 2 && args[0] == String16("get-uid-state")) {
        return handleGetUidState(args, out, err);
    } else if (args.size() == 1 && args[0] == String16("help")) {
        printHelp(out);
        return NO_ERROR;
    }
    printHelp(err);
    return BAD_VALUE;
}

status_t AudioPolicyService::handleSetUidState(Vector<String16>& args, int err) {
    PermissionController pc;
    int uid = pc.getPackageUid(args[1], 0);
    if (uid <= 0) {
        ALOGE("Unknown package: '%s'", String8(args[1]).string());
        dprintf(err, "Unknown package: '%s'\n", String8(args[1]).string());
        return BAD_VALUE;
    }
    bool active = false;
    if (args[2] == String16("active")) {
        active = true;
    } else if ((args[2] != String16("idle"))) {
        ALOGE("Expected active or idle but got: '%s'", String8(args[2]).string());
        return BAD_VALUE;
    }
    mUidPolicy->addOverrideUid(uid, active);
    return NO_ERROR;
}

status_t AudioPolicyService::handleResetUidState(Vector<String16>& args, int err) {
    PermissionController pc;
    int uid = pc.getPackageUid(args[1], 0);
    if (uid < 0) {
        ALOGE("Unknown package: '%s'", String8(args[1]).string());
        dprintf(err, "Unknown package: '%s'\n", String8(args[1]).string());
        return BAD_VALUE;
    }
    mUidPolicy->removeOverrideUid(uid);
    return NO_ERROR;
}

status_t AudioPolicyService::handleGetUidState(Vector<String16>& args, int out, int err) {
    PermissionController pc;
    int uid = pc.getPackageUid(args[1], 0);
    if (uid < 0) {
        ALOGE("Unknown package: '%s'", String8(args[1]).string());
        dprintf(err, "Unknown package: '%s'\n", String8(args[1]).string());
        return BAD_VALUE;
    }
    if (mUidPolicy->isUidActive(uid)) {
        return dprintf(out, "active\n");
    } else {
        return dprintf(out, "idle\n");
    }
}

status_t AudioPolicyService::printHelp(int out) {
    return dprintf(out, "Audio policy service commands:\n"
        "  get-uid-state <PACKAGE> gets the uid state\n"
        "  set-uid-state <PACKAGE> <active|idle> overrides the uid state\n"
        "  reset-uid-state <PACKAGE> clears the uid state override\n"
        "  help print this message\n");
}

// -----------  AudioPolicyService::UidPolicy implementation ----------

void AudioPolicyService::UidPolicy::registerSelf() {
    ActivityManager am;
    am.registerUidObserver(this, ActivityManager::UID_OBSERVER_GONE
            | ActivityManager::UID_OBSERVER_IDLE
            | ActivityManager::UID_OBSERVER_ACTIVE,
            ActivityManager::PROCESS_STATE_UNKNOWN,
            String16("audioserver"));
    status_t res = am.linkToDeath(this);
    if (!res) {
        Mutex::Autolock _l(mLock);
        mObserverRegistered = true;
    } else {
        ALOGE("UidPolicy::registerSelf linkToDeath failed: %d", res);
        am.unregisterUidObserver(this);
    }
}

void AudioPolicyService::UidPolicy::unregisterSelf() {
    ActivityManager am;
    am.unlinkToDeath(this);
    am.unregisterUidObserver(this);
    Mutex::Autolock _l(mLock);
    mObserverRegistered = false;
}

void AudioPolicyService::UidPolicy::binderDied(__unused const wp<IBinder> &who) {
    Mutex::Autolock _l(mLock);
    mCachedUids.clear();
    mObserverRegistered = false;
}

bool AudioPolicyService::UidPolicy::isUidActive(uid_t uid) {
    if (isServiceUid(uid)) return true;
    bool needToReregister = false;
    {
        Mutex::Autolock _l(mLock);
        needToReregister = !mObserverRegistered;
    }
    if (needToReregister) {
        // Looks like ActivityManager has died previously, attempt to re-register.
        registerSelf();
    }
    {
        Mutex::Autolock _l(mLock);
        auto overrideIter = mOverrideUids.find(uid);
        if (overrideIter != mOverrideUids.end()) {
            return overrideIter->second;
        }
        // In an absense of the ActivityManager, assume everything to be active.
        if (!mObserverRegistered) return true;
        auto cacheIter = mCachedUids.find(uid);
        if (cacheIter != mCachedUids.end()) {
            return cacheIter->second;
        }
    }
    ActivityManager am;
    bool active = am.isUidActive(uid, String16("audioserver"));
    {
        Mutex::Autolock _l(mLock);
        mCachedUids.insert(std::pair<uid_t, bool>(uid, active));
    }
    return active;
}

void AudioPolicyService::UidPolicy::onUidActive(uid_t uid) {
    updateUidCache(uid, true, true);
}

void AudioPolicyService::UidPolicy::onUidGone(uid_t uid, __unused bool disabled) {
    updateUidCache(uid, false, false);
}

void AudioPolicyService::UidPolicy::onUidIdle(uid_t uid, __unused bool disabled) {
    updateUidCache(uid, false, true);
}

bool AudioPolicyService::UidPolicy::isServiceUid(uid_t uid) const {
    return multiuser_get_app_id(uid) < AID_APP_START;
}

void AudioPolicyService::UidPolicy::notifyService(uid_t uid, bool active) {
    sp<AudioPolicyService> service = mService.promote();
    if (service != nullptr) {
        service->setRecordSilenced(uid, !active);
    }
}

void AudioPolicyService::UidPolicy::updateOverrideUid(uid_t uid, bool active, bool insert) {
    if (isServiceUid(uid)) return;
    bool wasOverridden = false, wasActive = false;
    {
        Mutex::Autolock _l(mLock);
        updateUidLocked(&mOverrideUids, uid, active, insert, &wasOverridden, &wasActive);
    }
    if (!wasOverridden && insert) {
        notifyService(uid, active);  // Started to override.
    } else if (wasOverridden && !insert) {
        notifyService(uid, isUidActive(uid));  // Override ceased, notify with ground truth.
    } else if (wasActive != active) {
        notifyService(uid, active);  // Override updated.
    }
}

void AudioPolicyService::UidPolicy::updateUidCache(uid_t uid, bool active, bool insert) {
    if (isServiceUid(uid)) return;
    bool wasActive = false;
    {
        Mutex::Autolock _l(mLock);
        updateUidLocked(&mCachedUids, uid, active, insert, nullptr, &wasActive);
        // Do not notify service if currently overridden.
        if (mOverrideUids.find(uid) != mOverrideUids.end()) return;
    }
    bool nowActive = active && insert;
    if (wasActive != nowActive) notifyService(uid, nowActive);
}

void AudioPolicyService::UidPolicy::updateUidLocked(std::unordered_map<uid_t, bool> *uids,
        uid_t uid, bool active, bool insert, bool *wasThere, bool *wasActive) {
    auto it = uids->find(uid);
    if (it != uids->end()) {
        if (wasThere != nullptr) *wasThere = true;
        if (wasActive != nullptr) *wasActive = it->second;
        if (insert) {
            it->second = active;
        } else {
            uids->erase(it);
        }
    } else if (insert) {
        uids->insert(std::pair<uid_t, bool>(uid, active));
    }
}

// -----------  AudioPolicyService::AudioCommandThread implementation ----------

AudioPolicyService::AudioCommandThread::AudioCommandThread(String8 name,
                                                           const wp<AudioPolicyService>& service)
    : Thread(false), mName(name), mService(service)
{
    mpToneGenerator = NULL;
}


AudioPolicyService::AudioCommandThread::~AudioCommandThread()
{
    if (!mAudioCommands.isEmpty()) {
        release_wake_lock(mName.string());
    }
    mAudioCommands.clear();
    delete mpToneGenerator;
}

void AudioPolicyService::AudioCommandThread::onFirstRef()
{
    run(mName.string(), ANDROID_PRIORITY_AUDIO);
}

bool AudioPolicyService::AudioCommandThread::threadLoop()
{
    nsecs_t waitTime = -1;

    mLock.lock();
    while (!exitPending())
    {
        sp<AudioPolicyService> svc;
        while (!mAudioCommands.isEmpty() && !exitPending()) {
            nsecs_t curTime = systemTime();
            // commands are sorted by increasing time stamp: execute them from index 0 and up
            if (mAudioCommands[0]->mTime <= curTime) {
                sp<AudioCommand> command = mAudioCommands[0];
                mAudioCommands.removeAt(0);
                mLastCommand = command;

                switch (command->mCommand) {
                case START_TONE: {
                    mLock.unlock();
                    ToneData *data = (ToneData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing start tone %d on stream %d",
                            data->mType, data->mStream);
                    delete mpToneGenerator;
                    mpToneGenerator = new ToneGenerator(data->mStream, 1.0);
                    mpToneGenerator->startTone(data->mType);
                    mLock.lock();
                    }break;
                case STOP_TONE: {
                    mLock.unlock();
                    ALOGV("AudioCommandThread() processing stop tone");
                    if (mpToneGenerator != NULL) {
                        mpToneGenerator->stopTone();
                        delete mpToneGenerator;
                        mpToneGenerator = NULL;
                    }
                    mLock.lock();
                    }break;
                case SET_VOLUME: {
                    VolumeData *data = (VolumeData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing set volume stream %d, \
                            volume %f, output %d", data->mStream, data->mVolume, data->mIO);
                    command->mStatus = AudioSystem::setStreamVolume(data->mStream,
                                                                    data->mVolume,
                                                                    data->mIO);
                    }break;
                case SET_PARAMETERS: {
                    ParametersData *data = (ParametersData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing set parameters string %s, io %d",
                            data->mKeyValuePairs.string(), data->mIO);
                    command->mStatus = AudioSystem::setParameters(data->mIO, data->mKeyValuePairs);
                    }break;
                case SET_VOICE_VOLUME: {
                    VoiceVolumeData *data = (VoiceVolumeData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing set voice volume volume %f",
                            data->mVolume);
                    command->mStatus = AudioSystem::setVoiceVolume(data->mVolume);
                    }break;
                case STOP_OUTPUT: {
                    StopOutputData *data = (StopOutputData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing stop output %d",
                            data->mIO);
                    svc = mService.promote();
                    if (svc == 0) {
                        break;
                    }
                    mLock.unlock();
                    svc->doStopOutput(data->mIO, data->mStream, data->mSession);
                    mLock.lock();
                    }break;
                case RELEASE_OUTPUT: {
                    ReleaseOutputData *data = (ReleaseOutputData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing release output %d",
                            data->mIO);
                    svc = mService.promote();
                    if (svc == 0) {
                        break;
                    }
                    mLock.unlock();
                    svc->doReleaseOutput(data->mIO, data->mStream, data->mSession);
                    mLock.lock();
                    }break;
                case CREATE_AUDIO_PATCH: {
                    CreateAudioPatchData *data = (CreateAudioPatchData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing create audio patch");
                    sp<IAudioFlinger> af = AudioSystem::get_audio_flinger();
                    if (af == 0) {
                        command->mStatus = PERMISSION_DENIED;
                    } else {
                        command->mStatus = af->createAudioPatch(&data->mPatch, &data->mHandle);
                    }
                    } break;
                case RELEASE_AUDIO_PATCH: {
                    ReleaseAudioPatchData *data = (ReleaseAudioPatchData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing release audio patch");
                    sp<IAudioFlinger> af = AudioSystem::get_audio_flinger();
                    if (af == 0) {
                        command->mStatus = PERMISSION_DENIED;
                    } else {
                        command->mStatus = af->releaseAudioPatch(data->mHandle);
                    }
                    } break;
                case UPDATE_AUDIOPORT_LIST: {
                    ALOGV("AudioCommandThread() processing update audio port list");
                    svc = mService.promote();
                    if (svc == 0) {
                        break;
                    }
                    mLock.unlock();
                    svc->doOnAudioPortListUpdate();
                    mLock.lock();
                    }break;
                case UPDATE_AUDIOPATCH_LIST: {
                    ALOGV("AudioCommandThread() processing update audio patch list");
                    svc = mService.promote();
                    if (svc == 0) {
                        break;
                    }
                    mLock.unlock();
                    svc->doOnAudioPatchListUpdate();
                    mLock.lock();
                    }break;
                case SET_AUDIOPORT_CONFIG: {
                    SetAudioPortConfigData *data = (SetAudioPortConfigData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing set port config");
                    sp<IAudioFlinger> af = AudioSystem::get_audio_flinger();
                    if (af == 0) {
                        command->mStatus = PERMISSION_DENIED;
                    } else {
                        command->mStatus = af->setAudioPortConfig(&data->mConfig);
                    }
                    } break;
                case DYN_POLICY_MIX_STATE_UPDATE: {
                    DynPolicyMixStateUpdateData *data =
                            (DynPolicyMixStateUpdateData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing dyn policy mix state update %s %d",
                            data->mRegId.string(), data->mState);
                    svc = mService.promote();
                    if (svc == 0) {
                        break;
                    }
                    mLock.unlock();
                    svc->doOnDynamicPolicyMixStateUpdate(data->mRegId, data->mState);
                    mLock.lock();
                    } break;
                case RECORDING_CONFIGURATION_UPDATE: {
                    RecordingConfigurationUpdateData *data =
                            (RecordingConfigurationUpdateData *)command->mParam.get();
                    ALOGV("AudioCommandThread() processing recording configuration update");
                    svc = mService.promote();
                    if (svc == 0) {
                        break;
                    }
                    mLock.unlock();
                    svc->doOnRecordingConfigurationUpdate(data->mEvent, &data->mClientInfo,
                            &data->mClientConfig, &data->mDeviceConfig,
                            data->mPatchHandle);
                    mLock.lock();
                    } break;
                default:
                    ALOGW("AudioCommandThread() unknown command %d", command->mCommand);
                }
                {
                    Mutex::Autolock _l(command->mLock);
                    if (command->mWaitStatus) {
                        command->mWaitStatus = false;
                        command->mCond.signal();
                    }
                }
                waitTime = -1;
                // release mLock before releasing strong reference on the service as
                // AudioPolicyService destructor calls AudioCommandThread::exit() which
                // acquires mLock.
                mLock.unlock();
                svc.clear();
                mLock.lock();
            } else {
                waitTime = mAudioCommands[0]->mTime - curTime;
                break;
            }
        }

        // release delayed commands wake lock if the queue is empty
        if (mAudioCommands.isEmpty()) {
            release_wake_lock(mName.string());
        }

        // At this stage we have either an empty command queue or the first command in the queue
        // has a finite delay. So unless we are exiting it is safe to wait.
        if (!exitPending()) {
            ALOGV("AudioCommandThread() going to sleep");
            if (waitTime == -1) {
                mWaitWorkCV.wait(mLock);
            } else {
                mWaitWorkCV.waitRelative(mLock, waitTime);
            }
        }
    }
    // release delayed commands wake lock before quitting
    if (!mAudioCommands.isEmpty()) {
        release_wake_lock(mName.string());
    }
    mLock.unlock();
    return false;
}

status_t AudioPolicyService::AudioCommandThread::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, "AudioCommandThread %p Dump\n", this);
    result.append(buffer);
    write(fd, result.string(), result.size());

    bool locked = tryLock(mLock);
    if (!locked) {
        String8 result2(kCmdDeadlockedString);
        write(fd, result2.string(), result2.size());
    }

    snprintf(buffer, SIZE, "- Commands:\n");
    result = String8(buffer);
    result.append("   Command Time        Wait pParam\n");
    for (size_t i = 0; i < mAudioCommands.size(); i++) {
        mAudioCommands[i]->dump(buffer, SIZE);
        result.append(buffer);
    }
    result.append("  Last Command\n");
    if (mLastCommand != 0) {
        mLastCommand->dump(buffer, SIZE);
        result.append(buffer);
    } else {
        result.append("     none\n");
    }

    write(fd, result.string(), result.size());

    if (locked) mLock.unlock();

    return NO_ERROR;
}

void AudioPolicyService::AudioCommandThread::startToneCommand(ToneGenerator::tone_type type,
        audio_stream_type_t stream)
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = START_TONE;
    sp<ToneData> data = new ToneData();
    data->mType = type;
    data->mStream = stream;
    command->mParam = data;
    ALOGV("AudioCommandThread() adding tone start type %d, stream %d", type, stream);
    sendCommand(command);
}

void AudioPolicyService::AudioCommandThread::stopToneCommand()
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = STOP_TONE;
    ALOGV("AudioCommandThread() adding tone stop");
    sendCommand(command);
}

status_t AudioPolicyService::AudioCommandThread::volumeCommand(audio_stream_type_t stream,
                                                               float volume,
                                                               audio_io_handle_t output,
                                                               int delayMs)
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = SET_VOLUME;
    sp<VolumeData> data = new VolumeData();
    data->mStream = stream;
    data->mVolume = volume;
    data->mIO = output;
    command->mParam = data;
    command->mWaitStatus = true;
    ALOGV("AudioCommandThread() adding set volume stream %d, volume %f, output %d",
            stream, volume, output);
    return sendCommand(command, delayMs);
}

status_t AudioPolicyService::AudioCommandThread::parametersCommand(audio_io_handle_t ioHandle,
                                                                   const char *keyValuePairs,
                                                                   int delayMs)
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = SET_PARAMETERS;
    sp<ParametersData> data = new ParametersData();
    data->mIO = ioHandle;
    data->mKeyValuePairs = String8(keyValuePairs);
    command->mParam = data;
    command->mWaitStatus = true;
    ALOGV("AudioCommandThread() adding set parameter string %s, io %d ,delay %d",
            keyValuePairs, ioHandle, delayMs);
    return sendCommand(command, delayMs);
}

status_t AudioPolicyService::AudioCommandThread::voiceVolumeCommand(float volume, int delayMs)
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = SET_VOICE_VOLUME;
    sp<VoiceVolumeData> data = new VoiceVolumeData();
    data->mVolume = volume;
    command->mParam = data;
    command->mWaitStatus = true;
    ALOGV("AudioCommandThread() adding set voice volume volume %f", volume);
    return sendCommand(command, delayMs);
}

void AudioPolicyService::AudioCommandThread::stopOutputCommand(audio_io_handle_t output,
                                                               audio_stream_type_t stream,
                                                               audio_session_t session)
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = STOP_OUTPUT;
    sp<StopOutputData> data = new StopOutputData();
    data->mIO = output;
    data->mStream = stream;
    data->mSession = session;
    command->mParam = data;
    ALOGV("AudioCommandThread() adding stop output %d", output);
    sendCommand(command);
}

void AudioPolicyService::AudioCommandThread::releaseOutputCommand(audio_io_handle_t output,
                                                                  audio_stream_type_t stream,
                                                                  audio_session_t session)
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = RELEASE_OUTPUT;
    sp<ReleaseOutputData> data = new ReleaseOutputData();
    data->mIO = output;
    data->mStream = stream;
    data->mSession = session;
    command->mParam = data;
    ALOGV("AudioCommandThread() adding release output %d", output);
    sendCommand(command);
}

status_t AudioPolicyService::AudioCommandThread::createAudioPatchCommand(
                                                const struct audio_patch *patch,
                                                audio_patch_handle_t *handle,
                                                int delayMs)
{
    status_t status = NO_ERROR;

    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = CREATE_AUDIO_PATCH;
    CreateAudioPatchData *data = new CreateAudioPatchData();
    data->mPatch = *patch;
    data->mHandle = *handle;
    command->mParam = data;
    command->mWaitStatus = true;
    ALOGV("AudioCommandThread() adding create patch delay %d", delayMs);
    status = sendCommand(command, delayMs);
    if (status == NO_ERROR) {
        *handle = data->mHandle;
    }
    return status;
}

status_t AudioPolicyService::AudioCommandThread::releaseAudioPatchCommand(audio_patch_handle_t handle,
                                                 int delayMs)
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = RELEASE_AUDIO_PATCH;
    ReleaseAudioPatchData *data = new ReleaseAudioPatchData();
    data->mHandle = handle;
    command->mParam = data;
    command->mWaitStatus = true;
    ALOGV("AudioCommandThread() adding release patch delay %d", delayMs);
    return sendCommand(command, delayMs);
}

void AudioPolicyService::AudioCommandThread::updateAudioPortListCommand()
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = UPDATE_AUDIOPORT_LIST;
    ALOGV("AudioCommandThread() adding update audio port list");
    sendCommand(command);
}

void AudioPolicyService::AudioCommandThread::updateAudioPatchListCommand()
{
    sp<AudioCommand>command = new AudioCommand();
    command->mCommand = UPDATE_AUDIOPATCH_LIST;
    ALOGV("AudioCommandThread() adding update audio patch list");
    sendCommand(command);
}

status_t AudioPolicyService::AudioCommandThread::setAudioPortConfigCommand(
                                            const struct audio_port_config *config, int delayMs)
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = SET_AUDIOPORT_CONFIG;
    SetAudioPortConfigData *data = new SetAudioPortConfigData();
    data->mConfig = *config;
    command->mParam = data;
    command->mWaitStatus = true;
    ALOGV("AudioCommandThread() adding set port config delay %d", delayMs);
    return sendCommand(command, delayMs);
}

void AudioPolicyService::AudioCommandThread::dynamicPolicyMixStateUpdateCommand(
        const String8& regId, int32_t state)
{
    sp<AudioCommand> command = new AudioCommand();
    command->mCommand = DYN_POLICY_MIX_STATE_UPDATE;
    DynPolicyMixStateUpdateData *data = new DynPolicyMixStateUpdateData();
    data->mRegId = regId;
    data->mState = state;
    command->mParam = data;
    ALOGV("AudioCommandThread() sending dynamic policy mix (id=%s) state update to %d",
            regId.string(), state);
    sendCommand(command);
}

void AudioPolicyService::AudioCommandThread::recordingConfigurationUpdateCommand(
        int event, const record_client_info_t *clientInfo,
        const audio_config_base_t *clientConfig, const audio_config_base_t *deviceConfig,
        audio_patch_handle_t patchHandle)
{
    sp<AudioCommand>command = new AudioCommand();
    command->mCommand = RECORDING_CONFIGURATION_UPDATE;
    RecordingConfigurationUpdateData *data = new RecordingConfigurationUpdateData();
    data->mEvent = event;
    data->mClientInfo = *clientInfo;
    data->mClientConfig = *clientConfig;
    data->mDeviceConfig = *deviceConfig;
    data->mPatchHandle = patchHandle;
    command->mParam = data;
    ALOGV("AudioCommandThread() adding recording configuration update event %d, source %d uid %u",
            event, clientInfo->source, clientInfo->uid);
    sendCommand(command);
}

status_t AudioPolicyService::AudioCommandThread::sendCommand(sp<AudioCommand>& command, int delayMs)
{
    {
        Mutex::Autolock _l(mLock);
        insertCommand_l(command, delayMs);
        mWaitWorkCV.signal();
    }
    Mutex::Autolock _l(command->mLock);
    while (command->mWaitStatus) {
        nsecs_t timeOutNs = kAudioCommandTimeoutNs + milliseconds(delayMs);
        if (command->mCond.waitRelative(command->mLock, timeOutNs) != NO_ERROR) {
            command->mStatus = TIMED_OUT;
            command->mWaitStatus = false;
        }
    }
    return command->mStatus;
}

// insertCommand_l() must be called with mLock held
void AudioPolicyService::AudioCommandThread::insertCommand_l(sp<AudioCommand>& command, int delayMs)
{
    ssize_t i;  // not size_t because i will count down to -1
    Vector < sp<AudioCommand> > removedCommands;
    command->mTime = systemTime() + milliseconds(delayMs);

    // acquire wake lock to make sure delayed commands are processed
    if (mAudioCommands.isEmpty()) {
        acquire_wake_lock(PARTIAL_WAKE_LOCK, mName.string());
    }

    // check same pending commands with later time stamps and eliminate them
    for (i = (ssize_t)mAudioCommands.size()-1; i >= 0; i--) {
        sp<AudioCommand> command2 = mAudioCommands[i];
        // commands are sorted by increasing time stamp: no need to scan the rest of mAudioCommands
        if (command2->mTime <= command->mTime) break;

        // create audio patch or release audio patch commands are equivalent
        // with regard to filtering
        if ((command->mCommand == CREATE_AUDIO_PATCH) ||
                (command->mCommand == RELEASE_AUDIO_PATCH)) {
            if ((command2->mCommand != CREATE_AUDIO_PATCH) &&
                    (command2->mCommand != RELEASE_AUDIO_PATCH)) {
                continue;
            }
        } else if (command2->mCommand != command->mCommand) continue;

        switch (command->mCommand) {
        case SET_PARAMETERS: {
            ParametersData *data = (ParametersData *)command->mParam.get();
            ParametersData *data2 = (ParametersData *)command2->mParam.get();
            if (data->mIO != data2->mIO) break;
            ALOGV("Comparing parameter command %s to new command %s",
                    data2->mKeyValuePairs.string(), data->mKeyValuePairs.string());
            AudioParameter param = AudioParameter(data->mKeyValuePairs);
            AudioParameter param2 = AudioParameter(data2->mKeyValuePairs);
            for (size_t j = 0; j < param.size(); j++) {
                String8 key;
                String8 value;
                param.getAt(j, key, value);
                for (size_t k = 0; k < param2.size(); k++) {
                    String8 key2;
                    String8 value2;
                    param2.getAt(k, key2, value2);
                    if (key2 == key) {
                        param2.remove(key2);
                        ALOGV("Filtering out parameter %s", key2.string());
                        break;
                    }
                }
            }
            // if all keys have been filtered out, remove the command.
            // otherwise, update the key value pairs
            if (param2.size() == 0) {
                removedCommands.add(command2);
            } else {
                data2->mKeyValuePairs = param2.toString();
            }
            command->mTime = command2->mTime;
            // force delayMs to non 0 so that code below does not request to wait for
            // command status as the command is now delayed
            delayMs = 1;
        } break;

        case SET_VOLUME: {
            VolumeData *data = (VolumeData *)command->mParam.get();
            VolumeData *data2 = (VolumeData *)command2->mParam.get();
            if (data->mIO != data2->mIO) break;
            if (data->mStream != data2->mStream) break;
            ALOGV("Filtering out volume command on output %d for stream %d",
                    data->mIO, data->mStream);
            removedCommands.add(command2);
            command->mTime = command2->mTime;
            // force delayMs to non 0 so that code below does not request to wait for
            // command status as the command is now delayed
            delayMs = 1;
        } break;

        case SET_VOICE_VOLUME: {
            VoiceVolumeData *data = (VoiceVolumeData *)command->mParam.get();
            VoiceVolumeData *data2 = (VoiceVolumeData *)command2->mParam.get();
            ALOGV("Filtering out voice volume command value %f replaced by %f",
                  data2->mVolume, data->mVolume);
            removedCommands.add(command2);
            command->mTime = command2->mTime;
            // force delayMs to non 0 so that code below does not request to wait for
            // command status as the command is now delayed
            delayMs = 1;
        } break;

        case CREATE_AUDIO_PATCH:
        case RELEASE_AUDIO_PATCH: {
            audio_patch_handle_t handle;
            struct audio_patch patch;
            if (command->mCommand == CREATE_AUDIO_PATCH) {
                handle = ((CreateAudioPatchData *)command->mParam.get())->mHandle;
                patch = ((CreateAudioPatchData *)command->mParam.get())->mPatch;
            } else {
                handle = ((ReleaseAudioPatchData *)command->mParam.get())->mHandle;
            }
            audio_patch_handle_t handle2;
            struct audio_patch patch2;
            if (command2->mCommand == CREATE_AUDIO_PATCH) {
                handle2 = ((CreateAudioPatchData *)command2->mParam.get())->mHandle;
                patch2 = ((CreateAudioPatchData *)command2->mParam.get())->mPatch;
            } else {
                handle2 = ((ReleaseAudioPatchData *)command2->mParam.get())->mHandle;
                memset(&patch2, 0, sizeof(patch2));
            }
            if (handle != handle2) break;
            /* Filter CREATE_AUDIO_PATCH commands only when they are issued for
               same output. */
            if( (command->mCommand == CREATE_AUDIO_PATCH) &&
                (command2->mCommand == CREATE_AUDIO_PATCH) ) {
                bool isOutputDiff = false;
                if (patch.num_sources == patch2.num_sources) {
                    for (unsigned count = 0; count < patch.num_sources; count++) {
                        if (patch.sources[count].id != patch2.sources[count].id) {
                            isOutputDiff = true;
                            break;
                        }
                    }
                    if (isOutputDiff)
                       break;
                }
            }
            ALOGV("Filtering out %s audio patch command for handle %d",
                  (command->mCommand == CREATE_AUDIO_PATCH) ? "create" : "release", handle);
            removedCommands.add(command2);
            command->mTime = command2->mTime;
            // force delayMs to non 0 so that code below does not request to wait for
            // command status as the command is now delayed
            delayMs = 1;
        } break;

        case DYN_POLICY_MIX_STATE_UPDATE: {

        } break;

        case RECORDING_CONFIGURATION_UPDATE: {

        } break;

        case START_TONE:
        case STOP_TONE:
        default:
            break;
        }
    }

    // remove filtered commands
    for (size_t j = 0; j < removedCommands.size(); j++) {
        // removed commands always have time stamps greater than current command
        for (size_t k = i + 1; k < mAudioCommands.size(); k++) {
            if (mAudioCommands[k].get() == removedCommands[j].get()) {
                ALOGV("suppressing command: %d", mAudioCommands[k]->mCommand);
                mAudioCommands.removeAt(k);
                break;
            }
        }
    }
    removedCommands.clear();

    // Disable wait for status if delay is not 0.
    // Except for create audio patch command because the returned patch handle
    // is needed by audio policy manager
    if (delayMs != 0 && command->mCommand != CREATE_AUDIO_PATCH) {
        command->mWaitStatus = false;
    }

    // insert command at the right place according to its time stamp
    ALOGV("inserting command: %d at index %zd, num commands %zu",
            command->mCommand, i+1, mAudioCommands.size());
    mAudioCommands.insertAt(command, i + 1);
}

void AudioPolicyService::AudioCommandThread::exit()
{
    ALOGV("AudioCommandThread::exit");
    {
        AutoMutex _l(mLock);
        requestExit();
        mWaitWorkCV.signal();
    }
    // Note that we can call it from the thread loop if all other references have been released
    // but it will safely return WOULD_BLOCK in this case
    requestExitAndWait();
}

void AudioPolicyService::AudioCommandThread::AudioCommand::dump(char* buffer, size_t size)
{
    snprintf(buffer, size, "   %02d      %06d.%03d  %01u    %p\n",
            mCommand,
            (int)ns2s(mTime),
            (int)ns2ms(mTime)%1000,
            mWaitStatus,
            mParam.get());
}

/******* helpers for the service_ops callbacks defined below *********/
void AudioPolicyService::setParameters(audio_io_handle_t ioHandle,
                                       const char *keyValuePairs,
                                       int delayMs)
{
    mAudioCommandThread->parametersCommand(ioHandle, keyValuePairs,
                                           delayMs);
}

int AudioPolicyService::setStreamVolume(audio_stream_type_t stream,
                                        float volume,
                                        audio_io_handle_t output,
                                        int delayMs)
{
    return (int)mAudioCommandThread->volumeCommand(stream, volume,
                                                   output, delayMs);
}

int AudioPolicyService::startTone(audio_policy_tone_t tone,
                                  audio_stream_type_t stream)
{
    if (tone != AUDIO_POLICY_TONE_IN_CALL_NOTIFICATION) {
        ALOGE("startTone: illegal tone requested (%d)", tone);
    }
    if (stream != AUDIO_STREAM_VOICE_CALL) {
        ALOGE("startTone: illegal stream (%d) requested for tone %d", stream,
            tone);
    }
    mTonePlaybackThread->startToneCommand(ToneGenerator::TONE_SUP_CALL_WAITING,
                                          AUDIO_STREAM_VOICE_CALL);
    return 0;
}

int AudioPolicyService::stopTone()
{
    mTonePlaybackThread->stopToneCommand();
    return 0;
}

int AudioPolicyService::setVoiceVolume(float volume, int delayMs)
{
    return (int)mAudioCommandThread->voiceVolumeCommand(volume, delayMs);
}

extern "C" {
audio_module_handle_t aps_load_hw_module(void *service __unused,
                                             const char *name);
audio_io_handle_t aps_open_output(void *service __unused,
                                         audio_devices_t *pDevices,
                                         uint32_t *pSamplingRate,
                                         audio_format_t *pFormat,
                                         audio_channel_mask_t *pChannelMask,
                                         uint32_t *pLatencyMs,
                                         audio_output_flags_t flags);

audio_io_handle_t aps_open_output_on_module(void *service __unused,
                                                   audio_module_handle_t module,
                                                   audio_devices_t *pDevices,
                                                   uint32_t *pSamplingRate,
                                                   audio_format_t *pFormat,
                                                   audio_channel_mask_t *pChannelMask,
                                                   uint32_t *pLatencyMs,
                                                   audio_output_flags_t flags,
                                                   const audio_offload_info_t *offloadInfo);
audio_io_handle_t aps_open_dup_output(void *service __unused,
                                                 audio_io_handle_t output1,
                                                 audio_io_handle_t output2);
int aps_close_output(void *service __unused, audio_io_handle_t output);
int aps_suspend_output(void *service __unused, audio_io_handle_t output);
int aps_restore_output(void *service __unused, audio_io_handle_t output);
audio_io_handle_t aps_open_input(void *service __unused,
                                        audio_devices_t *pDevices,
                                        uint32_t *pSamplingRate,
                                        audio_format_t *pFormat,
                                        audio_channel_mask_t *pChannelMask,
                                        audio_in_acoustics_t acoustics __unused);
audio_io_handle_t aps_open_input_on_module(void *service __unused,
                                                  audio_module_handle_t module,
                                                  audio_devices_t *pDevices,
                                                  uint32_t *pSamplingRate,
                                                  audio_format_t *pFormat,
                                                  audio_channel_mask_t *pChannelMask);
int aps_close_input(void *service __unused, audio_io_handle_t input);
int aps_invalidate_stream(void *service __unused, audio_stream_type_t stream);
int aps_move_effects(void *service __unused, audio_session_t session,
                                audio_io_handle_t src_output,
                                audio_io_handle_t dst_output);
char * aps_get_parameters(void *service __unused, audio_io_handle_t io_handle,
                                     const char *keys);
void aps_set_parameters(void *service, audio_io_handle_t io_handle,
                                   const char *kv_pairs, int delay_ms);
int aps_set_stream_volume(void *service, audio_stream_type_t stream,
                                     float volume, audio_io_handle_t output,
                                     int delay_ms);
int aps_start_tone(void *service, audio_policy_tone_t tone,
                              audio_stream_type_t stream);
int aps_stop_tone(void *service);
int aps_set_voice_volume(void *service, float volume, int delay_ms);
};

} // namespace android
