/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.cts.verifier.sensors;

// ----------------------------------------------------------------------

import android.content.Context;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;

import java.io.IOException;
import java.lang.Math;

import com.android.cts.verifier.sensors.RVCVRecordActivity;
import com.android.cts.verifier.sensors.RVCVRecordActivity.RecordProcedureControllerCallback;

/** Camera preview class */
public class RVCVCameraPreview extends SurfaceView implements SurfaceHolder.Callback {
    private static final String TAG = "RVCVCameraPreview";
    private static final boolean LOCAL_LOGD = true;

    private SurfaceHolder mHolder;
    private Camera mCamera;
    private float mAspect;
    private int mRotation;
    private boolean mCheckStartTest = false;
    private boolean mPreviewStarted = false;

    private RVCVRecordActivity.RecordProcedureControllerCallback mRecordProcedureControllerCallback;

    /**
     * Constructor
     * @param context Activity context
     */
    public RVCVCameraPreview(Context context) {
        super(context);
        mCamera = null;
        initSurface();
    }

    /**
     * Constructor
     * @param context Activity context
     * @param attrs
     */
    public RVCVCameraPreview(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void init(Camera camera, float aspectRatio, int rotation)  {
        this.mCamera = camera;
        mAspect = aspectRatio;
        mRotation = rotation;
        initSurface();
    }

    private void initSurface() {
        // Install a SurfaceHolder.Callback so we get notified when the
        // underlying surface is created and destroyed.
        mHolder = getHolder();
        mHolder.addCallback(this);

        // deprecated
        // TODO: update this code to match new API level.
        mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    }

    /**
     *  SurfaceHolder.Callback
     *  Surface is created, it is OK to start the camera preview now.
     */
    public void surfaceCreated(SurfaceHolder holder) {
        // The Surface has been created, now tell the camera where to draw the preview.

        if (mCamera == null) {
            // preview camera does not exist
            return;
        }
    }
    /**
     *  SurfaceHolder.Callback
     */
    public void surfaceDestroyed(SurfaceHolder holder) {
        // empty. Take care of releasing the Camera preview in your activity.
    }

    /**
     *  SurfaceHolder.Callback
     *  Restart camera preview if surface changed
     */
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {

        if (mHolder.getSurface() == null || mCamera == null){
            // preview surface or camera does not exist
            return;
        }
        if (adjustLayoutParamsIfNeeded()) {
            // Wait on next surfaceChanged() call before proceeding
            Log.d(TAG, "Waiting on surface change before starting preview");
            return;
        }

        if (mPreviewStarted) {
            Log.w(TAG, "Re-starting camera preview");
            if (mCheckStartTest && mRecordProcedureControllerCallback != null) {
                mRecordProcedureControllerCallback.stopRecordProcedureController();
            }
            mCamera.stopPreview();
            mPreviewStarted = false;
        }
        mCheckStartTest = false;

        mCamera.setDisplayOrientation(mRotation);
        try {
            mCamera.setPreviewDisplay(holder);
            mCamera.startPreview();
            mPreviewStarted = true;
            if (mRecordProcedureControllerCallback != null) {
                mCheckStartTest = true;
                mRecordProcedureControllerCallback.startRecordProcedureController();
            }
        } catch (IOException e) {
            if (LOCAL_LOGD) Log.d(TAG, "Error when starting camera preview: " + e.getMessage());
        }
    }

    /**
     * Resize the layout to more closely match the desired aspect ratio, if necessary.
     *
     * @return true if we updated the layout params, false if the params look good
     */
    private boolean adjustLayoutParamsIfNeeded() {
        ViewGroup.LayoutParams layoutParams = getLayoutParams();
        int curWidth = getWidth();
        int curHeight = getHeight();
        float curAspect = (float)curHeight / (float)curWidth;
        float aspectDelta = Math.abs(mAspect - curAspect);
        if ((aspectDelta / mAspect) >= 0.01) {
            if (curAspect > mAspect) {
                layoutParams.height = (int)Math.round(curWidth * mAspect);
                layoutParams.width = curWidth;
            } else {
                layoutParams.height = curHeight;
                layoutParams.width = (int)Math.round(curHeight / mAspect);
            }

            if (layoutParams.height != curHeight || layoutParams.width != curWidth) {
                Log.d(TAG, String.format("Layout (%d, %d) -> (%d, %d)", curWidth, curHeight,
                        layoutParams.width, layoutParams.height));
                setLayoutParams(layoutParams);
                return true;
            }
        }
        return false;
    }

    public void setRecordProcedureControllerCallback(
            RVCVRecordActivity.RecordProcedureControllerCallback callback) {
        mRecordProcedureControllerCallback = callback;
    }
}
