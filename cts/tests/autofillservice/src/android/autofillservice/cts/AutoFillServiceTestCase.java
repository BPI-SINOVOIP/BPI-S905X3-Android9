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

package android.autofillservice.cts;

import static android.autofillservice.cts.Helper.getContext;
import static android.autofillservice.cts.Helper.getLoggingLevel;
import static android.autofillservice.cts.Helper.hasAutofillFeature;
import static android.autofillservice.cts.Helper.setLoggingLevel;
import static android.autofillservice.cts.InstrumentedAutoFillService.SERVICE_NAME;
import static android.autofillservice.cts.common.ShellHelper.runShellCommand;

import android.autofillservice.cts.InstrumentedAutoFillService.Replier;
import android.autofillservice.cts.common.SettingsStateKeeperRule;
import android.content.Context;
import android.content.pm.PackageManager;
import android.provider.Settings;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;
import android.widget.RemoteViews;

import com.android.compatibility.common.util.RequiredFeatureRule;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.rules.TestWatcher;
import org.junit.runner.Description;
import org.junit.runner.RunWith;

/**
 * Base class for all other tests.
 */
@RunWith(AndroidJUnit4.class)
// NOTE: @ClassRule requires it to be public
public abstract class AutoFillServiceTestCase {
    private static final String TAG = "AutoFillServiceTestCase";

    static final UiBot sDefaultUiBot = new UiBot();

    protected static final Replier sReplier = InstrumentedAutoFillService.getReplier();

    private static final Context sContext = InstrumentationRegistry.getTargetContext();

    @ClassRule
    public static final SettingsStateKeeperRule mServiceSettingsKeeper =
            new SettingsStateKeeperRule(sContext, Settings.Secure.AUTOFILL_SERVICE);

    @Rule
    public final TestWatcher watcher = new TestWatcher() {
        @Override
        protected void starting(Description description) {
            JUnitHelper.setCurrentTestName(description.getDisplayName());
        }

        @Override
        protected void finished(Description description) {
            JUnitHelper.setCurrentTestName(null);
        }
    };

    @Rule
    public final RetryRule mRetryRule = new RetryRule(2);

    @Rule
    public final AutofillLoggingTestRule mLoggingRule = new AutofillLoggingTestRule(TAG);

    @Rule
    public final RequiredFeatureRule mRequiredFeatureRule =
            new RequiredFeatureRule(PackageManager.FEATURE_AUTOFILL);

    @Rule
    public final SafeCleanerRule mSafeCleanerRule = new SafeCleanerRule()
            .setDumper(mLoggingRule)
            .run(() -> sReplier.assertNoUnhandledFillRequests())
            .run(() -> sReplier.assertNoUnhandledSaveRequests())
            .add(() -> { return sReplier.getExceptions(); });

    protected final Context mContext = sContext;
    protected final String mPackageName;
    protected final UiBot mUiBot;

    /**
     * Stores the previous logging level so it's restored after the test.
     */
    private String mLoggingLevel;

    protected AutoFillServiceTestCase() {
        this(sDefaultUiBot);
    }

    protected AutoFillServiceTestCase(UiBot uiBot) {
        mPackageName = mContext.getPackageName();
        mUiBot = uiBot;
        mUiBot.reset();
    }

    @BeforeClass
    public static void prepareScreen() throws Exception {
        if (!hasAutofillFeature()) return;

        // Unlock screen.
        runShellCommand("input keyevent KEYCODE_WAKEUP");

        // Collapse notifications.
        runShellCommand("cmd statusbar collapse");

        // Set orientation as portrait, otherwise some tests might fail due to elements not fitting
        // in, IME orientation, etc...
        sDefaultUiBot.setScreenOrientation(UiBot.PORTRAIT);
    }

    @Before
    public void cleanupStaticState() {
        Helper.preTestCleanup();
        sReplier.reset();
    }

    @Before
    public void setVerboseLogging() {
        try {
            mLoggingLevel = getLoggingLevel();
        } catch (Exception e) {
            Log.w(TAG, "Could not get previous logging level: " + e);
            mLoggingLevel = "debug";
        }
        try {
            setLoggingLevel("verbose");
        } catch (Exception e) {
            Log.w(TAG, "Could not change logging level to verbose: " + e);
        }
    }

    /**
     * Cleans up activities that might have been left over.
     */
    @Before
    @After
    public void finishActivities() {
        WelcomeActivity.finishIt(mUiBot);
    }

    @After
    public void resetVerboseLogging() {
        try {
            setLoggingLevel(mLoggingLevel);
        } catch (Exception e) {
            Log.w(TAG, "Could not restore logging level to " + mLoggingLevel + ": " + e);
        }
    }

    @After
    public void ignoreFurtherRequests() {
        InstrumentedAutoFillService.setIgnoreUnexpectedRequests(true);
    }

    /**
     * Enables the {@link InstrumentedAutoFillService} for autofill for the current user.
     */
    protected void enableService() {
        Helper.enableAutofillService(getContext(), SERVICE_NAME);
        InstrumentedAutoFillService.setIgnoreUnexpectedRequests(false);
    }

    /**
     * Disables the {@link InstrumentedAutoFillService} for autofill for the current user.
     */
    protected void disableService() {
        if (!hasAutofillFeature()) return;

        Helper.disableAutofillService(getContext(), SERVICE_NAME);
        InstrumentedAutoFillService.setIgnoreUnexpectedRequests(true);
    }

    /**
     * Asserts that the {@link InstrumentedAutoFillService} is enabled for the default user.
     */
    protected void assertServiceEnabled() {
        Helper.assertAutofillServiceStatus(SERVICE_NAME, true);
    }

    /**
     * Asserts that the {@link InstrumentedAutoFillService} is disabled for the default user.
     */
    protected void assertServiceDisabled() {
        Helper.assertAutofillServiceStatus(SERVICE_NAME, false);
    }

    protected RemoteViews createPresentation(String message) {
        final RemoteViews presentation = new RemoteViews(getContext()
                .getPackageName(), R.layout.list_item);
        presentation.setTextViewText(R.id.text1, message);
        return presentation;
    }
}
