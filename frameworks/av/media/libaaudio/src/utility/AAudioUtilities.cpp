/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "AAudio"
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#include <cutils/properties.h>
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>

#include "aaudio/AAudio.h"
#include <aaudio/AAudioTesting.h>
#include <math.h>
#include <system/audio-base.h>
#include <assert.h>

#include "utility/AAudioUtilities.h"

using namespace android;

// This is 3 dB, (10^(3/20)), to match the maximum headroom in AudioTrack for float data.
// It is designed to allow occasional transient peaks.
#define MAX_HEADROOM (1.41253754f)
#define MIN_HEADROOM (0 - MAX_HEADROOM)

int32_t AAudioConvert_formatToSizeInBytes(aaudio_format_t format) {
    int32_t size = AAUDIO_ERROR_ILLEGAL_ARGUMENT;
    switch (format) {
        case AAUDIO_FORMAT_PCM_I16:
            size = sizeof(int16_t);
            break;
        case AAUDIO_FORMAT_PCM_FLOAT:
            size = sizeof(float);
            break;
        default:
            break;
    }
    return size;
}

// TODO expose and call clamp16_from_float function in primitives.h
static inline int16_t clamp16_from_float(float f) {
    static const float scale = 1 << 15;
    return (int16_t) roundf(fmaxf(fminf(f * scale, scale - 1.f), -scale));
}

// Clip to valid range of a float sample to prevent excessive volume.
// By using fmin and fmax we also protect against NaN.
static float clipToMinMaxHeadroom(float input) {
    return fmin(MAX_HEADROOM, fmax(MIN_HEADROOM, input));
}

static float clipAndClampFloatToPcm16(float sample, float scaler) {
    // Clip to valid range of a float sample to prevent excessive volume.
    sample = clipToMinMaxHeadroom(sample);

    // Scale and convert to a short.
    float fval = sample * scaler;
    return clamp16_from_float(fval);
}

void AAudioConvert_floatToPcm16(const float *source,
                                int16_t *destination,
                                int32_t numSamples,
                                float amplitude) {
    const float scaler = amplitude;
    for (int i = 0; i < numSamples; i++) {
        float sample = *source++;
        *destination++ = clipAndClampFloatToPcm16(sample, scaler);
    }
}

void AAudioConvert_floatToPcm16(const float *source,
                                int16_t *destination,
                                int32_t numFrames,
                                int32_t samplesPerFrame,
                                float amplitude1,
                                float amplitude2) {
    float scaler = amplitude1;
    // divide by numFrames so that we almost reach amplitude2
    float delta = (amplitude2 - amplitude1) / numFrames;
    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        for (int sampleIndex = 0; sampleIndex < samplesPerFrame; sampleIndex++) {
            float sample = *source++;
            *destination++ = clipAndClampFloatToPcm16(sample, scaler);
        }
        scaler += delta;
    }
}

#define SHORT_SCALE  32768

void AAudioConvert_pcm16ToFloat(const int16_t *source,
                                float *destination,
                                int32_t numSamples,
                                float amplitude) {
    const float scaler = amplitude / SHORT_SCALE;
    for (int i = 0; i < numSamples; i++) {
        destination[i] = source[i] * scaler;
    }
}

// This code assumes amplitude1 and amplitude2 are between 0.0 and 1.0
void AAudioConvert_pcm16ToFloat(const int16_t *source,
                                float *destination,
                                int32_t numFrames,
                                int32_t samplesPerFrame,
                                float amplitude1,
                                float amplitude2) {
    float scaler = amplitude1 / SHORT_SCALE;
    const float delta = (amplitude2 - amplitude1) / (SHORT_SCALE * (float) numFrames);
    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        for (int sampleIndex = 0; sampleIndex < samplesPerFrame; sampleIndex++) {
            *destination++ = *source++ * scaler;
        }
        scaler += delta;
    }
}


// This code assumes amplitude1 and amplitude2 are between 0.0 and 1.0
void AAudio_linearRamp(const float *source,
                       float *destination,
                       int32_t numFrames,
                       int32_t samplesPerFrame,
                       float amplitude1,
                       float amplitude2) {
    float scaler = amplitude1;
    const float delta = (amplitude2 - amplitude1) / numFrames;
    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        for (int sampleIndex = 0; sampleIndex < samplesPerFrame; sampleIndex++) {
            float sample = *source++;
            // Clip to valid range of a float sample to prevent excessive volume.
            sample = clipToMinMaxHeadroom(sample);

            *destination++ = sample * scaler;
        }
        scaler += delta;
    }
}

// This code assumes amplitude1 and amplitude2 are between 0.0 and 1.0
void AAudio_linearRamp(const int16_t *source,
                       int16_t *destination,
                       int32_t numFrames,
                       int32_t samplesPerFrame,
                       float amplitude1,
                       float amplitude2) {
    // Because we are converting from int16 to 1nt16, we do not have to scale by 1/32768.
    float scaler = amplitude1;
    const float delta = (amplitude2 - amplitude1) / numFrames;
    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        for (int sampleIndex = 0; sampleIndex < samplesPerFrame; sampleIndex++) {
            // No need to clip because int16_t range is inherently limited.
            float sample =  *source++ * scaler;
            *destination++ = (int16_t) roundf(sample);
        }
        scaler += delta;
    }
}

// *************************************************************************************
// Convert Mono To Stereo at the same time as converting format.
void AAudioConvert_formatMonoToStereo(const float *source,
                                      int16_t *destination,
                                      int32_t numFrames,
                                      float amplitude) {
    const float scaler = amplitude;
    for (int i = 0; i < numFrames; i++) {
        float sample = *source++;
        int16_t sample16 = clipAndClampFloatToPcm16(sample, scaler);
        *destination++ = sample16;
        *destination++ = sample16;
    }
}

void AAudioConvert_formatMonoToStereo(const float *source,
                                      int16_t *destination,
                                      int32_t numFrames,
                                      float amplitude1,
                                      float amplitude2) {
    // divide by numFrames so that we almost reach amplitude2
    const float delta = (amplitude2 - amplitude1) / numFrames;
    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        const float scaler = amplitude1 + (frameIndex * delta);
        const float sample = *source++;
        int16_t sample16 = clipAndClampFloatToPcm16(sample, scaler);
        *destination++ = sample16;
        *destination++ = sample16;
    }
}

void AAudioConvert_formatMonoToStereo(const int16_t *source,
                                      float *destination,
                                      int32_t numFrames,
                                      float amplitude) {
    const float scaler = amplitude / SHORT_SCALE;
    for (int i = 0; i < numFrames; i++) {
        float sample = source[i] * scaler;
        *destination++ = sample;
        *destination++ = sample;
    }
}

// This code assumes amplitude1 and amplitude2 are between 0.0 and 1.0
void AAudioConvert_formatMonoToStereo(const int16_t *source,
                                      float *destination,
                                      int32_t numFrames,
                                      float amplitude1,
                                      float amplitude2) {
    const float scaler1 = amplitude1 / SHORT_SCALE;
    const float delta = (amplitude2 - amplitude1) / (SHORT_SCALE * (float) numFrames);
    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        float scaler = scaler1 + (frameIndex * delta);
        float sample = source[frameIndex] * scaler;
        *destination++ = sample;
        *destination++ = sample;
    }
}

// This code assumes amplitude1 and amplitude2 are between 0.0 and 1.0
void AAudio_linearRampMonoToStereo(const float *source,
                                   float *destination,
                                   int32_t numFrames,
                                   float amplitude1,
                                   float amplitude2) {
    const float delta = (amplitude2 - amplitude1) / numFrames;
    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        float sample = *source++;

        // Clip to valid range of a float sample to prevent excessive volume.
        sample = clipToMinMaxHeadroom(sample);

        const float scaler = amplitude1 + (frameIndex * delta);
        float sampleScaled = sample * scaler;
        *destination++ = sampleScaled;
        *destination++ = sampleScaled;
    }
}

// This code assumes amplitude1 and amplitude2 are between 0.0 and 1.0
void AAudio_linearRampMonoToStereo(const int16_t *source,
                                   int16_t *destination,
                                   int32_t numFrames,
                                   float amplitude1,
                                   float amplitude2) {
    // Because we are converting from int16 to 1nt16, we do not have to scale by 1/32768.
    const float delta = (amplitude2 - amplitude1) / numFrames;
    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        const float scaler = amplitude1 + (frameIndex * delta);
        // No need to clip because int16_t range is inherently limited.
        const float sample =  *source++ * scaler;
        int16_t sample16 = (int16_t) roundf(sample);
        *destination++ = sample16;
        *destination++ = sample16;
    }
}

// *************************************************************************************
void AAudioDataConverter::convert(
        const FormattedData &source,
        const FormattedData &destination,
        int32_t numFrames,
        float levelFrom,
        float levelTo) {

    if (source.channelCount == 1 && destination.channelCount == 2) {
        convertMonoToStereo(source,
                            destination,
                            numFrames,
                            levelFrom,
                            levelTo);
    } else {
        // We only support mono to stereo conversion. Otherwise source and destination
        // must match.
        assert(source.channelCount == destination.channelCount);
        convertChannelsMatch(source,
                             destination,
                             numFrames,
                             levelFrom,
                             levelTo);
    }
}

void AAudioDataConverter::convertMonoToStereo(
        const FormattedData &source,
        const FormattedData &destination,
        int32_t numFrames,
        float levelFrom,
        float levelTo) {

    // The formats are validated when the stream is opened so we do not have to
    // check for illegal combinations here.
    if (source.format == AAUDIO_FORMAT_PCM_FLOAT) {
        if (destination.format == AAUDIO_FORMAT_PCM_FLOAT) {
            AAudio_linearRampMonoToStereo(
                    (const float *) source.data,
                    (float *) destination.data,
                    numFrames,
                    levelFrom,
                    levelTo);
        } else if (destination.format == AAUDIO_FORMAT_PCM_I16) {
            if (levelFrom != levelTo) {
                AAudioConvert_formatMonoToStereo(
                        (const float *) source.data,
                        (int16_t *) destination.data,
                        numFrames,
                        levelFrom,
                        levelTo);
            } else {
                AAudioConvert_formatMonoToStereo(
                        (const float *) source.data,
                        (int16_t *) destination.data,
                        numFrames,
                        levelTo);
            }
        }
    } else if (source.format == AAUDIO_FORMAT_PCM_I16) {
        if (destination.format == AAUDIO_FORMAT_PCM_FLOAT) {
            if (levelFrom != levelTo) {
                AAudioConvert_formatMonoToStereo(
                        (const int16_t *) source.data,
                        (float *) destination.data,
                        numFrames,
                        levelFrom,
                        levelTo);
            } else {
                AAudioConvert_formatMonoToStereo(
                        (const int16_t *) source.data,
                        (float *) destination.data,
                        numFrames,
                        levelTo);
            }
        } else if (destination.format == AAUDIO_FORMAT_PCM_I16) {
            AAudio_linearRampMonoToStereo(
                    (const int16_t *) source.data,
                    (int16_t *) destination.data,
                    numFrames,
                    levelFrom,
                    levelTo);
        }
    }
}

void AAudioDataConverter::convertChannelsMatch(
        const FormattedData &source,
        const FormattedData &destination,
        int32_t numFrames,
        float levelFrom,
        float levelTo) {
    const int32_t numSamples = numFrames * source.channelCount;

    // The formats are validated when the stream is opened so we do not have to
    // check for illegal combinations here.
    if (source.format == AAUDIO_FORMAT_PCM_FLOAT) {
        if (destination.format == AAUDIO_FORMAT_PCM_FLOAT) {
            AAudio_linearRamp(
                    (const float *) source.data,
                    (float *) destination.data,
                    numFrames,
                    source.channelCount,
                    levelFrom,
                    levelTo);
        } else if (destination.format == AAUDIO_FORMAT_PCM_I16) {
            if (levelFrom != levelTo) {
                AAudioConvert_floatToPcm16(
                        (const float *) source.data,
                        (int16_t *) destination.data,
                        numFrames,
                        source.channelCount,
                        levelFrom,
                        levelTo);
            } else {
                AAudioConvert_floatToPcm16(
                        (const float *) source.data,
                        (int16_t *) destination.data,
                        numSamples,
                        levelTo);
            }
        }
    } else if (source.format == AAUDIO_FORMAT_PCM_I16) {
        if (destination.format == AAUDIO_FORMAT_PCM_FLOAT) {
            if (levelFrom != levelTo) {
                AAudioConvert_pcm16ToFloat(
                        (const int16_t *) source.data,
                        (float *) destination.data,
                        numFrames,
                        source.channelCount,
                        levelFrom,
                        levelTo);
            } else {
                AAudioConvert_pcm16ToFloat(
                        (const int16_t *) source.data,
                        (float *) destination.data,
                        numSamples,
                        levelTo);
            }
        } else if (destination.format == AAUDIO_FORMAT_PCM_I16) {
            AAudio_linearRamp(
                    (const int16_t *) source.data,
                    (int16_t *) destination.data,
                    numFrames,
                    source.channelCount,
                    levelFrom,
                    levelTo);
        }
    }
}

status_t AAudioConvert_aaudioToAndroidStatus(aaudio_result_t result) {
    // This covers the case for AAUDIO_OK and for positive results.
    if (result >= 0) {
        return result;
    }
    status_t status;
    switch (result) {
    case AAUDIO_ERROR_DISCONNECTED:
    case AAUDIO_ERROR_NO_SERVICE:
        status = DEAD_OBJECT;
        break;
    case AAUDIO_ERROR_INVALID_HANDLE:
        status = BAD_TYPE;
        break;
    case AAUDIO_ERROR_INVALID_STATE:
        status = INVALID_OPERATION;
        break;
    case AAUDIO_ERROR_INVALID_RATE:
    case AAUDIO_ERROR_INVALID_FORMAT:
    case AAUDIO_ERROR_ILLEGAL_ARGUMENT:
    case AAUDIO_ERROR_OUT_OF_RANGE:
        status = BAD_VALUE;
        break;
    case AAUDIO_ERROR_WOULD_BLOCK:
        status = WOULD_BLOCK;
        break;
    case AAUDIO_ERROR_NULL:
        status = UNEXPECTED_NULL;
        break;
    case AAUDIO_ERROR_UNAVAILABLE:
        status = NOT_ENOUGH_DATA;
        break;

    // TODO translate these result codes
    case AAUDIO_ERROR_INTERNAL:
    case AAUDIO_ERROR_UNIMPLEMENTED:
    case AAUDIO_ERROR_NO_FREE_HANDLES:
    case AAUDIO_ERROR_NO_MEMORY:
    case AAUDIO_ERROR_TIMEOUT:
    default:
        status = UNKNOWN_ERROR;
        break;
    }
    return status;
}

aaudio_result_t AAudioConvert_androidToAAudioResult(status_t status) {
    // This covers the case for OK and for positive result.
    if (status >= 0) {
        return status;
    }
    aaudio_result_t result;
    switch (status) {
    case BAD_TYPE:
        result = AAUDIO_ERROR_INVALID_HANDLE;
        break;
    case DEAD_OBJECT:
        result = AAUDIO_ERROR_NO_SERVICE;
        break;
    case INVALID_OPERATION:
        result = AAUDIO_ERROR_INVALID_STATE;
        break;
    case UNEXPECTED_NULL:
        result = AAUDIO_ERROR_NULL;
        break;
    case BAD_VALUE:
        result = AAUDIO_ERROR_ILLEGAL_ARGUMENT;
        break;
    case WOULD_BLOCK:
        result = AAUDIO_ERROR_WOULD_BLOCK;
        break;
    case NOT_ENOUGH_DATA:
        result = AAUDIO_ERROR_UNAVAILABLE;
        break;
    default:
        result = AAUDIO_ERROR_INTERNAL;
        break;
    }
    return result;
}

audio_session_t AAudioConvert_aaudioToAndroidSessionId(aaudio_session_id_t sessionId) {
    // If not a regular sessionId then convert to a safe value of AUDIO_SESSION_ALLOCATE.
    return (sessionId == AAUDIO_SESSION_ID_ALLOCATE || sessionId == AAUDIO_SESSION_ID_NONE)
           ? AUDIO_SESSION_ALLOCATE
           : (audio_session_t) sessionId;
}

audio_format_t AAudioConvert_aaudioToAndroidDataFormat(aaudio_format_t aaudioFormat) {
    audio_format_t androidFormat;
    switch (aaudioFormat) {
    case AAUDIO_FORMAT_PCM_I16:
        androidFormat = AUDIO_FORMAT_PCM_16_BIT;
        break;
    case AAUDIO_FORMAT_PCM_FLOAT:
        androidFormat = AUDIO_FORMAT_PCM_FLOAT;
        break;
    default:
        androidFormat = AUDIO_FORMAT_DEFAULT;
        ALOGE("AAudioConvert_aaudioToAndroidDataFormat 0x%08X unrecognized", aaudioFormat);
        break;
    }
    return androidFormat;
}

aaudio_format_t AAudioConvert_androidToAAudioDataFormat(audio_format_t androidFormat) {
    aaudio_format_t aaudioFormat = AAUDIO_FORMAT_INVALID;
    switch (androidFormat) {
    case AUDIO_FORMAT_PCM_16_BIT:
        aaudioFormat = AAUDIO_FORMAT_PCM_I16;
        break;
    case AUDIO_FORMAT_PCM_FLOAT:
        aaudioFormat = AAUDIO_FORMAT_PCM_FLOAT;
        break;
    default:
        aaudioFormat = AAUDIO_FORMAT_INVALID;
        ALOGE("AAudioConvert_androidToAAudioDataFormat 0x%08X unrecognized", androidFormat);
        break;
    }
    return aaudioFormat;
}

// Make a message string from the condition.
#define STATIC_ASSERT(condition) static_assert(condition, #condition)

audio_usage_t AAudioConvert_usageToInternal(aaudio_usage_t usage) {
    // The public aaudio_content_type_t constants are supposed to have the same
    // values as the internal audio_content_type_t values.
    STATIC_ASSERT(AAUDIO_USAGE_MEDIA == AUDIO_USAGE_MEDIA);
    STATIC_ASSERT(AAUDIO_USAGE_VOICE_COMMUNICATION == AUDIO_USAGE_VOICE_COMMUNICATION);
    STATIC_ASSERT(AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING
                  == AUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING);
    STATIC_ASSERT(AAUDIO_USAGE_ALARM == AUDIO_USAGE_ALARM);
    STATIC_ASSERT(AAUDIO_USAGE_NOTIFICATION == AUDIO_USAGE_NOTIFICATION);
    STATIC_ASSERT(AAUDIO_USAGE_NOTIFICATION_RINGTONE
                  == AUDIO_USAGE_NOTIFICATION_TELEPHONY_RINGTONE);
    STATIC_ASSERT(AAUDIO_USAGE_NOTIFICATION_EVENT == AUDIO_USAGE_NOTIFICATION_EVENT);
    STATIC_ASSERT(AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY == AUDIO_USAGE_ASSISTANCE_ACCESSIBILITY);
    STATIC_ASSERT(AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE
                  == AUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE);
    STATIC_ASSERT(AAUDIO_USAGE_ASSISTANCE_SONIFICATION == AUDIO_USAGE_ASSISTANCE_SONIFICATION);
    STATIC_ASSERT(AAUDIO_USAGE_GAME == AUDIO_USAGE_GAME);
    STATIC_ASSERT(AAUDIO_USAGE_ASSISTANT == AUDIO_USAGE_ASSISTANT);
    if (usage == AAUDIO_UNSPECIFIED) {
        usage = AAUDIO_USAGE_MEDIA;
    }
    return (audio_usage_t) usage; // same value
}

audio_content_type_t AAudioConvert_contentTypeToInternal(aaudio_content_type_t contentType) {
    // The public aaudio_content_type_t constants are supposed to have the same
    // values as the internal audio_content_type_t values.
    STATIC_ASSERT(AAUDIO_CONTENT_TYPE_MUSIC == AUDIO_CONTENT_TYPE_MUSIC);
    STATIC_ASSERT(AAUDIO_CONTENT_TYPE_SPEECH == AUDIO_CONTENT_TYPE_SPEECH);
    STATIC_ASSERT(AAUDIO_CONTENT_TYPE_SONIFICATION == AUDIO_CONTENT_TYPE_SONIFICATION);
    STATIC_ASSERT(AAUDIO_CONTENT_TYPE_MOVIE == AUDIO_CONTENT_TYPE_MOVIE);
    if (contentType == AAUDIO_UNSPECIFIED) {
        contentType = AAUDIO_CONTENT_TYPE_MUSIC;
    }
    return (audio_content_type_t) contentType; // same value
}

audio_source_t AAudioConvert_inputPresetToAudioSource(aaudio_input_preset_t preset) {
    // The public aaudio_input_preset_t constants are supposed to have the same
    // values as the internal audio_source_t values.
    STATIC_ASSERT(AAUDIO_UNSPECIFIED == AUDIO_SOURCE_DEFAULT);
    STATIC_ASSERT(AAUDIO_INPUT_PRESET_GENERIC == AUDIO_SOURCE_MIC);
    STATIC_ASSERT(AAUDIO_INPUT_PRESET_CAMCORDER == AUDIO_SOURCE_CAMCORDER);
    STATIC_ASSERT(AAUDIO_INPUT_PRESET_VOICE_RECOGNITION == AUDIO_SOURCE_VOICE_RECOGNITION);
    STATIC_ASSERT(AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION == AUDIO_SOURCE_VOICE_COMMUNICATION);
    STATIC_ASSERT(AAUDIO_INPUT_PRESET_UNPROCESSED == AUDIO_SOURCE_UNPROCESSED);
    if (preset == AAUDIO_UNSPECIFIED) {
        preset = AAUDIO_INPUT_PRESET_VOICE_RECOGNITION;
    }
    return (audio_source_t) preset; // same value
}

int32_t AAudioConvert_framesToBytes(int32_t numFrames,
                                    int32_t bytesPerFrame,
                                    int32_t *sizeInBytes) {
    *sizeInBytes = 0;

    if (numFrames < 0 || bytesPerFrame < 0) {
        ALOGE("negative size, numFrames = %d, frameSize = %d", numFrames, bytesPerFrame);
        return AAUDIO_ERROR_OUT_OF_RANGE;
    }

    // Prevent numeric overflow.
    if (numFrames > (INT32_MAX / bytesPerFrame)) {
        ALOGE("size overflow, numFrames = %d, frameSize = %d", numFrames, bytesPerFrame);
        return AAUDIO_ERROR_OUT_OF_RANGE;
    }

    *sizeInBytes = numFrames * bytesPerFrame;
    return AAUDIO_OK;
}

static int32_t AAudioProperty_getMMapProperty(const char *propName,
                                              int32_t defaultValue,
                                              const char * caller) {
    int32_t prop = property_get_int32(propName, defaultValue);
    switch (prop) {
        case AAUDIO_UNSPECIFIED:
        case AAUDIO_POLICY_NEVER:
        case AAUDIO_POLICY_ALWAYS:
        case AAUDIO_POLICY_AUTO:
            break;
        default:
            ALOGE("%s: invalid = %d", caller, prop);
            prop = defaultValue;
            break;
    }
    return prop;
}

int32_t AAudioProperty_getMMapPolicy() {
    return AAudioProperty_getMMapProperty(AAUDIO_PROP_MMAP_POLICY,
                                          AAUDIO_UNSPECIFIED, __func__);
}

int32_t AAudioProperty_getMMapExclusivePolicy() {
    return AAudioProperty_getMMapProperty(AAUDIO_PROP_MMAP_EXCLUSIVE_POLICY,
                                          AAUDIO_UNSPECIFIED, __func__);
}

int32_t AAudioProperty_getMixerBursts() {
    const int32_t defaultBursts = 2; // arbitrary, use 2 for double buffered
    const int32_t maxBursts = 1024; // arbitrary
    int32_t prop = property_get_int32(AAUDIO_PROP_MIXER_BURSTS, defaultBursts);
    if (prop < 1 || prop > maxBursts) {
        ALOGE("AAudioProperty_getMixerBursts: invalid = %d", prop);
        prop = defaultBursts;
    }
    return prop;
}

int32_t AAudioProperty_getWakeupDelayMicros() {
    const int32_t minMicros = 0; // arbitrary
    const int32_t defaultMicros = 200; // arbitrary, based on some observed jitter
    const int32_t maxMicros = 5000; // arbitrary, probably don't want more than 500
    int32_t prop = property_get_int32(AAUDIO_PROP_WAKEUP_DELAY_USEC, defaultMicros);
    if (prop < minMicros) {
        ALOGW("AAudioProperty_getWakeupDelayMicros: clipped %d to %d", prop, minMicros);
        prop = minMicros;
    } else if (prop > maxMicros) {
        ALOGW("AAudioProperty_getWakeupDelayMicros: clipped %d to %d", prop, maxMicros);
        prop = maxMicros;
    }
    return prop;
}

int32_t AAudioProperty_getMinimumSleepMicros() {
    const int32_t minMicros = 20; // arbitrary
    const int32_t defaultMicros = 200; // arbitrary
    const int32_t maxMicros = 2000; // arbitrary
    int32_t prop = property_get_int32(AAUDIO_PROP_MINIMUM_SLEEP_USEC, defaultMicros);
    if (prop < minMicros) {
        ALOGW("AAudioProperty_getMinimumSleepMicros: clipped %d to %d", prop, minMicros);
        prop = minMicros;
    } else if (prop > maxMicros) {
        ALOGW("AAudioProperty_getMinimumSleepMicros: clipped %d to %d", prop, maxMicros);
        prop = maxMicros;
    }
    return prop;
}

int32_t AAudioProperty_getHardwareBurstMinMicros() {
    const int32_t defaultMicros = 1000; // arbitrary
    const int32_t maxMicros = 1000 * 1000; // arbitrary
    int32_t prop = property_get_int32(AAUDIO_PROP_HW_BURST_MIN_USEC, defaultMicros);
    if (prop < 1 || prop > maxMicros) {
        ALOGE("AAudioProperty_getHardwareBurstMinMicros: invalid = %d, use %d",
              prop, defaultMicros);
        prop = defaultMicros;
    }
    return prop;
}

aaudio_result_t AAudio_isFlushAllowed(aaudio_stream_state_t state) {
    aaudio_result_t result = AAUDIO_OK;
    switch (state) {
// Proceed with flushing.
        case AAUDIO_STREAM_STATE_OPEN:
        case AAUDIO_STREAM_STATE_PAUSED:
        case AAUDIO_STREAM_STATE_STOPPED:
        case AAUDIO_STREAM_STATE_FLUSHED:
            break;

// Transition from one inactive state to another.
        case AAUDIO_STREAM_STATE_STARTING:
        case AAUDIO_STREAM_STATE_STARTED:
        case AAUDIO_STREAM_STATE_STOPPING:
        case AAUDIO_STREAM_STATE_PAUSING:
        case AAUDIO_STREAM_STATE_FLUSHING:
        case AAUDIO_STREAM_STATE_CLOSING:
        case AAUDIO_STREAM_STATE_CLOSED:
        case AAUDIO_STREAM_STATE_DISCONNECTED:
        default:
            ALOGE("can only flush stream when PAUSED, OPEN or STOPPED, state = %s",
                  AAudio_convertStreamStateToText(state));
            result =  AAUDIO_ERROR_INVALID_STATE;
            break;
    }
    return result;
}
