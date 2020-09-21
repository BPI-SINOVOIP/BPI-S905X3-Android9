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

package android.mediastress.cts;

import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.media.CamcorderProfile;
import android.media.MediaPlayer;
import android.media.MediaRecorder;
import android.os.Handler;
import android.os.Looper;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Log;
import android.view.SurfaceHolder;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.Writer;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

public class MediaRecorderStressTest extends ActivityInstrumentationTestCase2<MediaFrameworkTest> {

    private static final String TAG = "MediaRecorderStressTest";
    private static final int NUMBER_OF_CAMERA_STRESS_LOOPS = 50;
    private static final int NUMBER_OF_RECORDER_STRESS_LOOPS = 50;
    private static final int NUMBER_OF_RECORDERANDPLAY_STRESS_LOOPS = 25;
    private static final int NUMBER_OF_SWTICHING_LOOPS_BW_CAMERA_AND_RECORDER = 50;
    private static final long WAIT_TIME_CAMERA_TEST = 3000;  // in ms
    private static final long WAIT_TIME_RECORDER_TEST = 5000;  // in ms
    private final String OUTPUT_FILE = WorkDir.getTopDirString() + "temp";
    private static final String OUTPUT_FILE_EXT = ".3gp";
    private static final String MEDIA_STRESS_OUTPUT ="mediaStressOutput.txt";
    private final CameraErrorCallback mCameraErrorCallback = new CameraErrorCallback();
    private final RecorderErrorCallback mRecorderErrorCallback = new RecorderErrorCallback();
    private final static int WAIT_TIMEOUT = 10000;
    private static final int VIDEO_WIDTH = 176;
    private static final int VIDEO_HEIGHT = 144;

    private MediaRecorder mRecorder;
    private Camera mCamera;
    private Thread mLooperThread;
    private Handler mHandler;

    private static int mCameraId;
    private static int mProfileQuality = CamcorderProfile.QUALITY_HIGH;
    private static CamcorderProfile profile =
                        CamcorderProfile.get(mCameraId, mProfileQuality);

    private int mVideoEncoder;
    private int mAudioEncoder;
    private int mFrameRate;
    private int mVideoWidth;
    private int mVideoHeight;
    private int mBitRate;
    private boolean mRemoveVideo = true;
    private int mRecordDuration = 5000;

    private boolean mHasRearCamera = false;
    private boolean mHasFrontCamera = false;

    public MediaRecorderStressTest() {
        super(MediaFrameworkTest.class);
    }

    protected void setUp() throws Exception {
        PackageManager packageManager =
                getInstrumentation().getTargetContext().getPackageManager();
        mHasRearCamera = packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA);
        mHasFrontCamera = packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA_FRONT);
        int cameraId = 0;
        CamcorderProfile profile = CamcorderProfile.get(cameraId, CamcorderProfile.QUALITY_HIGH);
        mVideoEncoder = profile.videoCodec;
        mAudioEncoder = profile.audioCodec;
        mFrameRate = profile.videoFrameRate;
        mVideoWidth = profile.videoFrameWidth;
        mVideoHeight = profile.videoFrameHeight;
        mBitRate = profile.videoBitRate;

        final Semaphore sem = new Semaphore(0);
        mLooperThread = new Thread() {
            @Override
            public void run() {
                Log.v(TAG, "starting looper");
                Looper.prepare();
                mHandler = new Handler();
                sem.release();
                Looper.loop();
                Log.v(TAG, "quit looper");
            }
        };
        mLooperThread.start();
        if (!sem.tryAcquire(WAIT_TIMEOUT, TimeUnit.MILLISECONDS)) {
            fail("Failed to start the looper.");
        }

        getActivity();
        super.setUp();
    }

    @Override
    protected void tearDown() throws Exception {
        if (mHandler != null) {
            mHandler.getLooper().quit();
            mHandler = null;
        }
        if (mLooperThread != null) {
            mLooperThread.join(WAIT_TIMEOUT);
            if (mLooperThread.isAlive()) {
                fail("Failed to stop the looper.");
            }
            mLooperThread = null;
        }

        super.tearDown();
    }

    private void runOnLooper(final Runnable command) throws InterruptedException {
        final Semaphore sem = new Semaphore(0);
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    command.run();
                } finally {
                    sem.release();
                }
            }
        });
        if (! sem.tryAcquire(WAIT_TIMEOUT, TimeUnit.MILLISECONDS)) {
            fail("Failed to run the command on the looper.");
        }
    }

    private final class CameraErrorCallback implements android.hardware.Camera.ErrorCallback {
        public void onError(int error, android.hardware.Camera camera) {
            assertTrue("Camera test mediaserver died", error !=
                    android.hardware.Camera.CAMERA_ERROR_SERVER_DIED);
        }
    }

    private final class RecorderErrorCallback implements MediaRecorder.OnErrorListener {
        public void onError(MediaRecorder mr, int what, int extra) {
            // fail the test case no matter what error come up
            fail("mediaRecorder error");
        }
    }

    //Test case for stressing the camera preview.
    @LargeTest
    public void testStressCamera() throws Exception {
        if (Camera.getNumberOfCameras() < 1) {
            return;
        }

        SurfaceHolder mSurfaceHolder;
        mSurfaceHolder = MediaFrameworkTest.getSurfaceView().getHolder();
        File stressOutFile = new File(WorkDir.getTopDir(), MEDIA_STRESS_OUTPUT);
        Writer output = new BufferedWriter(new FileWriter(stressOutFile, true));
        output.write("Camera start preview stress:\n");
        output.write("Total number of loops:" +
                NUMBER_OF_CAMERA_STRESS_LOOPS + "\n");

        Log.v(TAG, "Start preview");
        output.write("No of loop: ");

        for (int i = 0; i< NUMBER_OF_CAMERA_STRESS_LOOPS; i++) {
            runOnLooper(new Runnable() {
                @Override
                public void run() {
                    if (mHasRearCamera) {
                        mCamera = Camera.open();
                    } else if (mHasFrontCamera) {
                        mCamera = Camera.open(0);
                    } else {
                        mCamera = null;
                    }
                }
            });
            if (mCamera == null) {
                break;
            }
            mCamera.setErrorCallback(mCameraErrorCallback);
            mCamera.setPreviewDisplay(mSurfaceHolder);
            mCamera.startPreview();
            Thread.sleep(WAIT_TIME_CAMERA_TEST);
            mCamera.stopPreview();
            mCamera.release();
            output.write(" ," + i);
        }

        output.write("\n\n");
        output.close();
    }

    //Test case for stressing the camera preview.
    @LargeTest
    public void testStressRecorder() throws Exception {
        String filename;
        SurfaceHolder mSurfaceHolder;
        mSurfaceHolder = MediaFrameworkTest.getSurfaceView().getHolder();
        File stressOutFile = new File(WorkDir.getTopDir(), MEDIA_STRESS_OUTPUT);
        Writer output = new BufferedWriter(new FileWriter(stressOutFile, true));
        int width;
        int height;
        Camera camera = null;

        if (!mHasRearCamera && !mHasFrontCamera) {
                output.write("No camera found. Skipping recorder stress test\n");
                return;
        }
        // Try to get camera smallest supported resolution.
        // If we fail for any reason, set the video size to default value.
        try {
            camera = Camera.open(0);
            List<Camera.Size> previewSizes = camera.getParameters().getSupportedPreviewSizes();
            width = previewSizes.get(0).width;
            height = previewSizes.get(0).height;
            for (Camera.Size size : previewSizes) {
                if (size.width < width || size.height < height) {
                    width = size.width;
                    height = size.height;
                }
            }
        } catch (Exception e) {
            width = VIDEO_WIDTH;
            height = VIDEO_HEIGHT;
        }
        if (camera != null) {
            camera.release();
        }
        Log.v(TAG, String.format("Camera video size used for test %dx%d", width, height));
        output.write("H263 video record- reset after prepare Stress test\n");
        output.write("Total number of loops:" +
                NUMBER_OF_RECORDER_STRESS_LOOPS + "\n");

        output.write("No of loop: ");
        Log.v(TAG, "Start preview");
        for (int i = 0; i < NUMBER_OF_RECORDER_STRESS_LOOPS; i++) {
            runOnLooper(new Runnable() {
                @Override
                public void run() {
                    mRecorder = new MediaRecorder();
                }
            });
            Log.v(TAG, "counter = " + i);
            filename = OUTPUT_FILE + i + OUTPUT_FILE_EXT;
            Log.v(TAG, filename);
            mRecorder.setOnErrorListener(mRecorderErrorCallback);
            mRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
            mRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
            mRecorder.setOutputFile(filename);
            mRecorder.setVideoFrameRate(mFrameRate);
            mRecorder.setVideoSize(width, height);
            Log.v(TAG, "setEncoder");
            mRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H263);
            mSurfaceHolder = MediaFrameworkTest.getSurfaceView().getHolder();
            Log.v(TAG, "setPreview");
            mRecorder.setPreviewDisplay(mSurfaceHolder.getSurface());
            Log.v(TAG, "prepare");
            mRecorder.prepare();
            Log.v(TAG, "before release");
            Thread.sleep(WAIT_TIME_RECORDER_TEST);
            mRecorder.reset();
            mRecorder.release();
            output.write(", " + i);
        }

        output.write("\n\n");
        output.close();
    }

    //Stress test case for switching camera and video recorder preview.
    @LargeTest
    public void testStressCameraSwitchRecorder() throws Exception {
        if (Camera.getNumberOfCameras() < 1) {
            return;
        }

        String filename;
        int width;
        int height;
        SurfaceHolder mSurfaceHolder;
        mSurfaceHolder = MediaFrameworkTest.getSurfaceView().getHolder();
        File stressOutFile = new File(WorkDir.getTopDir(), MEDIA_STRESS_OUTPUT);
        Writer output = new BufferedWriter(new FileWriter(stressOutFile, true));
        output.write("Camera and video recorder preview switching\n");
        output.write("Total number of loops:"
                + NUMBER_OF_SWTICHING_LOOPS_BW_CAMERA_AND_RECORDER + "\n");

        Log.v(TAG, "Start preview");
        output.write("No of loop: ");
        for (int i = 0; i < NUMBER_OF_SWTICHING_LOOPS_BW_CAMERA_AND_RECORDER; i++) {
            runOnLooper(new Runnable() {
                @Override
                public void run() {
                    if (mHasRearCamera) {
                        mCamera = Camera.open();
                    } else if (mHasFrontCamera) {
                        mCamera = Camera.open(0);
                    } else {
                        mCamera = null;
                    }
                }
            });
            if (mCamera == null) {
                break;
            }
            // Try to get camera smallest supported resolution.
            // If we fail for any reason, set the video size to default value.
            List<Camera.Size> previewSizes = mCamera.getParameters().getSupportedPreviewSizes();
            width = previewSizes.get(0).width;
            height = previewSizes.get(0).height;
            for (Camera.Size size : previewSizes) {
                if (size.width < width || size.height < height) {
                    width = size.width;
                    height = size.height;
                }
            }
            Log.v(TAG, String.format("Camera video size used for test %dx%d", width, height));

            mCamera.setErrorCallback(mCameraErrorCallback);
            mCamera.setPreviewDisplay(mSurfaceHolder);
            mCamera.startPreview();
            Thread.sleep(WAIT_TIME_CAMERA_TEST);
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
            Log.v(TAG, "release camera");
            filename = OUTPUT_FILE + i + OUTPUT_FILE_EXT;
            Log.v(TAG, filename);
            runOnLooper(new Runnable() {
                @Override
                public void run() {
                    mRecorder = new MediaRecorder();
                }
            });
            mRecorder.setOnErrorListener(mRecorderErrorCallback);
            mRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
            mRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
            mRecorder.setOutputFile(filename);
            mRecorder.setVideoFrameRate(mFrameRate);
            mRecorder.setVideoSize(width, height);
            Log.v(TAG, "Media recorder setEncoder");
            mRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H263);
            Log.v(TAG, "mediaRecorder setPreview");
            mRecorder.setPreviewDisplay(mSurfaceHolder.getSurface());
            Log.v(TAG, "prepare");
            mRecorder.prepare();
            Log.v(TAG, "before release");
            Thread.sleep(WAIT_TIME_CAMERA_TEST);
            mRecorder.reset();
            mRecorder.release();
            Log.v(TAG, "release video recorder");
            output.write(", " + i);
        }

        output.write("\n\n");
        output.close();
    }

    public void validateRecordedVideo(String recordedFile) throws Exception {
        MediaPlayer mp = new MediaPlayer();
        mp.setDataSource(recordedFile);
        mp.prepare();
        int duration = mp.getDuration();
        if (duration <= 0){
            assertTrue("stressRecordAndPlayback", false);
        }
        mp.release();
    }

    public void removeRecodedVideo(String filename){
        File video = new File(filename);
        Log.v(TAG, "remove recorded video " + filename);
        video.delete();
    }

    //Stress test case for record a video and play right away.
    @LargeTest
    public void testStressRecordVideoAndPlayback() throws Exception {
        if (Camera.getNumberOfCameras() < 1) {
            return;
        }

        String filename;
        SurfaceHolder mSurfaceHolder;
        mSurfaceHolder = MediaFrameworkTest.getSurfaceView().getHolder();
        File stressOutFile = new File(WorkDir.getTopDir(), MEDIA_STRESS_OUTPUT);
        Writer output = new BufferedWriter(
                new FileWriter(stressOutFile, true));

        output.write("Video record and play back stress test:\n");
        output.write("Total number of loops:"
                + NUMBER_OF_RECORDERANDPLAY_STRESS_LOOPS + "\n");

        output.write("No of loop: ");
        for (int i = 0; i < NUMBER_OF_RECORDERANDPLAY_STRESS_LOOPS; i++){
            filename = OUTPUT_FILE + i + OUTPUT_FILE_EXT;
            Log.v(TAG, filename);
            runOnLooper(new Runnable() {
                @Override
                public void run() {
                    mRecorder = new MediaRecorder();
                }
            });
            Log.v(TAG, "iterations : " + i);
            Log.v(TAG, "videoEncoder : " + mVideoEncoder);
            Log.v(TAG, "audioEncoder : " + mAudioEncoder);
            Log.v(TAG, "frameRate : " + mFrameRate);
            Log.v(TAG, "videoWidth : " + mVideoWidth);
            Log.v(TAG, "videoHeight : " + mVideoHeight);
            Log.v(TAG, "bitRate : " + mBitRate);
            Log.v(TAG, "recordDuration : " + mRecordDuration);

            mRecorder.setOnErrorListener(mRecorderErrorCallback);
            mRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
            mRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
            mRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
            mRecorder.setOutputFile(filename);
            mRecorder.setVideoFrameRate(mFrameRate);
            mRecorder.setVideoSize(mVideoWidth, mVideoHeight);
            mRecorder.setVideoEncoder(mVideoEncoder);
            mRecorder.setAudioEncoder(mAudioEncoder);
            Log.v(TAG, "mediaRecorder setPreview");
            mRecorder.setPreviewDisplay(mSurfaceHolder.getSurface());
            mRecorder.prepare();
            mRecorder.start();
            Thread.sleep(mRecordDuration);
            Log.v(TAG, "Before stop");
            mRecorder.stop();
            mRecorder.release();
            //start the playback
            MediaPlayer mp = new MediaPlayer();
            mp.setDataSource(filename);
            mp.setDisplay(MediaFrameworkTest.getSurfaceView().getHolder());
            mp.prepare();
            mp.start();
            Thread.sleep(mRecordDuration);
            mp.release();
            validateRecordedVideo(filename);
            if (mRemoveVideo) {
                removeRecodedVideo(filename);
            }
            output.write(", " + i);
        }

        output.write("\n\n");
        output.close();
    }
}
