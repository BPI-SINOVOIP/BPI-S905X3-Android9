/*
 * Copyright (C) 2015 The Android Open Source Project
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

#define LOG_TAG "APM::AudioInputDescriptor"
//#define LOG_NDEBUG 0

#include <AudioPolicyInterface.h>
#include "AudioInputDescriptor.h"
#include "IOProfile.h"
#include "AudioGain.h"
#include "HwModule.h"
#include <media/AudioPolicy.h>
#include <policy.h>

namespace android {

AudioInputDescriptor::AudioInputDescriptor(const sp<IOProfile>& profile,
                                           AudioPolicyClientInterface *clientInterface)
    : mIoHandle(0),
      mDevice(AUDIO_DEVICE_NONE), mPolicyMix(NULL),
      mProfile(profile), mPatchHandle(AUDIO_PATCH_HANDLE_NONE), mId(0),
      mClientInterface(clientInterface)
{
    if (profile != NULL) {
        profile->pickAudioProfile(mSamplingRate, mChannelMask, mFormat);
        if (profile->mGains.size() > 0) {
            profile->mGains[0]->getDefaultConfig(&mGain);
        }
    }
}

audio_module_handle_t AudioInputDescriptor::getModuleHandle() const
{
    if (mProfile == 0) {
        return AUDIO_MODULE_HANDLE_NONE;
    }
    return mProfile->getModuleHandle();
}

uint32_t AudioInputDescriptor::getOpenRefCount() const
{
    return mSessions.getOpenCount();
}

audio_port_handle_t AudioInputDescriptor::getId() const
{
    return mId;
}

audio_source_t AudioInputDescriptor::inputSource(bool activeOnly) const
{
    return getHighestPrioritySource(activeOnly);
}

void AudioInputDescriptor::toAudioPortConfig(struct audio_port_config *dstConfig,
                                             const struct audio_port_config *srcConfig) const
{
    ALOG_ASSERT(mProfile != 0,
                "toAudioPortConfig() called on input with null profile %d", mIoHandle);
    dstConfig->config_mask = AUDIO_PORT_CONFIG_SAMPLE_RATE|AUDIO_PORT_CONFIG_CHANNEL_MASK|
                            AUDIO_PORT_CONFIG_FORMAT|AUDIO_PORT_CONFIG_GAIN;
    if (srcConfig != NULL) {
        dstConfig->config_mask |= srcConfig->config_mask;
    }

    AudioPortConfig::toAudioPortConfig(dstConfig, srcConfig);

    dstConfig->id = mId;
    dstConfig->role = AUDIO_PORT_ROLE_SINK;
    dstConfig->type = AUDIO_PORT_TYPE_MIX;
    dstConfig->ext.mix.hw_module = getModuleHandle();
    dstConfig->ext.mix.handle = mIoHandle;
    dstConfig->ext.mix.usecase.source = inputSource();
}

void AudioInputDescriptor::toAudioPort(struct audio_port *port) const
{
    ALOG_ASSERT(mProfile != 0, "toAudioPort() called on input with null profile %d", mIoHandle);

    mProfile->toAudioPort(port);
    port->id = mId;
    toAudioPortConfig(&port->active_config);
    port->ext.mix.hw_module = getModuleHandle();
    port->ext.mix.handle = mIoHandle;
    port->ext.mix.latency_class = AUDIO_LATENCY_NORMAL;
}

void AudioInputDescriptor::setPreemptedSessions(const SortedVector<audio_session_t>& sessions)
{
    mPreemptedSessions = sessions;
}

SortedVector<audio_session_t> AudioInputDescriptor::getPreemptedSessions() const
{
    return mPreemptedSessions;
}

bool AudioInputDescriptor::hasPreemptedSession(audio_session_t session) const
{
    return (mPreemptedSessions.indexOf(session) >= 0);
}

void AudioInputDescriptor::clearPreemptedSessions()
{
    mPreemptedSessions.clear();
}

bool AudioInputDescriptor::isActive() const {
    return mSessions.hasActiveSession();
}

bool AudioInputDescriptor::isSourceActive(audio_source_t source) const
{
    return mSessions.isSourceActive(source);
}

audio_source_t AudioInputDescriptor::getHighestPrioritySource(bool activeOnly) const
{

    return mSessions.getHighestPrioritySource(activeOnly);
}

bool AudioInputDescriptor::isSoundTrigger() const {
    // sound trigger and non sound trigger sessions are not mixed
    // on a given input
    return mSessions.valueAt(0)->isSoundTrigger();
}

sp<AudioSession> AudioInputDescriptor::getAudioSession(
                                              audio_session_t session) const {
    return mSessions.valueFor(session);
}

AudioSessionCollection AudioInputDescriptor::getAudioSessions(bool activeOnly) const
{
    if (activeOnly) {
        return mSessions.getActiveSessions();
    } else {
        return mSessions;
    }
}

size_t AudioInputDescriptor::getAudioSessionCount(bool activeOnly) const
{
    if (activeOnly) {
        return mSessions.getActiveSessionCount();
    } else {
        return mSessions.size();
    }
}

status_t AudioInputDescriptor::addAudioSession(audio_session_t session,
                         const sp<AudioSession>& audioSession) {
    return mSessions.addSession(session, audioSession, /*AudioSessionInfoProvider*/this);
}

status_t AudioInputDescriptor::removeAudioSession(audio_session_t session) {
    return mSessions.removeSession(session);
}

audio_patch_handle_t AudioInputDescriptor::getPatchHandle() const
{
    return mPatchHandle;
}

void AudioInputDescriptor::setPatchHandle(audio_patch_handle_t handle)
{
    mPatchHandle = handle;
    mSessions.onSessionInfoUpdate();
}

audio_config_base_t AudioInputDescriptor::getConfig() const
{
    const audio_config_base_t config = { .sample_rate = mSamplingRate, .channel_mask = mChannelMask,
            .format = mFormat };
    return config;
}

status_t AudioInputDescriptor::open(const audio_config_t *config,
                                       audio_devices_t device,
                                       const String8& address,
                                       audio_source_t source,
                                       audio_input_flags_t flags,
                                       audio_io_handle_t *input)
{
    audio_config_t lConfig;
    if (config == nullptr) {
        lConfig = AUDIO_CONFIG_INITIALIZER;
        lConfig.sample_rate = mSamplingRate;
        lConfig.channel_mask = mChannelMask;
        lConfig.format = mFormat;
    } else {
        lConfig = *config;
    }

    mDevice = device;

    ALOGV("opening input for device %08x address %s profile %p name %s",
          mDevice, address.string(), mProfile.get(), mProfile->getName().string());

    status_t status = mClientInterface->openInput(mProfile->getModuleHandle(),
                                                  input,
                                                  &lConfig,
                                                  &mDevice,
                                                  address,
                                                  source,
                                                  flags);
    LOG_ALWAYS_FATAL_IF(mDevice != device,
                        "%s openInput returned device %08x when given device %08x",
                        __FUNCTION__, mDevice, device);

    if (status == NO_ERROR) {
        LOG_ALWAYS_FATAL_IF(*input == AUDIO_IO_HANDLE_NONE,
                            "%s openInput returned input handle %d for device %08x",
                            __FUNCTION__, *input, device);
        mSamplingRate = lConfig.sample_rate;
        mChannelMask = lConfig.channel_mask;
        mFormat = lConfig.format;
        mId = AudioPort::getNextUniqueId();
        mIoHandle = *input;
        mProfile->curOpenCount++;
    }

    return status;
}

status_t AudioInputDescriptor::start()
{
    if (getAudioSessionCount(true/*activeOnly*/) == 1) {
        if (!mProfile->canStartNewIo()) {
            ALOGI("%s mProfile->curActiveCount %d", __func__, mProfile->curActiveCount);
            return INVALID_OPERATION;
        }
        mProfile->curActiveCount++;
    }
    return NO_ERROR;
}

void AudioInputDescriptor::stop()
{
    if (!isActive()) {
        LOG_ALWAYS_FATAL_IF(mProfile->curActiveCount < 1,
                            "%s invalid profile active count %u",
                            __func__, mProfile->curActiveCount);
        mProfile->curActiveCount--;
    }
}

void AudioInputDescriptor::close()
{
    if (mIoHandle != AUDIO_IO_HANDLE_NONE) {
        mClientInterface->closeInput(mIoHandle);
        LOG_ALWAYS_FATAL_IF(mProfile->curOpenCount < 1, "%s profile open count %u",
                            __FUNCTION__, mProfile->curOpenCount);
        // do not call stop() here as stop() is supposed to be called after
        // AudioSession::changeActiveCount(-1) and we don't know how many sessions
        // are still active at this time
        if (isActive()) {
            mProfile->curActiveCount--;
        }
        mProfile->curOpenCount--;
        mIoHandle = AUDIO_IO_HANDLE_NONE;
    }
}

status_t AudioInputDescriptor::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, " ID: %d\n", getId());
    result.append(buffer);
    snprintf(buffer, SIZE, " Sampling rate: %d\n", mSamplingRate);
    result.append(buffer);
    snprintf(buffer, SIZE, " Format: %d\n", mFormat);
    result.append(buffer);
    snprintf(buffer, SIZE, " Channels: %08x\n", mChannelMask);
    result.append(buffer);
    snprintf(buffer, SIZE, " Devices %08x\n", mDevice);
    result.append(buffer);

    write(fd, result.string(), result.size());

    mSessions.dump(fd, 1);

    return NO_ERROR;
}

bool AudioInputCollection::isSourceActive(audio_source_t source) const
{
    for (size_t i = 0; i < size(); i++) {
        const sp<AudioInputDescriptor>  inputDescriptor = valueAt(i);
        if (inputDescriptor->isSourceActive(source)) {
            return true;
        }
    }
    return false;
}

sp<AudioInputDescriptor> AudioInputCollection::getInputFromId(audio_port_handle_t id) const
{
    sp<AudioInputDescriptor> inputDesc = NULL;
    for (size_t i = 0; i < size(); i++) {
        inputDesc = valueAt(i);
        if (inputDesc->getId() == id) {
            break;
        }
    }
    return inputDesc;
}

uint32_t AudioInputCollection::activeInputsCountOnDevices(audio_devices_t devices) const
{
    uint32_t count = 0;
    for (size_t i = 0; i < size(); i++) {
        const sp<AudioInputDescriptor>  inputDescriptor = valueAt(i);
        if (inputDescriptor->isActive() &&
                ((devices == AUDIO_DEVICE_IN_DEFAULT) ||
                 ((inputDescriptor->mDevice & devices & ~AUDIO_DEVICE_BIT_IN) != 0))) {
            count++;
        }
    }
    return count;
}

Vector<sp <AudioInputDescriptor> > AudioInputCollection::getActiveInputs(bool ignoreVirtualInputs)
{
    Vector<sp <AudioInputDescriptor> > activeInputs;

    for (size_t i = 0; i < size(); i++) {
        const sp<AudioInputDescriptor>  inputDescriptor = valueAt(i);
        if ((inputDescriptor->isActive())
                && (!ignoreVirtualInputs ||
                    !is_virtual_input_device(inputDescriptor->mDevice))) {
            activeInputs.add(inputDescriptor);
        }
    }
    return activeInputs;
}

audio_devices_t AudioInputCollection::getSupportedDevices(audio_io_handle_t handle) const
{
    sp<AudioInputDescriptor> inputDesc = valueFor(handle);
    audio_devices_t devices = inputDesc->mProfile->getSupportedDevicesType();
    return devices;
}

status_t AudioInputCollection::dump(int fd) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];

    snprintf(buffer, SIZE, "\nInputs dump:\n");
    write(fd, buffer, strlen(buffer));
    for (size_t i = 0; i < size(); i++) {
        snprintf(buffer, SIZE, "- Input %d dump:\n", keyAt(i));
        write(fd, buffer, strlen(buffer));
        valueAt(i)->dump(fd);
    }

    return NO_ERROR;
}

}; //namespace android
