/*
 * Copyright (C) 2018 The Android Open Source Project
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
package android.os.cts.batterysaving;

import static android.os.cts.batterysaving.common.Values.APP_25_PACKAGE;
import static android.os.cts.batterysaving.common.Values.getRandomInt;

import static com.android.compatibility.common.util.AmUtils.runKill;
import static com.android.compatibility.common.util.AmUtils.runMakeUidIdle;
import static com.android.compatibility.common.util.BatteryUtils.enableBatterySaver;
import static com.android.compatibility.common.util.BatteryUtils.runDumpsysBatteryUnplug;
import static com.android.compatibility.common.util.TestUtils.waitUntil;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload;
import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload.TestServiceRequest;
import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload.TestServiceRequest.StartServiceRequest;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.atomic.AtomicReference;

/**
 * Tests related to battery saver:
 *
 * atest $ANDROID_BUILD_TOP/cts/tests/tests/batterysaving/src/android/os/cts/batterysaving/BatterySaverBgServiceTest.java
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class BatterySaverBgServiceTest extends BatterySavingTestBase {
    private static final String TAG = "BatterySaverBgServiceTest";

    /**
     * Make sure BG services on pre-O apps can't be started when BS is on.
     */
    @Test
    public void testBgServiceThrottled() throws Exception {

        final String targetPackage = APP_25_PACKAGE;

        runDumpsysBatteryUnplug();

        enableBatterySaver(false);

        // Make sure a BG service on pre-O app can be started with BS off.
        {
            runMakeUidIdle(targetPackage);
            runKill(targetPackage);
            Thread.sleep(500);

            requestClearIntent(targetPackage);

            final String action = tryStartTestServiceAndReturnAction(targetPackage, false);

            assertEquals(action, waitForLastIntentAction(targetPackage));
        }

        // Enable battery saver.
        enableBatterySaver(true);
        waitUntilForceBackgroundCheck(true);

        // Make sure a BG service on pre-O app *cannot* be started with BS on.
        {
            // Do the same thing again.
            runMakeUidIdle(targetPackage);
            runKill(targetPackage);
            Thread.sleep(500);

            requestClearIntent(targetPackage);

            assertNull(requestLastIntent(targetPackage));

            tryStartTestServiceAndReturnAction(targetPackage, false);

            // Wait a little bit and make sure the service didn't start.
            Thread.sleep(5000);

            assertNull(requestLastIntent(targetPackage));
        }

        // Make sure an FG service on pre-O app *can* be started with BS on.
        {
            runMakeUidIdle(targetPackage);
            runKill(targetPackage);
            Thread.sleep(500);

            requestClearIntent(targetPackage);

            final String action = tryStartTestServiceAndReturnAction(targetPackage, true);

            assertEquals(action, waitForLastIntentAction(targetPackage));
        }
    }

    /** Ask to clear the last received intent in the test service. */
    private void requestClearIntent(String targetPackage) throws Exception {
        final Payload response = mRpc.sendRequest(targetPackage,
                Payload.newBuilder().setTestServiceRequest(
                        TestServiceRequest.newBuilder().setClearLastIntent(true))
                        .build());
        assertTrue(response.hasTestServiceResponse()
                && response.getTestServiceResponse().getClearLastIntentAck());
    }

    /** Get the last received intent in the test service. */
    private String requestLastIntent(String targetPackage) throws Exception {
        final Payload response = mRpc.sendRequest(targetPackage,
                Payload.newBuilder().setTestServiceRequest(
                        TestServiceRequest.newBuilder().setGetLastIntent(true))
                        .build());
        assertTrue(response.hasTestServiceResponse());

        return response.getTestServiceResponse().hasGetLastIntentAction()
                ? response.getTestServiceResponse().getGetLastIntentAction()
                : null;
    }

    /** Wait until the last intent action is non-null. */
    private String waitForLastIntentAction(String targetPackage) throws Exception {
        final AtomicReference<String> result = new AtomicReference<>();
        waitUntil("Service didn't start", () -> {
            String action = requestLastIntent(targetPackage);
            if (action != null) {
                result.set(action);
                return true;
            }
            return false;
        });
        return result.get();
    }

    private String tryStartTestServiceAndReturnAction(String targetPackage, boolean foreground)
            throws Exception {
        final String action = "start_service_" + getRandomInt() + "_fg=" + foreground;

        final Payload response = mRpc.sendRequest(targetPackage,
                Payload.newBuilder().setTestServiceRequest(
                        TestServiceRequest.newBuilder().setStartService(
                            StartServiceRequest.newBuilder()
                                    .setForeground(foreground)
                                    .setAction(action).build()
                        )).build());
        assertTrue(response.hasTestServiceResponse()
                && response.getTestServiceResponse().getStartServiceAck());
        return action;
    }
}
