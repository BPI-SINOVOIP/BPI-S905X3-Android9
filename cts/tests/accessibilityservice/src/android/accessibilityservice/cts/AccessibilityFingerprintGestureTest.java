/**
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.accessibilityservice.cts;

import static android.content.pm.PackageManager.FEATURE_FINGERPRINT;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;

import android.accessibilityservice.FingerprintGestureController;
import android.accessibilityservice.FingerprintGestureController.FingerprintGestureCallback;
import android.accessibilityservice.cts.activities.AccessibilityEndToEndActivity;
import android.app.Instrumentation;
import android.hardware.fingerprint.FingerprintManager;
import android.os.CancellationSignal;
import android.platform.test.annotations.AppModeFull;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/**
 * Verify that a service listening for fingerprint gestures gets called back when apps
 * use the fingerprint sensor to authenticate.
 */
@AppModeFull
@RunWith(AndroidJUnit4.class)
public class AccessibilityFingerprintGestureTest {
    private static final int FINGERPRINT_CALLBACK_TIMEOUT = 3000;

    FingerprintManager mFingerprintManager;
    StubFingerprintGestureService mFingerprintGestureService;
    FingerprintGestureController mFingerprintGestureController;
    CancellationSignal mCancellationSignal = new CancellationSignal();

    @Rule
    public ActivityTestRule<AccessibilityEndToEndActivity> mActivityRule =
            new ActivityTestRule<>(AccessibilityEndToEndActivity.class, false, false);


    @Mock FingerprintManager.AuthenticationCallback mMockAuthenticationCallback;
    @Mock FingerprintGestureCallback mMockFingerprintGestureCallback;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        mFingerprintManager = instrumentation.getContext().getPackageManager()
                .hasSystemFeature(FEATURE_FINGERPRINT)
                ? instrumentation.getContext().getSystemService(FingerprintManager.class) : null;
        mFingerprintGestureService = StubFingerprintGestureService.enableSelf(instrumentation);
        mFingerprintGestureController =
                mFingerprintGestureService.getFingerprintGestureController();
    }

    @After
    public void tearDown() throws Exception {
        mFingerprintGestureService.runOnServiceSync(() -> mFingerprintGestureService.disableSelf());
    }

    @Test
    public void testGestureDetectionListener_whenAuthenticationStartsAndStops_calledBack() {
        if (!mFingerprintGestureController.isGestureDetectionAvailable()) {
            return;
        }
        // Launch an activity to make sure we're in the foreground
        mActivityRule.launchActivity(null);
        mFingerprintGestureController.registerFingerprintGestureCallback(
                mMockFingerprintGestureCallback, null);
        try {
            mFingerprintManager.authenticate(
                    null, mCancellationSignal, 0, mMockAuthenticationCallback, null);

            verify(mMockFingerprintGestureCallback,
                    timeout(FINGERPRINT_CALLBACK_TIMEOUT).atLeastOnce())
                    .onGestureDetectionAvailabilityChanged(false);
            assertFalse(mFingerprintGestureController.isGestureDetectionAvailable());
            reset(mMockFingerprintGestureCallback);
        } finally {
            mCancellationSignal.cancel();
        }
        verify(mMockFingerprintGestureCallback, timeout(FINGERPRINT_CALLBACK_TIMEOUT).atLeastOnce())
                .onGestureDetectionAvailabilityChanged(true);
        assertTrue(mFingerprintGestureController.isGestureDetectionAvailable());
        mFingerprintGestureController.unregisterFingerprintGestureCallback(
                mMockFingerprintGestureCallback);
    }
}
