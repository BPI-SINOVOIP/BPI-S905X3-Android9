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

#pragma once

#include <atomic>
#include <memory>
#include <unordered_set>

#include <stdint.h>
#include <sys/types.h>
#include <cutils/config_utils.h>
#include <cutils/misc.h>
#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/SortedVector.h>
#include <media/AudioParameter.h>
#include <media/AudioPolicy.h>
#include "AudioPolicyInterface.h"

#include <AudioPolicyManagerInterface.h>
#include <AudioPolicyManagerObserver.h>
#include <AudioGain.h>
#include <AudioPolicyConfig.h>
#include <AudioPort.h>
#include <AudioPatch.h>
#include <DeviceDescriptor.h>
#include <IOProfile.h>
#include <HwModule.h>
#include <AudioInputDescriptor.h>
#include <AudioOutputDescriptor.h>
#include <AudioPolicyMix.h>
#include <EffectDescriptor.h>
#include <SoundTriggerSession.h>
#include <SessionRoute.h>
#include <VolumeCurve.h>

namespace android {

// ----------------------------------------------------------------------------

// Attenuation applied to STRATEGY_SONIFICATION streams when a headset is connected: 6dB
#define SONIFICATION_HEADSET_VOLUME_FACTOR_DB (-6)
// Min volume for STRATEGY_SONIFICATION streams when limited by music volume: -36dB
#define SONIFICATION_HEADSET_VOLUME_MIN_DB  (-36)
// Max volume difference on A2DP between playing media and STRATEGY_SONIFICATION streams: 12dB
#define SONIFICATION_A2DP_MAX_MEDIA_DIFF_DB (12)

// Time in milliseconds during which we consider that music is still active after a music
// track was stopped - see computeVolume()
#define SONIFICATION_HEADSET_MUSIC_DELAY  5000

// Time in milliseconds during witch some streams are muted while the audio path
// is switched
#define MUTE_TIME_MS 2000

// multiplication factor applied to output latency when calculating a safe mute delay when
// invalidating tracks
#define LATENCY_MUTE_FACTOR 4

#define NUM_TEST_OUTPUTS 5

#define NUM_VOL_CURVE_KNEES 2

// Default minimum length allowed for offloading a compressed track
// Can be overridden by the audio.offload.min.duration.secs property
#define OFFLOAD_DEFAULT_MIN_DURATION_SECS 60

// ----------------------------------------------------------------------------
// AudioPolicyManager implements audio policy manager behavior common to all platforms.
// ----------------------------------------------------------------------------

class AudioPolicyManager : public AudioPolicyInterface, public AudioPolicyManagerObserver
{

public:
        explicit AudioPolicyManager(AudioPolicyClientInterface *clientInterface);
        virtual ~AudioPolicyManager();

        // AudioPolicyInterface
        virtual status_t setDeviceConnectionState(audio_devices_t device,
                                                          audio_policy_dev_state_t state,
                                                          const char *device_address,
                                                          const char *device_name);
        virtual audio_policy_dev_state_t getDeviceConnectionState(audio_devices_t device,
                                                                              const char *device_address);
        virtual status_t handleDeviceConfigChange(audio_devices_t device,
                                                  const char *device_address,
                                                  const char *device_name);
        virtual void setPhoneState(audio_mode_t state);
        virtual void setForceUse(audio_policy_force_use_t usage,
                                 audio_policy_forced_cfg_t config);
        virtual audio_policy_forced_cfg_t getForceUse(audio_policy_force_use_t usage);

        virtual void setSystemProperty(const char* property, const char* value);
        virtual status_t initCheck();
        virtual audio_io_handle_t getOutput(audio_stream_type_t stream);
        virtual status_t getOutputForAttr(const audio_attributes_t *attr,
                                          audio_io_handle_t *output,
                                          audio_session_t session,
                                          audio_stream_type_t *stream,
                                          uid_t uid,
                                          const audio_config_t *config,
                                          audio_output_flags_t *flags,
                                          audio_port_handle_t *selectedDeviceId,
                                          audio_port_handle_t *portId);
        virtual status_t startOutput(audio_io_handle_t output,
                                     audio_stream_type_t stream,
                                     audio_session_t session);
        virtual status_t stopOutput(audio_io_handle_t output,
                                    audio_stream_type_t stream,
                                    audio_session_t session);
        virtual void releaseOutput(audio_io_handle_t output,
                                   audio_stream_type_t stream,
                                   audio_session_t session);
        virtual status_t getInputForAttr(const audio_attributes_t *attr,
                                         audio_io_handle_t *input,
                                         audio_session_t session,
                                         uid_t uid,
                                         const audio_config_base_t *config,
                                         audio_input_flags_t flags,
                                         audio_port_handle_t *selectedDeviceId,
                                         input_type_t *inputType,
                                         audio_port_handle_t *portId);

        // indicates to the audio policy manager that the input starts being used.
        virtual status_t startInput(audio_io_handle_t input,
                                    audio_session_t session,
                                    bool silenced,
                                    concurrency_type__mask_t *concurrency);

        // indicates to the audio policy manager that the input stops being used.
        virtual status_t stopInput(audio_io_handle_t input,
                                   audio_session_t session);
        virtual void releaseInput(audio_io_handle_t input,
                                  audio_session_t session);
        virtual void closeAllInputs();
        virtual void initStreamVolume(audio_stream_type_t stream,
                                                    int indexMin,
                                                    int indexMax);
        virtual status_t setStreamVolumeIndex(audio_stream_type_t stream,
                                              int index,
                                              audio_devices_t device);
        virtual status_t getStreamVolumeIndex(audio_stream_type_t stream,
                                              int *index,
                                              audio_devices_t device);

        // return the strategy corresponding to a given stream type
        virtual uint32_t getStrategyForStream(audio_stream_type_t stream);
        // return the strategy corresponding to the given audio attributes
        virtual uint32_t getStrategyForAttr(const audio_attributes_t *attr);

        // return the enabled output devices for the given stream type
        virtual audio_devices_t getDevicesForStream(audio_stream_type_t stream);

        virtual audio_io_handle_t getOutputForEffect(const effect_descriptor_t *desc = NULL);
        virtual status_t registerEffect(const effect_descriptor_t *desc,
                                        audio_io_handle_t io,
                                        uint32_t strategy,
                                        int session,
                                        int id);
        virtual status_t unregisterEffect(int id)
        {
            return mEffects.unregisterEffect(id);
        }
        virtual status_t setEffectEnabled(int id, bool enabled)
        {
            return mEffects.setEffectEnabled(id, enabled);
        }

        virtual bool isStreamActive(audio_stream_type_t stream, uint32_t inPastMs = 0) const;
        // return whether a stream is playing remotely, override to change the definition of
        //   local/remote playback, used for instance by notification manager to not make
        //   media players lose audio focus when not playing locally
        //   For the base implementation, "remotely" means playing during screen mirroring which
        //   uses an output for playback with a non-empty, non "0" address.
        virtual bool isStreamActiveRemotely(audio_stream_type_t stream,
                                            uint32_t inPastMs = 0) const;

        virtual bool isSourceActive(audio_source_t source) const;

        virtual status_t dump(int fd);

        virtual bool isOffloadSupported(const audio_offload_info_t& offloadInfo);

        virtual status_t listAudioPorts(audio_port_role_t role,
                                        audio_port_type_t type,
                                        unsigned int *num_ports,
                                        struct audio_port *ports,
                                        unsigned int *generation);
        virtual status_t getAudioPort(struct audio_port *port);
        virtual status_t createAudioPatch(const struct audio_patch *patch,
                                           audio_patch_handle_t *handle,
                                           uid_t uid);
        virtual status_t releaseAudioPatch(audio_patch_handle_t handle,
                                              uid_t uid);
        virtual status_t listAudioPatches(unsigned int *num_patches,
                                          struct audio_patch *patches,
                                          unsigned int *generation);
        virtual status_t setAudioPortConfig(const struct audio_port_config *config);

        virtual void releaseResourcesForUid(uid_t uid);

        virtual status_t acquireSoundTriggerSession(audio_session_t *session,
                                               audio_io_handle_t *ioHandle,
                                               audio_devices_t *device);

        virtual status_t releaseSoundTriggerSession(audio_session_t session)
        {
            return mSoundTriggerSessions.releaseSession(session);
        }

        virtual status_t registerPolicyMixes(const Vector<AudioMix>& mixes);
        virtual status_t unregisterPolicyMixes(Vector<AudioMix> mixes);

        virtual status_t startAudioSource(const struct audio_port_config *source,
                                          const audio_attributes_t *attributes,
                                          audio_patch_handle_t *handle,
                                          uid_t uid);
        virtual status_t stopAudioSource(audio_patch_handle_t handle);

        virtual status_t setMasterMono(bool mono);
        virtual status_t getMasterMono(bool *mono);
        virtual float    getStreamVolumeDB(
                    audio_stream_type_t stream, int index, audio_devices_t device);

        virtual status_t getSurroundFormats(unsigned int *numSurroundFormats,
                                            audio_format_t *surroundFormats,
                                            bool *surroundFormatsEnabled,
                                            bool reported);
        virtual status_t setSurroundFormatEnabled(audio_format_t audioFormat, bool enabled);

        // return the strategy corresponding to a given stream type
        routing_strategy getStrategy(audio_stream_type_t stream) const;

        virtual void setRecordSilenced(uid_t uid, bool silenced);

protected:
        // A constructor that allows more fine-grained control over initialization process,
        // used in automatic tests.
        AudioPolicyManager(AudioPolicyClientInterface *clientInterface, bool forTesting);

        // These methods should be used when finer control over APM initialization
        // is needed, e.g. in tests. Must be used in conjunction with the constructor
        // that only performs fields initialization. The public constructor comprises
        // these steps in the following sequence:
        //   - field initializing constructor;
        //   - loadConfig;
        //   - initialize.
        AudioPolicyConfig& getConfig() { return mConfig; }
        void loadConfig();
        status_t initialize();

        // From AudioPolicyManagerObserver
        virtual const AudioPatchCollection &getAudioPatches() const
        {
            return mAudioPatches;
        }
        virtual const SoundTriggerSessionCollection &getSoundTriggerSessionCollection() const
        {
            return mSoundTriggerSessions;
        }
        virtual const AudioPolicyMixCollection &getAudioPolicyMixCollection() const
        {
            return mPolicyMixes;
        }
        virtual const SwAudioOutputCollection &getOutputs() const
        {
            return mOutputs;
        }
        virtual const AudioInputCollection &getInputs() const
        {
            return mInputs;
        }
        virtual const DeviceVector &getAvailableOutputDevices() const
        {
            return mAvailableOutputDevices;
        }
        virtual const DeviceVector &getAvailableInputDevices() const
        {
            return mAvailableInputDevices;
        }
        virtual IVolumeCurvesCollection &getVolumeCurves() { return *mVolumeCurves; }
        virtual const sp<DeviceDescriptor> &getDefaultOutputDevice() const
        {
            return mDefaultOutputDevice;
        }

        void addOutput(audio_io_handle_t output, const sp<SwAudioOutputDescriptor>& outputDesc);
        void removeOutput(audio_io_handle_t output);
        void addInput(audio_io_handle_t input, const sp<AudioInputDescriptor>& inputDesc);

        // return appropriate device for streams handled by the specified strategy according to current
        // phone state, connected devices...
        // if fromCache is true, the device is returned from mDeviceForStrategy[],
        // otherwise it is determine by current state
        // (device connected,phone state, force use, a2dp output...)
        // This allows to:
        //  1 speed up process when the state is stable (when starting or stopping an output)
        //  2 access to either current device selection (fromCache == true) or
        // "future" device selection (fromCache == false) when called from a context
        //  where conditions are changing (setDeviceConnectionState(), setPhoneState()...) AND
        //  before updateDevicesAndOutputs() is called.
        virtual audio_devices_t getDeviceForStrategy(routing_strategy strategy,
                                                     bool fromCache);

        bool isStrategyActive(const sp<AudioOutputDescriptor>& outputDesc, routing_strategy strategy,
                              uint32_t inPastMs = 0, nsecs_t sysTime = 0) const;

        // change the route of the specified output. Returns the number of ms we have slept to
        // allow new routing to take effect in certain cases.
        virtual uint32_t setOutputDevice(const sp<AudioOutputDescriptor>& outputDesc,
                             audio_devices_t device,
                             bool force = false,
                             int delayMs = 0,
                             audio_patch_handle_t *patchHandle = NULL,
                             const char *address = nullptr,
                             bool requiresMuteCheck = true);
        status_t resetOutputDevice(const sp<AudioOutputDescriptor>& outputDesc,
                                   int delayMs = 0,
                                   audio_patch_handle_t *patchHandle = NULL);
        status_t setInputDevice(audio_io_handle_t input,
                                audio_devices_t device,
                                bool force = false,
                                audio_patch_handle_t *patchHandle = NULL);
        status_t resetInputDevice(audio_io_handle_t input,
                                  audio_patch_handle_t *patchHandle = NULL);

        // select input device corresponding to requested audio source
        virtual audio_devices_t getDeviceForInputSource(audio_source_t inputSource);

        // compute the actual volume for a given stream according to the requested index and a particular
        // device
        virtual float computeVolume(audio_stream_type_t stream,
                                    int index,
                                    audio_devices_t device);

        // check that volume change is permitted, compute and send new volume to audio hardware
        virtual status_t checkAndSetVolume(audio_stream_type_t stream, int index,
                                           const sp<AudioOutputDescriptor>& outputDesc,
                                           audio_devices_t device,
                                           int delayMs = 0, bool force = false);

        // apply all stream volumes to the specified output and device
        void applyStreamVolumes(const sp<AudioOutputDescriptor>& outputDesc,
                                audio_devices_t device, int delayMs = 0, bool force = false);

        // Mute or unmute all streams handled by the specified strategy on the specified output
        void setStrategyMute(routing_strategy strategy,
                             bool on,
                             const sp<AudioOutputDescriptor>& outputDesc,
                             int delayMs = 0,
                             audio_devices_t device = (audio_devices_t)0);

        // Mute or unmute the stream on the specified output
        void setStreamMute(audio_stream_type_t stream,
                           bool on,
                           const sp<AudioOutputDescriptor>& outputDesc,
                           int delayMs = 0,
                           audio_devices_t device = (audio_devices_t)0);

        // handle special cases for sonification strategy while in call: mute streams or replace by
        // a special tone in the device used for communication
        void handleIncallSonification(audio_stream_type_t stream, bool starting, bool stateChange);

        audio_mode_t getPhoneState();

        // true if device is in a telephony or VoIP call
        virtual bool isInCall();
        // true if given state represents a device in a telephony or VoIP call
        virtual bool isStateInCall(int state);

        // when a device is connected, checks if an open output can be routed
        // to this device. If none is open, tries to open one of the available outputs.
        // Returns an output suitable to this device or 0.
        // when a device is disconnected, checks if an output is not used any more and
        // returns its handle if any.
        // transfers the audio tracks and effects from one output thread to another accordingly.
        status_t checkOutputsForDevice(const sp<DeviceDescriptor>& devDesc,
                                       audio_policy_dev_state_t state,
                                       SortedVector<audio_io_handle_t>& outputs,
                                       const String8& address);

        status_t checkInputsForDevice(const sp<DeviceDescriptor>& devDesc,
                                      audio_policy_dev_state_t state,
                                      SortedVector<audio_io_handle_t>& inputs,
                                      const String8& address);

        // close an output and its companion duplicating output.
        void closeOutput(audio_io_handle_t output);

        // close an input.
        void closeInput(audio_io_handle_t input);

        // checks and if necessary changes outputs used for all strategies.
        // must be called every time a condition that affects the output choice for a given strategy
        // changes: connected device, phone state, force use...
        // Must be called before updateDevicesAndOutputs()
        void checkOutputForStrategy(routing_strategy strategy);

        // Same as checkOutputForStrategy() but for a all strategies in order of priority
        void checkOutputForAllStrategies();

        // manages A2DP output suspend/restore according to phone state and BT SCO usage
        void checkA2dpSuspend();

        // selects the most appropriate device on output for current state
        // must be called every time a condition that affects the device choice for a given output is
        // changed: connected device, phone state, force use, output start, output stop..
        // see getDeviceForStrategy() for the use of fromCache parameter
        audio_devices_t getNewOutputDevice(const sp<AudioOutputDescriptor>& outputDesc,
                                           bool fromCache);

        // updates cache of device used by all strategies (mDeviceForStrategy[])
        // must be called every time a condition that affects the device choice for a given strategy is
        // changed: connected device, phone state, force use...
        // cached values are used by getDeviceForStrategy() if parameter fromCache is true.
         // Must be called after checkOutputForAllStrategies()
        void updateDevicesAndOutputs();

        // selects the most appropriate device on input for current state
        audio_devices_t getNewInputDevice(const sp<AudioInputDescriptor>& inputDesc);

        virtual uint32_t getMaxEffectsCpuLoad()
        {
            return mEffects.getMaxEffectsCpuLoad();
        }

        virtual uint32_t getMaxEffectsMemory()
        {
            return mEffects.getMaxEffectsMemory();
        }

        SortedVector<audio_io_handle_t> getOutputsForDevice(audio_devices_t device,
                                                            const SwAudioOutputCollection& openOutputs);
        bool vectorsEqual(SortedVector<audio_io_handle_t>& outputs1,
                                           SortedVector<audio_io_handle_t>& outputs2);

        // mute/unmute strategies using an incompatible device combination
        // if muting, wait for the audio in pcm buffer to be drained before proceeding
        // if unmuting, unmute only after the specified delay
        // Returns the number of ms waited
        virtual uint32_t  checkDeviceMuteStrategies(const sp<AudioOutputDescriptor>& outputDesc,
                                            audio_devices_t prevDevice,
                                            uint32_t delayMs);

        audio_io_handle_t selectOutput(const SortedVector<audio_io_handle_t>& outputs,
                                       audio_output_flags_t flags,
                                       audio_format_t format);
        // samplingRate, format, channelMask are in/out and so may be modified
        sp<IOProfile> getInputProfile(audio_devices_t device,
                                      const String8& address,
                                      uint32_t& samplingRate,
                                      audio_format_t& format,
                                      audio_channel_mask_t& channelMask,
                                      audio_input_flags_t flags);
        sp<IOProfile> getProfileForDirectOutput(audio_devices_t device,
                                                       uint32_t samplingRate,
                                                       audio_format_t format,
                                                       audio_channel_mask_t channelMask,
                                                       audio_output_flags_t flags);

        audio_io_handle_t selectOutputForMusicEffects();

        virtual status_t addAudioPatch(audio_patch_handle_t handle, const sp<AudioPatch>& patch)
        {
            return mAudioPatches.addAudioPatch(handle, patch);
        }
        virtual status_t removeAudioPatch(audio_patch_handle_t handle)
        {
            return mAudioPatches.removeAudioPatch(handle);
        }

        audio_devices_t availablePrimaryOutputDevices() const
        {
            if (!hasPrimaryOutput()) {
                return AUDIO_DEVICE_NONE;
            }
            return mPrimaryOutput->supportedDevices() & mAvailableOutputDevices.types();
        }
        audio_devices_t availablePrimaryInputDevices() const
        {
            if (!hasPrimaryOutput()) {
                return AUDIO_DEVICE_NONE;
            }
            return mAvailableInputDevices.getDevicesFromHwModule(mPrimaryOutput->getModuleHandle());
        }

        uint32_t updateCallRouting(audio_devices_t rxDevice, uint32_t delayMs = 0);
        sp<AudioPatch> createTelephonyPatch(bool isRx, audio_devices_t device, uint32_t delayMs);
        sp<DeviceDescriptor> fillAudioPortConfigForDevice(
                const DeviceVector& devices, audio_devices_t device, audio_port_config *config);

        // if argument "device" is different from AUDIO_DEVICE_NONE,  startSource() will force
        // the re-evaluation of the output device.
        status_t startSource(const sp<AudioOutputDescriptor>& outputDesc,
                             audio_stream_type_t stream,
                             audio_devices_t device,
                             const char *address,
                             uint32_t *delayMs);
        status_t stopSource(const sp<AudioOutputDescriptor>& outputDesc,
                            audio_stream_type_t stream,
                            bool forceDeviceUpdate);

        void clearAudioPatches(uid_t uid);
        void clearSessionRoutes(uid_t uid);
        void checkStrategyRoute(routing_strategy strategy, audio_io_handle_t ouptutToSkip);

        status_t hasPrimaryOutput() const { return mPrimaryOutput != 0; }

        status_t connectAudioSource(const sp<AudioSourceDescriptor>& sourceDesc);
        status_t disconnectAudioSource(const sp<AudioSourceDescriptor>& sourceDesc);

        sp<AudioSourceDescriptor> getSourceForStrategyOnOutput(audio_io_handle_t output,
                                                               routing_strategy strategy);

        void cleanUpForDevice(const sp<DeviceDescriptor>& deviceDesc);

        void clearAudioSources(uid_t uid);

        static bool isConcurrentSource(audio_source_t source);

        bool isConcurentCaptureAllowed(const sp<AudioInputDescriptor>& inputDesc,
                const sp<AudioSession>& audioSession);

        static bool streamsMatchForvolume(audio_stream_type_t stream1,
                                          audio_stream_type_t stream2);

        uid_t mUidCached;
        AudioPolicyClientInterface *mpClientInterface;  // audio policy client interface
        sp<SwAudioOutputDescriptor> mPrimaryOutput;     // primary output descriptor
        // list of descriptors for outputs currently opened

        SwAudioOutputCollection mOutputs;
        // copy of mOutputs before setDeviceConnectionState() opens new outputs
        // reset to mOutputs when updateDevicesAndOutputs() is called.
        SwAudioOutputCollection mPreviousOutputs;
        AudioInputCollection mInputs;     // list of input descriptors

        DeviceVector  mAvailableOutputDevices; // all available output devices
        DeviceVector  mAvailableInputDevices;  // all available input devices

        SessionRouteMap mOutputRoutes = SessionRouteMap(SessionRouteMap::MAPTYPE_OUTPUT);
        SessionRouteMap mInputRoutes = SessionRouteMap(SessionRouteMap::MAPTYPE_INPUT);

        bool    mLimitRingtoneVolume;        // limit ringtone volume to music volume if headset connected
        audio_devices_t mDeviceForStrategy[NUM_STRATEGIES];
        float   mLastVoiceVolume;            // last voice volume value sent to audio HAL
        bool    mA2dpSuspended;  // true if A2DP output is suspended

        std::unique_ptr<IVolumeCurvesCollection> mVolumeCurves; // Volume Curves per use case and device category
        EffectDescriptorCollection mEffects;  // list of registered audio effects
        sp<DeviceDescriptor> mDefaultOutputDevice; // output device selected by default at boot time
        HwModuleCollection mHwModules; // contains only modules that have been loaded successfully
        HwModuleCollection mHwModulesAll; // normally not needed, used during construction and for
                                          // dumps
        AudioPolicyConfig mConfig;

        std::atomic<uint32_t> mAudioPortGeneration;

        AudioPatchCollection mAudioPatches;

        SoundTriggerSessionCollection mSoundTriggerSessions;

        sp<AudioPatch> mCallTxPatch;
        sp<AudioPatch> mCallRxPatch;

        HwAudioOutputCollection mHwOutputs;
        AudioSourceCollection mAudioSources;

        // for supporting "beacon" streams, i.e. streams that only play on speaker, and never
        // when something other than STREAM_TTS (a.k.a. "Transmitted Through Speaker") is playing
        enum {
            STARTING_OUTPUT,
            STARTING_BEACON,
            STOPPING_OUTPUT,
            STOPPING_BEACON
        };
        uint32_t mBeaconMuteRefCount;   // ref count for stream that would mute beacon
        uint32_t mBeaconPlayingRefCount;// ref count for the playing beacon streams
        bool mBeaconMuted;              // has STREAM_TTS been muted
        bool mTtsOutputAvailable;       // true if a dedicated output for TTS stream is available

        bool mMasterMono;               // true if we wish to force all outputs to mono
        AudioPolicyMixCollection mPolicyMixes; // list of registered mixes
        audio_io_handle_t mMusicEffectOutput;     // output selected for music effects

        uint32_t nextAudioPortGeneration();

        // Audio Policy Engine Interface.
        AudioPolicyManagerInterface *mEngine;

        // Surround formats that are enabled.
        std::unordered_set<audio_format_t> mSurroundFormats;
private:
        // Add or remove AC3 DTS encodings based on user preferences.
        void filterSurroundFormats(FormatVector *formatsPtr);
        void filterSurroundChannelMasks(ChannelsVector *channelMasksPtr);

        status_t getSupportedFormats(audio_io_handle_t ioHandle, FormatVector& formats);

        // If any, resolve any "dynamic" fields of an Audio Profiles collection
        void updateAudioProfiles(audio_devices_t device, audio_io_handle_t ioHandle,
                AudioProfileVector &profiles);

        // Notify the policy client of any change of device state with AUDIO_IO_HANDLE_NONE,
        // so that the client interprets it as global to audio hardware interfaces.
        // It can give a chance to HAL implementer to retrieve dynamic capabilities associated
        // to this device for example.
        // TODO avoid opening stream to retrieve capabilities of a profile.
        void broadcastDeviceConnectionState(audio_devices_t device,
                                            audio_policy_dev_state_t state,
                                            const String8 &device_address);

        // updates device caching and output for streams that can influence the
        //    routing of notifications
        void handleNotificationRoutingForStream(audio_stream_type_t stream);
        // find the outputs on a given output descriptor that have the given address.
        // to be called on an AudioOutputDescriptor whose supported devices (as defined
        //   in mProfile->mSupportedDevices) matches the device whose address is to be matched.
        // see deviceDistinguishesOnAddress(audio_devices_t) for whether the device type is one
        //   where addresses are used to distinguish between one connected device and another.
        void findIoHandlesByAddress(const sp<SwAudioOutputDescriptor>& desc /*in*/,
                const audio_devices_t device /*in*/,
                const String8& address /*in*/,
                SortedVector<audio_io_handle_t>& outputs /*out*/);
        uint32_t curAudioPortGeneration() const { return mAudioPortGeneration; }
        // internal method to return the output handle for the given device and format
        audio_io_handle_t getOutputForDevice(
                audio_devices_t device,
                audio_session_t session,
                audio_stream_type_t stream,
                const audio_config_t *config,
                audio_output_flags_t *flags);
        // internal method to return the input handle for the given device and format
        audio_io_handle_t getInputForDevice(audio_devices_t device,
                String8 address,
                audio_session_t session,
                uid_t uid,
                audio_source_t inputSource,
                const audio_config_base_t *config,
                audio_input_flags_t flags,
                AudioMix *policyMix);

        // internal function to derive a stream type value from audio attributes
        audio_stream_type_t streamTypefromAttributesInt(const audio_attributes_t *attr);
        // event is one of STARTING_OUTPUT, STARTING_BEACON, STOPPING_OUTPUT, STOPPING_BEACON
        // returns 0 if no mute/unmute event happened, the largest latency of the device where
        //   the mute/unmute happened
        uint32_t handleEventForBeacon(int event);
        uint32_t setBeaconMute(bool mute);
        bool     isValidAttributes(const audio_attributes_t *paa);

        // select input device corresponding to requested audio source and return associated policy
        // mix if any. Calls getDeviceForInputSource().
        audio_devices_t getDeviceAndMixForInputSource(audio_source_t inputSource,
                                                        AudioMix **policyMix = NULL);

        // Called by setDeviceConnectionState().
        status_t setDeviceConnectionStateInt(audio_devices_t device,
                                                          audio_policy_dev_state_t state,
                                                          const char *device_address,
                                                          const char *device_name);
        void updateMono(audio_io_handle_t output) {
            AudioParameter param;
            param.addInt(String8(AudioParameter::keyMonoOutput), (int)mMasterMono);
            mpClientInterface->setParameters(output, param.toString());
        }

        bool soundTriggerSupportsConcurrentCapture();
        bool mSoundTriggerSupportsConcurrentCapture;
        bool mHasComputedSoundTriggerSupportsConcurrentCapture;
};

};
