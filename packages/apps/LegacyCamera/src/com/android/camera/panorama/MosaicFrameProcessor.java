/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.camera.panorama;

import android.util.Log;

/**
 * Class to handle the processing of each frame by Mosaicer.
 */
public class MosaicFrameProcessor {
    private static final boolean LOGV = true;
    private static final String TAG = "MosaicFrameProcessor";
    private static final int NUM_FRAMES_IN_BUFFER = 2;
    private static final int MAX_NUMBER_OF_FRAMES = 100;
    private static final int MOSAIC_RET_CODE_INDEX = 10;
    private static final int FRAME_COUNT_INDEX = 9;
    private static final int X_COORD_INDEX = 2;
    private static final int Y_COORD_INDEX = 5;
    private static final int HR_TO_LR_DOWNSAMPLE_FACTOR = 4;
    private static final int WINDOW_SIZE = 3;

    private Mosaic mMosaicer;
    private boolean mIsMosaicMemoryAllocated = false;
    private final long [] mFrameTimestamp = new long[NUM_FRAMES_IN_BUFFER];
    private float mTranslationLastX;
    private float mTranslationLastY;

    private int mFillIn = 0;
    private int mTotalFrameCount = 0;
    private long mLastProcessedFrameTimestamp = 0;
    private int mLastProcessFrameIdx = -1;
    private int mCurrProcessFrameIdx = -1;

    // Panning rate is in unit of percentage of image content translation / second.
    // Use the moving average to calculate the panning rate.
    private float mPanningRateX;
    private float mPanningRateY;

    private float[] mDeltaX = new float[WINDOW_SIZE];
    private float[] mDeltaY = new float[WINDOW_SIZE];
    private float[] mDeltaTime = new float[WINDOW_SIZE];
    private int mOldestIdx = 0;
    private float mTotalTranslationX = 0f;
    private float mTotalTranslationY = 0f;
    private float mTotalDeltaTime = 0f;

    private ProgressListener mProgressListener;

    private int mPreviewWidth;
    private int mPreviewHeight;
    private int mPreviewBufferSize;

    public interface ProgressListener {
        public void onProgress(boolean isFinished, float panningRateX, float panningRateY,
                float progressX, float progressY);
    }

    public MosaicFrameProcessor(int previewWidth, int previewHeight, int bufSize) {
        mMosaicer = new Mosaic();
        mPreviewWidth = previewWidth;
        mPreviewHeight = previewHeight;
        mPreviewBufferSize = bufSize;
    }

    public void setProgressListener(ProgressListener listener) {
        mProgressListener = listener;
    }

    public int reportProgress(boolean hires, boolean cancel) {
        return mMosaicer.reportProgress(hires, cancel);
    }

    public void initialize() {
        setupMosaicer(mPreviewWidth, mPreviewHeight, mPreviewBufferSize);
        setStripType(Mosaic.STRIPTYPE_WIDE);
        reset();
    }

    public void clear() {
        if (mIsMosaicMemoryAllocated) {
            mIsMosaicMemoryAllocated = false;
            mMosaicer.freeMosaicMemory();
        }
    }

    public void setStripType(int type) {
        mMosaicer.setStripType(type);
    }

    private void setupMosaicer(int previewWidth, int previewHeight, int bufSize) {
        Log.v(TAG, "setupMosaicer w, h=" + previewWidth + ',' + previewHeight + ',' + bufSize);
        mMosaicer.allocateMosaicMemory(previewWidth, previewHeight);
        mIsMosaicMemoryAllocated = true;

        mFillIn = 0;
        if  (mMosaicer != null) {
            mMosaicer.reset();
        }
    }

    public void reset() {
        // reset() can be called even if MosaicFrameProcessor is not initialized.
        // Only counters will be changed.
        mTotalFrameCount = 0;
        mFillIn = 0;
        mLastProcessedFrameTimestamp = 0;
        mTotalTranslationX = 0;
        mTranslationLastX = 0;
        mTotalTranslationY = 0;
        mTranslationLastY = 0;
        mTotalDeltaTime = 0;
        mPanningRateX = 0;
        mPanningRateY = 0;
        mLastProcessFrameIdx = -1;
        mCurrProcessFrameIdx = -1;
        for (int i = 0; i < WINDOW_SIZE; ++i) {
            mDeltaX[i] = 0f;
            mDeltaY[i] = 0f;
            mDeltaTime[i] = 0f;
        }
        mMosaicer.reset();
    }

    public int createMosaic(boolean highRes) {
        return mMosaicer.createMosaic(highRes);
    }

    public byte[] getFinalMosaicNV21() {
        return mMosaicer.getFinalMosaicNV21();
    }

    // Processes the last filled image frame through the mosaicer and
    // updates the UI to show progress.
    // When done, processes and displays the final mosaic.
    public void processFrame() {
        if (!mIsMosaicMemoryAllocated) {
            // clear() is called and buffers are cleared, stop computation.
            // This can happen when the onPause() is called in the activity, but still some frames
            // are not processed yet and thus the callback may be invoked.
            return;
        }
        long t1 = System.currentTimeMillis();
        mFrameTimestamp[mFillIn] = t1;

        mCurrProcessFrameIdx = mFillIn;
        mFillIn = ((mFillIn + 1) % NUM_FRAMES_IN_BUFFER);

        // Check that we are trying to process a frame different from the
        // last one processed (useful if this class was running asynchronously)
        if (mCurrProcessFrameIdx != mLastProcessFrameIdx) {
            mLastProcessFrameIdx = mCurrProcessFrameIdx;

            // Access the timestamp associated with it...
            long timestamp = mFrameTimestamp[mCurrProcessFrameIdx];

            // TODO: make the termination condition regarding reaching
            // MAX_NUMBER_OF_FRAMES solely determined in the library.
            if (mTotalFrameCount < MAX_NUMBER_OF_FRAMES) {
                // If we are still collecting new frames for the current mosaic,
                // process the new frame.
                calculateTranslationRate(timestamp);

                // Publish progress of the ongoing processing
                if (mProgressListener != null) {
                    mProgressListener.onProgress(false, mPanningRateX, mPanningRateY,
                            mTranslationLastX * HR_TO_LR_DOWNSAMPLE_FACTOR / mPreviewWidth,
                            mTranslationLastY * HR_TO_LR_DOWNSAMPLE_FACTOR / mPreviewHeight);
                }
            } else {
                if (mProgressListener != null) {
                    mProgressListener.onProgress(true, mPanningRateX, mPanningRateY,
                            mTranslationLastX * HR_TO_LR_DOWNSAMPLE_FACTOR / mPreviewWidth,
                            mTranslationLastY * HR_TO_LR_DOWNSAMPLE_FACTOR / mPreviewHeight);
                }
            }
        }
    }

    public void calculateTranslationRate(long now) {
        float[] frameData = mMosaicer.setSourceImageFromGPU();
        int ret_code = (int) frameData[MOSAIC_RET_CODE_INDEX];
        mTotalFrameCount  = (int) frameData[FRAME_COUNT_INDEX];
        float translationCurrX = frameData[X_COORD_INDEX];
        float translationCurrY = frameData[Y_COORD_INDEX];

        if (mLastProcessedFrameTimestamp == 0f) {
            // First time: no need to update delta values.
            mTranslationLastX = translationCurrX;
            mTranslationLastY = translationCurrY;
            mLastProcessedFrameTimestamp = now;
            return;
        }

        // Moving average: remove the oldest translation/deltaTime and
        // add the newest translation/deltaTime in
        int idx = mOldestIdx;
        mTotalTranslationX -= mDeltaX[idx];
        mTotalTranslationY -= mDeltaY[idx];
        mTotalDeltaTime -= mDeltaTime[idx];
        mDeltaX[idx] = Math.abs(translationCurrX - mTranslationLastX);
        mDeltaY[idx] = Math.abs(translationCurrY - mTranslationLastY);
        mDeltaTime[idx] = (now - mLastProcessedFrameTimestamp) / 1000.0f;
        mTotalTranslationX += mDeltaX[idx];
        mTotalTranslationY += mDeltaY[idx];
        mTotalDeltaTime += mDeltaTime[idx];

        // The panning rate is measured as the rate of the translation percentage in
        // image width/height. Take the horizontal panning rate for example, the image width
        // used in finding the translation is (PreviewWidth / HR_TO_LR_DOWNSAMPLE_FACTOR).
        // To get the horizontal translation percentage, the horizontal translation,
        // (translationCurrX - mTranslationLastX), is divided by the
        // image width. We then get the rate by dividing the translation percentage with deltaTime.
        mPanningRateX = mTotalTranslationX /
                (mPreviewWidth / HR_TO_LR_DOWNSAMPLE_FACTOR) / mTotalDeltaTime;
        mPanningRateY = mTotalTranslationY /
                (mPreviewHeight / HR_TO_LR_DOWNSAMPLE_FACTOR) / mTotalDeltaTime;

        mTranslationLastX = translationCurrX;
        mTranslationLastY = translationCurrY;
        mLastProcessedFrameTimestamp = now;
        mOldestIdx = (mOldestIdx + 1) % WINDOW_SIZE;
    }
}
