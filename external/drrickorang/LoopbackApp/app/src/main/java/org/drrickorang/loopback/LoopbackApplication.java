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

package org.drrickorang.loopback;

import android.app.Application;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Build;
import android.util.Log;


/**
 * This class maintain global application states, so it also keeps and computes the default
 * values of all the audio settings.
 */

public class LoopbackApplication extends Application {
    private static final String TAG = "LoopbackApplication";

    // here defines all the initial setting values, some get modified in ComputeDefaults()
    private int mSamplingRate = 48000;
    private int mChannelIndex = -1;
    private int mPlayerBufferSizeInBytes = 0; // for both native and java
    private int mRecorderBuffSizeInBytes = 0; // for both native and java
    private int mAudioThreadType = Constant.AUDIO_THREAD_TYPE_JAVA; //0:Java, 1:Native (JNI)
    private int mMicSource = 3; //maps to MediaRecorder.AudioSource.VOICE_RECOGNITION;
    private int mPerformanceMode = -1; // DEFAULT
    private int mIgnoreFirstFrames = 0;
    private int mBufferTestDurationInSeconds = 5;
    private int mBufferTestWavePlotDurationInSeconds = 7;
    private int mNumberOfLoadThreads = 4;
    private boolean mCaptureSysTraceEnabled = false;
    private boolean mCaptureBugreportEnabled = false;
    private boolean mCaptureWavSnippetsEnabled = false;
    private boolean mSoundLevelCalibrationEnabled = false;
    private int mNumStateCaptures = Constant.DEFAULT_NUM_CAPTURES;

    public void setDefaults() {
        if (isSafeToUseSles()) {
            mAudioThreadType = Constant.AUDIO_THREAD_TYPE_NATIVE;
        } else {
            mAudioThreadType = Constant.AUDIO_THREAD_TYPE_JAVA;
        }

        computeDefaults();
    }

    int getSamplingRate() {
        return mSamplingRate;
    }

    void setSamplingRate(int samplingRate) {
        mSamplingRate = clamp(samplingRate, Constant.SAMPLING_RATE_MIN, Constant.SAMPLING_RATE_MAX);
    }

    int getChannelIndex() { return mChannelIndex; }

    void setChannelIndex(int channelIndex) { mChannelIndex = channelIndex; }

    int getAudioThreadType() {
        return mAudioThreadType;
    }


    void setAudioThreadType(int audioThreadType) {
        if (isSafeToUseSles() && audioThreadType != Constant.AUDIO_THREAD_TYPE_JAVA) {
            //safe to use native and Java thread not selected
            mAudioThreadType = Constant.AUDIO_THREAD_TYPE_NATIVE;
        } else {
            mAudioThreadType = Constant.AUDIO_THREAD_TYPE_JAVA;
        }
    }


    int getMicSource() {
        return mMicSource;
    }


    int mapMicSource(int threadType, int source) {
        int mappedSource = 0;

        //experiment with remote submix
        if (threadType == Constant.AUDIO_THREAD_TYPE_JAVA) {
            switch (source) {
            default:
            case 0: //DEFAULT
                mappedSource = MediaRecorder.AudioSource.DEFAULT;
                break;
            case 1: //MIC
                mappedSource = MediaRecorder.AudioSource.MIC;
                break;
            case 2: //CAMCORDER
                mappedSource = MediaRecorder.AudioSource.CAMCORDER;
                break;
            case 3: //VOICE_RECOGNITION
                mappedSource = MediaRecorder.AudioSource.VOICE_RECOGNITION;
                break;
            case 4: //VOICE_COMMUNICATION
                mappedSource = MediaRecorder.AudioSource.VOICE_COMMUNICATION;
                break;
            case 5: //REMOTE_SUBMIX (JAVA ONLY)
                mappedSource = MediaRecorder.AudioSource.REMOTE_SUBMIX;
                break;
            case 6: //UNPROCESSED
                if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
                    mappedSource = 9 /*MediaRecorder.AudioSource.UNPROCESSED*/;
                } else {
                    mappedSource = MediaRecorder.AudioSource.DEFAULT;
                }
                break;
            }
        } else if (threadType == Constant.AUDIO_THREAD_TYPE_NATIVE) {
            // FIXME taken from OpenSLES_AndroidConfiguration.h
            switch (source) {
            default:
            case 0: //DEFAULT
                mappedSource = 0x00; //SL_ANDROID_RECORDING_PRESET_NONE
                break;
            case 1: //MIC
                mappedSource = 0x01; //SL_ANDROID_RECORDING_PRESET_GENERIC
                break;
            case 2: //CAMCORDER
                mappedSource = 0x02; //SL_ANDROID_RECORDING_PRESET_CAMCORDER
                break;
            case 3: //VOICE_RECOGNITION
                mappedSource = 0x03; //SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION
                break;
            case 4: //VOICE_COMMUNICATION
                mappedSource = 0x04; //SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION
                break;
            case 5: //REMOTE_SUBMIX (JAVA ONLY)
                mappedSource = 0x00; //SL_ANDROID_RECORDING_PRESET_NONE;
                break;
            case 6: //UNPROCESSED
                // FIXME should use >=
                if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
                    mappedSource = 0x05; //SL_ANDROID_RECORDING_PRESET_UNPROCESSED;
                } else {
                    mappedSource = 0x00; //SL_ANDROID_RECORDING_PRESET_NONE
                }
                break;
            }
        }

        return mappedSource;
    }


    String getMicSourceString(int source) {
        String name = null;
        String[] myArray = getResources().getStringArray(R.array.mic_source_array);

        if (myArray != null && source >= 0 && source < myArray.length) {
            name = myArray[source];
        }
        return name;
    }

    void setMicSource(int micSource) { mMicSource = micSource; }

    int mapPerformanceMode(int performanceMode) {
        int mappedPerformanceMode = -1;

        // FIXME taken from OpenSLES_AndroidConfiguration.h
        switch (performanceMode) {
        case 0: // NONE
        case 1: // LATENCY
        case 2: // LATENCY_EFFECTS
        case 3: // POWER_SAVING
            // FIXME should use >=
            if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
                mappedPerformanceMode = performanceMode;
                break;
            }
            // fall through
        case -1:
        default:
            mappedPerformanceMode = -1;
            break;
            }

        return mappedPerformanceMode;
    }


    int getPerformanceMode() {
        return mPerformanceMode;
    }

    String getPerformanceModeString(int performanceMode) {
        String name = null;
        String[] myArray = getResources().getStringArray(R.array.performance_mode_array);

        if (myArray != null && performanceMode >= -1 && performanceMode < myArray.length - 1) {
            name = myArray[performanceMode + 1];
        }
        return name;
    }


    void setPerformanceMode(int performanceMode) { mPerformanceMode = performanceMode; }

    int getIgnoreFirstFrames() {
        return mIgnoreFirstFrames;
    }

    void setIgnoreFirstFrames(int ignoreFirstFrames) {
        mIgnoreFirstFrames = ignoreFirstFrames;
    }

    int getPlayerBufferSizeInBytes() {
        return mPlayerBufferSizeInBytes;
    }


    void setPlayerBufferSizeInBytes(int playerBufferSizeInBytes) {
        mPlayerBufferSizeInBytes = clamp(playerBufferSizeInBytes, Constant.PLAYER_BUFFER_FRAMES_MIN,
                Constant.PLAYER_BUFFER_FRAMES_MAX);
    }


    int getRecorderBufferSizeInBytes() {
        return mRecorderBuffSizeInBytes;
    }


    void setRecorderBufferSizeInBytes(int recorderBufferSizeInBytes) {
        mRecorderBuffSizeInBytes = clamp(recorderBufferSizeInBytes,
                Constant.RECORDER_BUFFER_FRAMES_MIN, Constant.RECORDER_BUFFER_FRAMES_MAX);
    }


    int getBufferTestDuration() {
        return mBufferTestDurationInSeconds;
    }


    void setBufferTestDuration(int bufferTestDurationInSeconds) {
        mBufferTestDurationInSeconds = clamp(bufferTestDurationInSeconds,
                Constant.BUFFER_TEST_DURATION_SECONDS_MIN,
                Constant.BUFFER_TEST_DURATION_SECONDS_MAX);
    }


    int getBufferTestWavePlotDuration() {
        return mBufferTestWavePlotDurationInSeconds;
    }


    void setBufferTestWavePlotDuration(int bufferTestWavePlotDurationInSeconds) {
        mBufferTestWavePlotDurationInSeconds = clamp(bufferTestWavePlotDurationInSeconds,
                Constant.BUFFER_TEST_WAVE_PLOT_DURATION_SECONDS_MIN,
                Constant.BUFFER_TEST_WAVE_PLOT_DURATION_SECONDS_MAX);
    }

    int getNumberOfLoadThreads() {
        return mNumberOfLoadThreads;
    }

    void setNumberOfLoadThreads(int numberOfLoadThreads) {
        mNumberOfLoadThreads = clamp(numberOfLoadThreads, Constant.MIN_NUM_LOAD_THREADS,
                Constant.MAX_NUM_LOAD_THREADS);
    }

    public void setNumberOfCaptures (int num){
        mNumStateCaptures = clamp(num, Constant.MIN_NUM_CAPTURES, Constant.MAX_NUM_CAPTURES);
    }

    public void setCaptureSysTraceEnabled (boolean enabled){
        mCaptureSysTraceEnabled = enabled;
    }

    public void setCaptureBugreportEnabled(boolean enabled) {
        mCaptureBugreportEnabled = enabled;
    }

    public void setCaptureWavsEnabled (boolean enabled){
        mCaptureWavSnippetsEnabled = enabled;
    }

    public void setSoundLevelCalibrationEnabled(boolean enabled) {
        mSoundLevelCalibrationEnabled = enabled;
    }

    public boolean isCaptureEnabled() {
        return isCaptureSysTraceEnabled() || isCaptureBugreportEnabled();
    }

    public boolean isCaptureSysTraceEnabled() {
        return mCaptureSysTraceEnabled;
    }

    public boolean isCaptureBugreportEnabled() {
        return mCaptureBugreportEnabled;
    }

    public boolean isSoundLevelCalibrationEnabled() {
        return mSoundLevelCalibrationEnabled;
    }

    public int getNumStateCaptures() {
        return mNumStateCaptures;
    }

    public boolean isCaptureWavSnippetsEnabled() {
        return mCaptureWavSnippetsEnabled;
    }

    /**
     * Returns value if value is within inclusive bounds min through max
     * otherwise returns min or max according to if value is less than or greater than the range
     */
    private int clamp(int value, int min, int max) {

        if (max < min) throw new UnsupportedOperationException("min must be <= max");

        if (value < min) return min;
        else if (value > max) return max;
        else return value;
    }


    /** Compute Default audio settings. */
    public void computeDefaults() {
        int samplingRate = AudioTrack.getNativeOutputSampleRate(AudioManager.STREAM_MUSIC);
        setSamplingRate(samplingRate);

        if (mAudioThreadType == Constant.AUDIO_THREAD_TYPE_NATIVE) {

            int minBufferSizeInFrames;
            if (isSafeToUseGetProperty()) {
                AudioManager am = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
                String value = am.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
                minBufferSizeInFrames = Integer.parseInt(value);
            } else {
                minBufferSizeInFrames = 1024;
                log("On button test micSource Name: ");
            }
            int minBufferSizeInBytes = Constant.BYTES_PER_FRAME * minBufferSizeInFrames;

            setPlayerBufferSizeInBytes(minBufferSizeInBytes);
            setRecorderBufferSizeInBytes(minBufferSizeInBytes);
        } else {
            int minPlayerBufferSizeInBytes = AudioTrack.getMinBufferSize(samplingRate,
                    AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
            setPlayerBufferSizeInBytes(minPlayerBufferSizeInBytes);

            int minRecorderBufferSizeInBytes =  AudioRecord.getMinBufferSize(samplingRate,
                    AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
            setRecorderBufferSizeInBytes(minRecorderBufferSizeInBytes);
        }

    }


    String getSystemInfo() {
        String info = null;
        try {
            int versionCode = getApplicationContext().getPackageManager().getPackageInfo(
                              getApplicationContext().getPackageName(), 0).versionCode;
            String versionName = getApplicationContext().getPackageManager().getPackageInfo(
                                 getApplicationContext().getPackageName(), 0).versionName;
            info = "App ver. " + versionCode + "." + versionName + " | " + Build.MODEL + " | " +
                    Build.FINGERPRINT;
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }

        return info;
    }


    /** Check if it's safe to use Open SLES. */
    boolean isSafeToUseSles() {
        return  Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD;
    }


    /** Check if it's safe to use getProperty(). */
    boolean isSafeToUseGetProperty() {
        return  Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1;
    }


    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }


    @Override
    public void onCreate() {
        super.onCreate();

        setDefaults();
    }


    @Override
    public void onLowMemory() {
        super.onLowMemory();
    }


    @Override
    public void onTerminate() {
        super.onTerminate();
    }


    private static void log(String msg) {
        Log.v(TAG, msg);
    }
}
