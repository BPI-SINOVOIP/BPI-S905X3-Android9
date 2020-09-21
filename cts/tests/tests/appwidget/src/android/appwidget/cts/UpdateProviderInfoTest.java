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

package android.appwidget.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import android.appwidget.AppWidgetHost;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProviderInfo;
import android.appwidget.cts.common.Constants;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.platform.test.annotations.AppModeFull;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@AppModeFull(reason = "Instant apps cannot provide or host app widgets")
public class UpdateProviderInfoTest extends AppWidgetTestCase {

    private static final String PROVIDER_PACKAGE = "android.appwidget.cts.widgetprovider";
    private static final String PROVIDER_CLASS = "android.appwidget.cts.packages.SimpleProvider";

    private static final String APK_PATH = "data/local/tmp/cts/widgetprovider/";
    private static final String APK_V1 = APK_PATH + "CtsAppWidgetProvider1.apk";
    private static final String APK_V2 = APK_PATH + "CtsAppWidgetProvider2.apk";
    private static final String APK_V3 = APK_PATH + "CtsAppWidgetProvider3.apk";

    private static final String EXTRA_CUSTOM_INFO = "my_custom_info";

    private static final int HOST_ID = 42;

    private static final int RETRY_COUNT = 3;

    private CountDownLatch mProviderChangeNotifier;
    AppWidgetHost mHost;

    @Before
    public void setUpProvider() throws Exception {
        uninstallProvider();
        createHost();
    }

    @After
    public void tearDownProvider() throws Exception {
        uninstallProvider();

        if (mHost != null) {
            mHost.deleteHost();
        }
    }

    @Test
    public void testInfoOverrides() throws Throwable {
        // On first install the provider does not have any config activity.
        installApk(APK_V1);
        assertNull(getProviderInfo().configure);

        // The provider info is updated
        updateInfo(EXTRA_CUSTOM_INFO);
        assertNotNull(getProviderInfo().configure);

        // The provider info is updated
        updateInfo(null);
        assertNull(getProviderInfo().configure);
    }

    @Test
    public void testOverridesPersistedOnUpdate() throws Exception {
        installApk(APK_V1);
        assertNull(getProviderInfo().configure);

        updateInfo(EXTRA_CUSTOM_INFO);
        assertNotNull(getProviderInfo().configure);
        assertEquals((AppWidgetProviderInfo.RESIZE_BOTH & getProviderInfo().resizeMode),
                AppWidgetProviderInfo.RESIZE_BOTH);

        // Apk updated, the info is also updated
        installApk(APK_V2);
        assertNotNull(getProviderInfo().configure);
        assertEquals((AppWidgetProviderInfo.RESIZE_BOTH & getProviderInfo().resizeMode), 0);

        // The provider info is reverted
        updateInfo(null);
        assertNull(getProviderInfo().configure);
    }

    @Test
    public void testOverrideClearedWhenMissingInfo() throws Exception {
        installApk(APK_V1);
        assertNull(getProviderInfo().configure);

        updateInfo(EXTRA_CUSTOM_INFO);
        assertNotNull(getProviderInfo().configure);

        // V3 does not have the custom info definition
        installApk(APK_V3);
        assertNull(getProviderInfo().configure);
    }

    private void createHost() throws Exception {
        try {
            (new Handler(Looper.getMainLooper())).post(() -> {
                mHost = new AppWidgetHost(getInstrumentation().getTargetContext(), HOST_ID) {

                    @Override
                    protected void onProvidersChanged() {
                        super.onProvidersChanged();

                        if (mProviderChangeNotifier != null) {
                            mProviderChangeNotifier.countDown();
                        }
                    }
                };
                mHost.startListening();
            });
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    private void updateInfo(String key) throws Exception {
        mProviderChangeNotifier = new CountDownLatch(1);
        Intent intent = new Intent(Constants.ACTION_APPLY_OVERRIDE)
                .setComponent(new ComponentName(PROVIDER_PACKAGE, PROVIDER_CLASS))
                .addFlags(Intent.FLAG_RECEIVER_FOREGROUND)
                .putExtra(Constants.EXTRA_REQUEST, key);
        getInstrumentation().getTargetContext().sendBroadcast(intent);

        // Wait until the app widget manager is notified
        mProviderChangeNotifier.await();
    }

    private void uninstallProvider() throws Exception {
        runShellCommand("pm uninstall " + PROVIDER_PACKAGE);
    }

    private void installApk(String path) throws Exception {
        mProviderChangeNotifier = new CountDownLatch(1);
        runShellCommand("pm install -r -d " + path);

        // Wait until the app widget manager is notified
        mProviderChangeNotifier.await();
    }

    private AppWidgetProviderInfo getProviderInfo() throws Exception {
        for (int i = 0; i < RETRY_COUNT; i++) {
            mProviderChangeNotifier = new CountDownLatch(1);
            List<AppWidgetProviderInfo> providers = AppWidgetManager.getInstance(getInstrumentation()
                    .getTargetContext()).getInstalledProvidersForPackage(
                    PROVIDER_PACKAGE, Process.myUserHandle());

            if (providers != null && !providers.isEmpty()) {
                return providers.get(0);
            }

            // Sometimes it could take time for the info to appear after the apk is just installed
            mProviderChangeNotifier.await(2, TimeUnit.SECONDS);
        }
        return null;
    }
}
