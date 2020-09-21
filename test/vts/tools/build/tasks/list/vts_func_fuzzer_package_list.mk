#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

vts_func_fuzzer_packages := \
  android.hardware.audio@2.0-vts.func_fuzzer.Device \
  android.hardware.audio@2.0-vts.func_fuzzer.DevicesFactory \
  android.hardware.audio@2.0-vts.func_fuzzer.PrimaryDevice \
  android.hardware.audio@2.0-vts.func_fuzzer.Stream \
  android.hardware.audio@2.0-vts.func_fuzzer.StreamIn \
  android.hardware.audio@2.0-vts.func_fuzzer.StreamOut \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.AcousticEchoCancelerEffect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.AutomaticGainControlEffect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.BassBoostEffect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.DownmixEffect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.Effect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.EffectsFactory \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.EnvironmentalReverbEffect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.EqualizerEffect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.LoudnessEnhancerEffect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.NoiseSuppressionEffect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.PresetReverbEffect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.VirtualizerEffect \
  android.hardware.audio.effect@2.0-vts.func_fuzzer.VisualizerEffect \
  android.hardware.automotive.evs@1.0-vts.func_fuzzer.EvsCamera \
  android.hardware.automotive.evs@1.0-vts.func_fuzzer.EvsCameraStream \
  android.hardware.automotive.evs@1.0-vts.func_fuzzer.EvsDisplay \
  android.hardware.automotive.evs@1.0-vts.func_fuzzer.EvsEnumerator \
  android.hardware.automotive.vehicle@2.0-vts.func_fuzzer.Vehicle \
  android.hardware.benchmarks.msgq@1.0-vts.func_fuzzer.BenchmarkMsgQ \
  android.hardware.biometrics.fingerprint@2.1-vts.func_fuzzer.BiometricsFingerprint \
  android.hardware.bluetooth@1.0-vts.func_fuzzer.BluetoothHci \
  android.hardware.bluetooth@1.0-vts.func_fuzzer.BluetoothHciCallbacks \
  android.hardware.boot@1.0-vts.func_fuzzer.BootControl \
  android.hardware.broadcastradio@1.0-vts.func_fuzzer.BroadcastRadio \
  android.hardware.broadcastradio@1.0-vts.func_fuzzer.BroadcastRadioFactory \
  android.hardware.broadcastradio@1.0-vts.func_fuzzer.Tuner \
  android.hardware.broadcastradio@1.1-vts.func_fuzzer.BroadcastRadioFactory \
  android.hardware.broadcastradio@1.1-vts.func_fuzzer.Tuner \
  android.hardware.camera.device@1.0-vts.func_fuzzer.CameraDevice \
  android.hardware.camera.device@3.2-vts.func_fuzzer.CameraDevice \
  android.hardware.camera.device@3.2-vts.func_fuzzer.CameraDeviceSession \
  android.hardware.camera.provider@2.4-vts.func_fuzzer.CameraProvider \
  android.hardware.configstore@1.0-vts.func_fuzzer.SurfaceFlingerConfigs \
  android.hardware.contexthub@1.0-vts.func_fuzzer.Contexthub \
  android.hardware.drm@1.0-vts.func_fuzzer.CryptoFactory \
  android.hardware.drm@1.0-vts.func_fuzzer.CryptoPlugin \
  android.hardware.drm@1.0-vts.func_fuzzer.DrmFactory \
  android.hardware.drm@1.0-vts.func_fuzzer.DrmPlugin \
  android.hardware.drm@1.0-vts.func_fuzzer.DrmPluginListener \
  android.hardware.dumpstate@1.0-vts.func_fuzzer.DumpstateDevice \
  android.hardware.gatekeeper@1.0-vts.func_fuzzer.Gatekeeper \
  android.hardware.gnss@1.0-vts.func_fuzzer.AGnss \
  android.hardware.gnss@1.0-vts.func_fuzzer.AGnssRil \
  android.hardware.gnss@1.0-vts.func_fuzzer.Gnss \
  android.hardware.gnss@1.0-vts.func_fuzzer.GnssBatching \
  android.hardware.gnss@1.0-vts.func_fuzzer.GnssConfiguration \
  android.hardware.gnss@1.0-vts.func_fuzzer.GnssDebug \
  android.hardware.gnss@1.0-vts.func_fuzzer.GnssGeofencing \
  android.hardware.gnss@1.0-vts.func_fuzzer.GnssMeasurement \
  android.hardware.gnss@1.0-vts.func_fuzzer.GnssNavigationMessage \
  android.hardware.gnss@1.0-vts.func_fuzzer.GnssNi \
  android.hardware.gnss@1.0-vts.func_fuzzer.GnssXtra \
  android.hardware.graphics.allocator@2.0-vts.func_fuzzer.Allocator \
  android.hardware.graphics.allocator@2.0-vts.func_fuzzer.AllocatorClient \
  android.hardware.graphics.composer@2.1-vts.func_fuzzer.Composer \
  android.hardware.graphics.composer@2.1-vts.func_fuzzer.ComposerClient \
  android.hardware.graphics.mapper@2.0-vts.func_fuzzer.Mapper \
  android.hardware.health@1.0-vts.func_fuzzer.Health \
  android.hardware.ir@1.0-vts.func_fuzzer.ConsumerIr \
  android.hardware.keymaster@3.0-vts.func_fuzzer.KeymasterDevice \
  android.hardware.light@2.0-vts.func_fuzzer.Light \
  android.hardware.media.omx@1.0-vts.func_fuzzer.GraphicBufferSource \
  android.hardware.media.omx@1.0-vts.func_fuzzer.Omx \
  android.hardware.media.omx@1.0-vts.func_fuzzer.OmxBufferProducer \
  android.hardware.media.omx@1.0-vts.func_fuzzer.OmxBufferSource \
  android.hardware.media.omx@1.0-vts.func_fuzzer.OmxNode \
  android.hardware.media.omx@1.0-vts.func_fuzzer.OmxObserver \
  android.hardware.media.omx@1.0-vts.func_fuzzer.OmxProducerListener \
  android.hardware.memtrack@1.0-vts.func_fuzzer.Memtrack \
  android.hardware.nfc@1.0-vts.func_fuzzer.Nfc \
  android.hardware.power@1.0-vts.func_fuzzer.Power \
  android.hardware.radio@1.0-vts.func_fuzzer.Radio \
  android.hardware.radio@1.0-vts.func_fuzzer.RadioIndication \
  android.hardware.radio@1.0-vts.func_fuzzer.RadioResponse \
  android.hardware.radio@1.0-vts.func_fuzzer.Sap \
  android.hardware.radio.deprecated@1.0-vts.func_fuzzer.OemHook \
  android.hardware.radio.deprecated@1.0-vts.func_fuzzer.OemHookIndication \
  android.hardware.radio.deprecated@1.0-vts.func_fuzzer.OemHookResponse \
  android.hardware.renderscript@1.0-vts.func_fuzzer.Context \
  android.hardware.renderscript@1.0-vts.func_fuzzer.Device \
  android.hardware.sensors@1.0-vts.func_fuzzer.Sensors \
  android.hardware.soundtrigger@2.0-vts.func_fuzzer.SoundTriggerHw \
  android.hardware.thermal@1.0-vts.func_fuzzer.Thermal \
  android.hardware.tv.cec@1.0-vts.func_fuzzer.HdmiCec \
  android.hardware.tv.input@1.0-vts.func_fuzzer.TvInput \
  android.hardware.usb@1.0-vts.func_fuzzer.Usb \
  android.hardware.vibrator@1.0-vts.func_fuzzer.Vibrator \
  android.hardware.vr@1.0-vts.func_fuzzer.Vr \
  android.hardware.wifi@1.0-vts.func_fuzzer.Wifi \
  android.hardware.wifi@1.0-vts.func_fuzzer.WifiApIface \
  android.hardware.wifi@1.0-vts.func_fuzzer.WifiChip \
  android.hardware.wifi@1.0-vts.func_fuzzer.WifiIface \
  android.hardware.wifi@1.0-vts.func_fuzzer.WifiNanIface \
  android.hardware.wifi@1.0-vts.func_fuzzer.WifiP2pIface \
  android.hardware.wifi@1.0-vts.func_fuzzer.WifiRttController \
  android.hardware.wifi@1.0-vts.func_fuzzer.WifiStaIface \
  android.hardware.wifi.supplicant@1.0-vts.func_fuzzer.Supplicant \
  android.hardware.wifi.supplicant@1.0-vts.func_fuzzer.SupplicantIface \
  android.hardware.wifi.supplicant@1.0-vts.func_fuzzer.SupplicantNetwork \
  android.hardware.wifi.supplicant@1.0-vts.func_fuzzer.SupplicantP2pIface \
  android.hardware.wifi.supplicant@1.0-vts.func_fuzzer.SupplicantP2pNetwork \
  android.hardware.wifi.supplicant@1.0-vts.func_fuzzer.SupplicantStaIface \
  android.hardware.wifi.supplicant@1.0-vts.func_fuzzer.SupplicantStaNetwork \

