/*
 * Copyright (C) 2017 The Android Open Source Project
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

import static org.junit.Assert.fail;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.platform.test.annotations.AppModeFull;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.SystemUtil;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

import java.io.IOException;

/**
 * Test for validating behaviors related to idle UIDs. Idle UIDs cannot
 * access camera. If the UID has a camera handle and becomes idle it would
 * get an error callback losing the camera handle. Similarly if the UID is
 * already idle it cannot obtain a camera handle.
 */
@AppModeFull
@RunWith(AndroidJUnit4.class)
public final class IdleUidTest {
    private static final long CAMERA_OPERATION_TIMEOUT_MILLIS = 5000; // 5 sec

    private static final HandlerThread sCallbackThread = new HandlerThread("Callback thread");

    @BeforeClass
    public static void startHandlerThread() {
        sCallbackThread.start();
    }

    @AfterClass
    public static void stopHandlerThread() {
        sCallbackThread.quit();
    }

    /**
     * Tests that a UID has access to the camera only in active state.
     */
    @Test
    public void testCameraAccessForIdleUid() throws Exception {
        final CameraManager cameraManager = InstrumentationRegistry.getTargetContext()
                .getSystemService(CameraManager.class);
        for (String cameraId : cameraManager.getCameraIdList()) {
            testCameraAccessForIdleUidByCamera(cameraManager, cameraId,
                    new Handler(sCallbackThread.getLooper()));
        }
    }

    /**
     * Tests that a UID loses access to the camera if it becomes inactive.
     */
    @Test
    public void testCameraAccessBecomingInactiveUid() throws Exception {
        final CameraManager cameraManager = InstrumentationRegistry.getTargetContext()
                .getSystemService(CameraManager.class);
        for (String cameraId : cameraManager.getCameraIdList()) {
            testCameraAccessBecomingInactiveUidByCamera(cameraManager, cameraId,
                    new Handler(sCallbackThread.getLooper()));
        }

    }

    private void testCameraAccessForIdleUidByCamera(CameraManager cameraManager,
            String cameraId, Handler handler) throws Exception {
        // Can access camera from an active UID.
        assertCameraAccess(cameraManager, cameraId, true, handler);

        // Make our UID idle
        makeMyPackageIdle();
        try {
            // Can not access camera from an idle UID.
            assertCameraAccess(cameraManager, cameraId, false, handler);
        } finally {
            // Restore our UID as active
            makeMyPackageActive();
        }

        // Can access camera from an active UID.
        assertCameraAccess(cameraManager, cameraId, true, handler);
    }

    private static void assertCameraAccess(CameraManager cameraManager,
            String cameraId, boolean hasAccess, Handler handler) {
        // Mock the callback used to observe camera state.
        final CameraDevice.StateCallback callback = mock(CameraDevice.StateCallback.class);

        // Open the camera
        try {
            cameraManager.openCamera(cameraId, callback, handler);
        } catch (CameraAccessException e) {
            if (hasAccess) {
                fail("Unexpected exception" + e);
            } else {
                assertThat(e.getReason()).isSameAs(CameraAccessException.CAMERA_DISABLED);
            }
        }

        // Verify access
        final ArgumentCaptor<CameraDevice> captor = ArgumentCaptor.forClass(CameraDevice.class);
        try {
            if (hasAccess) {
                // The camera should open fine as we are in the foreground
                verify(callback, timeout(CAMERA_OPERATION_TIMEOUT_MILLIS)
                        .times(1)).onOpened(captor.capture());
                verifyNoMoreInteractions(callback);
            } else {
                // The camera should not open as we are in the background
                verify(callback, timeout(CAMERA_OPERATION_TIMEOUT_MILLIS)
                        .times(1)).onError(captor.capture(),
                        eq(CameraDevice.StateCallback.ERROR_CAMERA_DISABLED));
                verifyNoMoreInteractions(callback);
            }
        } finally {
            final CameraDevice cameraDevice = captor.getValue();
            assertThat(cameraDevice).isNotNull();
            cameraDevice.close();
        }
    }

    private void testCameraAccessBecomingInactiveUidByCamera(CameraManager cameraManager,
            String cameraId, Handler handler) throws Exception {
        // Mock the callback used to observe camera state.
        final CameraDevice.StateCallback callback = mock(CameraDevice.StateCallback.class);

        // Open the camera
        try {
            cameraManager.openCamera(cameraId, callback, handler);
        } catch (CameraAccessException e) {
            fail("Unexpected exception" + e);
        }

        // Verify access
        final ArgumentCaptor<CameraDevice> captor = ArgumentCaptor.forClass(CameraDevice.class);
        try {
            // The camera should open fine as we are in the foreground
            verify(callback, timeout(CAMERA_OPERATION_TIMEOUT_MILLIS)
                    .times(1)).onOpened(captor.capture());
            verifyNoMoreInteractions(callback);

            // Ready for a new verification
            reset(callback);

            // Now we are moving in the background
            makeMyPackageIdle();

            // The camera should be closed if the UID became idle
            verify(callback, timeout(CAMERA_OPERATION_TIMEOUT_MILLIS)
                    .times(1)).onError(captor.capture(),
                    eq(CameraDevice.StateCallback.ERROR_CAMERA_DISABLED));
            verifyNoMoreInteractions(callback);
        } finally {
            // Restore to active state
            makeMyPackageActive();

            final CameraDevice cameraDevice = captor.getValue();
            assertThat(cameraDevice).isNotNull();
            cameraDevice.close();
        }
    }

    private static void makeMyPackageActive() throws IOException {
        final String command = "cmd media.camera reset-uid-state "
                +  InstrumentationRegistry.getTargetContext().getPackageName();
        SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(), command);
    }

    private static void makeMyPackageIdle() throws IOException {
        final String command = "cmd media.camera set-uid-state "
                + InstrumentationRegistry.getTargetContext().getPackageName() + " idle";
        SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(), command);
    }
}
