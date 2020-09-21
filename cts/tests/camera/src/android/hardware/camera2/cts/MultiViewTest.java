/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.hardware.camera2.cts;

import static android.hardware.camera2.cts.CameraTestUtils.*;

import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.cts.CameraTestUtils.ImageVerifierListener;
import android.hardware.camera2.cts.helpers.StaticMetadata;
import android.hardware.camera2.cts.testcases.Camera2MultiViewTestCase;
import android.hardware.camera2.cts.testcases.Camera2MultiViewTestCase.CameraPreviewListener;
import android.hardware.camera2.params.OutputConfiguration;
import android.media.Image;
import android.media.ImageReader;
import android.os.SystemClock;
import android.os.ConditionVariable;
import android.platform.test.annotations.AppModeFull;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.TextureView;

import com.android.ex.camera2.blocking.BlockingCameraManager.BlockingOpenException;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * CameraDevice test by using combination of SurfaceView, TextureView and ImageReader
 */
@AppModeFull
public class MultiViewTest extends Camera2MultiViewTestCase {
    private static final String TAG = "MultiViewTest";
    private final static long WAIT_FOR_COMMAND_TO_COMPLETE = 5000; //ms
    private final static long PREVIEW_TIME_MS = 2000;
    private final static int NUM_SURFACE_SWITCHES = 10;
    private final static int IMG_READER_COUNT = 2;
    private final static int YUV_IMG_READER_COUNT = 3;

    public void testTextureViewPreview() throws Exception {
        for (String cameraId : mCameraIds) {
            Exception prior = null;

            try {
                openCamera(cameraId);
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId + " does not support color outputs, skipping");
                    continue;
                }
                List<TextureView> views = Arrays.asList(mTextureView[0]);
                textureViewPreview(cameraId, views, /*ImageReader*/null);
            } catch (Exception e) {
                prior = e;
            } finally {
                try {
                    closeCamera(cameraId);
                } catch (Exception e) {
                    if (prior != null) {
                        Log.e(TAG, "Prior exception received: " + prior);
                    }
                    prior = e;
                }
                if (prior != null) throw prior; // Rethrow last exception.
            }
        }
    }

    public void testTextureViewPreviewWithImageReader() throws Exception {
        for (String cameraId : mCameraIds) {
            Exception prior = null;

            ImageVerifierListener yuvListener;
            ImageReader yuvReader = null;

            try {
                openCamera(cameraId);
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId + " does not support color outputs, skipping");
                    continue;
                }
                Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
                yuvListener =
                        new ImageVerifierListener(previewSize, ImageFormat.YUV_420_888);
                yuvReader = makeImageReader(previewSize,
                        ImageFormat.YUV_420_888, MAX_READER_IMAGES, yuvListener, mHandler);
                int maxNumStreamsProc =
                        getStaticInfo(cameraId).getMaxNumOutputStreamsProcessedChecked();
                if (maxNumStreamsProc < 2) {
                    continue;
                }
                List<TextureView> views = Arrays.asList(mTextureView[0]);
                textureViewPreview(cameraId, views, yuvReader);
            } catch (Exception e) {
                prior = e;
            } finally {
                try {
                    // Close camera device first. This will give some more time for
                    // ImageVerifierListener to finish the validation before yuvReader is closed
                    // (all image will be closed after that)
                    closeCamera(cameraId);
                    if (yuvReader != null) {
                        yuvReader.close();
                    }
                } catch (Exception e) {
                    if (prior != null) {
                        Log.e(TAG, "Prior exception received: " + prior);
                    }
                    prior = e;
                }
                if (prior != null) throw prior; // Rethrow last exception.
            }
        }
    }

    public void testDualTextureViewPreview() throws Exception {
        for (String cameraId : mCameraIds) {
            Exception prior = null;
            try {
                openCamera(cameraId);
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId + " does not support color outputs, skipping");
                    continue;
                }
                int maxNumStreamsProc =
                        getStaticInfo(cameraId).getMaxNumOutputStreamsProcessedChecked();
                if (maxNumStreamsProc < 2) {
                    continue;
                }
                List<TextureView> views = Arrays.asList(mTextureView[0], mTextureView[1]);
                textureViewPreview(cameraId, views, /*ImageReader*/null);
            } catch (Exception e) {
                prior = e;
            } finally {
                try {
                    closeCamera(cameraId);
                } catch (Exception e) {
                    if (prior != null) {
                        Log.e(TAG, "Prior exception received: " + prior);
                    }
                    prior = e;
                }
                if (prior != null) throw prior; // Rethrow last exception.
            }
        }
    }

    public void testDualTextureViewAndImageReaderPreview() throws Exception {
        for (String cameraId : mCameraIds) {
            Exception prior = null;

            ImageVerifierListener yuvListener;
            ImageReader yuvReader = null;

            try {
                openCamera(cameraId);
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId + " does not support color outputs, skipping");
                    continue;
                }
                Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
                yuvListener =
                        new ImageVerifierListener(previewSize, ImageFormat.YUV_420_888);
                yuvReader = makeImageReader(previewSize,
                        ImageFormat.YUV_420_888, MAX_READER_IMAGES, yuvListener, mHandler);
                int maxNumStreamsProc =
                        getStaticInfo(cameraId).getMaxNumOutputStreamsProcessedChecked();
                if (maxNumStreamsProc < 3) {
                    continue;
                }
                List<TextureView> views = Arrays.asList(mTextureView[0], mTextureView[1]);
                textureViewPreview(cameraId, views, yuvReader);
            } catch (Exception e) {
                prior = e;
            } finally {
                try {
                    if (yuvReader != null) {
                        yuvReader.close();
                    }
                    closeCamera(cameraId);
                } catch (Exception e) {
                    if (prior != null) {
                        Log.e(TAG, "Prior exception received: " + prior);
                    }
                    prior = e;
                }
                if (prior != null) throw prior; // Rethrow last exception.
            }
        }
    }

    public void testDualCameraPreview() throws Exception {
        final int NUM_CAMERAS_TESTED = 2;
        if (mCameraIds.length < NUM_CAMERAS_TESTED) {
            return;
        }

        try {
            for (int i = 0; i < NUM_CAMERAS_TESTED; i++) {
                openCamera(mCameraIds[i]);
                if (!getStaticInfo(mCameraIds[i]).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + mCameraIds[i] +
                            " does not support color outputs, skipping");
                    continue;
                }
                List<TextureView> views = Arrays.asList(mTextureView[i]);

                startTextureViewPreview(mCameraIds[i], views, /*ImageReader*/null);
            }
            // TODO: check the framerate is correct
            SystemClock.sleep(PREVIEW_TIME_MS);
            for (int i = 0; i < NUM_CAMERAS_TESTED; i++) {
                stopPreview(mCameraIds[i]);
            }
        } catch (BlockingOpenException e) {
            // The only error accepted is ERROR_MAX_CAMERAS_IN_USE, which means HAL doesn't support
            // concurrent camera streaming
            assertEquals("Camera device open failed",
                    CameraDevice.StateCallback.ERROR_MAX_CAMERAS_IN_USE, e.getCode());
            Log.i(TAG, "Camera HAL does not support dual camera preview. Skip the test");
        } finally {
            for (int i = 0; i < NUM_CAMERAS_TESTED; i++) {
                closeCamera(mCameraIds[i]);
            }
        }
    }

    /*
     * Verify dynamic shared surface behavior.
     */
    public void testSharedSurfaceBasic() throws Exception {
        for (String cameraId : mCameraIds) {
            try {
                openCamera(cameraId);
                if (getStaticInfo(cameraId).isHardwareLevelLegacy()) {
                    Log.i(TAG, "Camera " + cameraId + " is legacy, skipping");
                    continue;
                }
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId +
                            " does not support color outputs, skipping");
                    continue;
                }

                testSharedSurfaceBasicByCamera(cameraId);
            }
            finally {
                closeCamera(cameraId);
            }
        }
    }

    private void testSharedSurfaceBasicByCamera(String cameraId) throws Exception {
        Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
        CameraPreviewListener[] previewListener = new CameraPreviewListener[2];
        SurfaceTexture[] previewTexture = new SurfaceTexture[2];
        Surface[] surfaces = new Surface[2];
        OutputConfiguration[] outputConfigs = new OutputConfiguration[2];
        List<OutputConfiguration> outputConfigurations = new ArrayList<>();

        // Create surface textures with the same size
        for (int i = 0; i < 2; i++) {
            previewListener[i] = new CameraPreviewListener();
            mTextureView[i].setSurfaceTextureListener(previewListener[i]);
            previewTexture[i] = getAvailableSurfaceTexture(
                    WAIT_FOR_COMMAND_TO_COMPLETE, mTextureView[i]);
            assertNotNull("Unable to get preview surface texture", previewTexture[i]);
            previewTexture[i].setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());
            // Correct the preview display rotation.
            updatePreviewDisplayRotation(previewSize, mTextureView[i]);
            surfaces[i] = new Surface(previewTexture[i]);
            outputConfigs[i] = new OutputConfiguration(surfaces[i]);
            outputConfigs[i].enableSurfaceSharing();
            outputConfigurations.add(outputConfigs[i]);
        }

        startPreviewWithConfigs(cameraId, outputConfigurations, null);

        boolean previewDone =
                previewListener[0].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
        assertTrue("Unable to start preview", previewDone);

        //try to dynamically add and remove any of the initial configured outputs
        try {
            outputConfigs[1].addSurface(surfaces[0]);
            updateOutputConfiguration(cameraId, outputConfigs[1]);
            fail("should get IllegalArgumentException due to invalid output");
        } catch (IllegalArgumentException e) {
            // expected exception
            outputConfigs[1].removeSurface(surfaces[0]);
        }

        try {
            outputConfigs[1].removeSurface(surfaces[1]);
            fail("should get IllegalArgumentException due to invalid output");
        } catch (IllegalArgumentException e) {
            // expected exception
        }

        try {
            outputConfigs[0].addSurface(surfaces[1]);
            updateOutputConfiguration(cameraId, outputConfigs[0]);
            fail("should get IllegalArgumentException due to invalid output");
        } catch (IllegalArgumentException e) {
            // expected exception
            outputConfigs[0].removeSurface(surfaces[1]);
        }

        try {
            outputConfigs[0].removeSurface(surfaces[0]);
            fail("should get IllegalArgumentException due to invalid output");
        } catch (IllegalArgumentException e) {
            // expected exception
        }

        //Check that we are able to add a shared texture surface with different size
        List<Size> orderedPreviewSizes = getOrderedPreviewSizes(cameraId);
        Size textureSize = previewSize;
        for (Size s : orderedPreviewSizes) {
            if (!s.equals(previewSize)) {
                textureSize = s;
                break;
            }
        }
        if (textureSize.equals(previewSize)) {
            return;
        }
        SurfaceTexture outputTexture = new SurfaceTexture(/* random texture ID*/ 5);
        outputTexture.setDefaultBufferSize(textureSize.getWidth(), textureSize.getHeight());
        Surface outputSurface = new Surface(outputTexture);
        //Add a valid output surface and then verify that it cannot be added any more
        outputConfigs[1].addSurface(outputSurface);
        updateOutputConfiguration(cameraId, outputConfigs[1]);
        try {
            outputConfigs[1].addSurface(outputSurface);
            fail("should get IllegalStateException due to duplicate output");
        } catch (IllegalStateException e) {
            // expected exception
        }

        outputConfigs[0].addSurface(outputSurface);
        try {
            updateOutputConfiguration(cameraId, outputConfigs[0]);
            fail("should get IllegalArgumentException due to duplicate output");
        } catch (IllegalArgumentException e) {
            // expected exception
            outputConfigs[0].removeSurface(outputSurface);
        }

        //Verify that the same surface cannot be removed twice
        outputConfigs[1].removeSurface(outputSurface);
        updateOutputConfiguration(cameraId, outputConfigs[1]);
        try {
            outputConfigs[0].removeSurface(outputSurface);
            fail("should get IllegalArgumentException due to invalid output");
        } catch (IllegalArgumentException e) {
            // expected exception
        }
        try {
            outputConfigs[1].removeSurface(outputSurface);
            fail("should get IllegalArgumentException due to invalid output");
        } catch (IllegalArgumentException e) {
            // expected exception
        }

        stopPreview(cameraId);
    }

    /*
     * Verify dynamic shared surface behavior using multiple ImageReaders.
     */
    public void testSharedSurfaceImageReaderSwitch() throws Exception {
        for (String cameraId : mCameraIds) {
            try {
                openCamera(cameraId);
                if (getStaticInfo(cameraId).isHardwareLevelLegacy()) {
                    Log.i(TAG, "Camera " + cameraId + " is legacy, skipping");
                    continue;
                }
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId +
                            " does not support color outputs, skipping");
                    continue;
                }

                testSharedSurfaceImageReaderSwitch(cameraId, NUM_SURFACE_SWITCHES);
            }
            finally {
                closeCamera(cameraId);
            }
        }
    }

    private void testSharedSurfaceImageReaderSwitch(String cameraId, int switchCount)
            throws Exception {
        SimpleImageListener imageListeners[] = new SimpleImageListener[IMG_READER_COUNT];
        SimpleCaptureCallback resultListener = new SimpleCaptureCallback();
        ImageReader imageReaders[] = new ImageReader[IMG_READER_COUNT];
        Surface readerSurfaces[] = new Surface[IMG_READER_COUNT];
        Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
        CameraPreviewListener previewListener = new CameraPreviewListener();
        mTextureView[0].setSurfaceTextureListener(previewListener);
        SurfaceTexture previewTexture = getAvailableSurfaceTexture(WAIT_FOR_COMMAND_TO_COMPLETE,
                mTextureView[0]);
        assertNotNull("Unable to get preview surface texture", previewTexture);
        previewTexture.setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());
        updatePreviewDisplayRotation(previewSize, mTextureView[0]);
        Surface previewSurface = new Surface(previewTexture);
        OutputConfiguration outputConfig = new OutputConfiguration(previewSurface);
        outputConfig.enableSurfaceSharing();
        List<OutputConfiguration> outputConfigurations = new ArrayList<>();
        outputConfigurations.add(outputConfig);

        if (outputConfig.getMaxSharedSurfaceCount() < (IMG_READER_COUNT + 1)) {
            return;
        }

        //Start regular preview streaming
        startPreviewWithConfigs(cameraId, outputConfigurations, null);

        boolean previewDone = previewListener.waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
        assertTrue("Unable to start preview", previewDone);

        //Test shared image reader outputs
        for (int i = 0; i < IMG_READER_COUNT; i++) {
            imageListeners[i] = new SimpleImageListener();
            imageReaders[i] = ImageReader.newInstance(previewSize.getWidth(),
                    previewSize.getHeight(), ImageFormat.PRIVATE, 2);
            imageReaders[i].setOnImageAvailableListener(imageListeners[i], mHandler);
            readerSurfaces[i] = imageReaders[i].getSurface();
        }

        for (int j = 0; j < switchCount; j++) {
            for (int i = 0; i < IMG_READER_COUNT; i++) {
                outputConfig.addSurface(readerSurfaces[i]);
                updateOutputConfiguration(cameraId, outputConfig);
                CaptureRequest.Builder imageReaderRequestBuilder = getCaptureBuilder(cameraId,
                        CameraDevice.TEMPLATE_PREVIEW);
                imageReaderRequestBuilder.addTarget(readerSurfaces[i]);
                capture(cameraId, imageReaderRequestBuilder.build(), resultListener);
                imageListeners[i].waitForAnyImageAvailable(PREVIEW_TIME_MS);
                Image img = imageReaders[i].acquireLatestImage();
                assertNotNull("Invalid image acquired!", img);
                img.close();
                outputConfig.removeSurface(readerSurfaces[i]);
                updateOutputConfiguration(cameraId, outputConfig);
            }
        }

        for (int i = 0; i < IMG_READER_COUNT; i++) {
            imageReaders[i].close();
        }

        stopPreview(cameraId);
    }

    /*
     * Verify dynamic shared surface behavior using YUV ImageReaders.
     */
    public void testSharedSurfaceYUVImageReaderSwitch() throws Exception {
        int YUVFormats[] = {ImageFormat.YUV_420_888, ImageFormat.YUV_422_888,
            ImageFormat.YUV_444_888, ImageFormat.YUY2, ImageFormat.YV12,
            ImageFormat.NV16, ImageFormat.NV21};
        for (String cameraId : mCameraIds) {
            try {
                openCamera(cameraId);
                if (getStaticInfo(cameraId).isHardwareLevelLegacy()) {
                    Log.i(TAG, "Camera " + cameraId + " is legacy, skipping");
                    continue;
                }
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId +
                            " does not support color outputs, skipping");
                    continue;
                }
                Size frameSize = null;
                int yuvFormat = -1;
                for (int it : YUVFormats) {
                    Size yuvSizes[] = getStaticInfo(cameraId).getAvailableSizesForFormatChecked(
                            it, StaticMetadata.StreamDirection.Output);
                    if (yuvSizes != null) {
                        frameSize = yuvSizes[0];
                        yuvFormat = it;
                        break;
                    }
                }

                if ((yuvFormat != -1) && (frameSize.getWidth() > 0) &&
                        (frameSize.getHeight() > 0)) {
                    testSharedSurfaceYUVImageReaderSwitch(cameraId, NUM_SURFACE_SWITCHES, yuvFormat,
                            frameSize);
                } else {
                    Log.i(TAG, "Camera " + cameraId +
                            " does not support YUV outputs, skipping");
                }
            }
            finally {
                closeCamera(cameraId);
            }
        }
    }

    private void testSharedSurfaceYUVImageReaderSwitch(String cameraId, int switchCount, int format,
            Size frameSize) throws Exception {

        assertTrue("YUV_IMG_READER_COUNT should be equal or greater than 2",
                (YUV_IMG_READER_COUNT >= 2));

        SimpleImageListener imageListeners[] = new SimpleImageListener[YUV_IMG_READER_COUNT];
        SimpleCaptureCallback resultListener = new SimpleCaptureCallback();
        ImageReader imageReaders[] = new ImageReader[YUV_IMG_READER_COUNT];
        Surface readerSurfaces[] = new Surface[YUV_IMG_READER_COUNT];

        for (int i = 0; i < YUV_IMG_READER_COUNT; i++) {
            imageListeners[i] = new SimpleImageListener();
            imageReaders[i] = ImageReader.newInstance(frameSize.getWidth(), frameSize.getHeight(),
                    format, 2);
            imageReaders[i].setOnImageAvailableListener(imageListeners[i], mHandler);
            readerSurfaces[i] = imageReaders[i].getSurface();
        }

        OutputConfiguration outputConfig = new OutputConfiguration(readerSurfaces[0]);
        outputConfig.enableSurfaceSharing();
        List<OutputConfiguration> outputConfigurations = new ArrayList<>();
        outputConfigurations.add(outputConfig);
        if (outputConfig.getMaxSharedSurfaceCount() < YUV_IMG_READER_COUNT) {
            return;
        }

        createSessionWithConfigs(cameraId, outputConfigurations);

        // Test YUV ImageReader surface sharing. The first ImageReader will
        // always be part of the capture request, the rest will switch on each
        // iteration.
        for (int j = 0; j < switchCount; j++) {
            for (int i = 1; i < YUV_IMG_READER_COUNT; i++) {
                outputConfig.addSurface(readerSurfaces[i]);
                updateOutputConfiguration(cameraId, outputConfig);
                CaptureRequest.Builder imageReaderRequestBuilder = getCaptureBuilder(cameraId,
                        CameraDevice.TEMPLATE_PREVIEW);
                imageReaderRequestBuilder.addTarget(readerSurfaces[i]);
                imageReaderRequestBuilder.addTarget(readerSurfaces[0]);
                capture(cameraId, imageReaderRequestBuilder.build(), resultListener);
                imageListeners[i].waitForAnyImageAvailable(PREVIEW_TIME_MS);
                Image img = imageReaders[i].acquireLatestImage();
                assertNotNull("Invalid image acquired!", img);
                assertNotNull("Image planes are invalid!", img.getPlanes());
                img.close();
                imageListeners[0].waitForAnyImageAvailable(PREVIEW_TIME_MS);
                img = imageReaders[0].acquireLatestImage();
                assertNotNull("Invalid image acquired!", img);
                img.close();
                outputConfig.removeSurface(readerSurfaces[i]);
                updateOutputConfiguration(cameraId, outputConfig);
            }
        }

        for (int i = 0; i < YUV_IMG_READER_COUNT; i++) {
            imageReaders[i].close();
        }
    }

    /*
     * Test the dynamic shared surface limit.
     */
    public void testSharedSurfaceLimit() throws Exception {
        for (String cameraId : mCameraIds) {
            try {
                openCamera(cameraId);
                if (getStaticInfo(cameraId).isHardwareLevelLegacy()) {
                    Log.i(TAG, "Camera " + cameraId + " is legacy, skipping");
                    continue;
                }
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId +
                            " does not support color outputs, skipping");
                    continue;
                }

                testSharedSurfaceLimitByCamera(cameraId,
                        Camera2MultiViewCtsActivity.MAX_TEXTURE_VIEWS);
            }
            finally {
                closeCamera(cameraId);
            }
        }
    }

    private void testSharedSurfaceLimitByCamera(String cameraId, int surfaceLimit)
            throws Exception {
        Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
        CameraPreviewListener[] previewListener = new CameraPreviewListener[surfaceLimit];
        SurfaceTexture[] previewTexture = new SurfaceTexture[surfaceLimit];
        Surface[] surfaces = new Surface[surfaceLimit];
        int sequenceId = -1;

        // Create surface textures with the same size
        for (int i = 0; i < surfaceLimit; i++) {
            previewListener[i] = new CameraPreviewListener();
            mTextureView[i].setSurfaceTextureListener(previewListener[i]);
            previewTexture[i] = getAvailableSurfaceTexture(
                    WAIT_FOR_COMMAND_TO_COMPLETE, mTextureView[i]);
            assertNotNull("Unable to get preview surface texture", previewTexture[i]);
            previewTexture[i].setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());
            // Correct the preview display rotation.
            updatePreviewDisplayRotation(previewSize, mTextureView[i]);
            surfaces[i] = new Surface(previewTexture[i]);
        }

        SimpleCaptureCallback resultListener = new SimpleCaptureCallback();

        // Create shared outputs for the two surface textures
        OutputConfiguration surfaceSharedOutput = new OutputConfiguration(surfaces[0]);
        surfaceSharedOutput.enableSurfaceSharing();

        if ((surfaceLimit <= 1) ||
                (surfaceLimit < surfaceSharedOutput.getMaxSharedSurfaceCount())) {
            return;
        }

        List<OutputConfiguration> outputConfigurations = new ArrayList<>();
        outputConfigurations.add(surfaceSharedOutput);

        startPreviewWithConfigs(cameraId, outputConfigurations, null);

        boolean previewDone =
                previewListener[0].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
        assertTrue("Unable to start preview", previewDone);
        mTextureView[0].setSurfaceTextureListener(null);

        SystemClock.sleep(PREVIEW_TIME_MS);

        int i = 1;
        for (; i < surfaceLimit; i++) {
            //Add one more output surface while preview is streaming
            if (i >= surfaceSharedOutput.getMaxSharedSurfaceCount()){
                try {
                    surfaceSharedOutput.addSurface(surfaces[i]);
                    fail("should get IllegalArgumentException due to output surface limit");
                } catch (IllegalArgumentException e) {
                    //expected
                    break;
                }
            } else {
                surfaceSharedOutput.addSurface(surfaces[i]);
                updateOutputConfiguration(cameraId, surfaceSharedOutput);
                sequenceId = updateRepeatingRequest(cameraId, outputConfigurations, resultListener);

                SystemClock.sleep(PREVIEW_TIME_MS);
            }
        }

        for (; i > 0; i--) {
            if (i >= surfaceSharedOutput.getMaxSharedSurfaceCount()) {
                try {
                    surfaceSharedOutput.removeSurface(surfaces[i]);
                    fail("should get IllegalArgumentException due to output surface limit");
                } catch (IllegalArgumentException e) {
                    // expected exception
                }
            } else {
                surfaceSharedOutput.removeSurface(surfaces[i]);

            }
        }
        //Remove all previously added shared outputs in one call
        updateRepeatingRequest(cameraId, outputConfigurations, resultListener);
        long lastSequenceFrameNumber = resultListener.getCaptureSequenceLastFrameNumber(
                sequenceId, PREVIEW_TIME_MS);
        checkForLastFrameInSequence(lastSequenceFrameNumber, resultListener);
        updateOutputConfiguration(cameraId, surfaceSharedOutput);
        SystemClock.sleep(PREVIEW_TIME_MS);

        stopPreview(cameraId);
    }

    /*
     * Test dynamic shared surface switch behavior.
     */
    public void testSharedSurfaceSwitch() throws Exception {
        for (String cameraId : mCameraIds) {
            try {
                openCamera(cameraId);
                if (getStaticInfo(cameraId).isHardwareLevelLegacy()) {
                    Log.i(TAG, "Camera " + cameraId + " is legacy, skipping");
                    continue;
                }
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId +
                            " does not support color outputs, skipping");
                    continue;
                }

                testSharedSurfaceSwitchByCamera(cameraId, NUM_SURFACE_SWITCHES);
            }
            finally {
                closeCamera(cameraId);
            }
        }
    }

    private void testSharedSurfaceSwitchByCamera(String cameraId, int switchCount)
            throws Exception {
        Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
        CameraPreviewListener[] previewListener = new CameraPreviewListener[2];
        SurfaceTexture[] previewTexture = new SurfaceTexture[2];
        Surface[] surfaces = new Surface[2];

        // Create surface textures with the same size
        for (int i = 0; i < 2; i++) {
            previewListener[i] = new CameraPreviewListener();
            mTextureView[i].setSurfaceTextureListener(previewListener[i]);
            previewTexture[i] = getAvailableSurfaceTexture(
                    WAIT_FOR_COMMAND_TO_COMPLETE, mTextureView[i]);
            assertNotNull("Unable to get preview surface texture", previewTexture[i]);
            previewTexture[i].setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());
            // Correct the preview display rotation.
            updatePreviewDisplayRotation(previewSize, mTextureView[i]);
            surfaces[i] = new Surface(previewTexture[i]);
        }

        SimpleCaptureCallback resultListener = new SimpleCaptureCallback();

        // Create shared outputs for the two surface textures
        OutputConfiguration surfaceSharedOutput = new OutputConfiguration(surfaces[0]);
        surfaceSharedOutput.enableSurfaceSharing();

        List<OutputConfiguration> outputConfigurations = new ArrayList<>();
        outputConfigurations.add(surfaceSharedOutput);

        startPreviewWithConfigs(cameraId, outputConfigurations, null);

        boolean previewDone =
                previewListener[0].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
        assertTrue("Unable to start preview", previewDone);
        mTextureView[0].setSurfaceTextureListener(null);

        SystemClock.sleep(PREVIEW_TIME_MS);

        for (int i = 0; i < switchCount; i++) {
            //Add one more output surface while preview is streaming
            surfaceSharedOutput.addSurface(surfaces[1]);
            updateOutputConfiguration(cameraId, surfaceSharedOutput);
            int sequenceId = updateRepeatingRequest(cameraId, outputConfigurations, resultListener);

            SystemClock.sleep(PREVIEW_TIME_MS);

            //Try to remove the shared surface while while we still have active requests that
            //use it as output.
            surfaceSharedOutput.removeSurface(surfaces[1]);
            try {
                updateOutputConfiguration(cameraId, surfaceSharedOutput);
                fail("should get IllegalArgumentException due to pending requests");
            } catch (IllegalArgumentException e) {
                // expected exception
            }

            //Wait for all pending requests to arrive and remove the shared output during active
            //streaming
            updateRepeatingRequest(cameraId, outputConfigurations, resultListener);
            long lastSequenceFrameNumber = resultListener.getCaptureSequenceLastFrameNumber(
                    sequenceId, PREVIEW_TIME_MS);
            checkForLastFrameInSequence(lastSequenceFrameNumber, resultListener);
            updateOutputConfiguration(cameraId, surfaceSharedOutput);

            SystemClock.sleep(PREVIEW_TIME_MS);
        }

        stopPreview(cameraId);
    }

    private void checkForLastFrameInSequence(long lastSequenceFrameNumber,
            SimpleCaptureCallback listener) throws Exception {
        // Find the last frame number received in results and failures.
        long lastFrameNumber = -1;
        while (listener.hasMoreResults()) {
            TotalCaptureResult result = listener.getTotalCaptureResult(PREVIEW_TIME_MS);
            if (lastFrameNumber < result.getFrameNumber()) {
                lastFrameNumber = result.getFrameNumber();
            }
        }

        while (listener.hasMoreFailures()) {
            ArrayList<CaptureFailure> failures = listener.getCaptureFailures(
                    /*maxNumFailures*/ 1);
            for (CaptureFailure failure : failures) {
                if (lastFrameNumber < failure.getFrameNumber()) {
                    lastFrameNumber = failure.getFrameNumber();
                }
            }
        }

        // Verify the last frame number received from capture sequence completed matches the
        // the last frame number of the results and failures.
        assertEquals(String.format("Last frame number from onCaptureSequenceCompleted " +
                "(%d) doesn't match the last frame number received from " +
                "results/failures (%d)", lastSequenceFrameNumber, lastFrameNumber),
                lastSequenceFrameNumber, lastFrameNumber);
    }

    /*
     * Verify behavior of sharing surfaces within one OutputConfiguration
     */
    public void testSharedSurfaces() throws Exception {
        for (String cameraId : mCameraIds) {
            try {
                openCamera(cameraId);
                if (getStaticInfo(cameraId).isHardwareLevelLegacy()) {
                    Log.i(TAG, "Camera " + cameraId + " is legacy, skipping");
                    continue;
                }
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId +
                            " does not support color outputs, skipping");
                    continue;
                }

                testSharedSurfacesConfigByCamera(cameraId);

                testSharedSurfacesCaptureSessionByCamera(cameraId);

                testSharedDeferredSurfacesByCamera(cameraId);
            }
            finally {
                closeCamera(cameraId);
            }
        }
    }


    /**
     * Start camera preview using input texture views and/or one image reader
     */
    private void startTextureViewPreview(
            String cameraId, List<TextureView> views, ImageReader imageReader)
            throws Exception {
        int numPreview = views.size();
        Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
        CameraPreviewListener[] previewListener =
                new CameraPreviewListener[numPreview];
        SurfaceTexture[] previewTexture = new SurfaceTexture[numPreview];
        List<Surface> surfaces = new ArrayList<Surface>();

        // Prepare preview surface.
        int i = 0;
        for (TextureView view : views) {
            previewListener[i] = new CameraPreviewListener();
            view.setSurfaceTextureListener(previewListener[i]);
            previewTexture[i] = getAvailableSurfaceTexture(WAIT_FOR_COMMAND_TO_COMPLETE, view);
            assertNotNull("Unable to get preview surface texture", previewTexture[i]);
            previewTexture[i].setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());
            // Correct the preview display rotation.
            updatePreviewDisplayRotation(previewSize, view);
            surfaces.add(new Surface(previewTexture[i]));
            i++;
        }
        if (imageReader != null) {
            surfaces.add(imageReader.getSurface());
        }

        startPreview(cameraId, surfaces, null);

        i = 0;
        for (TextureView view : views) {
            boolean previewDone =
                    previewListener[i].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
            assertTrue("Unable to start preview " + i, previewDone);
            view.setSurfaceTextureListener(null);
            i++;
        }
    }

    /**
     * Test camera preview using input texture views and/or one image reader
     */
    private void textureViewPreview(
            String cameraId, List<TextureView> views, ImageReader testImagerReader)
            throws Exception {
        startTextureViewPreview(cameraId, views, testImagerReader);

        // TODO: check the framerate is correct
        SystemClock.sleep(PREVIEW_TIME_MS);

        stopPreview(cameraId);
    }

    /*
     * Verify behavior of OutputConfiguration when sharing surfaces
     */
    private void testSharedSurfacesConfigByCamera(String cameraId) throws Exception {
        Size previewSize = getOrderedPreviewSizes(cameraId).get(0);

        SurfaceTexture[] previewTexture = new SurfaceTexture[2];
        Surface[] surfaces = new Surface[2];

        // Create surface textures with the same size
        for (int i = 0; i < 2; i++) {
            previewTexture[i] = getAvailableSurfaceTexture(
                    WAIT_FOR_COMMAND_TO_COMPLETE, mTextureView[i]);
            assertNotNull("Unable to get preview surface texture", previewTexture[i]);
            previewTexture[i].setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());
            // Correct the preview display rotation.
            updatePreviewDisplayRotation(previewSize, mTextureView[i]);
            surfaces[i] = new Surface(previewTexture[i]);
        }

        // Verify that outputConfiguration can be created with 2 surfaces with the same setting.
        OutputConfiguration previewConfiguration = new OutputConfiguration(
                OutputConfiguration.SURFACE_GROUP_ID_NONE, surfaces[0]);
        previewConfiguration.enableSurfaceSharing();
        previewConfiguration.addSurface(surfaces[1]);
        List<Surface> previewSurfaces = previewConfiguration.getSurfaces();
        List<Surface> inputSurfaces = Arrays.asList(surfaces);
        assertTrue(
                String.format("Surfaces returned from getSurfaces() don't match those passed in"),
                previewSurfaces.equals(inputSurfaces));

        // Verify that createCaptureSession fails if 2 surfaces are different size
        SurfaceTexture outputTexture2 = new SurfaceTexture(/* random texture ID*/ 5);
        outputTexture2.setDefaultBufferSize(previewSize.getWidth()/2,
                previewSize.getHeight()/2);
        Surface outputSurface2 = new Surface(outputTexture2);
        OutputConfiguration configuration = new OutputConfiguration(
                OutputConfiguration.SURFACE_GROUP_ID_NONE, surfaces[0]);
        configuration.enableSurfaceSharing();
        configuration.addSurface(outputSurface2);
        List<OutputConfiguration> outputConfigurations = new ArrayList<>();
        outputConfigurations.add(configuration);
        verifyCreateSessionWithConfigsFailure(cameraId, outputConfigurations);

        // Verify that outputConfiguration throws exception if 2 surfaces are different format
        ImageReader imageReader = makeImageReader(previewSize, ImageFormat.YUV_420_888,
                MAX_READER_IMAGES, new ImageDropperListener(), mHandler);
        try {
            configuration = new OutputConfiguration(OutputConfiguration.SURFACE_GROUP_ID_NONE,
                    surfaces[0]);
            configuration.enableSurfaceSharing();
            configuration.addSurface(imageReader.getSurface());
            fail("No error for invalid output config created from different format surfaces");
        } catch (IllegalArgumentException e) {
            // expected
        }

        // Verify that outputConfiguration can be created with deferred surface with the same
        // setting.
        OutputConfiguration deferredPreviewConfigure = new OutputConfiguration(
                previewSize, SurfaceTexture.class);
        deferredPreviewConfigure.addSurface(surfaces[0]);
        assertTrue(String.format("Number of surfaces %d doesn't match expected value 1",
                deferredPreviewConfigure.getSurfaces().size()),
                deferredPreviewConfigure.getSurfaces().size() == 1);
        assertEquals("Surface 0 in OutputConfiguration doesn't match input",
                deferredPreviewConfigure.getSurfaces().get(0), surfaces[0]);

        // Verify that outputConfiguration throws exception if deferred surface and non-deferred
        // surface properties don't match
        try {
            configuration = new OutputConfiguration(previewSize, SurfaceTexture.class);
            configuration.addSurface(imageReader.getSurface());
            fail("No error for invalid output config created deferred class with different type");
        } catch (IllegalArgumentException e) {
            // expected;
        }
    }

    private void testSharedSurfacesCaptureSessionByCamera(String cameraId) throws Exception {
        Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
        CameraPreviewListener[] previewListener = new CameraPreviewListener[2];
        SurfaceTexture[] previewTexture = new SurfaceTexture[2];
        Surface[] surfaces = new Surface[2];

        // Create surface textures with the same size
        for (int i = 0; i < 2; i++) {
            previewListener[i] = new CameraPreviewListener();
            mTextureView[i].setSurfaceTextureListener(previewListener[i]);
            previewTexture[i] = getAvailableSurfaceTexture(
                    WAIT_FOR_COMMAND_TO_COMPLETE, mTextureView[i]);
            assertNotNull("Unable to get preview surface texture", previewTexture[i]);
            previewTexture[i].setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());
            // Correct the preview display rotation.
            updatePreviewDisplayRotation(previewSize, mTextureView[i]);
            surfaces[i] = new Surface(previewTexture[i]);
        }

        SimpleCaptureCallback resultListener = new SimpleCaptureCallback();

        // Create shared outputs for the two surface textures
        OutputConfiguration surfaceSharedOutput = new OutputConfiguration(
                OutputConfiguration.SURFACE_GROUP_ID_NONE, surfaces[0]);
        surfaceSharedOutput.enableSurfaceSharing();
        surfaceSharedOutput.addSurface(surfaces[1]);

        List<OutputConfiguration> outputConfigurations = new ArrayList<>();
        outputConfigurations.add(surfaceSharedOutput);

        startPreviewWithConfigs(cameraId, outputConfigurations, null);

        for (int i = 0; i < 2; i++) {
            boolean previewDone =
                    previewListener[i].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
            assertTrue("Unable to start preview " + i, previewDone);
            mTextureView[i].setSurfaceTextureListener(null);
        }

        SystemClock.sleep(PREVIEW_TIME_MS);

        stopPreview(cameraId);
    }

    private void testSharedDeferredSurfacesByCamera(String cameraId) throws Exception {
        Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
        CameraPreviewListener[] previewListener = new CameraPreviewListener[2];
        SurfaceTexture[] previewTexture = new SurfaceTexture[2];
        Surface[] surfaces = new Surface[2];

        // Create surface textures with the same size
        for (int i = 0; i < 2; i++) {
            previewListener[i] = new CameraPreviewListener();
            mTextureView[i].setSurfaceTextureListener(previewListener[i]);
            previewTexture[i] = getAvailableSurfaceTexture(
                    WAIT_FOR_COMMAND_TO_COMPLETE, mTextureView[i]);
            assertNotNull("Unable to get preview surface texture", previewTexture[i]);
            previewTexture[i].setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());
            // Correct the preview display rotation.
            updatePreviewDisplayRotation(previewSize, mTextureView[i]);
            surfaces[i] = new Surface(previewTexture[i]);
        }

        SimpleCaptureCallback resultListener = new SimpleCaptureCallback();

        //
        // Create deferred outputConfiguration, addSurface, createCaptureSession, addSurface, and
        // finalizeOutputConfigurations.
        //

        OutputConfiguration surfaceSharedOutput = new OutputConfiguration(
                previewSize, SurfaceTexture.class);
        surfaceSharedOutput.enableSurfaceSharing();
        surfaceSharedOutput.addSurface(surfaces[0]);

        List<OutputConfiguration> outputConfigurations = new ArrayList<>();
        outputConfigurations.add(surfaceSharedOutput);

        // Run preview with one surface, and verify at least one frame is received.
        startPreviewWithConfigs(cameraId, outputConfigurations, null);
        boolean previewDone =
                previewListener[0].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
        assertTrue("Unable to start preview 0", previewDone);

        SystemClock.sleep(PREVIEW_TIME_MS);

        // Add deferred surface to the output configuration
        surfaceSharedOutput.addSurface(surfaces[1]);
        List<OutputConfiguration> deferredConfigs = new ArrayList<OutputConfiguration>();
        deferredConfigs.add(surfaceSharedOutput);

        // Run preview with both surfaces, and verify at least one frame is received for each
        // surface.
        finalizeOutputConfigs(cameraId, deferredConfigs, null);
        previewDone =
                previewListener[1].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
        assertTrue("Unable to start preview 1", previewDone);

        stopPreview(cameraId);

        previewListener[0].reset();
        previewListener[1].reset();

        //
        // Create outputConfiguration with a surface, createCaptureSession, addSurface, and
        // finalizeOutputConfigurations.
        //

        surfaceSharedOutput = new OutputConfiguration(
                OutputConfiguration.SURFACE_GROUP_ID_NONE, surfaces[0]);
        surfaceSharedOutput.enableSurfaceSharing();
        outputConfigurations.clear();
        outputConfigurations.add(surfaceSharedOutput);

        startPreviewWithConfigs(cameraId, outputConfigurations, null);
        previewDone =
                previewListener[0].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
        assertTrue("Unable to start preview 0", previewDone);

        // Add deferred surface to the output configuration, and continue running preview
        surfaceSharedOutput.addSurface(surfaces[1]);
        deferredConfigs.clear();
        deferredConfigs.add(surfaceSharedOutput);
        finalizeOutputConfigs(cameraId, deferredConfigs, null);
        previewDone =
                previewListener[1].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
        assertTrue("Unable to start preview 1", previewDone);

        SystemClock.sleep(PREVIEW_TIME_MS);
        stopPreview(cameraId);

        previewListener[0].reset();
        previewListener[1].reset();

        //
        // Create deferred output configuration, createCaptureSession, addSurface, addSurface, and
        // finalizeOutputConfigurations.

        surfaceSharedOutput = new OutputConfiguration(
                previewSize, SurfaceTexture.class);
        surfaceSharedOutput.enableSurfaceSharing();
        outputConfigurations.clear();
        outputConfigurations.add(surfaceSharedOutput);
        createSessionWithConfigs(cameraId, outputConfigurations);

        // Add 2 surfaces to the output configuration, and run preview
        surfaceSharedOutput.addSurface(surfaces[0]);
        surfaceSharedOutput.addSurface(surfaces[1]);
        deferredConfigs.clear();
        deferredConfigs.add(surfaceSharedOutput);
        finalizeOutputConfigs(cameraId, deferredConfigs, null);
        for (int i = 0; i < 2; i++) {
            previewDone =
                    previewListener[i].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
            assertTrue("Unable to start preview " + i, previewDone);
        }

        SystemClock.sleep(PREVIEW_TIME_MS);
        stopPreview(cameraId);
    }

    private final class SimpleImageListener implements ImageReader.OnImageAvailableListener {
        private final ConditionVariable imageAvailable = new ConditionVariable();
        @Override
        public void onImageAvailable(ImageReader reader) {
            imageAvailable.open();
        }

        public void waitForAnyImageAvailable(long timeout) {
            if (imageAvailable.block(timeout)) {
                imageAvailable.close();
            } else {
                fail("wait for image available timed out after " + timeout + "ms");
            }
        }
    }
}
