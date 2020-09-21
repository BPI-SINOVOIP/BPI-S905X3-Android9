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

#define LOG_TAG "APM_AudioPolicyMix"
//#define LOG_NDEBUG 0

#include "AudioPolicyMix.h"
#include "TypeConverter.h"
#include "HwModule.h"
#include "AudioPort.h"
#include "IOProfile.h"
#include "AudioGain.h"
#include <AudioOutputDescriptor.h>

namespace android {

void AudioPolicyMix::setOutput(sp<SwAudioOutputDescriptor> &output)
{
    mOutput = output;
}

const sp<SwAudioOutputDescriptor> &AudioPolicyMix::getOutput() const
{
    return mOutput;
}

void AudioPolicyMix::clearOutput()
{
    mOutput.clear();
}

void AudioPolicyMix::setMix(AudioMix &mix)
{
    mMix = mix;
}

android::AudioMix *AudioPolicyMix::getMix()
{
    return &mMix;
}

status_t AudioPolicyMix::dump(int fd, int spaces, int index) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, "%*sAudio Policy Mix %d:\n", spaces, "", index+1);
    result.append(buffer);
    std::string mixTypeLiteral;
    if (!MixTypeConverter::toString(mMix.mMixType, mixTypeLiteral)) {
        ALOGE("%s: failed to convert mix type %d", __FUNCTION__, mMix.mMixType);
        return BAD_VALUE;
    }
    snprintf(buffer, SIZE, "%*s- mix type: %s\n", spaces, "", mixTypeLiteral.c_str());
    result.append(buffer);
    std::string routeFlagLiteral;
    RouteFlagTypeConverter::maskToString(mMix.mRouteFlags, routeFlagLiteral);
    snprintf(buffer, SIZE, "%*s- Route Flags: %s\n", spaces, "", routeFlagLiteral.c_str());
    result.append(buffer);
    std::string deviceLiteral;
    deviceToString(mMix.mDeviceType, deviceLiteral);
    snprintf(buffer, SIZE, "%*s- device type: %s\n", spaces, "", deviceLiteral.c_str());
    result.append(buffer);
    snprintf(buffer, SIZE, "%*s- device address: %s\n", spaces, "", mMix.mDeviceAddress.string());
    result.append(buffer);

    int indexCriterion = 0;
    for (const auto &criterion : mMix.mCriteria) {
        snprintf(buffer, SIZE, "%*s- Criterion %d:\n", spaces + 2, "", indexCriterion++);
        result.append(buffer);
        std::string usageLiteral;
        if (!UsageTypeConverter::toString(criterion.mValue.mUsage, usageLiteral)) {
            ALOGE("%s: failed to convert usage %d", __FUNCTION__, criterion.mValue.mUsage);
            return BAD_VALUE;
        }
        snprintf(buffer, SIZE, "%*s- Usage:%s\n", spaces + 4, "", usageLiteral.c_str());
        result.append(buffer);
        if (mMix.mMixType == MIX_TYPE_RECORDERS) {
            std::string sourceLiteral;
            if (!SourceTypeConverter::toString(criterion.mValue.mSource, sourceLiteral)) {
                ALOGE("%s: failed to convert source %d", __FUNCTION__, criterion.mValue.mSource);
                return BAD_VALUE;
            }
            snprintf(buffer, SIZE, "%*s- Source:%s\n", spaces + 4, "", sourceLiteral.c_str());
            result.append(buffer);
        }
        snprintf(buffer, SIZE, "%*s- Uid:%d\n", spaces + 4, "", criterion.mValue.mUid);
        result.append(buffer);
        std::string ruleLiteral;
        if (!RuleTypeConverter::toString(criterion.mRule, ruleLiteral)) {
            ALOGE("%s: failed to convert source %d", __FUNCTION__,criterion.mRule);
            return BAD_VALUE;
        }
        snprintf(buffer, SIZE, "%*s- Rule:%s\n", spaces + 4, "", ruleLiteral.c_str());
        result.append(buffer);
    }
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

status_t AudioPolicyMixCollection::registerMix(const String8& address, AudioMix mix,
                                               sp<SwAudioOutputDescriptor> desc)
{
    ssize_t index = indexOfKey(address);
    if (index >= 0) {
        ALOGE("registerPolicyMixes(): mix for address %s already registered", address.string());
        return BAD_VALUE;
    }
    sp<AudioPolicyMix> policyMix = new AudioPolicyMix();
    policyMix->setMix(mix);
    add(address, policyMix);

    if (desc != 0) {
        desc->mPolicyMix = policyMix->getMix();
        policyMix->setOutput(desc);
    }
    return NO_ERROR;
}

status_t AudioPolicyMixCollection::unregisterMix(const String8& address)
{
    ssize_t index = indexOfKey(address);
    if (index < 0) {
        ALOGE("unregisterPolicyMixes(): mix for address %s not registered", address.string());
        return BAD_VALUE;
    }

    removeItemsAt(index);
    return NO_ERROR;
}

status_t AudioPolicyMixCollection::getAudioPolicyMix(const String8& address,
                                                     sp<AudioPolicyMix> &policyMix) const
{
    ssize_t index = indexOfKey(address);
    if (index < 0) {
        ALOGE("unregisterPolicyMixes(): mix for address %s not registered", address.string());
        return BAD_VALUE;
    }
    policyMix = valueAt(index);
    return NO_ERROR;
}

void AudioPolicyMixCollection::closeOutput(sp<SwAudioOutputDescriptor> &desc)
{
    for (size_t i = 0; i < size(); i++) {
        sp<AudioPolicyMix> policyMix = valueAt(i);
        if (policyMix->getOutput() == desc) {
            policyMix->clearOutput();
        }
    }
}

status_t AudioPolicyMixCollection::getOutputForAttr(audio_attributes_t attributes, uid_t uid,
                                                    sp<SwAudioOutputDescriptor> &desc)
{
    ALOGV("getOutputForAttr() querying %zu mixes:", size());
    desc = 0;
    for (size_t i = 0; i < size(); i++) {
        sp<AudioPolicyMix> policyMix = valueAt(i);
        AudioMix *mix = policyMix->getMix();

        if (mix->mMixType == MIX_TYPE_PLAYERS) {
            // TODO if adding more player rules (currently only 2), make rule handling "generic"
            //      as there is no difference in the treatment of usage- or uid-based rules
            bool hasUsageMatchRules = false;
            bool hasUsageExcludeRules = false;
            bool usageMatchFound = false;
            bool usageExclusionFound = false;

            bool hasUidMatchRules = false;
            bool hasUidExcludeRules = false;
            bool uidMatchFound = false;
            bool uidExclusionFound = false;

            bool hasAddrMatch = false;

            // iterate over all mix criteria to list what rules this mix contains
            for (size_t j = 0; j < mix->mCriteria.size(); j++) {
                ALOGV(" getOutputForAttr: mix %zu: inspecting mix criteria %zu of %zu",
                        i, j, mix->mCriteria.size());

                // if there is an address match, prioritize that match
                if (strncmp(attributes.tags, "addr=", strlen("addr=")) == 0 &&
                        strncmp(attributes.tags + strlen("addr="),
                                mix->mDeviceAddress.string(),
                                AUDIO_ATTRIBUTES_TAGS_MAX_SIZE - strlen("addr=") - 1) == 0) {
                    hasAddrMatch = true;
                    break;
                }

                switch (mix->mCriteria[j].mRule) {
                case RULE_MATCH_ATTRIBUTE_USAGE:
                    ALOGV("\tmix has RULE_MATCH_ATTRIBUTE_USAGE for usage %d",
                                                mix->mCriteria[j].mValue.mUsage);
                    hasUsageMatchRules = true;
                    if (mix->mCriteria[j].mValue.mUsage == attributes.usage) {
                        // found one match against all allowed usages
                        usageMatchFound = true;
                    }
                    break;
                case RULE_EXCLUDE_ATTRIBUTE_USAGE:
                    ALOGV("\tmix has RULE_EXCLUDE_ATTRIBUTE_USAGE for usage %d",
                            mix->mCriteria[j].mValue.mUsage);
                    hasUsageExcludeRules = true;
                    if (mix->mCriteria[j].mValue.mUsage == attributes.usage) {
                        // found this usage is to be excluded
                        usageExclusionFound = true;
                    }
                    break;
                case RULE_MATCH_UID:
                    ALOGV("\tmix has RULE_MATCH_UID for uid %d", mix->mCriteria[j].mValue.mUid);
                    hasUidMatchRules = true;
                    if (mix->mCriteria[j].mValue.mUid == uid) {
                        // found one UID match against all allowed UIDs
                        uidMatchFound = true;
                    }
                    break;
                case RULE_EXCLUDE_UID:
                    ALOGV("\tmix has RULE_EXCLUDE_UID for uid %d", mix->mCriteria[j].mValue.mUid);
                    hasUidExcludeRules = true;
                    if (mix->mCriteria[j].mValue.mUid == uid) {
                        // found this UID is to be excluded
                        uidExclusionFound = true;
                    }
                    break;
                default:
                    break;
                }

                // consistency checks: for each "dimension" of rules (usage, uid...), we can
                // only have MATCH rules, or EXCLUDE rules in each dimension, not a combination
                if (hasUsageMatchRules && hasUsageExcludeRules) {
                    ALOGE("getOutputForAttr: invalid combination of RULE_MATCH_ATTRIBUTE_USAGE"
                            " and RULE_EXCLUDE_ATTRIBUTE_USAGE in mix %zu", i);
                    return BAD_VALUE;
                }
                if (hasUidMatchRules && hasUidExcludeRules) {
                    ALOGE("getOutputForAttr: invalid combination of RULE_MATCH_UID"
                            " and RULE_EXCLUDE_UID in mix %zu", i);
                    return BAD_VALUE;
                }

                if ((hasUsageExcludeRules && usageExclusionFound)
                        || (hasUidExcludeRules && uidExclusionFound)) {
                    break; // stop iterating on criteria because an exclusion was found (will fail)
                }

            }//iterate on mix criteria

            // determine if exiting on success (or implicit failure as desc is 0)
            if (hasAddrMatch ||
                    !((hasUsageExcludeRules && usageExclusionFound) ||
                      (hasUsageMatchRules && !usageMatchFound)  ||
                      (hasUidExcludeRules && uidExclusionFound) ||
                      (hasUidMatchRules && !uidMatchFound))) {
                ALOGV("\tgetOutputForAttr will use mix %zu", i);
                desc = policyMix->getOutput();
            }

        } else if (mix->mMixType == MIX_TYPE_RECORDERS) {
            if (attributes.usage == AUDIO_USAGE_VIRTUAL_SOURCE &&
                    strncmp(attributes.tags, "addr=", strlen("addr=")) == 0 &&
                    strncmp(attributes.tags + strlen("addr="),
                            mix->mDeviceAddress.string(),
                            AUDIO_ATTRIBUTES_TAGS_MAX_SIZE - strlen("addr=") - 1) == 0) {
                desc = policyMix->getOutput();
            }
        }
        if (desc != 0) {
            desc->mPolicyMix = mix;
            return NO_ERROR;
        }
    }
    return BAD_VALUE;
}

audio_devices_t AudioPolicyMixCollection::getDeviceAndMixForInputSource(audio_source_t inputSource,
                                                                        audio_devices_t availDevices,
                                                                        AudioMix **policyMix)
{
    for (size_t i = 0; i < size(); i++) {
        AudioMix *mix = valueAt(i)->getMix();

        if (mix->mMixType != MIX_TYPE_RECORDERS) {
            continue;
        }
        for (size_t j = 0; j < mix->mCriteria.size(); j++) {
            if ((RULE_MATCH_ATTRIBUTE_CAPTURE_PRESET == mix->mCriteria[j].mRule &&
                    mix->mCriteria[j].mValue.mSource == inputSource) ||
               (RULE_EXCLUDE_ATTRIBUTE_CAPTURE_PRESET == mix->mCriteria[j].mRule &&
                    mix->mCriteria[j].mValue.mSource != inputSource)) {
                if (availDevices & AUDIO_DEVICE_IN_REMOTE_SUBMIX) {
                    if (policyMix != NULL) {
                        *policyMix = mix;
                    }
                    return AUDIO_DEVICE_IN_REMOTE_SUBMIX;
                }
                break;
            }
        }
    }
    return AUDIO_DEVICE_NONE;
}

status_t AudioPolicyMixCollection::getInputMixForAttr(audio_attributes_t attr, AudioMix **policyMix)
{
    if (strncmp(attr.tags, "addr=", strlen("addr=")) != 0) {
        return BAD_VALUE;
    }
    String8 address(attr.tags + strlen("addr="));

#ifdef LOG_NDEBUG
    ALOGV("getInputMixForAttr looking for address %s\n  mixes available:", address.string());
    for (size_t i = 0; i < size(); i++) {
            sp<AudioPolicyMix> policyMix = valueAt(i);
            AudioMix *mix = policyMix->getMix();
            ALOGV("\tmix %zu address=%s", i, mix->mDeviceAddress.string());
    }
#endif

    ssize_t index = indexOfKey(address);
    if (index < 0) {
        ALOGW("getInputMixForAttr() no policy for address %s", address.string());
        return BAD_VALUE;
    }
    sp<AudioPolicyMix> audioPolicyMix = valueAt(index);
    AudioMix *mix = audioPolicyMix->getMix();

    if (mix->mMixType != MIX_TYPE_PLAYERS) {
        ALOGW("getInputMixForAttr() bad policy mix type for address %s", address.string());
        return BAD_VALUE;
    }
    *policyMix = mix;
    return NO_ERROR;
}

status_t AudioPolicyMixCollection::dump(int fd) const
{
    std::string log("\nAudio Policy Mix:\n");
    write(fd, log.c_str(), log.size());
    for (size_t i = 0; i < size(); i++) {
        valueAt(i)->dump(fd, 2, i);
    }
    return NO_ERROR;
}

}; //namespace android
