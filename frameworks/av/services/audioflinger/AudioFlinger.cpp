/*
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/


#define LOG_TAG "AudioFlinger"
//#define LOG_NDEBUG 0

#include "Configuration.h"
#include <dirent.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <cutils/multiuser.h>
#include <utils/Log.h>
#include <utils/Trace.h>
#include <binder/Parcel.h>
#include <media/audiohal/DeviceHalInterface.h>
#include <media/audiohal/DevicesFactoryHalInterface.h>
#include <media/audiohal/EffectsFactoryHalInterface.h>
#include <media/AudioParameter.h>
#include <media/TypeConverter.h>
#include <memunreachable/memunreachable.h>
#include <utils/String16.h>
#include <utils/threads.h>

#include <cutils/atomic.h>
#include <cutils/properties.h>

#include <system/audio.h>

#include "AudioFlinger.h"
#include "ServiceUtilities.h"

#include <media/AudioResamplerPublic.h>

#include <system/audio_effects/effect_visualizer.h>
#include <system/audio_effects/effect_ns.h>
#include <system/audio_effects/effect_aec.h>

#include <audio_utils/primitives.h>
#include <audio_utils/string.h>

#include <powermanager/PowerManager.h>

#include <media/IMediaLogService.h>
#include <media/MemoryLeakTrackUtil.h>
#include <media/nbaio/Pipe.h>
#include <media/nbaio/PipeReader.h>
#include <media/AudioParameter.h>
#include <mediautils/BatteryNotifier.h>
#include <private/android_filesystem_config.h>

//#define BUFLOG_NDEBUG 0
#include <BufLog.h>

#include "TypedLogger.h"

// ----------------------------------------------------------------------------

// Note: the following macro is used for extremely verbose logging message.  In
// order to run with ALOG_ASSERT turned on, we need to have LOG_NDEBUG set to
// 0; but one side effect of this is to turn all LOGV's as well.  Some messages
// are so verbose that we want to suppress them even when we have ALOG_ASSERT
// turned on.  Do not uncomment the #def below unless you really know what you
// are doing and want to see all of the extremely verbose messages.
//#define VERY_VERY_VERBOSE_LOGGING
#ifdef VERY_VERY_VERBOSE_LOGGING
#define ALOGVV ALOGV
#else
#define ALOGVV(a...) do { } while(0)
#endif

namespace android {

static const char kDeadlockedString[] = "AudioFlinger may be deadlocked\n";
static const char kHardwareLockedString[] = "Hardware lock is taken\n";
static const char kClientLockedString[] = "Client lock is taken\n";
static const char kNoEffectsFactory[] = "Effects Factory is absent\n";


nsecs_t AudioFlinger::mStandbyTimeInNsecs = kDefaultStandbyTimeInNsecs;

uint32_t AudioFlinger::mScreenState;


#ifdef TEE_SINK
bool AudioFlinger::mTeeSinkInputEnabled = false;
bool AudioFlinger::mTeeSinkOutputEnabled = false;
bool AudioFlinger::mTeeSinkTrackEnabled = false;

size_t AudioFlinger::mTeeSinkInputFrames = kTeeSinkInputFramesDefault;
size_t AudioFlinger::mTeeSinkOutputFrames = kTeeSinkOutputFramesDefault;
size_t AudioFlinger::mTeeSinkTrackFrames = kTeeSinkTrackFramesDefault;
#endif

// In order to avoid invalidating offloaded tracks each time a Visualizer is turned on and off
// we define a minimum time during which a global effect is considered enabled.
static const nsecs_t kMinGlobalEffectEnabletimeNs = seconds(7200);

Mutex gLock;
wp<AudioFlinger> gAudioFlinger;

// Keep a strong reference to media.log service around forever.
// The service is within our parent process so it can never die in a way that we could observe.
// These two variables are const after initialization.
static sp<IBinder> sMediaLogServiceAsBinder;
static sp<IMediaLogService> sMediaLogService;

static pthread_once_t sMediaLogOnce = PTHREAD_ONCE_INIT;

static void sMediaLogInit()
{
    sMediaLogServiceAsBinder = defaultServiceManager()->getService(String16("media.log"));
    if (sMediaLogServiceAsBinder != 0) {
        sMediaLogService = interface_cast<IMediaLogService>(sMediaLogServiceAsBinder);
    }
}

// ----------------------------------------------------------------------------

std::string formatToString(audio_format_t format) {
    std::string result;
    FormatConverter::toString(format, result);
    return result;
}

// ----------------------------------------------------------------------------

AudioFlinger::AudioFlinger()
    : BnAudioFlinger(),
      mMediaLogNotifier(new AudioFlinger::MediaLogNotifier()),
      mPrimaryHardwareDev(NULL),
      mAudioHwDevs(NULL),
      mHardwareStatus(AUDIO_HW_IDLE),
      mMasterVolume(1.0f),
      mMasterMute(false),
      // mNextUniqueId(AUDIO_UNIQUE_ID_USE_MAX),
      mMode(AUDIO_MODE_INVALID),
      mBtNrecIsOff(false),
      mIsLowRamDevice(true),
      mIsDeviceTypeKnown(false),
      mTotalMemory(0),
      mClientSharedHeapSize(kMinimumClientSharedHeapSizeBytes),
      mGlobalEffectEnableTime(0),
      mSystemReady(false)
{
    // unsigned instead of audio_unique_id_use_t, because ++ operator is unavailable for enum
    for (unsigned use = AUDIO_UNIQUE_ID_USE_UNSPECIFIED; use < AUDIO_UNIQUE_ID_USE_MAX; use++) {
        // zero ID has a special meaning, so unavailable
        mNextUniqueIds[use] = AUDIO_UNIQUE_ID_USE_MAX;
    }

    getpid_cached = getpid();
    const bool doLog = property_get_bool("ro.test_harness", false);
    if (doLog) {
        mLogMemoryDealer = new MemoryDealer(kLogMemorySize, "LogWriters",
                MemoryHeapBase::READ_ONLY);
        (void) pthread_once(&sMediaLogOnce, sMediaLogInit);
    }

    // reset battery stats.
    // if the audio service has crashed, battery stats could be left
    // in bad state, reset the state upon service start.
    BatteryNotifier::getInstance().noteResetAudio();

    mDevicesFactoryHal = DevicesFactoryHalInterface::create();
    mEffectsFactoryHal = EffectsFactoryHalInterface::create();

    mMediaLogNotifier->run("MediaLogNotifier");

#ifdef TEE_SINK
    char value[PROPERTY_VALUE_MAX];
    (void) property_get("ro.debuggable", value, "0");
    int debuggable = atoi(value);
    int teeEnabled = 0;
    if (debuggable) {
        (void) property_get("af.tee", value, "0");
        teeEnabled = atoi(value);
    }
    // FIXME symbolic constants here
    if (teeEnabled & 1) {
        mTeeSinkInputEnabled = true;
    }
    if (teeEnabled & 2) {
        mTeeSinkOutputEnabled = true;
    }
    if (teeEnabled & 4) {
        mTeeSinkTrackEnabled = true;
    }
#endif
}

void AudioFlinger::onFirstRef()
{
    Mutex::Autolock _l(mLock);

    /* TODO: move all this work into an Init() function */
    char val_str[PROPERTY_VALUE_MAX] = { 0 };
    if (property_get("ro.audio.flinger_standbytime_ms", val_str, NULL) >= 0) {
        uint32_t int_val;
        if (1 == sscanf(val_str, "%u", &int_val)) {
            mStandbyTimeInNsecs = milliseconds(int_val);
            ALOGI("Using %u mSec as standby time.", int_val);
        } else {
            mStandbyTimeInNsecs = kDefaultStandbyTimeInNsecs;
            ALOGI("Using default %u mSec as standby time.",
                    (uint32_t)(mStandbyTimeInNsecs / 1000000));
        }
    }

    mPatchPanel = new PatchPanel(this);

    mMode = AUDIO_MODE_NORMAL;

    gAudioFlinger = this;
}

AudioFlinger::~AudioFlinger()
{
    while (!mRecordThreads.isEmpty()) {
        // closeInput_nonvirtual() will remove specified entry from mRecordThreads
        closeInput_nonvirtual(mRecordThreads.keyAt(0));
    }
    while (!mPlaybackThreads.isEmpty()) {
        // closeOutput_nonvirtual() will remove specified entry from mPlaybackThreads
        closeOutput_nonvirtual(mPlaybackThreads.keyAt(0));
    }

    for (size_t i = 0; i < mAudioHwDevs.size(); i++) {
        // no mHardwareLock needed, as there are no other references to this
        delete mAudioHwDevs.valueAt(i);
    }

    // Tell media.log service about any old writers that still need to be unregistered
    if (sMediaLogService != 0) {
        for (size_t count = mUnregisteredWriters.size(); count > 0; count--) {
            sp<IMemory> iMemory(mUnregisteredWriters.top()->getIMemory());
            mUnregisteredWriters.pop();
            sMediaLogService->unregisterWriter(iMemory);
        }
    }
}

//static
__attribute__ ((visibility ("default")))
status_t MmapStreamInterface::openMmapStream(MmapStreamInterface::stream_direction_t direction,
                                             const audio_attributes_t *attr,
                                             audio_config_base_t *config,
                                             const AudioClient& client,
                                             audio_port_handle_t *deviceId,
                                             audio_session_t *sessionId,
                                             const sp<MmapStreamCallback>& callback,
                                             sp<MmapStreamInterface>& interface,
                                             audio_port_handle_t *handle)
{
    sp<AudioFlinger> af;
    {
        Mutex::Autolock _l(gLock);
        af = gAudioFlinger.promote();
    }
    status_t ret = NO_INIT;
    if (af != 0) {
        ret = af->openMmapStream(
                direction, attr, config, client, deviceId,
                sessionId, callback, interface, handle);
    }
    return ret;
}

status_t AudioFlinger::openMmapStream(MmapStreamInterface::stream_direction_t direction,
                                      const audio_attributes_t *attr,
                                      audio_config_base_t *config,
                                      const AudioClient& client,
                                      audio_port_handle_t *deviceId,
                                      audio_session_t *sessionId,
                                      const sp<MmapStreamCallback>& callback,
                                      sp<MmapStreamInterface>& interface,
                                      audio_port_handle_t *handle)
{
    status_t ret = initCheck();
    if (ret != NO_ERROR) {
        return ret;
    }
    audio_session_t actualSessionId = *sessionId;
    if (actualSessionId == AUDIO_SESSION_ALLOCATE) {
        actualSessionId = (audio_session_t) newAudioUniqueId(AUDIO_UNIQUE_ID_USE_SESSION);
    }
    audio_stream_type_t streamType = AUDIO_STREAM_DEFAULT;
    audio_io_handle_t io = AUDIO_IO_HANDLE_NONE;
    audio_port_handle_t portId = AUDIO_PORT_HANDLE_NONE;
    if (direction == MmapStreamInterface::DIRECTION_OUTPUT) {
        audio_config_t fullConfig = AUDIO_CONFIG_INITIALIZER;
        fullConfig.sample_rate = config->sample_rate;
        fullConfig.channel_mask = config->channel_mask;
        fullConfig.format = config->format;
        ret = AudioSystem::getOutputForAttr(attr, &io,
                                            actualSessionId,
                                            &streamType, client.clientPid, client.clientUid,
                                            &fullConfig,
                                            (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_MMAP_NOIRQ |
                                                    AUDIO_OUTPUT_FLAG_DIRECT),
                                            deviceId, &portId);
    } else {
        ret = AudioSystem::getInputForAttr(attr, &io,
                                              actualSessionId,
                                              client.clientPid,
                                              client.clientUid,
                                              client.packageName,
                                              config,
                                              AUDIO_INPUT_FLAG_MMAP_NOIRQ, deviceId, &portId);
    }
    if (ret != NO_ERROR) {
        return ret;
    }

    // at this stage, a MmapThread was created when openOutput() or openInput() was called by
    // audio policy manager and we can retrieve it
    sp<MmapThread> thread = mMmapThreads.valueFor(io);
    if (thread != 0) {
        interface = new MmapThreadHandle(thread);
        thread->configure(attr, streamType, actualSessionId, callback, *deviceId, portId);
        *handle = portId;
        *sessionId = actualSessionId;
    } else {
        if (direction == MmapStreamInterface::DIRECTION_OUTPUT) {
            AudioSystem::releaseOutput(io, streamType, actualSessionId);
        } else {
            AudioSystem::releaseInput(portId);
        }
        ret = NO_INIT;
    }

    ALOGV("%s done status %d portId %d", __FUNCTION__, ret, portId);

    return ret;
}

static const char * const audio_interfaces[] = {
    AUDIO_HARDWARE_MODULE_ID_PRIMARY,
    AUDIO_HARDWARE_MODULE_ID_A2DP,
    AUDIO_HARDWARE_MODULE_ID_USB,
};

AudioHwDevice* AudioFlinger::findSuitableHwDev_l(
        audio_module_handle_t module,
        audio_devices_t devices)
{
    // if module is 0, the request comes from an old policy manager and we should load
    // well known modules
    if (module == 0) {
        ALOGW("findSuitableHwDev_l() loading well know audio hw modules");
        for (size_t i = 0; i < arraysize(audio_interfaces); i++) {
            loadHwModule_l(audio_interfaces[i]);
        }
        // then try to find a module supporting the requested device.
        for (size_t i = 0; i < mAudioHwDevs.size(); i++) {
            AudioHwDevice *audioHwDevice = mAudioHwDevs.valueAt(i);
            sp<DeviceHalInterface> dev = audioHwDevice->hwDevice();
            uint32_t supportedDevices;
            if (dev->getSupportedDevices(&supportedDevices) == OK &&
                    (supportedDevices & devices) == devices) {
                return audioHwDevice;
            }
        }
    } else {
        // check a match for the requested module handle
        AudioHwDevice *audioHwDevice = mAudioHwDevs.valueFor(module);
        if (audioHwDevice != NULL) {
            return audioHwDevice;
        }
    }

    return NULL;
}

void AudioFlinger::dumpClients(int fd, const Vector<String16>& args __unused)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    result.append("Clients:\n");
    for (size_t i = 0; i < mClients.size(); ++i) {
        sp<Client> client = mClients.valueAt(i).promote();
        if (client != 0) {
            snprintf(buffer, SIZE, "  pid: %d\n", client->pid());
            result.append(buffer);
        }
    }

    result.append("Notification Clients:\n");
    for (size_t i = 0; i < mNotificationClients.size(); ++i) {
        snprintf(buffer, SIZE, "  pid: %d\n", mNotificationClients.keyAt(i));
        result.append(buffer);
    }

    result.append("Global session refs:\n");
    result.append("  session   pid count\n");
    for (size_t i = 0; i < mAudioSessionRefs.size(); i++) {
        AudioSessionRef *r = mAudioSessionRefs[i];
        snprintf(buffer, SIZE, "  %7d %5d %5d\n", r->mSessionid, r->mPid, r->mCnt);
        result.append(buffer);
    }
    write(fd, result.string(), result.size());
}


void AudioFlinger::dumpInternals(int fd, const Vector<String16>& args __unused)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    hardware_call_state hardwareStatus = mHardwareStatus;

    snprintf(buffer, SIZE, "Hardware status: %d\n"
                           "Standby Time mSec: %u\n",
                            hardwareStatus,
                            (uint32_t)(mStandbyTimeInNsecs / 1000000));
    result.append(buffer);
    write(fd, result.string(), result.size());
}

void AudioFlinger::dumpPermissionDenial(int fd, const Vector<String16>& args __unused)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    snprintf(buffer, SIZE, "Permission Denial: "
            "can't dump AudioFlinger from pid=%d, uid=%d\n",
            IPCThreadState::self()->getCallingPid(),
            IPCThreadState::self()->getCallingUid());
    result.append(buffer);
    write(fd, result.string(), result.size());
}

bool AudioFlinger::dumpTryLock(Mutex& mutex)
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

status_t AudioFlinger::dump(int fd, const Vector<String16>& args)
{
    if (!dumpAllowed()) {
        dumpPermissionDenial(fd, args);
    } else {
        // get state of hardware lock
        bool hardwareLocked = dumpTryLock(mHardwareLock);
        if (!hardwareLocked) {
            String8 result(kHardwareLockedString);
            write(fd, result.string(), result.size());
        } else {
            mHardwareLock.unlock();
        }

        bool locked = dumpTryLock(mLock);

        // failed to lock - AudioFlinger is probably deadlocked
        if (!locked) {
            String8 result(kDeadlockedString);
            write(fd, result.string(), result.size());
        }

        bool clientLocked = dumpTryLock(mClientLock);
        if (!clientLocked) {
            String8 result(kClientLockedString);
            write(fd, result.string(), result.size());
        }

        if (mEffectsFactoryHal != 0) {
            mEffectsFactoryHal->dumpEffects(fd);
        } else {
            String8 result(kNoEffectsFactory);
            write(fd, result.string(), result.size());
        }

        dumpClients(fd, args);
        if (clientLocked) {
            mClientLock.unlock();
        }

        dumpInternals(fd, args);

        // dump playback threads
        for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
            mPlaybackThreads.valueAt(i)->dump(fd, args);
        }

        // dump record threads
        for (size_t i = 0; i < mRecordThreads.size(); i++) {
            mRecordThreads.valueAt(i)->dump(fd, args);
        }

        // dump mmap threads
        for (size_t i = 0; i < mMmapThreads.size(); i++) {
            mMmapThreads.valueAt(i)->dump(fd, args);
        }

        // dump orphan effect chains
        if (mOrphanEffectChains.size() != 0) {
            write(fd, "  Orphan Effect Chains\n", strlen("  Orphan Effect Chains\n"));
            for (size_t i = 0; i < mOrphanEffectChains.size(); i++) {
                mOrphanEffectChains.valueAt(i)->dump(fd, args);
            }
        }
        // dump all hardware devs
        for (size_t i = 0; i < mAudioHwDevs.size(); i++) {
            sp<DeviceHalInterface> dev = mAudioHwDevs.valueAt(i)->hwDevice();
            dev->dump(fd);
        }

#ifdef TEE_SINK
        // dump the serially shared record tee sink
        if (mRecordTeeSource != 0) {
            dumpTee(fd, mRecordTeeSource, AUDIO_IO_HANDLE_NONE, 'C');
        }
#endif

        BUFLOG_RESET;

        if (locked) {
            mLock.unlock();
        }

        // append a copy of media.log here by forwarding fd to it, but don't attempt
        // to lookup the service if it's not running, as it will block for a second
        if (sMediaLogServiceAsBinder != 0) {
            dprintf(fd, "\nmedia.log:\n");
            Vector<String16> args;
            sMediaLogServiceAsBinder->dump(fd, args);
        }

        // check for optional arguments
        bool dumpMem = false;
        bool unreachableMemory = false;
        for (const auto &arg : args) {
            if (arg == String16("-m")) {
                dumpMem = true;
            } else if (arg == String16("--unreachable")) {
                unreachableMemory = true;
            }
        }

        if (dumpMem) {
            dprintf(fd, "\nDumping memory:\n");
            std::string s = dumpMemoryAddresses(100 /* limit */);
            write(fd, s.c_str(), s.size());
        }
        if (unreachableMemory) {
            dprintf(fd, "\nDumping unreachable memory:\n");
            // TODO - should limit be an argument parameter?
            std::string s = GetUnreachableMemoryString(true /* contents */, 100 /* limit */);
            write(fd, s.c_str(), s.size());
        }
    }
    return NO_ERROR;
}

sp<AudioFlinger::Client> AudioFlinger::registerPid(pid_t pid)
{
    Mutex::Autolock _cl(mClientLock);
    // If pid is already in the mClients wp<> map, then use that entry
    // (for which promote() is always != 0), otherwise create a new entry and Client.
    sp<Client> client = mClients.valueFor(pid).promote();
    if (client == 0) {
        client = new Client(this, pid);
        mClients.add(pid, client);
    }

    return client;
}

sp<NBLog::Writer> AudioFlinger::newWriter_l(size_t size, const char *name)
{
    // If there is no memory allocated for logs, return a dummy writer that does nothing.
    // Similarly if we can't contact the media.log service, also return a dummy writer.
    if (mLogMemoryDealer == 0 || sMediaLogService == 0) {
        return new NBLog::Writer();
    }
    sp<IMemory> shared = mLogMemoryDealer->allocate(NBLog::Timeline::sharedSize(size));
    // If allocation fails, consult the vector of previously unregistered writers
    // and garbage-collect one or more them until an allocation succeeds
    if (shared == 0) {
        Mutex::Autolock _l(mUnregisteredWritersLock);
        for (size_t count = mUnregisteredWriters.size(); count > 0; count--) {
            {
                // Pick the oldest stale writer to garbage-collect
                sp<IMemory> iMemory(mUnregisteredWriters[0]->getIMemory());
                mUnregisteredWriters.removeAt(0);
                sMediaLogService->unregisterWriter(iMemory);
                // Now the media.log remote reference to IMemory is gone.  When our last local
                // reference to IMemory also drops to zero at end of this block,
                // the IMemory destructor will deallocate the region from mLogMemoryDealer.
            }
            // Re-attempt the allocation
            shared = mLogMemoryDealer->allocate(NBLog::Timeline::sharedSize(size));
            if (shared != 0) {
                goto success;
            }
        }
        // Even after garbage-collecting all old writers, there is still not enough memory,
        // so return a dummy writer
        return new NBLog::Writer();
    }
success:
    NBLog::Shared *sharedRawPtr = (NBLog::Shared *) shared->pointer();
    new((void *) sharedRawPtr) NBLog::Shared(); // placement new here, but the corresponding
                                                // explicit destructor not needed since it is POD
    sMediaLogService->registerWriter(shared, size, name);
    return new NBLog::Writer(shared, size);
}

void AudioFlinger::unregisterWriter(const sp<NBLog::Writer>& writer)
{
    if (writer == 0) {
        return;
    }
    sp<IMemory> iMemory(writer->getIMemory());
    if (iMemory == 0) {
        return;
    }
    // Rather than removing the writer immediately, append it to a queue of old writers to
    // be garbage-collected later.  This allows us to continue to view old logs for a while.
    Mutex::Autolock _l(mUnregisteredWritersLock);
    mUnregisteredWriters.push(writer);
}

// IAudioFlinger interface

sp<IAudioTrack> AudioFlinger::createTrack(const CreateTrackInput& input,
                                          CreateTrackOutput& output,
                                          status_t *status)
{
    sp<PlaybackThread::Track> track;
    sp<TrackHandle> trackHandle;
    sp<Client> client;
    status_t lStatus;
    audio_stream_type_t streamType;
    audio_port_handle_t portId = AUDIO_PORT_HANDLE_NONE;

    bool updatePid = (input.clientInfo.clientPid == -1);
    const uid_t callingUid = IPCThreadState::self()->getCallingUid();
    uid_t clientUid = input.clientInfo.clientUid;
    if (!isTrustedCallingUid(callingUid)) {
        ALOGW_IF(clientUid != callingUid,
                "%s uid %d tried to pass itself off as %d",
                __FUNCTION__, callingUid, clientUid);
        clientUid = callingUid;
        updatePid = true;
    }
    pid_t clientPid = input.clientInfo.clientPid;
    if (updatePid) {
        const pid_t callingPid = IPCThreadState::self()->getCallingPid();
        ALOGW_IF(clientPid != -1 && clientPid != callingPid,
                 "%s uid %d pid %d tried to pass itself off as pid %d",
                 __func__, callingUid, callingPid, clientPid);
        clientPid = callingPid;
    }

    audio_session_t sessionId = input.sessionId;
    if (sessionId == AUDIO_SESSION_ALLOCATE) {
        sessionId = (audio_session_t) newAudioUniqueId(AUDIO_UNIQUE_ID_USE_SESSION);
    } else if (audio_unique_id_get_use(sessionId) != AUDIO_UNIQUE_ID_USE_SESSION) {
        lStatus = BAD_VALUE;
        goto Exit;
    }

    output.sessionId = sessionId;
    output.outputId = AUDIO_IO_HANDLE_NONE;
    output.selectedDeviceId = input.selectedDeviceId;

    lStatus = AudioSystem::getOutputForAttr(&input.attr, &output.outputId, sessionId, &streamType,
                                            clientPid, clientUid, &input.config, input.flags,
                                            &output.selectedDeviceId, &portId);

    if (lStatus != NO_ERROR || output.outputId == AUDIO_IO_HANDLE_NONE) {
        ALOGE("createTrack() getOutputForAttr() return error %d or invalid output handle", lStatus);
        goto Exit;
    }
    // client AudioTrack::set already implements AUDIO_STREAM_DEFAULT => AUDIO_STREAM_MUSIC,
    // but if someone uses binder directly they could bypass that and cause us to crash
    if (uint32_t(streamType) >= AUDIO_STREAM_CNT) {
        ALOGE("createTrack() invalid stream type %d", streamType);
        lStatus = BAD_VALUE;
        goto Exit;
    }

    // further channel mask checks are performed by createTrack_l() depending on the thread type
    if (!audio_is_output_channel(input.config.channel_mask)) {
        ALOGE("createTrack() invalid channel mask %#x", input.config.channel_mask);
        lStatus = BAD_VALUE;
        goto Exit;
    }

    // further format checks are performed by createTrack_l() depending on the thread type
    if (!audio_is_valid_format(input.config.format)) {
        ALOGE("createTrack() invalid format %#x", input.config.format);
        lStatus = BAD_VALUE;
        goto Exit;
    }

    {
        Mutex::Autolock _l(mLock);
        PlaybackThread *thread = checkPlaybackThread_l(output.outputId);
        if (thread == NULL) {
            ALOGE("no playback thread found for output handle %d", output.outputId);
            lStatus = BAD_VALUE;
            goto Exit;
        }

        client = registerPid(clientPid);

        PlaybackThread *effectThread = NULL;
        // check if an effect chain with the same session ID is present on another
        // output thread and move it here.
        for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
            sp<PlaybackThread> t = mPlaybackThreads.valueAt(i);
            if (mPlaybackThreads.keyAt(i) != output.outputId) {
                uint32_t sessions = t->hasAudioSession(sessionId);
                if (sessions & ThreadBase::EFFECT_SESSION) {
                    effectThread = t.get();
                    break;
                }
            }
        }
        ALOGV("createTrack() sessionId: %d", sessionId);

        output.sampleRate = input.config.sample_rate;
        output.frameCount = input.frameCount;
        output.notificationFrameCount = input.notificationFrameCount;
        output.flags = input.flags;

        track = thread->createTrack_l(client, streamType, input.attr, &output.sampleRate,
                                      input.config.format, input.config.channel_mask,
                                      &output.frameCount, &output.notificationFrameCount,
                                      input.notificationsPerBuffer, input.speed,
                                      input.sharedBuffer, sessionId, &output.flags,
                                      input.clientInfo.clientTid, clientUid, &lStatus, portId);
        LOG_ALWAYS_FATAL_IF((lStatus == NO_ERROR) && (track == 0));
        // we don't abort yet if lStatus != NO_ERROR; there is still work to be done regardless

        output.afFrameCount = thread->frameCount();
        output.afSampleRate = thread->sampleRate();
        output.afLatencyMs = thread->latency();

        // move effect chain to this output thread if an effect on same session was waiting
        // for a track to be created
        if (lStatus == NO_ERROR && effectThread != NULL) {
            // no risk of deadlock because AudioFlinger::mLock is held
            Mutex::Autolock _dl(thread->mLock);
            Mutex::Autolock _sl(effectThread->mLock);
            moveEffectChain_l(sessionId, effectThread, thread, true);
        }

        // Look for sync events awaiting for a session to be used.
        for (size_t i = 0; i < mPendingSyncEvents.size(); i++) {
            if (mPendingSyncEvents[i]->triggerSession() == sessionId) {
                if (thread->isValidSyncEvent(mPendingSyncEvents[i])) {
                    if (lStatus == NO_ERROR) {
                        (void) track->setSyncEvent(mPendingSyncEvents[i]);
                    } else {
                        mPendingSyncEvents[i]->cancel();
                    }
                    mPendingSyncEvents.removeAt(i);
                    i--;
                }
            }
        }

        setAudioHwSyncForSession_l(thread, sessionId);
    }

    if (lStatus != NO_ERROR) {
        // remove local strong reference to Client before deleting the Track so that the
        // Client destructor is called by the TrackBase destructor with mClientLock held
        // Don't hold mClientLock when releasing the reference on the track as the
        // destructor will acquire it.
        {
            Mutex::Autolock _cl(mClientLock);
            client.clear();
        }
        track.clear();
        goto Exit;
    }

    // return handle to client
    trackHandle = new TrackHandle(track);

Exit:
    if (lStatus != NO_ERROR && output.outputId != AUDIO_IO_HANDLE_NONE) {
        AudioSystem::releaseOutput(output.outputId, streamType, sessionId);
    }
    *status = lStatus;
    return trackHandle;
}

uint32_t AudioFlinger::sampleRate(audio_io_handle_t ioHandle) const
{
    Mutex::Autolock _l(mLock);
    ThreadBase *thread = checkThread_l(ioHandle);
    if (thread == NULL) {
        ALOGW("sampleRate() unknown thread %d", ioHandle);
        return 0;
    }
    return thread->sampleRate();
}

audio_format_t AudioFlinger::format(audio_io_handle_t output) const
{
    Mutex::Autolock _l(mLock);
    PlaybackThread *thread = checkPlaybackThread_l(output);
    if (thread == NULL) {
        ALOGW("format() unknown thread %d", output);
        return AUDIO_FORMAT_INVALID;
    }
    return thread->format();
}

size_t AudioFlinger::frameCount(audio_io_handle_t ioHandle) const
{
    Mutex::Autolock _l(mLock);
    ThreadBase *thread = checkThread_l(ioHandle);
    if (thread == NULL) {
        ALOGW("frameCount() unknown thread %d", ioHandle);
        return 0;
    }
    // FIXME currently returns the normal mixer's frame count to avoid confusing legacy callers;
    //       should examine all callers and fix them to handle smaller counts
    return thread->frameCount();
}

size_t AudioFlinger::frameCountHAL(audio_io_handle_t ioHandle) const
{
    Mutex::Autolock _l(mLock);
    ThreadBase *thread = checkThread_l(ioHandle);
    if (thread == NULL) {
        ALOGW("frameCountHAL() unknown thread %d", ioHandle);
        return 0;
    }
    return thread->frameCountHAL();
}

uint32_t AudioFlinger::latency(audio_io_handle_t output) const
{
    Mutex::Autolock _l(mLock);
    PlaybackThread *thread = checkPlaybackThread_l(output);
    if (thread == NULL) {
        ALOGW("latency(): no playback thread found for output handle %d", output);
        return 0;
    }
    return thread->latency();
}

status_t AudioFlinger::setMasterVolume(float value)
{
    status_t ret = initCheck();
    if (ret != NO_ERROR) {
        return ret;
    }

    // check calling permissions
    if (!settingsAllowed()) {
        return PERMISSION_DENIED;
    }

    Mutex::Autolock _l(mLock);
    mMasterVolume = value;

    // Set master volume in the HALs which support it.
    for (size_t i = 0; i < mAudioHwDevs.size(); i++) {
        AutoMutex lock(mHardwareLock);
        AudioHwDevice *dev = mAudioHwDevs.valueAt(i);

        mHardwareStatus = AUDIO_HW_SET_MASTER_VOLUME;
        if (dev->canSetMasterVolume()) {
            dev->hwDevice()->setMasterVolume(value);
        }
        mHardwareStatus = AUDIO_HW_IDLE;
    }

    // Now set the master volume in each playback thread.  Playback threads
    // assigned to HALs which do not have master volume support will apply
    // master volume during the mix operation.  Threads with HALs which do
    // support master volume will simply ignore the setting.
    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        if (mPlaybackThreads.valueAt(i)->isDuplicating()) {
            continue;
        }
        mPlaybackThreads.valueAt(i)->setMasterVolume(value);
    }

    return NO_ERROR;
}

status_t AudioFlinger::setMode(audio_mode_t mode)
{
    status_t ret = initCheck();
    if (ret != NO_ERROR) {
        return ret;
    }

    // check calling permissions
    if (!settingsAllowed()) {
        return PERMISSION_DENIED;
    }
    if (uint32_t(mode) >= AUDIO_MODE_CNT) {
        ALOGW("Illegal value: setMode(%d)", mode);
        return BAD_VALUE;
    }

    { // scope for the lock
        AutoMutex lock(mHardwareLock);
        sp<DeviceHalInterface> dev = mPrimaryHardwareDev->hwDevice();
        mHardwareStatus = AUDIO_HW_SET_MODE;
        ret = dev->setMode(mode);
        mHardwareStatus = AUDIO_HW_IDLE;
    }

    if (NO_ERROR == ret) {
        Mutex::Autolock _l(mLock);
        mMode = mode;
        for (size_t i = 0; i < mPlaybackThreads.size(); i++)
            mPlaybackThreads.valueAt(i)->setMode(mode);
    }

    return ret;
}

status_t AudioFlinger::setMicMute(bool state)
{
    status_t ret = initCheck();
    if (ret != NO_ERROR) {
        return ret;
    }

    // check calling permissions
    if (!settingsAllowed()) {
        return PERMISSION_DENIED;
    }

    AutoMutex lock(mHardwareLock);
    mHardwareStatus = AUDIO_HW_SET_MIC_MUTE;
    for (size_t i = 0; i < mAudioHwDevs.size(); i++) {
        sp<DeviceHalInterface> dev = mAudioHwDevs.valueAt(i)->hwDevice();
        status_t result = dev->setMicMute(state);
        if (result != NO_ERROR) {
            ret = result;
        }
    }
    mHardwareStatus = AUDIO_HW_IDLE;
    return ret;
}

bool AudioFlinger::getMicMute() const
{
    status_t ret = initCheck();
    if (ret != NO_ERROR) {
        return false;
    }
    bool mute = true;
    bool state = AUDIO_MODE_INVALID;
    AutoMutex lock(mHardwareLock);
    mHardwareStatus = AUDIO_HW_GET_MIC_MUTE;
    for (size_t i = 0; i < mAudioHwDevs.size(); i++) {
        sp<DeviceHalInterface> dev = mAudioHwDevs.valueAt(i)->hwDevice();
        status_t result = dev->getMicMute(&state);
        if (result == NO_ERROR) {
            mute = mute && state;
        }
    }
    mHardwareStatus = AUDIO_HW_IDLE;

    return mute;
}

void AudioFlinger::setRecordSilenced(uid_t uid, bool silenced)
{
    ALOGV("AudioFlinger::setRecordSilenced(uid:%d, silenced:%d)", uid, silenced);

    AutoMutex lock(mLock);
    for (size_t i = 0; i < mRecordThreads.size(); i++) {
        mRecordThreads[i]->setRecordSilenced(uid, silenced);
    }
    for (size_t i = 0; i < mMmapThreads.size(); i++) {
        mMmapThreads[i]->setRecordSilenced(uid, silenced);
    }
}

status_t AudioFlinger::setMasterMute(bool muted)
{
    status_t ret = initCheck();
    if (ret != NO_ERROR) {
        return ret;
    }

    // check calling permissions
    if (!settingsAllowed()) {
        return PERMISSION_DENIED;
    }

    Mutex::Autolock _l(mLock);
    mMasterMute = muted;

    // Set master mute in the HALs which support it.
    for (size_t i = 0; i < mAudioHwDevs.size(); i++) {
        AutoMutex lock(mHardwareLock);
        AudioHwDevice *dev = mAudioHwDevs.valueAt(i);

        mHardwareStatus = AUDIO_HW_SET_MASTER_MUTE;
        if (dev->canSetMasterMute()) {
            dev->hwDevice()->setMasterMute(muted);
        }
        mHardwareStatus = AUDIO_HW_IDLE;
    }

    // Now set the master mute in each playback thread.  Playback threads
    // assigned to HALs which do not have master mute support will apply master
    // mute during the mix operation.  Threads with HALs which do support master
    // mute will simply ignore the setting.
    Vector<VolumeInterface *> volumeInterfaces = getAllVolumeInterfaces_l();
    for (size_t i = 0; i < volumeInterfaces.size(); i++) {
        volumeInterfaces[i]->setMasterMute(muted);
    }

    return NO_ERROR;
}

float AudioFlinger::masterVolume() const
{
    Mutex::Autolock _l(mLock);
    return masterVolume_l();
}

bool AudioFlinger::masterMute() const
{
    Mutex::Autolock _l(mLock);
    return masterMute_l();
}

float AudioFlinger::masterVolume_l() const
{
    return mMasterVolume;
}

bool AudioFlinger::masterMute_l() const
{
    return mMasterMute;
}

status_t AudioFlinger::checkStreamType(audio_stream_type_t stream) const
{
    if (uint32_t(stream) >= AUDIO_STREAM_CNT) {
        ALOGW("checkStreamType() invalid stream %d", stream);
        return BAD_VALUE;
    }
    pid_t caller = IPCThreadState::self()->getCallingPid();
    if (uint32_t(stream) >= AUDIO_STREAM_PUBLIC_CNT && caller != getpid_cached) {
        ALOGW("checkStreamType() pid %d cannot use internal stream type %d", caller, stream);
        return PERMISSION_DENIED;
    }

    return NO_ERROR;
}

status_t AudioFlinger::setStreamVolume(audio_stream_type_t stream, float value,
        audio_io_handle_t output)
{
    // check calling permissions
    if (!settingsAllowed()) {
        return PERMISSION_DENIED;
    }

    status_t status = checkStreamType(stream);
    if (status != NO_ERROR) {
        return status;
    }
    if (output == AUDIO_IO_HANDLE_NONE) {
        return BAD_VALUE;
    }
    ALOG_ASSERT(stream != AUDIO_STREAM_PATCH, "attempt to change AUDIO_STREAM_PATCH volume");

    AutoMutex lock(mLock);
    VolumeInterface *volumeInterface = getVolumeInterface_l(output);
    if (volumeInterface == NULL) {
        return BAD_VALUE;
    }
    volumeInterface->setStreamVolume(stream, value);

    return NO_ERROR;
}

status_t AudioFlinger::setStreamMute(audio_stream_type_t stream, bool muted)
{
    // check calling permissions
    if (!settingsAllowed()) {
        return PERMISSION_DENIED;
    }

    status_t status = checkStreamType(stream);
    if (status != NO_ERROR) {
        return status;
    }
    ALOG_ASSERT(stream != AUDIO_STREAM_PATCH, "attempt to mute AUDIO_STREAM_PATCH");

    if (uint32_t(stream) == AUDIO_STREAM_ENFORCED_AUDIBLE) {
        ALOGE("setStreamMute() invalid stream %d", stream);
        return BAD_VALUE;
    }

    AutoMutex lock(mLock);
    mStreamTypes[stream].mute = muted;
    Vector<VolumeInterface *> volumeInterfaces = getAllVolumeInterfaces_l();
    for (size_t i = 0; i < volumeInterfaces.size(); i++) {
        volumeInterfaces[i]->setStreamMute(stream, muted);
    }

    return NO_ERROR;
}

float AudioFlinger::streamVolume(audio_stream_type_t stream, audio_io_handle_t output) const
{
    status_t status = checkStreamType(stream);
    if (status != NO_ERROR) {
        return 0.0f;
    }
    if (output == AUDIO_IO_HANDLE_NONE) {
        return 0.0f;
    }

    AutoMutex lock(mLock);
    VolumeInterface *volumeInterface = getVolumeInterface_l(output);
    if (volumeInterface == NULL) {
        return 0.0f;
    }

    return volumeInterface->streamVolume(stream);
}

bool AudioFlinger::streamMute(audio_stream_type_t stream) const
{
    status_t status = checkStreamType(stream);
    if (status != NO_ERROR) {
        return true;
    }

    AutoMutex lock(mLock);
    return streamMute_l(stream);
}


void AudioFlinger::broacastParametersToRecordThreads_l(const String8& keyValuePairs)
{
    for (size_t i = 0; i < mRecordThreads.size(); i++) {
        mRecordThreads.valueAt(i)->setParameters(keyValuePairs);
    }
}

// Filter reserved keys from setParameters() before forwarding to audio HAL or acting upon.
// Some keys are used for audio routing and audio path configuration and should be reserved for use
// by audio policy and audio flinger for functional, privacy and security reasons.
void AudioFlinger::filterReservedParameters(String8& keyValuePairs, uid_t callingUid)
{
    static const String8 kReservedParameters[] = {
        String8(AudioParameter::keyRouting),
        String8(AudioParameter::keySamplingRate),
        String8(AudioParameter::keyFormat),
        String8(AudioParameter::keyChannels),
        String8(AudioParameter::keyFrameCount),
        String8(AudioParameter::keyInputSource),
        String8(AudioParameter::keyMonoOutput),
        String8(AudioParameter::keyStreamConnect),
        String8(AudioParameter::keyStreamDisconnect),
        String8(AudioParameter::keyStreamSupportedFormats),
        String8(AudioParameter::keyStreamSupportedChannels),
        String8(AudioParameter::keyStreamSupportedSamplingRates),
    };

    // multiuser friendly app ID check for requests coming from audioserver
    if (multiuser_get_app_id(callingUid) == AID_AUDIOSERVER) {
        return;
    }

    AudioParameter param = AudioParameter(keyValuePairs);
    String8 value;
    for (auto& key : kReservedParameters) {
        if (param.get(key, value) == NO_ERROR) {
            ALOGW("%s: filtering key %s value %s from uid %d",
                  __func__, key.string(), value.string(), callingUid);
            param.remove(key);
        }
    }
    keyValuePairs = param.toString();
}

status_t AudioFlinger::setParameters(audio_io_handle_t ioHandle, const String8& keyValuePairs)
{
    ALOGV("setParameters(): io %d, keyvalue %s, calling pid %d calling uid %d",
            ioHandle, keyValuePairs.string(),
            IPCThreadState::self()->getCallingPid(), IPCThreadState::self()->getCallingUid());

    // check calling permissions
    if (!settingsAllowed()) {
        return PERMISSION_DENIED;
    }

    String8 filteredKeyValuePairs = keyValuePairs;
    filterReservedParameters(filteredKeyValuePairs, IPCThreadState::self()->getCallingUid());

    ALOGV("%s: filtered keyvalue %s", __func__, filteredKeyValuePairs.string());

    // AUDIO_IO_HANDLE_NONE means the parameters are global to the audio hardware interface
    if (ioHandle == AUDIO_IO_HANDLE_NONE) {
        Mutex::Autolock _l(mLock);
        // result will remain NO_INIT if no audio device is present
        status_t final_result = NO_INIT;
        {
            AutoMutex lock(mHardwareLock);
            mHardwareStatus = AUDIO_HW_SET_PARAMETER;
            for (size_t i = 0; i < mAudioHwDevs.size(); i++) {
                sp<DeviceHalInterface> dev = mAudioHwDevs.valueAt(i)->hwDevice();
                status_t result = dev->setParameters(filteredKeyValuePairs);
                // return success if at least one audio device accepts the parameters as not all
                // HALs are requested to support all parameters. If no audio device supports the
                // requested parameters, the last error is reported.
                if (final_result != NO_ERROR) {
                    final_result = result;
                }
            }
            mHardwareStatus = AUDIO_HW_IDLE;
        }
        // disable AEC and NS if the device is a BT SCO headset supporting those pre processings
        AudioParameter param = AudioParameter(filteredKeyValuePairs);
        String8 value;
        if (param.get(String8(AudioParameter::keyBtNrec), value) == NO_ERROR) {
            bool btNrecIsOff = (value == AudioParameter::valueOff);
            if (mBtNrecIsOff.exchange(btNrecIsOff) != btNrecIsOff) {
                for (size_t i = 0; i < mRecordThreads.size(); i++) {
                    mRecordThreads.valueAt(i)->checkBtNrec();
                }
            }
        }
        String8 screenState;
        if (param.get(String8(AudioParameter::keyScreenState), screenState) == NO_ERROR) {
            bool isOff = (screenState == AudioParameter::valueOff);
            if (isOff != (AudioFlinger::mScreenState & 1)) {
                AudioFlinger::mScreenState = ((AudioFlinger::mScreenState & ~1) + 2) | isOff;
            }
        }
        return final_result;
    }

    // hold a strong ref on thread in case closeOutput() or closeInput() is called
    // and the thread is exited once the lock is released
    sp<ThreadBase> thread;
    {
        Mutex::Autolock _l(mLock);
        thread = checkPlaybackThread_l(ioHandle);
        if (thread == 0) {
            thread = checkRecordThread_l(ioHandle);
            if (thread == 0) {
                thread = checkMmapThread_l(ioHandle);
            }
        } else if (thread == primaryPlaybackThread_l()) {
            // indicate output device change to all input threads for pre processing
            AudioParameter param = AudioParameter(filteredKeyValuePairs);
            int value;
            if ((param.getInt(String8(AudioParameter::keyRouting), value) == NO_ERROR) &&
                    (value != 0)) {
                broacastParametersToRecordThreads_l(filteredKeyValuePairs);
            }
        }
    }
    if (thread != 0) {
        return thread->setParameters(filteredKeyValuePairs);
    }
    return BAD_VALUE;
}

String8 AudioFlinger::getParameters(audio_io_handle_t ioHandle, const String8& keys) const
{
    ALOGVV("getParameters() io %d, keys %s, calling pid %d",
            ioHandle, keys.string(), IPCThreadState::self()->getCallingPid());

    Mutex::Autolock _l(mLock);

    if (ioHandle == AUDIO_IO_HANDLE_NONE) {
        String8 out_s8;

        for (size_t i = 0; i < mAudioHwDevs.size(); i++) {
            String8 s;
            status_t result;
            {
            AutoMutex lock(mHardwareLock);
            mHardwareStatus = AUDIO_HW_GET_PARAMETER;
            sp<DeviceHalInterface> dev = mAudioHwDevs.valueAt(i)->hwDevice();
            result = dev->getParameters(keys, &s);
            mHardwareStatus = AUDIO_HW_IDLE;
            }
            if (result == OK) out_s8 += s;
        }
        return out_s8;
    }

    ThreadBase *thread = (ThreadBase *)checkPlaybackThread_l(ioHandle);
    if (thread == NULL) {
        thread = (ThreadBase *)checkRecordThread_l(ioHandle);
        if (thread == NULL) {
            thread = (ThreadBase *)checkMmapThread_l(ioHandle);
            if (thread == NULL) {
                return String8("");
            }
        }
    }
    return thread->getParameters(keys);
}

size_t AudioFlinger::getInputBufferSize(uint32_t sampleRate, audio_format_t format,
        audio_channel_mask_t channelMask) const
{
    status_t ret = initCheck();
    if (ret != NO_ERROR) {
        return 0;
    }
    if ((sampleRate == 0) ||
            !audio_is_valid_format(format) || !audio_has_proportional_frames(format) ||
            !audio_is_input_channel(channelMask)) {
        return 0;
    }

    AutoMutex lock(mHardwareLock);
    mHardwareStatus = AUDIO_HW_GET_INPUT_BUFFER_SIZE;
    audio_config_t config, proposed;
    memset(&proposed, 0, sizeof(proposed));
    proposed.sample_rate = sampleRate;
    proposed.channel_mask = channelMask;
    proposed.format = format;

    sp<DeviceHalInterface> dev = mPrimaryHardwareDev->hwDevice();
    size_t frames;
    for (;;) {
        // Note: config is currently a const parameter for get_input_buffer_size()
        // but we use a copy from proposed in case config changes from the call.
        config = proposed;
        status_t result = dev->getInputBufferSize(&config, &frames);
        if (result == OK && frames != 0) {
            break; // hal success, config is the result
        }
        // change one parameter of the configuration each iteration to a more "common" value
        // to see if the device will support it.
        if (proposed.format != AUDIO_FORMAT_PCM_16_BIT) {
            proposed.format = AUDIO_FORMAT_PCM_16_BIT;
        } else if (proposed.sample_rate != 44100) { // 44.1 is claimed as must in CDD as well as
            proposed.sample_rate = 44100;           // legacy AudioRecord.java. TODO: Query hw?
        } else {
            ALOGW("getInputBufferSize failed with minimum buffer size sampleRate %u, "
                    "format %#x, channelMask 0x%X",
                    sampleRate, format, channelMask);
            break; // retries failed, break out of loop with frames == 0.
        }
    }
    mHardwareStatus = AUDIO_HW_IDLE;
    if (frames > 0 && config.sample_rate != sampleRate) {
        frames = destinationFramesPossible(frames, sampleRate, config.sample_rate);
    }
    return frames; // may be converted to bytes at the Java level.
}

uint32_t AudioFlinger::getInputFramesLost(audio_io_handle_t ioHandle) const
{
    Mutex::Autolock _l(mLock);

    RecordThread *recordThread = checkRecordThread_l(ioHandle);
    if (recordThread != NULL) {
        return recordThread->getInputFramesLost();
    }
    return 0;
}

status_t AudioFlinger::setVoiceVolume(float value)
{
    status_t ret = initCheck();
    if (ret != NO_ERROR) {
        return ret;
    }

    // check calling permissions
    if (!settingsAllowed()) {
        return PERMISSION_DENIED;
    }

    AutoMutex lock(mHardwareLock);
    sp<DeviceHalInterface> dev = mPrimaryHardwareDev->hwDevice();
    mHardwareStatus = AUDIO_HW_SET_VOICE_VOLUME;
    ret = dev->setVoiceVolume(value);
    mHardwareStatus = AUDIO_HW_IDLE;

    return ret;
}

status_t AudioFlinger::getRenderPosition(uint32_t *halFrames, uint32_t *dspFrames,
        audio_io_handle_t output) const
{
    Mutex::Autolock _l(mLock);

    PlaybackThread *playbackThread = checkPlaybackThread_l(output);
    if (playbackThread != NULL) {
        return playbackThread->getRenderPosition(halFrames, dspFrames);
    }

    return BAD_VALUE;
}

void AudioFlinger::registerClient(const sp<IAudioFlingerClient>& client)
{
    Mutex::Autolock _l(mLock);
    if (client == 0) {
        return;
    }
    pid_t pid = IPCThreadState::self()->getCallingPid();
    {
        Mutex::Autolock _cl(mClientLock);
        if (mNotificationClients.indexOfKey(pid) < 0) {
            sp<NotificationClient> notificationClient = new NotificationClient(this,
                                                                                client,
                                                                                pid);
            ALOGV("registerClient() client %p, pid %d", notificationClient.get(), pid);

            mNotificationClients.add(pid, notificationClient);

            sp<IBinder> binder = IInterface::asBinder(client);
            binder->linkToDeath(notificationClient);
        }
    }

    // mClientLock should not be held here because ThreadBase::sendIoConfigEvent() will lock the
    // ThreadBase mutex and the locking order is ThreadBase::mLock then AudioFlinger::mClientLock.
    // the config change is always sent from playback or record threads to avoid deadlock
    // with AudioSystem::gLock
    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        mPlaybackThreads.valueAt(i)->sendIoConfigEvent(AUDIO_OUTPUT_REGISTERED, pid);
    }

    for (size_t i = 0; i < mRecordThreads.size(); i++) {
        mRecordThreads.valueAt(i)->sendIoConfigEvent(AUDIO_INPUT_REGISTERED, pid);
    }
}

void AudioFlinger::removeNotificationClient(pid_t pid)
{
    Mutex::Autolock _l(mLock);
    {
        Mutex::Autolock _cl(mClientLock);
        mNotificationClients.removeItem(pid);
    }

    ALOGV("%d died, releasing its sessions", pid);
    size_t num = mAudioSessionRefs.size();
    bool removed = false;
    for (size_t i = 0; i < num; ) {
        AudioSessionRef *ref = mAudioSessionRefs.itemAt(i);
        ALOGV(" pid %d @ %zu", ref->mPid, i);
        if (ref->mPid == pid) {
            ALOGV(" removing entry for pid %d session %d", pid, ref->mSessionid);
            mAudioSessionRefs.removeAt(i);
            delete ref;
            removed = true;
            num--;
        } else {
            i++;
        }
    }
    if (removed) {
        purgeStaleEffects_l();
    }
}

void AudioFlinger::ioConfigChanged(audio_io_config_event event,
                                   const sp<AudioIoDescriptor>& ioDesc,
                                   pid_t pid)
{
    Mutex::Autolock _l(mClientLock);
    size_t size = mNotificationClients.size();
    for (size_t i = 0; i < size; i++) {
        if ((pid == 0) || (mNotificationClients.keyAt(i) == pid)) {
            mNotificationClients.valueAt(i)->audioFlingerClient()->ioConfigChanged(event, ioDesc);
        }
    }
}

// removeClient_l() must be called with AudioFlinger::mClientLock held
void AudioFlinger::removeClient_l(pid_t pid)
{
    ALOGV("removeClient_l() pid %d, calling pid %d", pid,
            IPCThreadState::self()->getCallingPid());
    mClients.removeItem(pid);
}

// getEffectThread_l() must be called with AudioFlinger::mLock held
sp<AudioFlinger::PlaybackThread> AudioFlinger::getEffectThread_l(audio_session_t sessionId,
        int EffectId)
{
    sp<PlaybackThread> thread;

    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        if (mPlaybackThreads.valueAt(i)->getEffect(sessionId, EffectId) != 0) {
            ALOG_ASSERT(thread == 0);
            thread = mPlaybackThreads.valueAt(i);
        }
    }

    return thread;
}



// ----------------------------------------------------------------------------

AudioFlinger::Client::Client(const sp<AudioFlinger>& audioFlinger, pid_t pid)
    :   RefBase(),
        mAudioFlinger(audioFlinger),
        mPid(pid)
{
    mMemoryDealer = new MemoryDealer(
            audioFlinger->getClientSharedHeapSize(),
            (std::string("AudioFlinger::Client(") + std::to_string(pid) + ")").c_str());
}

// Client destructor must be called with AudioFlinger::mClientLock held
AudioFlinger::Client::~Client()
{
    mAudioFlinger->removeClient_l(mPid);
}

sp<MemoryDealer> AudioFlinger::Client::heap() const
{
    return mMemoryDealer;
}

// ----------------------------------------------------------------------------

AudioFlinger::NotificationClient::NotificationClient(const sp<AudioFlinger>& audioFlinger,
                                                     const sp<IAudioFlingerClient>& client,
                                                     pid_t pid)
    : mAudioFlinger(audioFlinger), mPid(pid), mAudioFlingerClient(client)
{
}

AudioFlinger::NotificationClient::~NotificationClient()
{
}

void AudioFlinger::NotificationClient::binderDied(const wp<IBinder>& who __unused)
{
    sp<NotificationClient> keep(this);
    mAudioFlinger->removeNotificationClient(mPid);
}

// ----------------------------------------------------------------------------
AudioFlinger::MediaLogNotifier::MediaLogNotifier()
    : mPendingRequests(false) {}


void AudioFlinger::MediaLogNotifier::requestMerge() {
    AutoMutex _l(mMutex);
    mPendingRequests = true;
    mCond.signal();
}

bool AudioFlinger::MediaLogNotifier::threadLoop() {
    // Should already have been checked, but just in case
    if (sMediaLogService == 0) {
        return false;
    }
    // Wait until there are pending requests
    {
        AutoMutex _l(mMutex);
        mPendingRequests = false; // to ignore past requests
        while (!mPendingRequests) {
            mCond.wait(mMutex);
            // TODO may also need an exitPending check
        }
        mPendingRequests = false;
    }
    // Execute the actual MediaLogService binder call and ignore extra requests for a while
    sMediaLogService->requestMergeWakeup();
    usleep(kPostTriggerSleepPeriod);
    return true;
}

void AudioFlinger::requestLogMerge() {
    mMediaLogNotifier->requestMerge();
}

// ----------------------------------------------------------------------------

sp<media::IAudioRecord> AudioFlinger::createRecord(const CreateRecordInput& input,
                                                   CreateRecordOutput& output,
                                                   status_t *status)
{
    sp<RecordThread::RecordTrack> recordTrack;
    sp<RecordHandle> recordHandle;
    sp<Client> client;
    status_t lStatus;
    audio_session_t sessionId = input.sessionId;
    audio_port_handle_t portId = AUDIO_PORT_HANDLE_NONE;

    output.cblk.clear();
    output.buffers.clear();
    output.inputId = AUDIO_IO_HANDLE_NONE;

    bool updatePid = (input.clientInfo.clientPid == -1);
    const uid_t callingUid = IPCThreadState::self()->getCallingUid();
    uid_t clientUid = input.clientInfo.clientUid;
    if (!isTrustedCallingUid(callingUid)) {
        ALOGW_IF(clientUid != callingUid,
                "%s uid %d tried to pass itself off as %d",
                __FUNCTION__, callingUid, clientUid);
        clientUid = callingUid;
        updatePid = true;
    }
    pid_t clientPid = input.clientInfo.clientPid;
    if (updatePid) {
        const pid_t callingPid = IPCThreadState::self()->getCallingPid();
        ALOGW_IF(clientPid != -1 && clientPid != callingPid,
                 "%s uid %d pid %d tried to pass itself off as pid %d",
                 __func__, callingUid, callingPid, clientPid);
        clientPid = callingPid;
    }

    // we don't yet support anything other than linear PCM
    if (!audio_is_valid_format(input.config.format) || !audio_is_linear_pcm(input.config.format)) {
        ALOGE("createRecord() invalid format %#x", input.config.format);
        lStatus = BAD_VALUE;
        goto Exit;
    }

    // further channel mask checks are performed by createRecordTrack_l()
    if (!audio_is_input_channel(input.config.channel_mask)) {
        ALOGE("createRecord() invalid channel mask %#x", input.config.channel_mask);
        lStatus = BAD_VALUE;
        goto Exit;
    }

    if (sessionId == AUDIO_SESSION_ALLOCATE) {
        sessionId = (audio_session_t) newAudioUniqueId(AUDIO_UNIQUE_ID_USE_SESSION);
    } else if (audio_unique_id_get_use(sessionId) != AUDIO_UNIQUE_ID_USE_SESSION) {
        lStatus = BAD_VALUE;
        goto Exit;
    }

    output.sessionId = sessionId;
    output.selectedDeviceId = input.selectedDeviceId;
    output.flags = input.flags;

    client = registerPid(clientPid);

    // Not a conventional loop, but a retry loop for at most two iterations total.
    // Try first maybe with FAST flag then try again without FAST flag if that fails.
    // Exits loop via break on no error of got exit on error
    // The sp<> references will be dropped when re-entering scope.
    // The lack of indentation is deliberate, to reduce code churn and ease merges.
    for (;;) {
    // release previously opened input if retrying.
    if (output.inputId != AUDIO_IO_HANDLE_NONE) {
        recordTrack.clear();
        AudioSystem::releaseInput(portId);
        output.inputId = AUDIO_IO_HANDLE_NONE;
        output.selectedDeviceId = input.selectedDeviceId;
        portId = AUDIO_PORT_HANDLE_NONE;
    }
    lStatus = AudioSystem::getInputForAttr(&input.attr, &output.inputId,
                                      sessionId,
                                    // FIXME compare to AudioTrack
                                      clientPid,
                                      clientUid,
                                      input.opPackageName,
                                      &input.config,
                                      output.flags, &output.selectedDeviceId, &portId);

    {
        Mutex::Autolock _l(mLock);
        RecordThread *thread = checkRecordThread_l(output.inputId);
        if (thread == NULL) {
            ALOGE("createRecord() checkRecordThread_l failed");
            lStatus = BAD_VALUE;
            goto Exit;
        }

        ALOGV("createRecord() lSessionId: %d input %d", sessionId, output.inputId);

        output.sampleRate = input.config.sample_rate;
        output.frameCount = input.frameCount;
        output.notificationFrameCount = input.notificationFrameCount;

        recordTrack = thread->createRecordTrack_l(client, input.attr, &output.sampleRate,
                                                  input.config.format, input.config.channel_mask,
                                                  &output.frameCount, sessionId,
                                                  &output.notificationFrameCount,
                                                  clientUid, &output.flags,
                                                  input.clientInfo.clientTid,
                                                  &lStatus, portId);
        LOG_ALWAYS_FATAL_IF((lStatus == NO_ERROR) && (recordTrack == 0));

        // lStatus == BAD_TYPE means FAST flag was rejected: request a new input from
        // audio policy manager without FAST constraint
        if (lStatus == BAD_TYPE) {
            continue;
        }

        if (lStatus != NO_ERROR) {
            goto Exit;
        }

        // Check if one effect chain was awaiting for an AudioRecord to be created on this
        // session and move it to this thread.
        sp<EffectChain> chain = getOrphanEffectChain_l(sessionId);
        if (chain != 0) {
            Mutex::Autolock _l(thread->mLock);
            thread->addEffectChain_l(chain);
        }
        break;
    }
    // End of retry loop.
    // The lack of indentation is deliberate, to reduce code churn and ease merges.
    }

    output.cblk = recordTrack->getCblk();
    output.buffers = recordTrack->getBuffers();

    // return handle to client
    recordHandle = new RecordHandle(recordTrack);

Exit:
    if (lStatus != NO_ERROR) {
        // remove local strong reference to Client before deleting the RecordTrack so that the
        // Client destructor is called by the TrackBase destructor with mClientLock held
        // Don't hold mClientLock when releasing the reference on the track as the
        // destructor will acquire it.
        {
            Mutex::Autolock _cl(mClientLock);
            client.clear();
        }
        recordTrack.clear();
        if (output.inputId != AUDIO_IO_HANDLE_NONE) {
            AudioSystem::releaseInput(portId);
        }
    }

    *status = lStatus;
    return recordHandle;
}



// ----------------------------------------------------------------------------

audio_module_handle_t AudioFlinger::loadHwModule(const char *name)
{
    if (name == NULL) {
        return AUDIO_MODULE_HANDLE_NONE;
    }
    if (!settingsAllowed()) {
        return AUDIO_MODULE_HANDLE_NONE;
    }
    Mutex::Autolock _l(mLock);
    return loadHwModule_l(name);
}

// loadHwModule_l() must be called with AudioFlinger::mLock held
audio_module_handle_t AudioFlinger::loadHwModule_l(const char *name)
{
    for (size_t i = 0; i < mAudioHwDevs.size(); i++) {
        if (strncmp(mAudioHwDevs.valueAt(i)->moduleName(), name, strlen(name)) == 0) {
            ALOGW("loadHwModule() module %s already loaded", name);
            return mAudioHwDevs.keyAt(i);
        }
    }

    sp<DeviceHalInterface> dev;

    int rc = mDevicesFactoryHal->openDevice(name, &dev);
    if (rc) {
        ALOGE("loadHwModule() error %d loading module %s", rc, name);
        return AUDIO_MODULE_HANDLE_NONE;
    }

    mHardwareStatus = AUDIO_HW_INIT;
    rc = dev->initCheck();
    mHardwareStatus = AUDIO_HW_IDLE;
    if (rc) {
        ALOGE("loadHwModule() init check error %d for module %s", rc, name);
        return AUDIO_MODULE_HANDLE_NONE;
    }

    // Check and cache this HAL's level of support for master mute and master
    // volume.  If this is the first HAL opened, and it supports the get
    // methods, use the initial values provided by the HAL as the current
    // master mute and volume settings.

    AudioHwDevice::Flags flags = static_cast<AudioHwDevice::Flags>(0);
    {  // scope for auto-lock pattern
        AutoMutex lock(mHardwareLock);

        if (0 == mAudioHwDevs.size()) {
            mHardwareStatus = AUDIO_HW_GET_MASTER_VOLUME;
            float mv;
            if (OK == dev->getMasterVolume(&mv)) {
                mMasterVolume = mv;
            }

            mHardwareStatus = AUDIO_HW_GET_MASTER_MUTE;
            bool mm;
            if (OK == dev->getMasterMute(&mm)) {
                mMasterMute = mm;
            }
        }

        mHardwareStatus = AUDIO_HW_SET_MASTER_VOLUME;
        if (OK == dev->setMasterVolume(mMasterVolume)) {
            flags = static_cast<AudioHwDevice::Flags>(flags |
                    AudioHwDevice::AHWD_CAN_SET_MASTER_VOLUME);
        }

        mHardwareStatus = AUDIO_HW_SET_MASTER_MUTE;
        if (OK == dev->setMasterMute(mMasterMute)) {
            flags = static_cast<AudioHwDevice::Flags>(flags |
                    AudioHwDevice::AHWD_CAN_SET_MASTER_MUTE);
        }

        mHardwareStatus = AUDIO_HW_IDLE;
    }

    audio_module_handle_t handle = (audio_module_handle_t) nextUniqueId(AUDIO_UNIQUE_ID_USE_MODULE);
    mAudioHwDevs.add(handle, new AudioHwDevice(handle, name, dev, flags));

    ALOGI("loadHwModule() Loaded %s audio interface, handle %d", name, handle);

    return handle;

}

// ----------------------------------------------------------------------------

uint32_t AudioFlinger::getPrimaryOutputSamplingRate()
{
    Mutex::Autolock _l(mLock);
    PlaybackThread *thread = fastPlaybackThread_l();
    return thread != NULL ? thread->sampleRate() : 0;
}

size_t AudioFlinger::getPrimaryOutputFrameCount()
{
    Mutex::Autolock _l(mLock);
    PlaybackThread *thread = fastPlaybackThread_l();
    return thread != NULL ? thread->frameCountHAL() : 0;
}

// ----------------------------------------------------------------------------

status_t AudioFlinger::setLowRamDevice(bool isLowRamDevice, int64_t totalMemory)
{
    uid_t uid = IPCThreadState::self()->getCallingUid();
    if (uid != AID_SYSTEM) {
        return PERMISSION_DENIED;
    }
    Mutex::Autolock _l(mLock);
    if (mIsDeviceTypeKnown) {
        return INVALID_OPERATION;
    }
    mIsLowRamDevice = isLowRamDevice;
    mTotalMemory = totalMemory;
    // mIsLowRamDevice and mTotalMemory are obtained through ActivityManager;
    // see ActivityManager.isLowRamDevice() and ActivityManager.getMemoryInfo().
    // mIsLowRamDevice generally represent devices with less than 1GB of memory,
    // though actual setting is determined through device configuration.
    constexpr int64_t GB = 1024 * 1024 * 1024;
    mClientSharedHeapSize =
            isLowRamDevice ? kMinimumClientSharedHeapSizeBytes
                    : mTotalMemory < 2 * GB ? 4 * kMinimumClientSharedHeapSizeBytes
                    : mTotalMemory < 3 * GB ? 8 * kMinimumClientSharedHeapSizeBytes
                    : mTotalMemory < 4 * GB ? 16 * kMinimumClientSharedHeapSizeBytes
                    : 32 * kMinimumClientSharedHeapSizeBytes;
    mIsDeviceTypeKnown = true;

    // TODO: Cache the client shared heap size in a persistent property.
    // It's possible that a native process or Java service or app accesses audioserver
    // after it is registered by system server, but before AudioService updates
    // the memory info.  This would occur immediately after boot or an audioserver
    // crash and restore. Before update from AudioService, the client would get the
    // minimum heap size.

    ALOGD("isLowRamDevice:%s totalMemory:%lld mClientSharedHeapSize:%zu",
            (isLowRamDevice ? "true" : "false"),
            (long long)mTotalMemory,
            mClientSharedHeapSize.load());
    return NO_ERROR;
}

size_t AudioFlinger::getClientSharedHeapSize() const
{
    size_t heapSizeInBytes = property_get_int32("ro.af.client_heap_size_kbyte", 0) * 1024;
    if (heapSizeInBytes != 0) { // read-only property overrides all.
        return heapSizeInBytes;
    }
    return mClientSharedHeapSize;
}

audio_hw_sync_t AudioFlinger::getAudioHwSyncForSession(audio_session_t sessionId)
{
    Mutex::Autolock _l(mLock);

    ssize_t index = mHwAvSyncIds.indexOfKey(sessionId);
    if (index >= 0) {
        ALOGV("getAudioHwSyncForSession found ID %d for session %d",
              mHwAvSyncIds.valueAt(index), sessionId);
        return mHwAvSyncIds.valueAt(index);
    }

    sp<DeviceHalInterface> dev = mPrimaryHardwareDev->hwDevice();
    if (dev == NULL) {
        return AUDIO_HW_SYNC_INVALID;
    }
    String8 reply;
    AudioParameter param;
    if (dev->getParameters(String8(AudioParameter::keyHwAvSync), &reply) == OK) {
        param = AudioParameter(reply);
    }

    int value;
    if (param.getInt(String8(AudioParameter::keyHwAvSync), value) != NO_ERROR) {
        ALOGW("getAudioHwSyncForSession error getting sync for session %d", sessionId);
        return AUDIO_HW_SYNC_INVALID;
    }

    // allow only one session for a given HW A/V sync ID.
    for (size_t i = 0; i < mHwAvSyncIds.size(); i++) {
        if (mHwAvSyncIds.valueAt(i) == (audio_hw_sync_t)value) {
            ALOGV("getAudioHwSyncForSession removing ID %d for session %d",
                  value, mHwAvSyncIds.keyAt(i));
            mHwAvSyncIds.removeItemsAt(i);
            break;
        }
    }

    mHwAvSyncIds.add(sessionId, value);

    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        sp<PlaybackThread> thread = mPlaybackThreads.valueAt(i);
        uint32_t sessions = thread->hasAudioSession(sessionId);
        if (sessions & ThreadBase::TRACK_SESSION) {
            AudioParameter param = AudioParameter();
            param.addInt(String8(AudioParameter::keyStreamHwAvSync), value);
            thread->setParameters(param.toString());
            break;
        }
    }

    ALOGV("getAudioHwSyncForSession adding ID %d for session %d", value, sessionId);
    return (audio_hw_sync_t)value;
}

status_t AudioFlinger::systemReady()
{
    Mutex::Autolock _l(mLock);
    ALOGI("%s", __FUNCTION__);
    if (mSystemReady) {
        ALOGW("%s called twice", __FUNCTION__);
        return NO_ERROR;
    }
    mSystemReady = true;
    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        ThreadBase *thread = (ThreadBase *)mPlaybackThreads.valueAt(i).get();
        thread->systemReady();
    }
    for (size_t i = 0; i < mRecordThreads.size(); i++) {
        ThreadBase *thread = (ThreadBase *)mRecordThreads.valueAt(i).get();
        thread->systemReady();
    }
    return NO_ERROR;
}

status_t AudioFlinger::getMicrophones(std::vector<media::MicrophoneInfo> *microphones)
{
    AutoMutex lock(mHardwareLock);
    sp<DeviceHalInterface> dev = mPrimaryHardwareDev->hwDevice();
    status_t status = dev->getMicrophones(microphones);
    return status;
}

// setAudioHwSyncForSession_l() must be called with AudioFlinger::mLock held
void AudioFlinger::setAudioHwSyncForSession_l(PlaybackThread *thread, audio_session_t sessionId)
{
    ssize_t index = mHwAvSyncIds.indexOfKey(sessionId);
    if (index >= 0) {
        audio_hw_sync_t syncId = mHwAvSyncIds.valueAt(index);
        ALOGV("setAudioHwSyncForSession_l found ID %d for session %d", syncId, sessionId);
        AudioParameter param = AudioParameter();
        param.addInt(String8(AudioParameter::keyStreamHwAvSync), syncId);
        thread->setParameters(param.toString());
    }
}


// ----------------------------------------------------------------------------


sp<AudioFlinger::ThreadBase> AudioFlinger::openOutput_l(audio_module_handle_t module,
                                                            audio_io_handle_t *output,
                                                            audio_config_t *config,
                                                            audio_devices_t devices,
                                                            const String8& address,
                                                            audio_output_flags_t flags)
{
    AudioHwDevice *outHwDev = findSuitableHwDev_l(module, devices);
    if (outHwDev == NULL) {
        return 0;
    }

    if (*output == AUDIO_IO_HANDLE_NONE) {
        *output = nextUniqueId(AUDIO_UNIQUE_ID_USE_OUTPUT);
    } else {
        // Audio Policy does not currently request a specific output handle.
        // If this is ever needed, see openInput_l() for example code.
        ALOGE("openOutput_l requested output handle %d is not AUDIO_IO_HANDLE_NONE", *output);
        return 0;
    }

    mHardwareStatus = AUDIO_HW_OUTPUT_OPEN;

    // FOR TESTING ONLY:
    // This if statement allows overriding the audio policy settings
    // and forcing a specific format or channel mask to the HAL/Sink device for testing.
    if (!(flags & (AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD | AUDIO_OUTPUT_FLAG_DIRECT))) {
        // Check only for Normal Mixing mode
        if (kEnableExtendedPrecision) {
            // Specify format (uncomment one below to choose)
            //config->format = AUDIO_FORMAT_PCM_FLOAT;
            //config->format = AUDIO_FORMAT_PCM_24_BIT_PACKED;
            //config->format = AUDIO_FORMAT_PCM_32_BIT;
            //config->format = AUDIO_FORMAT_PCM_8_24_BIT;
            // ALOGV("openOutput_l() upgrading format to %#08x", config->format);
        }
        if (kEnableExtendedChannels) {
            // Specify channel mask (uncomment one below to choose)
            //config->channel_mask = audio_channel_out_mask_from_count(4);  // for USB 4ch
            //config->channel_mask = audio_channel_mask_from_representation_and_bits(
            //        AUDIO_CHANNEL_REPRESENTATION_INDEX, (1 << 4) - 1);  // another 4ch example
        }
    }

    AudioStreamOut *outputStream = NULL;
    status_t status = outHwDev->openOutputStream(
            &outputStream,
            *output,
            devices,
            flags,
            config,
            address.string());

    mHardwareStatus = AUDIO_HW_IDLE;

    if (status == NO_ERROR) {
        if (flags & AUDIO_OUTPUT_FLAG_MMAP_NOIRQ) {
            sp<MmapPlaybackThread> thread =
                    new MmapPlaybackThread(this, *output, outHwDev, outputStream,
                                          devices, AUDIO_DEVICE_NONE, mSystemReady);
            mMmapThreads.add(*output, thread);
            ALOGV("openOutput_l() created mmap playback thread: ID %d thread %p",
                  *output, thread.get());
            return thread;
        } else {
            sp<PlaybackThread> thread;
            if (flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) {
                thread = new OffloadThread(this, outputStream, *output, devices, mSystemReady);
                ALOGV("openOutput_l() created offload output: ID %d thread %p",
                      *output, thread.get());
            } else if ((flags & AUDIO_OUTPUT_FLAG_DIRECT)
                    || !isValidPcmSinkFormat(config->format)
                    || !isValidPcmSinkChannelMask(config->channel_mask)) {
                thread = new DirectOutputThread(this, outputStream, *output, devices, mSystemReady);
                ALOGV("openOutput_l() created direct output: ID %d thread %p",
                      *output, thread.get());
            } else {
                thread = new MixerThread(this, outputStream, *output, devices, mSystemReady);
                ALOGV("openOutput_l() created mixer output: ID %d thread %p",
                      *output, thread.get());
            }
            mPlaybackThreads.add(*output, thread);
            return thread;
        }
    }

    return 0;
}

status_t AudioFlinger::openOutput(audio_module_handle_t module,
                                  audio_io_handle_t *output,
                                  audio_config_t *config,
                                  audio_devices_t *devices,
                                  const String8& address,
                                  uint32_t *latencyMs,
                                  audio_output_flags_t flags)
{
    ALOGI("openOutput() this %p, module %d Device %#x, SamplingRate %d, Format %#08x, "
              "Channels %#x, flags %#x",
              this, module,
              (devices != NULL) ? *devices : 0,
              config->sample_rate,
              config->format,
              config->channel_mask,
              flags);

    if (devices == NULL || *devices == AUDIO_DEVICE_NONE) {
        return BAD_VALUE;
    }

    Mutex::Autolock _l(mLock);

    sp<ThreadBase> thread = openOutput_l(module, output, config, *devices, address, flags);
    if (thread != 0) {
        if ((flags & AUDIO_OUTPUT_FLAG_MMAP_NOIRQ) == 0) {
            PlaybackThread *playbackThread = (PlaybackThread *)thread.get();
            *latencyMs = playbackThread->latency();

            // notify client processes of the new output creation
            playbackThread->ioConfigChanged(AUDIO_OUTPUT_OPENED);

            // the first primary output opened designates the primary hw device
            if ((mPrimaryHardwareDev == NULL) && (flags & AUDIO_OUTPUT_FLAG_PRIMARY)) {
                ALOGI("Using module %d as the primary audio interface", module);
                mPrimaryHardwareDev = playbackThread->getOutput()->audioHwDev;

                AutoMutex lock(mHardwareLock);
                mHardwareStatus = AUDIO_HW_SET_MODE;
                mPrimaryHardwareDev->hwDevice()->setMode(mMode);
                mHardwareStatus = AUDIO_HW_IDLE;
            }
        } else {
            MmapThread *mmapThread = (MmapThread *)thread.get();
            mmapThread->ioConfigChanged(AUDIO_OUTPUT_OPENED);
        }
        return NO_ERROR;
    }

    return NO_INIT;
}

audio_io_handle_t AudioFlinger::openDuplicateOutput(audio_io_handle_t output1,
        audio_io_handle_t output2)
{
    Mutex::Autolock _l(mLock);
    MixerThread *thread1 = checkMixerThread_l(output1);
    MixerThread *thread2 = checkMixerThread_l(output2);

    if (thread1 == NULL || thread2 == NULL) {
        ALOGW("openDuplicateOutput() wrong output mixer type for output %d or %d", output1,
                output2);
        return AUDIO_IO_HANDLE_NONE;
    }

    audio_io_handle_t id = nextUniqueId(AUDIO_UNIQUE_ID_USE_OUTPUT);
    DuplicatingThread *thread = new DuplicatingThread(this, thread1, id, mSystemReady);
    thread->addOutputTrack(thread2);
    mPlaybackThreads.add(id, thread);
    // notify client processes of the new output creation
    thread->ioConfigChanged(AUDIO_OUTPUT_OPENED);
    return id;
}

status_t AudioFlinger::closeOutput(audio_io_handle_t output)
{
    return closeOutput_nonvirtual(output);
}

status_t AudioFlinger::closeOutput_nonvirtual(audio_io_handle_t output)
{
    // keep strong reference on the playback thread so that
    // it is not destroyed while exit() is executed
    sp<PlaybackThread> playbackThread;
    sp<MmapPlaybackThread> mmapThread;
    {
        Mutex::Autolock _l(mLock);
        playbackThread = checkPlaybackThread_l(output);
        if (playbackThread != NULL) {
            ALOGV("closeOutput() %d", output);

            if (playbackThread->type() == ThreadBase::MIXER) {
                for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
                    if (mPlaybackThreads.valueAt(i)->isDuplicating()) {
                        DuplicatingThread *dupThread =
                                (DuplicatingThread *)mPlaybackThreads.valueAt(i).get();
                        dupThread->removeOutputTrack((MixerThread *)playbackThread.get());
                    }
                }
            }


            mPlaybackThreads.removeItem(output);
            // save all effects to the default thread
            if (mPlaybackThreads.size()) {
                PlaybackThread *dstThread = checkPlaybackThread_l(mPlaybackThreads.keyAt(0));
                if (dstThread != NULL) {
                    // audioflinger lock is held so order of thread lock acquisition doesn't matter
                    Mutex::Autolock _dl(dstThread->mLock);
                    Mutex::Autolock _sl(playbackThread->mLock);
                    Vector< sp<EffectChain> > effectChains = playbackThread->getEffectChains_l();
                    for (size_t i = 0; i < effectChains.size(); i ++) {
                        moveEffectChain_l(effectChains[i]->sessionId(), playbackThread.get(),
                                dstThread, true);
                    }
                }
            }
        } else {
            mmapThread = (MmapPlaybackThread *)checkMmapThread_l(output);
            if (mmapThread == 0) {
                return BAD_VALUE;
            }
            mMmapThreads.removeItem(output);
            ALOGD("closing mmapThread %p", mmapThread.get());
        }
        const sp<AudioIoDescriptor> ioDesc = new AudioIoDescriptor();
        ioDesc->mIoHandle = output;
        ioConfigChanged(AUDIO_OUTPUT_CLOSED, ioDesc);
    }
    // The thread entity (active unit of execution) is no longer running here,
    // but the ThreadBase container still exists.

    if (playbackThread != 0) {
        playbackThread->exit();
        if (!playbackThread->isDuplicating()) {
            closeOutputFinish(playbackThread);
        }
    } else if (mmapThread != 0) {
        ALOGD("mmapThread exit()");
        mmapThread->exit();
        AudioStreamOut *out = mmapThread->clearOutput();
        ALOG_ASSERT(out != NULL, "out shouldn't be NULL");
        // from now on thread->mOutput is NULL
        delete out;
    }
    return NO_ERROR;
}

void AudioFlinger::closeOutputFinish(const sp<PlaybackThread>& thread)
{
    AudioStreamOut *out = thread->clearOutput();
    ALOG_ASSERT(out != NULL, "out shouldn't be NULL");
    // from now on thread->mOutput is NULL
    delete out;
}

void AudioFlinger::closeOutputInternal_l(const sp<PlaybackThread>& thread)
{
    mPlaybackThreads.removeItem(thread->mId);
    thread->exit();
    closeOutputFinish(thread);
}

status_t AudioFlinger::suspendOutput(audio_io_handle_t output)
{
    Mutex::Autolock _l(mLock);
    PlaybackThread *thread = checkPlaybackThread_l(output);

    if (thread == NULL) {
        return BAD_VALUE;
    }

    ALOGV("suspendOutput() %d", output);
    thread->suspend();

    return NO_ERROR;
}

status_t AudioFlinger::restoreOutput(audio_io_handle_t output)
{
    Mutex::Autolock _l(mLock);
    PlaybackThread *thread = checkPlaybackThread_l(output);

    if (thread == NULL) {
        return BAD_VALUE;
    }

    ALOGV("restoreOutput() %d", output);

    thread->restore();

    return NO_ERROR;
}

status_t AudioFlinger::openInput(audio_module_handle_t module,
                                          audio_io_handle_t *input,
                                          audio_config_t *config,
                                          audio_devices_t *devices,
                                          const String8& address,
                                          audio_source_t source,
                                          audio_input_flags_t flags)
{
    Mutex::Autolock _l(mLock);

    if (*devices == AUDIO_DEVICE_NONE) {
        return BAD_VALUE;
    }

    sp<ThreadBase> thread = openInput_l(module, input, config, *devices, address, source, flags);

    if (thread != 0) {
        // notify client processes of the new input creation
        thread->ioConfigChanged(AUDIO_INPUT_OPENED);
        return NO_ERROR;
    }
    return NO_INIT;
}

sp<AudioFlinger::ThreadBase> AudioFlinger::openInput_l(audio_module_handle_t module,
                                                         audio_io_handle_t *input,
                                                         audio_config_t *config,
                                                         audio_devices_t devices,
                                                         const String8& address,
                                                         audio_source_t source,
                                                         audio_input_flags_t flags)
{
    AudioHwDevice *inHwDev = findSuitableHwDev_l(module, devices);
    if (inHwDev == NULL) {
        *input = AUDIO_IO_HANDLE_NONE;
        return 0;
    }

    // Audio Policy can request a specific handle for hardware hotword.
    // The goal here is not to re-open an already opened input.
    // It is to use a pre-assigned I/O handle.
    if (*input == AUDIO_IO_HANDLE_NONE) {
        *input = nextUniqueId(AUDIO_UNIQUE_ID_USE_INPUT);
    } else if (audio_unique_id_get_use(*input) != AUDIO_UNIQUE_ID_USE_INPUT) {
        ALOGE("openInput_l() requested input handle %d is invalid", *input);
        return 0;
    } else if (mRecordThreads.indexOfKey(*input) >= 0) {
        // This should not happen in a transient state with current design.
        ALOGE("openInput_l() requested input handle %d is already assigned", *input);
        return 0;
    }

    audio_config_t halconfig = *config;
    sp<DeviceHalInterface> inHwHal = inHwDev->hwDevice();
    sp<StreamInHalInterface> inStream;
    status_t status = inHwHal->openInputStream(
            *input, devices, &halconfig, flags, address.string(), source, &inStream);
    ALOGV("openInput_l() openInputStream returned input %p, devices %#x, SamplingRate %d"
           ", Format %#x, Channels %#x, flags %#x, status %d addr %s",
            inStream.get(),
            devices,
            halconfig.sample_rate,
            halconfig.format,
            halconfig.channel_mask,
            flags,
            status, address.string());

    // If the input could not be opened with the requested parameters and we can handle the
    // conversion internally, try to open again with the proposed parameters.
    if (status == BAD_VALUE &&
        audio_is_linear_pcm(config->format) &&
        audio_is_linear_pcm(halconfig.format) &&
        (halconfig.sample_rate <= AUDIO_RESAMPLER_DOWN_RATIO_MAX * config->sample_rate) &&
        (audio_channel_count_from_in_mask(halconfig.channel_mask) <= FCC_8) &&
        (audio_channel_count_from_in_mask(config->channel_mask) <= FCC_8)) {
        // FIXME describe the change proposed by HAL (save old values so we can log them here)
        ALOGV("openInput_l() reopening with proposed sampling rate and channel mask");
        inStream.clear();
        status = inHwHal->openInputStream(
                *input, devices, &halconfig, flags, address.string(), source, &inStream);
        // FIXME log this new status; HAL should not propose any further changes
    }

    if (status == NO_ERROR && inStream != 0) {
        AudioStreamIn *inputStream = new AudioStreamIn(inHwDev, inStream, flags);
        if ((flags & AUDIO_INPUT_FLAG_MMAP_NOIRQ) != 0) {
            sp<MmapCaptureThread> thread =
                    new MmapCaptureThread(this, *input,
                                          inHwDev, inputStream,
                                          primaryOutputDevice_l(), devices, mSystemReady);
            mMmapThreads.add(*input, thread);
            ALOGV("openInput_l() created mmap capture thread: ID %d thread %p", *input,
                    thread.get());
            return thread;
        } else {
#ifdef TEE_SINK
            // Try to re-use most recently used Pipe to archive a copy of input for dumpsys,
            // or (re-)create if current Pipe is idle and does not match the new format
            sp<NBAIO_Sink> teeSink;
            enum {
                TEE_SINK_NO,    // don't copy input
                TEE_SINK_NEW,   // copy input using a new pipe
                TEE_SINK_OLD,   // copy input using an existing pipe
            } kind;
            NBAIO_Format format = Format_from_SR_C(halconfig.sample_rate,
                    audio_channel_count_from_in_mask(halconfig.channel_mask), halconfig.format);
            if (!mTeeSinkInputEnabled) {
                kind = TEE_SINK_NO;
            } else if (!Format_isValid(format)) {
                kind = TEE_SINK_NO;
            } else if (mRecordTeeSink == 0) {
                kind = TEE_SINK_NEW;
            } else if (mRecordTeeSink->getStrongCount() != 1) {
                kind = TEE_SINK_NO;
            } else if (Format_isEqual(format, mRecordTeeSink->format())) {
                kind = TEE_SINK_OLD;
            } else {
                kind = TEE_SINK_NEW;
            }
            switch (kind) {
            case TEE_SINK_NEW: {
                Pipe *pipe = new Pipe(mTeeSinkInputFrames, format);
                size_t numCounterOffers = 0;
                const NBAIO_Format offers[1] = {format};
                ssize_t index = pipe->negotiate(offers, 1, NULL, numCounterOffers);
                ALOG_ASSERT(index == 0);
                PipeReader *pipeReader = new PipeReader(*pipe);
                numCounterOffers = 0;
                index = pipeReader->negotiate(offers, 1, NULL, numCounterOffers);
                ALOG_ASSERT(index == 0);
                mRecordTeeSink = pipe;
                mRecordTeeSource = pipeReader;
                teeSink = pipe;
                }
                break;
            case TEE_SINK_OLD:
                teeSink = mRecordTeeSink;
                break;
            case TEE_SINK_NO:
            default:
                break;
            }
#endif

            // Start record thread
            // RecordThread requires both input and output device indication to forward to audio
            // pre processing modules
            sp<RecordThread> thread = new RecordThread(this,
                                      inputStream,
                                      *input,
                                      primaryOutputDevice_l(),
                                      devices,
                                      mSystemReady
#ifdef TEE_SINK
                                      , teeSink
#endif
                                      );
            mRecordThreads.add(*input, thread);
            ALOGV("openInput_l() created record thread: ID %d thread %p", *input, thread.get());
            return thread;
        }
    }

    *input = AUDIO_IO_HANDLE_NONE;
    return 0;
}

status_t AudioFlinger::closeInput(audio_io_handle_t input)
{
    return closeInput_nonvirtual(input);
}

status_t AudioFlinger::closeInput_nonvirtual(audio_io_handle_t input)
{
    // keep strong reference on the record thread so that
    // it is not destroyed while exit() is executed
    sp<RecordThread> recordThread;
    sp<MmapCaptureThread> mmapThread;
    {
        Mutex::Autolock _l(mLock);
        recordThread = checkRecordThread_l(input);
        if (recordThread != 0) {
            ALOGV("closeInput() %d", input);

            // If we still have effect chains, it means that a client still holds a handle
            // on at least one effect. We must either move the chain to an existing thread with the
            // same session ID or put it aside in case a new record thread is opened for a
            // new capture on the same session
            sp<EffectChain> chain;
            {
                Mutex::Autolock _sl(recordThread->mLock);
                Vector< sp<EffectChain> > effectChains = recordThread->getEffectChains_l();
                // Note: maximum one chain per record thread
                if (effectChains.size() != 0) {
                    chain = effectChains[0];
                }
            }
            if (chain != 0) {
                // first check if a record thread is already opened with a client on same session.
                // This should only happen in case of overlap between one thread tear down and the
                // creation of its replacement
                size_t i;
                for (i = 0; i < mRecordThreads.size(); i++) {
                    sp<RecordThread> t = mRecordThreads.valueAt(i);
                    if (t == recordThread) {
                        continue;
                    }
                    if (t->hasAudioSession(chain->sessionId()) != 0) {
                        Mutex::Autolock _l(t->mLock);
                        ALOGV("closeInput() found thread %d for effect session %d",
                              t->id(), chain->sessionId());
                        t->addEffectChain_l(chain);
                        break;
                    }
                }
                // put the chain aside if we could not find a record thread with the same session id
                if (i == mRecordThreads.size()) {
                    putOrphanEffectChain_l(chain);
                }
            }
            mRecordThreads.removeItem(input);
        } else {
            mmapThread = (MmapCaptureThread *)checkMmapThread_l(input);
            if (mmapThread == 0) {
                return BAD_VALUE;
            }
            mMmapThreads.removeItem(input);
        }
        const sp<AudioIoDescriptor> ioDesc = new AudioIoDescriptor();
        ioDesc->mIoHandle = input;
        ioConfigChanged(AUDIO_INPUT_CLOSED, ioDesc);
    }
    // FIXME: calling thread->exit() without mLock held should not be needed anymore now that
    // we have a different lock for notification client
    if (recordThread != 0) {
        closeInputFinish(recordThread);
    } else if (mmapThread != 0) {
        mmapThread->exit();
        AudioStreamIn *in = mmapThread->clearInput();
        ALOG_ASSERT(in != NULL, "in shouldn't be NULL");
        // from now on thread->mInput is NULL
        delete in;
    }
    return NO_ERROR;
}

void AudioFlinger::closeInputFinish(const sp<RecordThread>& thread)
{
    thread->exit();
    AudioStreamIn *in = thread->clearInput();
    ALOG_ASSERT(in != NULL, "in shouldn't be NULL");
    // from now on thread->mInput is NULL
    delete in;
}

void AudioFlinger::closeInputInternal_l(const sp<RecordThread>& thread)
{
    mRecordThreads.removeItem(thread->mId);
    closeInputFinish(thread);
}

status_t AudioFlinger::invalidateStream(audio_stream_type_t stream)
{
    Mutex::Autolock _l(mLock);
    ALOGV("invalidateStream() stream %d", stream);

    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        PlaybackThread *thread = mPlaybackThreads.valueAt(i).get();
        thread->invalidateTracks(stream);
    }
    for (size_t i = 0; i < mMmapThreads.size(); i++) {
        mMmapThreads[i]->invalidateTracks(stream);
    }
    return NO_ERROR;
}


audio_unique_id_t AudioFlinger::newAudioUniqueId(audio_unique_id_use_t use)
{
    // This is a binder API, so a malicious client could pass in a bad parameter.
    // Check for that before calling the internal API nextUniqueId().
    if ((unsigned) use >= (unsigned) AUDIO_UNIQUE_ID_USE_MAX) {
        ALOGE("newAudioUniqueId invalid use %d", use);
        return AUDIO_UNIQUE_ID_ALLOCATE;
    }
    return nextUniqueId(use);
}

void AudioFlinger::acquireAudioSessionId(audio_session_t audioSession, pid_t pid)
{
    Mutex::Autolock _l(mLock);
    pid_t caller = IPCThreadState::self()->getCallingPid();
    ALOGV("acquiring %d from %d, for %d", audioSession, caller, pid);
    if (pid != -1 && (caller == getpid_cached)) {
        caller = pid;
    }

    {
        Mutex::Autolock _cl(mClientLock);
        // Ignore requests received from processes not known as notification client. The request
        // is likely proxied by mediaserver (e.g CameraService) and releaseAudioSessionId() can be
        // called from a different pid leaving a stale session reference.  Also we don't know how
        // to clear this reference if the client process dies.
        if (mNotificationClients.indexOfKey(caller) < 0) {
            ALOGW("acquireAudioSessionId() unknown client %d for session %d", caller, audioSession);
            return;
        }
    }

    size_t num = mAudioSessionRefs.size();
    for (size_t i = 0; i < num; i++) {
        AudioSessionRef *ref = mAudioSessionRefs.editItemAt(i);
        if (ref->mSessionid == audioSession && ref->mPid == caller) {
            ref->mCnt++;
            ALOGV(" incremented refcount to %d", ref->mCnt);
            return;
        }
    }
    mAudioSessionRefs.push(new AudioSessionRef(audioSession, caller));
    ALOGV(" added new entry for %d", audioSession);
}

void AudioFlinger::releaseAudioSessionId(audio_session_t audioSession, pid_t pid)
{
    Mutex::Autolock _l(mLock);
    pid_t caller = IPCThreadState::self()->getCallingPid();
    ALOGV("releasing %d from %d for %d", audioSession, caller, pid);
    if (pid != -1 && (caller == getpid_cached)) {
        caller = pid;
    }
    size_t num = mAudioSessionRefs.size();
    for (size_t i = 0; i < num; i++) {
        AudioSessionRef *ref = mAudioSessionRefs.itemAt(i);
        if (ref->mSessionid == audioSession && ref->mPid == caller) {
            ref->mCnt--;
            ALOGV(" decremented refcount to %d", ref->mCnt);
            if (ref->mCnt == 0) {
                mAudioSessionRefs.removeAt(i);
                delete ref;
                purgeStaleEffects_l();
            }
            return;
        }
    }
    // If the caller is mediaserver it is likely that the session being released was acquired
    // on behalf of a process not in notification clients and we ignore the warning.
    ALOGW_IF(caller != getpid_cached, "session id %d not found for pid %d", audioSession, caller);
}

bool AudioFlinger::isSessionAcquired_l(audio_session_t audioSession)
{
    size_t num = mAudioSessionRefs.size();
    for (size_t i = 0; i < num; i++) {
        AudioSessionRef *ref = mAudioSessionRefs.itemAt(i);
        if (ref->mSessionid == audioSession) {
            return true;
        }
    }
    return false;
}

void AudioFlinger::purgeStaleEffects_l() {

    ALOGV("purging stale effects");

    Vector< sp<EffectChain> > chains;

    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        sp<PlaybackThread> t = mPlaybackThreads.valueAt(i);
        Mutex::Autolock _l(t->mLock);
        for (size_t j = 0; j < t->mEffectChains.size(); j++) {
            sp<EffectChain> ec = t->mEffectChains[j];
            if (ec->sessionId() > AUDIO_SESSION_OUTPUT_MIX) {
                chains.push(ec);
            }
        }
    }
    for (size_t i = 0; i < mRecordThreads.size(); i++) {
        sp<RecordThread> t = mRecordThreads.valueAt(i);
        Mutex::Autolock _l(t->mLock);
        for (size_t j = 0; j < t->mEffectChains.size(); j++) {
            sp<EffectChain> ec = t->mEffectChains[j];
            chains.push(ec);
        }
    }

    for (size_t i = 0; i < chains.size(); i++) {
        sp<EffectChain> ec = chains[i];
        int sessionid = ec->sessionId();
        sp<ThreadBase> t = ec->mThread.promote();
        if (t == 0) {
            continue;
        }
        size_t numsessionrefs = mAudioSessionRefs.size();
        bool found = false;
        for (size_t k = 0; k < numsessionrefs; k++) {
            AudioSessionRef *ref = mAudioSessionRefs.itemAt(k);
            if (ref->mSessionid == sessionid) {
                ALOGV(" session %d still exists for %d with %d refs",
                    sessionid, ref->mPid, ref->mCnt);
                found = true;
                break;
            }
        }
        if (!found) {
            Mutex::Autolock _l(t->mLock);
            // remove all effects from the chain
            while (ec->mEffects.size()) {
                sp<EffectModule> effect = ec->mEffects[0];
                effect->unPin();
                t->removeEffect_l(effect, /*release*/ true);
                if (effect->purgeHandles()) {
                    t->checkSuspendOnEffectEnabled_l(effect, false, effect->sessionId());
                }
                AudioSystem::unregisterEffect(effect->id());
            }
        }
    }
    return;
}

// checkThread_l() must be called with AudioFlinger::mLock held
AudioFlinger::ThreadBase *AudioFlinger::checkThread_l(audio_io_handle_t ioHandle) const
{
    ThreadBase *thread = checkMmapThread_l(ioHandle);
    if (thread == 0) {
        switch (audio_unique_id_get_use(ioHandle)) {
        case AUDIO_UNIQUE_ID_USE_OUTPUT:
            thread = checkPlaybackThread_l(ioHandle);
            break;
        case AUDIO_UNIQUE_ID_USE_INPUT:
            thread = checkRecordThread_l(ioHandle);
            break;
        default:
            break;
        }
    }
    return thread;
}

// checkPlaybackThread_l() must be called with AudioFlinger::mLock held
AudioFlinger::PlaybackThread *AudioFlinger::checkPlaybackThread_l(audio_io_handle_t output) const
{
    return mPlaybackThreads.valueFor(output).get();
}

// checkMixerThread_l() must be called with AudioFlinger::mLock held
AudioFlinger::MixerThread *AudioFlinger::checkMixerThread_l(audio_io_handle_t output) const
{
    PlaybackThread *thread = checkPlaybackThread_l(output);
    return thread != NULL && thread->type() != ThreadBase::DIRECT ? (MixerThread *) thread : NULL;
}

// checkRecordThread_l() must be called with AudioFlinger::mLock held
AudioFlinger::RecordThread *AudioFlinger::checkRecordThread_l(audio_io_handle_t input) const
{
    return mRecordThreads.valueFor(input).get();
}

// checkMmapThread_l() must be called with AudioFlinger::mLock held
AudioFlinger::MmapThread *AudioFlinger::checkMmapThread_l(audio_io_handle_t io) const
{
    return mMmapThreads.valueFor(io).get();
}


// checkPlaybackThread_l() must be called with AudioFlinger::mLock held
AudioFlinger::VolumeInterface *AudioFlinger::getVolumeInterface_l(audio_io_handle_t output) const
{
    VolumeInterface *volumeInterface = mPlaybackThreads.valueFor(output).get();
    if (volumeInterface == nullptr) {
        MmapThread *mmapThread = mMmapThreads.valueFor(output).get();
        if (mmapThread != nullptr) {
            if (mmapThread->isOutput()) {
                MmapPlaybackThread *mmapPlaybackThread =
                        static_cast<MmapPlaybackThread *>(mmapThread);
                volumeInterface = mmapPlaybackThread;
            }
        }
    }
    return volumeInterface;
}

Vector <AudioFlinger::VolumeInterface *> AudioFlinger::getAllVolumeInterfaces_l() const
{
    Vector <VolumeInterface *> volumeInterfaces;
    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        volumeInterfaces.add(mPlaybackThreads.valueAt(i).get());
    }
    for (size_t i = 0; i < mMmapThreads.size(); i++) {
        if (mMmapThreads.valueAt(i)->isOutput()) {
            MmapPlaybackThread *mmapPlaybackThread =
                    static_cast<MmapPlaybackThread *>(mMmapThreads.valueAt(i).get());
            volumeInterfaces.add(mmapPlaybackThread);
        }
    }
    return volumeInterfaces;
}

audio_unique_id_t AudioFlinger::nextUniqueId(audio_unique_id_use_t use)
{
    // This is the internal API, so it is OK to assert on bad parameter.
    LOG_ALWAYS_FATAL_IF((unsigned) use >= (unsigned) AUDIO_UNIQUE_ID_USE_MAX);
    const int maxRetries = use == AUDIO_UNIQUE_ID_USE_SESSION ? 3 : 1;
    for (int retry = 0; retry < maxRetries; retry++) {
        // The cast allows wraparound from max positive to min negative instead of abort
        uint32_t base = (uint32_t) atomic_fetch_add_explicit(&mNextUniqueIds[use],
                (uint_fast32_t) AUDIO_UNIQUE_ID_USE_MAX, memory_order_acq_rel);
        ALOG_ASSERT(audio_unique_id_get_use(base) == AUDIO_UNIQUE_ID_USE_UNSPECIFIED);
        // allow wrap by skipping 0 and -1 for session ids
        if (!(base == 0 || base == (~0u & ~AUDIO_UNIQUE_ID_USE_MASK))) {
            ALOGW_IF(retry != 0, "unique ID overflow for use %d", use);
            return (audio_unique_id_t) (base | use);
        }
    }
    // We have no way of recovering from wraparound
    LOG_ALWAYS_FATAL("unique ID overflow for use %d", use);
    // TODO Use a floor after wraparound.  This may need a mutex.
}

AudioFlinger::PlaybackThread *AudioFlinger::primaryPlaybackThread_l() const
{
    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        PlaybackThread *thread = mPlaybackThreads.valueAt(i).get();
        if(thread->isDuplicating()) {
            continue;
        }
        AudioStreamOut *output = thread->getOutput();
        if (output != NULL && output->audioHwDev == mPrimaryHardwareDev) {
            return thread;
        }
    }
    return NULL;
}

audio_devices_t AudioFlinger::primaryOutputDevice_l() const
{
    PlaybackThread *thread = primaryPlaybackThread_l();

    if (thread == NULL) {
        return 0;
    }

    return thread->outDevice();
}

AudioFlinger::PlaybackThread *AudioFlinger::fastPlaybackThread_l() const
{
    size_t minFrameCount = 0;
    PlaybackThread *minThread = NULL;
    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        PlaybackThread *thread = mPlaybackThreads.valueAt(i).get();
        if (!thread->isDuplicating()) {
            size_t frameCount = thread->frameCountHAL();
            if (frameCount != 0 && (minFrameCount == 0 || frameCount < minFrameCount ||
                    (frameCount == minFrameCount && thread->hasFastMixer() &&
                    /*minThread != NULL &&*/ !minThread->hasFastMixer()))) {
                minFrameCount = frameCount;
                minThread = thread;
            }
        }
    }
    return minThread;
}

sp<AudioFlinger::SyncEvent> AudioFlinger::createSyncEvent(AudioSystem::sync_event_t type,
                                    audio_session_t triggerSession,
                                    audio_session_t listenerSession,
                                    sync_event_callback_t callBack,
                                    const wp<RefBase>& cookie)
{
    Mutex::Autolock _l(mLock);

    sp<SyncEvent> event = new SyncEvent(type, triggerSession, listenerSession, callBack, cookie);
    status_t playStatus = NAME_NOT_FOUND;
    status_t recStatus = NAME_NOT_FOUND;
    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        playStatus = mPlaybackThreads.valueAt(i)->setSyncEvent(event);
        if (playStatus == NO_ERROR) {
            return event;
        }
    }
    for (size_t i = 0; i < mRecordThreads.size(); i++) {
        recStatus = mRecordThreads.valueAt(i)->setSyncEvent(event);
        if (recStatus == NO_ERROR) {
            return event;
        }
    }
    if (playStatus == NAME_NOT_FOUND || recStatus == NAME_NOT_FOUND) {
        mPendingSyncEvents.add(event);
    } else {
        ALOGV("createSyncEvent() invalid event %d", event->type());
        event.clear();
    }
    return event;
}

// ----------------------------------------------------------------------------
//  Effect management
// ----------------------------------------------------------------------------

sp<EffectsFactoryHalInterface> AudioFlinger::getEffectsFactory() {
    return mEffectsFactoryHal;
}

status_t AudioFlinger::queryNumberEffects(uint32_t *numEffects) const
{
    Mutex::Autolock _l(mLock);
    if (mEffectsFactoryHal.get()) {
        return mEffectsFactoryHal->queryNumberEffects(numEffects);
    } else {
        return -ENODEV;
    }
}

status_t AudioFlinger::queryEffect(uint32_t index, effect_descriptor_t *descriptor) const
{
    Mutex::Autolock _l(mLock);
    if (mEffectsFactoryHal.get()) {
        return mEffectsFactoryHal->getDescriptor(index, descriptor);
    } else {
        return -ENODEV;
    }
}

status_t AudioFlinger::getEffectDescriptor(const effect_uuid_t *pUuid,
        effect_descriptor_t *descriptor) const
{
    Mutex::Autolock _l(mLock);
    if (mEffectsFactoryHal.get()) {
        return mEffectsFactoryHal->getDescriptor(pUuid, descriptor);
    } else {
        return -ENODEV;
    }
}


sp<IEffect> AudioFlinger::createEffect(
        effect_descriptor_t *pDesc,
        const sp<IEffectClient>& effectClient,
        int32_t priority,
        audio_io_handle_t io,
        audio_session_t sessionId,
        const String16& opPackageName,
        pid_t pid,
        status_t *status,
        int *id,
        int *enabled)
{
    status_t lStatus = NO_ERROR;
    sp<EffectHandle> handle;
    effect_descriptor_t desc;

    const uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (pid == -1 || !isTrustedCallingUid(callingUid)) {
        const pid_t callingPid = IPCThreadState::self()->getCallingPid();
        ALOGW_IF(pid != -1 && pid != callingPid,
                 "%s uid %d pid %d tried to pass itself off as pid %d",
                 __func__, callingUid, callingPid, pid);
        pid = callingPid;
    }

    ALOGV("createEffect pid %d, effectClient %p, priority %d, sessionId %d, io %d, factory %p",
            pid, effectClient.get(), priority, sessionId, io, mEffectsFactoryHal.get());

    if (pDesc == NULL) {
        lStatus = BAD_VALUE;
        goto Exit;
    }

    // check audio settings permission for global effects
    if (sessionId == AUDIO_SESSION_OUTPUT_MIX && !settingsAllowed()) {
        lStatus = PERMISSION_DENIED;
        goto Exit;
    }

    // Session AUDIO_SESSION_OUTPUT_STAGE is reserved for output stage effects
    // that can only be created by audio policy manager (running in same process)
    if (sessionId == AUDIO_SESSION_OUTPUT_STAGE && getpid_cached != pid) {
        lStatus = PERMISSION_DENIED;
        goto Exit;
    }

    if (mEffectsFactoryHal == 0) {
        lStatus = NO_INIT;
        goto Exit;
    }

    {
        if (!EffectsFactoryHalInterface::isNullUuid(&pDesc->uuid)) {
            // if uuid is specified, request effect descriptor
            lStatus = mEffectsFactoryHal->getDescriptor(&pDesc->uuid, &desc);
            if (lStatus < 0) {
                ALOGW("createEffect() error %d from EffectGetDescriptor", lStatus);
                goto Exit;
            }
        } else {
            // if uuid is not specified, look for an available implementation
            // of the required type in effect factory
            if (EffectsFactoryHalInterface::isNullUuid(&pDesc->type)) {
                ALOGW("createEffect() no effect type");
                lStatus = BAD_VALUE;
                goto Exit;
            }
            uint32_t numEffects = 0;
            effect_descriptor_t d;
            d.flags = 0; // prevent compiler warning
            bool found = false;

            lStatus = mEffectsFactoryHal->queryNumberEffects(&numEffects);
            if (lStatus < 0) {
                ALOGW("createEffect() error %d from EffectQueryNumberEffects", lStatus);
                goto Exit;
            }
            for (uint32_t i = 0; i < numEffects; i++) {
                lStatus = mEffectsFactoryHal->getDescriptor(i, &desc);
                if (lStatus < 0) {
                    ALOGW("createEffect() error %d from EffectQueryEffect", lStatus);
                    continue;
                }
                if (memcmp(&desc.type, &pDesc->type, sizeof(effect_uuid_t)) == 0) {
                    // If matching type found save effect descriptor. If the session is
                    // 0 and the effect is not auxiliary, continue enumeration in case
                    // an auxiliary version of this effect type is available
                    found = true;
                    d = desc;
                    if (sessionId != AUDIO_SESSION_OUTPUT_MIX ||
                            (desc.flags & EFFECT_FLAG_TYPE_MASK) == EFFECT_FLAG_TYPE_AUXILIARY) {
                        break;
                    }
                }
            }
            if (!found) {
                lStatus = BAD_VALUE;
                ALOGW("createEffect() effect not found");
                goto Exit;
            }
            // For same effect type, chose auxiliary version over insert version if
            // connect to output mix (Compliance to OpenSL ES)
            if (sessionId == AUDIO_SESSION_OUTPUT_MIX &&
                    (d.flags & EFFECT_FLAG_TYPE_MASK) != EFFECT_FLAG_TYPE_AUXILIARY) {
                desc = d;
            }
        }

        // Do not allow auxiliary effects on a session different from 0 (output mix)
        if (sessionId != AUDIO_SESSION_OUTPUT_MIX &&
             (desc.flags & EFFECT_FLAG_TYPE_MASK) == EFFECT_FLAG_TYPE_AUXILIARY) {
            lStatus = INVALID_OPERATION;
            goto Exit;
        }

        // check recording permission for visualizer
        if ((memcmp(&desc.type, SL_IID_VISUALIZATION, sizeof(effect_uuid_t)) == 0) &&
            // TODO: Do we need to start/stop op - i.e. is there recording being performed?
            !recordingAllowed(opPackageName, pid, IPCThreadState::self()->getCallingUid())) {
            lStatus = PERMISSION_DENIED;
            goto Exit;
        }

        // return effect descriptor
        *pDesc = desc;
        if (io == AUDIO_IO_HANDLE_NONE && sessionId == AUDIO_SESSION_OUTPUT_MIX) {
            // if the output returned by getOutputForEffect() is removed before we lock the
            // mutex below, the call to checkPlaybackThread_l(io) below will detect it
            // and we will exit safely
            io = AudioSystem::getOutputForEffect(&desc);
            ALOGV("createEffect got output %d", io);
        }

        Mutex::Autolock _l(mLock);

        // If output is not specified try to find a matching audio session ID in one of the
        // output threads.
        // If output is 0 here, sessionId is neither SESSION_OUTPUT_STAGE nor SESSION_OUTPUT_MIX
        // because of code checking output when entering the function.
        // Note: io is never 0 when creating an effect on an input
        if (io == AUDIO_IO_HANDLE_NONE) {
            if (sessionId == AUDIO_SESSION_OUTPUT_STAGE) {
                // output must be specified by AudioPolicyManager when using session
                // AUDIO_SESSION_OUTPUT_STAGE
                lStatus = BAD_VALUE;
                goto Exit;
            }
            // look for the thread where the specified audio session is present
            for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
                if (mPlaybackThreads.valueAt(i)->hasAudioSession(sessionId) != 0) {
                    io = mPlaybackThreads.keyAt(i);
                    break;
                }
            }
            if (io == AUDIO_IO_HANDLE_NONE) {
                for (size_t i = 0; i < mRecordThreads.size(); i++) {
                    if (mRecordThreads.valueAt(i)->hasAudioSession(sessionId) != 0) {
                        io = mRecordThreads.keyAt(i);
                        break;
                    }
                }
            }
            if (io == AUDIO_IO_HANDLE_NONE) {
                for (size_t i = 0; i < mMmapThreads.size(); i++) {
                    if (mMmapThreads.valueAt(i)->hasAudioSession(sessionId) != 0) {
                        io = mMmapThreads.keyAt(i);
                        break;
                    }
                }
            }
            // If no output thread contains the requested session ID, default to
            // first output. The effect chain will be moved to the correct output
            // thread when a track with the same session ID is created
            if (io == AUDIO_IO_HANDLE_NONE && mPlaybackThreads.size() > 0) {
                io = mPlaybackThreads.keyAt(0);
            }
            ALOGV("createEffect() got io %d for effect %s", io, desc.name);
        }
        ThreadBase *thread = checkRecordThread_l(io);
        if (thread == NULL) {
            thread = checkPlaybackThread_l(io);
            if (thread == NULL) {
                thread = checkMmapThread_l(io);
                if (thread == NULL) {
                    ALOGE("createEffect() unknown output thread");
                    lStatus = BAD_VALUE;
                    goto Exit;
                }
            }
        } else {
            // Check if one effect chain was awaiting for an effect to be created on this
            // session and used it instead of creating a new one.
            sp<EffectChain> chain = getOrphanEffectChain_l(sessionId);
            if (chain != 0) {
                Mutex::Autolock _l(thread->mLock);
                thread->addEffectChain_l(chain);
            }
        }

        sp<Client> client = registerPid(pid);

        // create effect on selected output thread
        bool pinned = (sessionId > AUDIO_SESSION_OUTPUT_MIX) && isSessionAcquired_l(sessionId);
        handle = thread->createEffect_l(client, effectClient, priority, sessionId,
                &desc, enabled, &lStatus, pinned);
        if (lStatus != NO_ERROR && lStatus != ALREADY_EXISTS) {
            // remove local strong reference to Client with mClientLock held
            Mutex::Autolock _cl(mClientLock);
            client.clear();
        } else {
            // handle must be valid here, but check again to be safe.
            if (handle.get() != nullptr && id != nullptr) *id = handle->id();
        }
    }

    if (lStatus != NO_ERROR && lStatus != ALREADY_EXISTS) {
        // handle must be cleared outside lock.
        handle.clear();
    }

Exit:
    *status = lStatus;
    return handle;
}

status_t AudioFlinger::moveEffects(audio_session_t sessionId, audio_io_handle_t srcOutput,
        audio_io_handle_t dstOutput)
{
    ALOGV("moveEffects() session %d, srcOutput %d, dstOutput %d",
            sessionId, srcOutput, dstOutput);
    Mutex::Autolock _l(mLock);
    if (srcOutput == dstOutput) {
        ALOGW("moveEffects() same dst and src outputs %d", dstOutput);
        return NO_ERROR;
    }
    PlaybackThread *srcThread = checkPlaybackThread_l(srcOutput);
    if (srcThread == NULL) {
        ALOGW("moveEffects() bad srcOutput %d", srcOutput);
        return BAD_VALUE;
    }
    PlaybackThread *dstThread = checkPlaybackThread_l(dstOutput);
    if (dstThread == NULL) {
        ALOGW("moveEffects() bad dstOutput %d", dstOutput);
        return BAD_VALUE;
    }

    Mutex::Autolock _dl(dstThread->mLock);
    Mutex::Autolock _sl(srcThread->mLock);
    return moveEffectChain_l(sessionId, srcThread, dstThread, false);
}

// moveEffectChain_l must be called with both srcThread and dstThread mLocks held
status_t AudioFlinger::moveEffectChain_l(audio_session_t sessionId,
                                   AudioFlinger::PlaybackThread *srcThread,
                                   AudioFlinger::PlaybackThread *dstThread,
                                   bool reRegister)
{
    ALOGV("moveEffectChain_l() session %d from thread %p to thread %p",
            sessionId, srcThread, dstThread);

    sp<EffectChain> chain = srcThread->getEffectChain_l(sessionId);
    if (chain == 0) {
        ALOGW("moveEffectChain_l() effect chain for session %d not on source thread %p",
                sessionId, srcThread);
        return INVALID_OPERATION;
    }

    // Check whether the destination thread and all effects in the chain are compatible
    if (!chain->isCompatibleWithThread_l(dstThread)) {
        ALOGW("moveEffectChain_l() effect chain failed because"
                " destination thread %p is not compatible with effects in the chain",
                dstThread);
        return INVALID_OPERATION;
    }

    // remove chain first. This is useful only if reconfiguring effect chain on same output thread,
    // so that a new chain is created with correct parameters when first effect is added. This is
    // otherwise unnecessary as removeEffect_l() will remove the chain when last effect is
    // removed.
    srcThread->removeEffectChain_l(chain);

    // transfer all effects one by one so that new effect chain is created on new thread with
    // correct buffer sizes and audio parameters and effect engines reconfigured accordingly
    sp<EffectChain> dstChain;
    uint32_t strategy = 0; // prevent compiler warning
    sp<EffectModule> effect = chain->getEffectFromId_l(0);
    Vector< sp<EffectModule> > removed;
    status_t status = NO_ERROR;
    while (effect != 0) {
        srcThread->removeEffect_l(effect);
        removed.add(effect);
        status = dstThread->addEffect_l(effect);
        if (status != NO_ERROR) {
            break;
        }
        // removeEffect_l() has stopped the effect if it was active so it must be restarted
        if (effect->state() == EffectModule::ACTIVE ||
                effect->state() == EffectModule::STOPPING) {
            effect->start();
        }
        // if the move request is not received from audio policy manager, the effect must be
        // re-registered with the new strategy and output
        if (dstChain == 0) {
            dstChain = effect->chain().promote();
            if (dstChain == 0) {
                ALOGW("moveEffectChain_l() cannot get chain from effect %p", effect.get());
                status = NO_INIT;
                break;
            }
            strategy = dstChain->strategy();
        }
        if (reRegister) {
            AudioSystem::unregisterEffect(effect->id());
            AudioSystem::registerEffect(&effect->desc(),
                                        dstThread->id(),
                                        strategy,
                                        sessionId,
                                        effect->id());
            AudioSystem::setEffectEnabled(effect->id(), effect->isEnabled());
        }
        effect = chain->getEffectFromId_l(0);
    }

    if (status != NO_ERROR) {
        for (size_t i = 0; i < removed.size(); i++) {
            srcThread->addEffect_l(removed[i]);
            if (dstChain != 0 && reRegister) {
                AudioSystem::unregisterEffect(removed[i]->id());
                AudioSystem::registerEffect(&removed[i]->desc(),
                                            srcThread->id(),
                                            strategy,
                                            sessionId,
                                            removed[i]->id());
                AudioSystem::setEffectEnabled(effect->id(), effect->isEnabled());
            }
        }
    }

    return status;
}

bool AudioFlinger::isNonOffloadableGlobalEffectEnabled_l()
{
    if (mGlobalEffectEnableTime != 0 &&
            ((systemTime() - mGlobalEffectEnableTime) < kMinGlobalEffectEnabletimeNs)) {
        return true;
    }

    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        sp<EffectChain> ec =
                mPlaybackThreads.valueAt(i)->getEffectChain_l(AUDIO_SESSION_OUTPUT_MIX);
        if (ec != 0 && ec->isNonOffloadableEnabled()) {
            return true;
        }
    }
    return false;
}

void AudioFlinger::onNonOffloadableGlobalEffectEnable()
{
    Mutex::Autolock _l(mLock);

    mGlobalEffectEnableTime = systemTime();

    for (size_t i = 0; i < mPlaybackThreads.size(); i++) {
        sp<PlaybackThread> t = mPlaybackThreads.valueAt(i);
        if (t->mType == ThreadBase::OFFLOAD) {
            t->invalidateTracks(AUDIO_STREAM_MUSIC);
        }
    }

}

status_t AudioFlinger::putOrphanEffectChain_l(const sp<AudioFlinger::EffectChain>& chain)
{
    // clear possible suspended state before parking the chain so that it starts in default state
    // when attached to a new record thread
    chain->setEffectSuspended_l(FX_IID_AEC, false);
    chain->setEffectSuspended_l(FX_IID_NS, false);

    audio_session_t session = chain->sessionId();
    ssize_t index = mOrphanEffectChains.indexOfKey(session);
    ALOGV("putOrphanEffectChain_l session %d index %zd", session, index);
    if (index >= 0) {
        ALOGW("putOrphanEffectChain_l chain for session %d already present", session);
        return ALREADY_EXISTS;
    }
    mOrphanEffectChains.add(session, chain);
    return NO_ERROR;
}

sp<AudioFlinger::EffectChain> AudioFlinger::getOrphanEffectChain_l(audio_session_t session)
{
    sp<EffectChain> chain;
    ssize_t index = mOrphanEffectChains.indexOfKey(session);
    ALOGV("getOrphanEffectChain_l session %d index %zd", session, index);
    if (index >= 0) {
        chain = mOrphanEffectChains.valueAt(index);
        mOrphanEffectChains.removeItemsAt(index);
    }
    return chain;
}

bool AudioFlinger::updateOrphanEffectChains(const sp<AudioFlinger::EffectModule>& effect)
{
    Mutex::Autolock _l(mLock);
    audio_session_t session = effect->sessionId();
    ssize_t index = mOrphanEffectChains.indexOfKey(session);
    ALOGV("updateOrphanEffectChains session %d index %zd", session, index);
    if (index >= 0) {
        sp<EffectChain> chain = mOrphanEffectChains.valueAt(index);
        if (chain->removeEffect_l(effect, true) == 0) {
            ALOGV("updateOrphanEffectChains removing effect chain at index %zd", index);
            mOrphanEffectChains.removeItemsAt(index);
        }
        return true;
    }
    return false;
}


struct Entry {
#define TEE_MAX_FILENAME 32 // %Y%m%d%H%M%S_%d.wav = 4+2+2+2+2+2+1+1+4+1 = 21
    char mFileName[TEE_MAX_FILENAME];
};

int comparEntry(const void *p1, const void *p2)
{
    return strcmp(((const Entry *) p1)->mFileName, ((const Entry *) p2)->mFileName);
}

#ifdef TEE_SINK
void AudioFlinger::dumpTee(int fd, const sp<NBAIO_Source>& source, audio_io_handle_t id, char suffix)
{
    NBAIO_Source *teeSource = source.get();
    if (teeSource != NULL) {
        // .wav rotation
        // There is a benign race condition if 2 threads call this simultaneously.
        // They would both traverse the directory, but the result would simply be
        // failures at unlink() which are ignored.  It's also unlikely since
        // normally dumpsys is only done by bugreport or from the command line.
        char teePath[PATH_MAX] = "/data/misc/audioserver";
        size_t teePathLen = strlen(teePath);
        DIR *dir = opendir(teePath);
        teePath[teePathLen++] = '/';
        if (dir != NULL) {
#define TEE_MAX_SORT 20 // number of entries to sort
#define TEE_MAX_KEEP 10 // number of entries to keep
            struct Entry entries[TEE_MAX_SORT];
            size_t entryCount = 0;
            while (entryCount < TEE_MAX_SORT) {
                errno = 0; // clear errno before readdir() to track potential errors.
                const struct dirent *result = readdir(dir);
                if (result == nullptr) {
                    ALOGW_IF(errno != 0, "tee readdir() failure %s", strerror(errno));
                    break;
                }
                // ignore non .wav file entries
                const size_t nameLen = strlen(result->d_name);
                if (nameLen <= 4 || nameLen >= TEE_MAX_FILENAME ||
                        strcmp(&result->d_name[nameLen - 4], ".wav")) {
                    continue;
                }
                (void)audio_utils_strlcpy(entries[entryCount++].mFileName, result->d_name);
            }
            (void) closedir(dir);
            if (entryCount > TEE_MAX_KEEP) {
                qsort(entries, entryCount, sizeof(Entry), comparEntry);
                for (size_t i = 0; i < entryCount - TEE_MAX_KEEP; ++i) {
                    strcpy(&teePath[teePathLen], entries[i].mFileName);
                    (void) unlink(teePath);
                }
            }
        } else {
            if (fd >= 0) {
                dprintf(fd, "unable to rotate tees in %.*s: %s\n", (int) teePathLen, teePath,
                        strerror(errno));
            }
        }
        char teeTime[16];
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm tm;
        localtime_r(&tv.tv_sec, &tm);
        strftime(teeTime, sizeof(teeTime), "%Y%m%d%H%M%S", &tm);
        snprintf(&teePath[teePathLen], sizeof(teePath) - teePathLen, "%s_%d_%c.wav", teeTime, id,
                suffix);
        // if 2 dumpsys are done within 1 second, and rotation didn't work, then discard 2nd
        int teeFd = open(teePath, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, S_IRUSR | S_IWUSR);
        if (teeFd >= 0) {
            // FIXME use libsndfile
            char wavHeader[44];
            memcpy(wavHeader,
                "RIFF\0\0\0\0WAVEfmt \20\0\0\0\1\0\2\0\104\254\0\0\0\0\0\0\4\0\20\0data\0\0\0\0",
                sizeof(wavHeader));
            NBAIO_Format format = teeSource->format();
            unsigned channelCount = Format_channelCount(format);
            uint32_t sampleRate = Format_sampleRate(format);
            size_t frameSize = Format_frameSize(format);
            wavHeader[22] = channelCount;       // number of channels
            wavHeader[24] = sampleRate;         // sample rate
            wavHeader[25] = sampleRate >> 8;
            wavHeader[32] = frameSize;          // block alignment
            wavHeader[33] = frameSize >> 8;
            write(teeFd, wavHeader, sizeof(wavHeader));
            size_t total = 0;
            bool firstRead = true;
#define TEE_SINK_READ 1024                      // frames per I/O operation
            void *buffer = malloc(TEE_SINK_READ * frameSize);
            for (;;) {
                size_t count = TEE_SINK_READ;
                ssize_t actual = teeSource->read(buffer, count);
                bool wasFirstRead = firstRead;
                firstRead = false;
                if (actual <= 0) {
                    if (actual == (ssize_t) OVERRUN && wasFirstRead) {
                        continue;
                    }
                    break;
                }
                ALOG_ASSERT(actual <= (ssize_t)count);
                write(teeFd, buffer, actual * frameSize);
                total += actual;
            }
            free(buffer);
            lseek(teeFd, (off_t) 4, SEEK_SET);
            uint32_t temp = 44 + total * frameSize - 8;
            // FIXME not big-endian safe
            write(teeFd, &temp, sizeof(temp));
            lseek(teeFd, (off_t) 40, SEEK_SET);
            temp =  total * frameSize;
            // FIXME not big-endian safe
            write(teeFd, &temp, sizeof(temp));
            close(teeFd);
            // TODO Should create file with temporary name and then rename to final if non-empty.
            if (total > 0) {
                if (fd >= 0) {
                    dprintf(fd, "tee copied to %s\n", teePath);
                }
            } else {
                unlink(teePath);
            }
        } else {
            if (fd >= 0) {
                dprintf(fd, "unable to create tee %s: %s\n", teePath, strerror(errno));
            }
        }
    }
}
#endif

// ----------------------------------------------------------------------------

status_t AudioFlinger::onTransact(
        uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    return BnAudioFlinger::onTransact(code, data, reply, flags);
}

} // namespace android
