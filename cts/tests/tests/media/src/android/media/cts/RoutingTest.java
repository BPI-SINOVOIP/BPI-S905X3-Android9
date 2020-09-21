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

package android.media.cts;

import android.content.Context;
import android.content.pm.PackageManager;

import android.media.AudioAttributes;
import android.media.AudioDeviceInfo;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioRouting;
import android.media.AudioTrack;
import android.media.MediaPlayer;
import android.media.MediaFormat;
import android.media.MediaRecorder;

import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;

import android.platform.test.annotations.AppModeFull;
import android.test.AndroidTestCase;

import android.util.Log;

import com.android.compatibility.common.util.MediaUtils;

import java.io.File;
import java.lang.Runnable;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * AudioTrack / AudioRecord / MediaPlayer / MediaRecorder preferred device
 * and routing listener tests.
 * The routing tests are mostly here to exercise the routing code, as an actual test would require
 * adding / removing an audio device for the listeners to be called.
 * The routing listener code is designed to run for two versions of the routing code:
 *  - the deprecated AudioTrack.OnRoutingChangedListener and AudioRecord.OnRoutingChangedListener
 *  - the N AudioRouting.OnRoutingChangedListener
 */
@AppModeFull(reason = "TODO: evaluate and port to instant")
public class RoutingTest extends AndroidTestCase {
    private static final String TAG = "RoutingTest";
    private static final int MAX_WAITING_ROUTING_CHANGED_COUNT = 3;
    private static final long WAIT_ROUTING_CHANGE_TIME_MS = 1000;
    private static final int AUDIO_BIT_RATE_IN_BPS = 12200;
    private static final int AUDIO_SAMPLE_RATE_HZ = 8000;
    private static final long MAX_FILE_SIZE_BYTE = 5000;
    private static final int RECORD_TIME_MS = 3000;
    private static final Set<Integer> AVAILABLE_INPUT_DEVICES_TYPE = new HashSet<>(
        Arrays.asList(AudioDeviceInfo.TYPE_BUILTIN_MIC));

    private AudioManager mAudioManager;
    private CountDownLatch mRoutingChangedLatch;
    private File mOutFile;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // get the AudioManager
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        assertNotNull(mAudioManager);
    }

    @Override
    protected void tearDown() throws Exception {
        if (mOutFile != null && mOutFile.exists()) {
            mOutFile.delete();
        }
        super.tearDown();
    }

    private AudioTrack allocAudioTrack() {
        int bufferSize =
                AudioTrack.getMinBufferSize(
                    41000,
                    AudioFormat.CHANNEL_OUT_STEREO,
                    AudioFormat.ENCODING_PCM_16BIT);
        AudioTrack audioTrack =
            new AudioTrack(
                AudioManager.STREAM_MUSIC,
                41000,
                AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize,
                AudioTrack.MODE_STREAM);
        return audioTrack;
    }

    public void test_audioTrack_preferredDevice() {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT)) {
            // Can't do it so skip this test
            return;
        }

        AudioTrack audioTrack = allocAudioTrack();
        assertNotNull(audioTrack);

        // None selected (new AudioTrack), so check for default
        assertNull(audioTrack.getPreferredDevice());

        // resets to default
        assertTrue(audioTrack.setPreferredDevice(null));

        // test each device
        AudioDeviceInfo[] deviceList = mAudioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
        for (int index = 0; index < deviceList.length; index++) {
            assertTrue(audioTrack.setPreferredDevice(deviceList[index]));
            assertTrue(audioTrack.getPreferredDevice() == deviceList[index]);
        }

        // Check defaults again
        assertTrue(audioTrack.setPreferredDevice(null));
        assertNull(audioTrack.getPreferredDevice());

        audioTrack.release();
    }

    public void test_audioTrack_incallMusicRoutingPermissions() {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT)) {
            // Can't do it so skip this test
            return;
        }

        // only apps with MODIFY_PHONE_STATE permission can route playback
        // to the uplink stream during a phone call, so this test makes sure that
        // audio is re-routed to default device when the permission is missing

        AudioDeviceInfo telephonyDevice = getTelephonyDeviceAndSetInCommunicationMode();
        if (telephonyDevice == null) {
            // Can't do it so skip this test
            return;
        }

        AudioTrack audioTrack = null;

        try {
            audioTrack = allocAudioTrack();
            assertNotNull(audioTrack);

            audioTrack.setPreferredDevice(telephonyDevice);
            assertEquals(AudioDeviceInfo.TYPE_TELEPHONY, audioTrack.getPreferredDevice().getType());

            audioTrack.play();
            assertTrue(audioTrack.getRoutedDevice().getType() != AudioDeviceInfo.TYPE_TELEPHONY);

        } finally {
            if (audioTrack != null) {
                audioTrack.stop();
                audioTrack.release();
            }
            mAudioManager.setMode(AudioManager.MODE_NORMAL);
        }
    }

    private AudioDeviceInfo getTelephonyDeviceAndSetInCommunicationMode() {
        // get the output device for telephony
        AudioDeviceInfo telephonyDevice = null;
        AudioDeviceInfo[] deviceList = mAudioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
        for (int index = 0; index < deviceList.length; index++) {
            if (deviceList[index].getType() == AudioDeviceInfo.TYPE_TELEPHONY) {
                telephonyDevice = deviceList[index];
            }
        }

        if (telephonyDevice == null) {
            return null;
        }

        // simulate an in call state using MODE_IN_COMMUNICATION since
        // AudioManager.setMode requires MODIFY_PHONE_STATE permission
        // for setMode with MODE_IN_CALL.
        mAudioManager.setMode(AudioManager.MODE_IN_COMMUNICATION);
        assertEquals(AudioManager.MODE_IN_COMMUNICATION, mAudioManager.getMode());

        return telephonyDevice;
    }

    /*
     * tests if the Looper for the current thread has been prepared,
     * If not, it makes one, prepares it and returns it.
     * If this returns non-null, the caller is reponsible for calling quit()
     * on the returned Looper.
     */
    private Looper prepareIfNeededLooper() {
        // non-null Handler
        Looper myLooper = null;
        if (Looper.myLooper() == null) {
            Looper.prepare();
            myLooper = Looper.myLooper();
            assertNotNull(myLooper);
        }
        return myLooper;
    }

    private class AudioTrackRoutingListener implements AudioTrack.OnRoutingChangedListener,
            AudioRouting.OnRoutingChangedListener
    {
        public void onRoutingChanged(AudioTrack audioTrack) {}
        public void onRoutingChanged(AudioRouting audioRouting) {}
    }


    public void test_audioTrack_RoutingListener() {
        test_audioTrack_RoutingListener(false /*usesAudioRouting*/);
    }

    public void test_audioTrack_audioRouting_RoutingListener() {
        test_audioTrack_RoutingListener(true /*usesAudioRouting*/);
    }

    private void test_audioTrack_RoutingListener(boolean usesAudioRouting) {
        AudioTrack audioTrack = allocAudioTrack();

        // null listener
        if (usesAudioRouting) {
            audioTrack.addOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) null, null);
        } else {
            audioTrack.addOnRoutingChangedListener(
                    (AudioTrack.OnRoutingChangedListener) null, null);
        }

        AudioTrackRoutingListener listener = new AudioTrackRoutingListener();
        AudioTrackRoutingListener someOtherListener = new AudioTrackRoutingListener();

        // add a listener
        if (usesAudioRouting) {
            audioTrack.addOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) listener, null);
        } else {
            audioTrack.addOnRoutingChangedListener(listener, null);
        }

        // remove listeners
        if (usesAudioRouting) {
            // remove a listener we didn't add
            audioTrack.removeOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) someOtherListener);
            // remove a valid listener
            audioTrack.removeOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) listener);
        } else {
            // remove a listener we didn't add
            audioTrack.removeOnRoutingChangedListener(
                    (AudioTrack.OnRoutingChangedListener) someOtherListener);
            // remove a valid listener
            audioTrack.removeOnRoutingChangedListener(
                    (AudioTrack.OnRoutingChangedListener) listener);
        }

        Looper myLooper = prepareIfNeededLooper();

        if (usesAudioRouting) {
            audioTrack.addOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) listener, new Handler());
            audioTrack.removeOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) listener);
        } else {
            audioTrack.addOnRoutingChangedListener(
                    (AudioTrack.OnRoutingChangedListener) listener, new Handler());
            audioTrack.removeOnRoutingChangedListener(
                    (AudioTrack.OnRoutingChangedListener) listener);
        }

        audioTrack.release();
        if (myLooper != null) {
            myLooper.quit();
        }
   }

    private AudioRecord allocAudioRecord() {
        int bufferSize =
                AudioRecord.getMinBufferSize(
                    41000,
                    AudioFormat.CHANNEL_OUT_DEFAULT,
                    AudioFormat.ENCODING_PCM_16BIT);
        AudioRecord audioRecord =
            new AudioRecord(
                MediaRecorder.AudioSource.DEFAULT,
                41000, AudioFormat.CHANNEL_OUT_DEFAULT,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize);
        return audioRecord;
    }

    private class AudioRecordRoutingListener implements AudioRecord.OnRoutingChangedListener,
            AudioRouting.OnRoutingChangedListener
    {
        public void onRoutingChanged(AudioRecord audioRecord) {}
        public void onRoutingChanged(AudioRouting audioRouting) {}
    }

    public void test_audioRecord_RoutingListener() {
        test_audioRecord_RoutingListener(false /*usesAudioRouting*/);
    }

    public void test_audioRecord_audioRouting_RoutingListener() {
        test_audioRecord_RoutingListener(true /*usesAudioRouting*/);
    }

    private void test_audioRecord_RoutingListener(boolean usesAudioRouting) {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MICROPHONE)) {
            // Can't do it so skip this test
            return;
        }
        AudioRecord audioRecord = allocAudioRecord();

        // null listener
        if (usesAudioRouting) {
            audioRecord.addOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) null, null);
        } else {
            audioRecord.addOnRoutingChangedListener(
                    (AudioRecord.OnRoutingChangedListener) null, null);
        }

        AudioRecordRoutingListener listener = new AudioRecordRoutingListener();
        AudioRecordRoutingListener someOtherListener = new AudioRecordRoutingListener();

        // add a listener
        if (usesAudioRouting) {
            audioRecord.addOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) listener, null);
        } else {
            audioRecord.addOnRoutingChangedListener(
                    (AudioRecord.OnRoutingChangedListener) listener, null);
        }

        // remove listeners
        if (usesAudioRouting) {
            // remove a listener we didn't add
            audioRecord.removeOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) someOtherListener);
            // remove a valid listener
            audioRecord.removeOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) listener);
        } else {
            // remove a listener we didn't add
            audioRecord.removeOnRoutingChangedListener(
                    (AudioRecord.OnRoutingChangedListener) someOtherListener);
            // remove a valid listener
            audioRecord.removeOnRoutingChangedListener(
                    (AudioRecord.OnRoutingChangedListener) listener);
        }

        Looper myLooper = prepareIfNeededLooper();
        if (usesAudioRouting) {
            audioRecord.addOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) listener, new Handler());
            audioRecord.removeOnRoutingChangedListener(
                    (AudioRouting.OnRoutingChangedListener) listener);
        } else {
            audioRecord.addOnRoutingChangedListener(
                    (AudioRecord.OnRoutingChangedListener) listener, new Handler());
            audioRecord.removeOnRoutingChangedListener(
                    (AudioRecord.OnRoutingChangedListener) listener);
        }

        audioRecord.release();
        if (myLooper != null) {
            myLooper.quit();
        }
    }

    public void test_audioRecord_preferredDevice() {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MICROPHONE)) {
            // Can't do it so skip this test
            return;
        }

        AudioRecord audioRecord = allocAudioRecord();
        assertNotNull(audioRecord);

        // None selected (new AudioRecord), so check for default
        assertNull(audioRecord.getPreferredDevice());

        // resets to default
        assertTrue(audioRecord.setPreferredDevice(null));

        // test each device
        AudioDeviceInfo[] deviceList = mAudioManager.getDevices(AudioManager.GET_DEVICES_INPUTS);
        for (int index = 0; index < deviceList.length; index++) {
            assertTrue(audioRecord.setPreferredDevice(deviceList[index]));
            assertTrue(audioRecord.getPreferredDevice() == deviceList[index]);
        }

        // Check defaults again
        assertTrue(audioRecord.setPreferredDevice(null));
        assertNull(audioRecord.getPreferredDevice());

        audioRecord.release();
    }

    private class AudioTrackFiller implements Runnable {
        AudioTrack mAudioTrack;
        int mBufferSize;

        boolean mPlaying;

        short[] mAudioData;

        public AudioTrackFiller(AudioTrack audioTrack, int bufferSize) {
            mAudioTrack = audioTrack;
            mBufferSize = bufferSize;
            mPlaying = false;

            // setup audio data (silence will suffice)
            mAudioData = new short[mBufferSize];
            for (int index = 0; index < mBufferSize; index++) {
                mAudioData[index] = 0;
            }
        }

        public void start() { mPlaying = true; }
        public void stop() { mPlaying = false; }

        @Override
        public void run() {
            while (mAudioTrack != null && mPlaying) {
                mAudioTrack.write(mAudioData, 0, mBufferSize);
            }
        }
    }

    public void test_audioTrack_getRoutedDevice() {
        if (!DeviceUtils.hasOutputDevice(mAudioManager)) {
            Log.i(TAG, "No output devices. Test skipped");
            return; // nothing to test here
        }

        int bufferSize =
                AudioTrack.getMinBufferSize(
                    41000,
                    AudioFormat.CHANNEL_OUT_STEREO,
                    AudioFormat.ENCODING_PCM_16BIT);
        AudioTrack audioTrack =
            new AudioTrack(
                AudioManager.STREAM_MUSIC,
                41000,
                AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize,
                AudioTrack.MODE_STREAM);

        AudioTrackFiller filler = new AudioTrackFiller(audioTrack, bufferSize);
        filler.start();

        audioTrack.play();

        Thread fillerThread = new Thread(filler);
        fillerThread.start();

        try { Thread.sleep(1000); } catch (InterruptedException ex) {}

        // No explicit route
        AudioDeviceInfo routedDevice = audioTrack.getRoutedDevice();
        assertNotNull(routedDevice); // we probably can't say anything more than this

        filler.stop();
        audioTrack.stop();
        audioTrack.release();
    }

    private class AudioRecordPuller implements Runnable {
        AudioRecord mAudioRecord;
        int mBufferSize;

        boolean mRecording;

        short[] mAudioData;

        public AudioRecordPuller(AudioRecord audioRecord, int bufferSize) {
            mAudioRecord = audioRecord;
            mBufferSize = bufferSize;
            mRecording = false;
        }

        public void start() { mRecording = true; }
        public void stop() { mRecording = false; }

        @Override
        public void run() {
            while (mAudioRecord != null && mRecording) {
                mAudioRecord.read(mAudioData, 0, mBufferSize);
           }
        }
    }

    public void test_audioRecord_getRoutedDevice() {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MICROPHONE)) {
            return;
        }

        if (!DeviceUtils.hasInputDevice(mAudioManager)) {
            Log.i(TAG, "No input devices. Test skipped");
            return; // nothing to test here
        }

        int bufferSize =
                AudioRecord.getMinBufferSize(
                    41000,
                    AudioFormat.CHANNEL_OUT_DEFAULT,
                    AudioFormat.ENCODING_PCM_16BIT);
        AudioRecord audioRecord =
            new AudioRecord(
                MediaRecorder.AudioSource.DEFAULT,
                41000, AudioFormat.CHANNEL_OUT_DEFAULT,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize);

        AudioRecordPuller puller = new AudioRecordPuller(audioRecord, bufferSize);
        puller.start();

        audioRecord.startRecording();

        Thread pullerThread = new Thread(puller);
        pullerThread.start();

        try { Thread.sleep(1000); } catch (InterruptedException ex) {}

        // No explicit route
        AudioDeviceInfo routedDevice = audioRecord.getRoutedDevice();
        assertNotNull(routedDevice); // we probably can't say anything more than this

        puller.stop();
        audioRecord.stop();
        audioRecord.release();
    }

    private class AudioRoutingListener implements AudioRouting.OnRoutingChangedListener
    {
        public void onRoutingChanged(AudioRouting audioRouting) {
            if (mRoutingChangedLatch != null) {
                mRoutingChangedLatch.countDown();
            }
        }
    }

    private MediaPlayer allocMediaPlayer() {
        final int resid = R.raw.testmp3_2;
        MediaPlayer mediaPlayer = MediaPlayer.create(mContext, resid);
        mediaPlayer.setAudioAttributes(
            new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).build());
        mediaPlayer.start();
        return mediaPlayer;
    }

    public void test_mediaPlayer_preferredDevice() {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT)) {
            // Can't do it so skip this test
            return;
        }

        MediaPlayer mediaPlayer = allocMediaPlayer();
        assertTrue(mediaPlayer.isPlaying());

        // None selected (new MediaPlayer), so check for default
        assertNull(mediaPlayer.getPreferredDevice());

        // resets to default
        assertTrue(mediaPlayer.setPreferredDevice(null));

        // test each device
        AudioDeviceInfo[] deviceList = mAudioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
        for (int index = 0; index < deviceList.length; index++) {
            assertTrue(mediaPlayer.setPreferredDevice(deviceList[index]));
            assertTrue(mediaPlayer.getPreferredDevice() == deviceList[index]);
        }

        // Check defaults again
        assertTrue(mediaPlayer.setPreferredDevice(null));
        assertNull(mediaPlayer.getPreferredDevice());

        mediaPlayer.stop();
        mediaPlayer.release();
    }

    public void test_mediaPlayer_getRoutedDevice() {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT)) {
            // Can't do it so skip this test
            return;
        }

        MediaPlayer mediaPlayer = allocMediaPlayer();
        assertTrue(mediaPlayer.isPlaying());

        // Sleep for 1s to ensure the output device open
        SystemClock.sleep(1000);

        // No explicit route
        AudioDeviceInfo routedDevice = mediaPlayer.getRoutedDevice();
        assertNotNull(routedDevice);

        mediaPlayer.stop();
        mediaPlayer.release();
    }

    public void test_MediaPlayer_RoutingListener() {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT)) {
            // Can't do it so skip this test
            return;
        }

        MediaPlayer mediaPlayer = allocMediaPlayer();

        // null listener
        mediaPlayer.addOnRoutingChangedListener(null, null);

        AudioRoutingListener listener = new AudioRoutingListener();
        AudioRoutingListener someOtherListener = new AudioRoutingListener();

        // add a listener
        mediaPlayer.addOnRoutingChangedListener(listener, null);

        // remove listeners
        // remove a listener we didn't add
        mediaPlayer.removeOnRoutingChangedListener(someOtherListener);
        // remove a valid listener
        mediaPlayer.removeOnRoutingChangedListener(listener);

        Looper myLooper = prepareIfNeededLooper();

        mediaPlayer.addOnRoutingChangedListener(listener, new Handler());
        mediaPlayer.removeOnRoutingChangedListener(listener);

        mediaPlayer.stop();
        mediaPlayer.release();
        if (myLooper != null) {
            myLooper.quit();
        }
    }

    public void test_MediaPlayer_RoutingChangedCallback() {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT)) {
            // Can't do it so skip this test
            return;
        }

        MediaPlayer mediaPlayer = allocMediaPlayer();
        AudioRoutingListener listener = new AudioRoutingListener();
        mediaPlayer.addOnRoutingChangedListener(listener, null);

        AudioDeviceInfo[] deviceList = mAudioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
        if (deviceList.length < 2) {
            // The available output device is less than 2, we can't switch output device.
            return;
        }
        for (int index = 0; index < deviceList.length; index++) {
            assertTrue(mediaPlayer.setPreferredDevice(deviceList[index]));
            boolean routingChanged = false;
            for (int i = 0; i < MAX_WAITING_ROUTING_CHANGED_COUNT; i++) {
                // Create a new CountDownLatch in case it is triggered by previous routing change.
                mRoutingChangedLatch = new CountDownLatch(1);
                try {
                    mRoutingChangedLatch.await(WAIT_ROUTING_CHANGE_TIME_MS, TimeUnit.MILLISECONDS);
                } catch (InterruptedException e) {
                }
                AudioDeviceInfo routedDevice = mediaPlayer.getRoutedDevice();
                if (routedDevice == null) {
                    continue;
                }
                if (routedDevice.getId() == deviceList[index].getId()) {
                    routingChanged = true;
                    break;
                }
            }
            assertTrue("Switching to device" + deviceList[index].getType() + " failed",
                    routingChanged);
        }

        mediaPlayer.removeOnRoutingChangedListener(listener);
        mediaPlayer.stop();
        mediaPlayer.release();
    }

    public void test_mediaPlayer_incallMusicRoutingPermissions() {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT)) {
            // Can't do it so skip this test
            return;
        }

        // only apps with MODIFY_PHONE_STATE permission can route playback
        // to the uplink stream during a phone call, so this test makes sure that
        // audio is re-routed to default device when the permission is missing

        AudioDeviceInfo telephonyDevice = getTelephonyDeviceAndSetInCommunicationMode();
        if (telephonyDevice == null) {
            // Can't do it so skip this test
            return;
        }

        MediaPlayer mediaPlayer = null;

        try {
            mediaPlayer = allocMediaPlayer();

            mediaPlayer.setPreferredDevice(telephonyDevice);
            assertEquals(AudioDeviceInfo.TYPE_TELEPHONY, mediaPlayer.getPreferredDevice().getType());

            // Sleep for 1s to ensure the output device open
            SystemClock.sleep(1000);
            assertTrue(mediaPlayer.getRoutedDevice().getType() != AudioDeviceInfo.TYPE_TELEPHONY);

        } finally {
            if (mediaPlayer != null) {
                mediaPlayer.stop();
                mediaPlayer.release();
            }
            mAudioManager.setMode(AudioManager.MODE_NORMAL);
        }
    }

    private MediaRecorder allocMediaRecorder() throws Exception {
        final String outputPath = new File(Environment.getExternalStorageDirectory(),
            "record.out").getAbsolutePath();
        mOutFile = new File(outputPath);
        MediaRecorder mediaRecorder = new MediaRecorder();
        mediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        assertEquals(0, mediaRecorder.getMaxAmplitude());
        mediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
        mediaRecorder.setOutputFile(outputPath);
        mediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AAC);
        mediaRecorder.setAudioChannels(AudioFormat.CHANNEL_OUT_DEFAULT);
        mediaRecorder.setAudioSamplingRate(AUDIO_SAMPLE_RATE_HZ);
        mediaRecorder.setAudioEncodingBitRate(AUDIO_BIT_RATE_IN_BPS);
        mediaRecorder.setMaxFileSize(MAX_FILE_SIZE_BYTE);
        mediaRecorder.prepare();
        mediaRecorder.start();
        // Sleep a while to ensure the underlying AudioRecord is initialized.
        Thread.sleep(1000);
        return mediaRecorder;
    }

    public void test_mediaRecorder_preferredDevice() throws Exception {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MICROPHONE)
                || !MediaUtils.hasEncoder(MediaFormat.MIMETYPE_AUDIO_AAC)) {
            MediaUtils.skipTest("no audio codecs or microphone");
            return;
        }

        MediaRecorder mediaRecorder = allocMediaRecorder();

        // None selected (new MediaPlayer), so check for default
        assertNull(mediaRecorder.getPreferredDevice());

        // resets to default
        assertTrue(mediaRecorder.setPreferredDevice(null));

        // test each device
        AudioDeviceInfo[] deviceList = mAudioManager.getDevices(AudioManager.GET_DEVICES_INPUTS);
        for (int index = 0; index < deviceList.length; index++) {
            if (!AVAILABLE_INPUT_DEVICES_TYPE.contains(deviceList[index].getType())) {
                // Only try to set devices whose type is contained in predefined set as preferred
                // device in case of permission denied when switching input device.
                continue;
            }
            assertTrue(mediaRecorder.setPreferredDevice(deviceList[index]));
            assertTrue(mediaRecorder.getPreferredDevice() == deviceList[index]);
        }

        // Check defaults again
        assertTrue(mediaRecorder.setPreferredDevice(null));
        assertNull(mediaRecorder.getPreferredDevice());
        Thread.sleep(RECORD_TIME_MS);

        mediaRecorder.stop();
        mediaRecorder.release();
    }

    public void test_mediaRecorder_getRoutedDeviceId() throws Exception {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MICROPHONE)
            || !MediaUtils.hasEncoder(MediaFormat.MIMETYPE_AUDIO_AAC)) {
            MediaUtils.skipTest("no audio codecs or microphone");
            return;
        }

        MediaRecorder mediaRecorder = allocMediaRecorder();

        AudioDeviceInfo routedDevice = mediaRecorder.getRoutedDevice();
        assertNotNull(routedDevice); // we probably can't say anything more than this
        Thread.sleep(RECORD_TIME_MS);

        mediaRecorder.stop();
        mediaRecorder.release();
    }

    public void test_mediaRecorder_RoutingListener() throws Exception {
        if (!mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MICROPHONE)
            || !MediaUtils.hasEncoder(MediaFormat.MIMETYPE_AUDIO_AAC)) {
            MediaUtils.skipTest("no audio codecs or microphone");
            return;
        }

        MediaRecorder mediaRecorder = allocMediaRecorder();

        // null listener
        mediaRecorder.addOnRoutingChangedListener(null, null);

        AudioRoutingListener listener = new AudioRoutingListener();
        AudioRoutingListener someOtherListener = new AudioRoutingListener();

        // add a listener
        mediaRecorder.addOnRoutingChangedListener(listener, null);

        // remove listeners we didn't add
        mediaRecorder.removeOnRoutingChangedListener(someOtherListener);
        // remove a valid listener
        mediaRecorder.removeOnRoutingChangedListener(listener);

        Looper myLooper = prepareIfNeededLooper();
        mediaRecorder.addOnRoutingChangedListener(listener, new Handler());
        mediaRecorder.removeOnRoutingChangedListener(listener);

        Thread.sleep(RECORD_TIME_MS);

        mediaRecorder.stop();
        mediaRecorder.release();
        if (myLooper != null) {
            myLooper.quit();
        }
    }
}
