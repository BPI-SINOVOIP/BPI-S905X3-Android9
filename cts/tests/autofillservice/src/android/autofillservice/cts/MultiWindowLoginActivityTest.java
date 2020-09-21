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
package android.autofillservice.cts;

import static android.app.ActivityManager.SPLIT_SCREEN_CREATE_MODE_TOP_OR_LEFT;
import static android.autofillservice.cts.Helper.ID_PASSWORD;
import static android.autofillservice.cts.Helper.ID_USERNAME;
import static android.autofillservice.cts.common.ShellHelper.runShellCommand;
import static android.autofillservice.cts.common.ShellHelper.tap;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assume.assumeTrue;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Intent;
import android.platform.test.annotations.AppModeFull;
import android.view.View;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

import java.util.concurrent.TimeoutException;

@AppModeFull // This test requires android.permission.MANAGE_ACTIVITY_STACKS
public class MultiWindowLoginActivityTest extends AutoFillServiceTestCase {

    @Rule
    public final AutofillActivityTestRule<MultiWindowLoginActivity> mActivityRule =
            new AutofillActivityTestRule<MultiWindowLoginActivity>(MultiWindowLoginActivity.class);

    private LoginActivity mActivity;
    protected ActivityManager mAm;

    @Before
    public void setActivity() {
        mActivity = mActivityRule.getActivity();
    }

    @Before
    public void setup() {
        assumeTrue("Skipping test: no split multi-window support",
                ActivityManager.supportsSplitScreenMultiWindow(mContext));
        mAm = mContext.getSystemService(ActivityManager.class);
    }

    /**
     * Touch a view and exepct autofill window change
     */
    protected void tapViewAndExpectWindowEvent(View view) throws TimeoutException {
        mUiBot.waitForWindowChange(() -> tap(view), Timeouts.UI_TIMEOUT.getMaxValue());
    }


    protected String runAmStartActivity(Class<? extends Activity> activityClass, int flags) {
        return runAmStartActivity(activityClass.getName(), flags);
    }

    protected String runAmStartActivity(String activity, int flags) {
        return runShellCommand("am start %s/%s -f 0x%s", mPackageName, activity,
                Integer.toHexString(flags));
    }

    /**
     * Put activity1 in TOP, will be followed by amStartActivity()
     */
    protected void splitWindow(Activity activity1) {
        mAm.setTaskWindowingModeSplitScreenPrimary(activity1.getTaskId(),
                SPLIT_SCREEN_CREATE_MODE_TOP_OR_LEFT, true, false, null, true);

    }

    protected void amStartActivity(Class<? extends Activity> activity2) {
        // it doesn't work using startActivity(intent), have to go through shell command.
        runAmStartActivity(activity2,
                Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_LAUNCH_ADJACENT);
    }

    @Test
    public void testSplitWindow() throws Exception {
        enableService();

        // Set expectations.
        sReplier.addResponse(new CannedFillResponse.CannedDataset.Builder()
                .setField(ID_USERNAME, "dude")
                .setField(ID_PASSWORD, "sweet")
                .setPresentation(createPresentation("The Dude"))
                .build());
        // Trigger auto-fill.
        mActivity.onUsername(View::requestFocus);
        sReplier.getNextFillRequest();
        mUiBot.assertDatasets("The Dude");

        // split window and launch EmptyActivity, note that LoginActivity will be recreated.
        MultiWindowLoginActivity.expectNewInstance(false);
        MultiWindowEmptyActivity.expectNewInstance(true);

        splitWindow(mActivity);
        MultiWindowLoginActivity loginActivity = MultiWindowLoginActivity.waitNewInstance();

        amStartActivity(MultiWindowEmptyActivity.class);
        MultiWindowEmptyActivity emptyActivity = MultiWindowEmptyActivity.waitNewInstance();

        // No dataset as LoginActivity loses window focus
        mUiBot.assertNoDatasets();
        // EmptyActivity will have window focus
        assertThat(emptyActivity.hasWindowFocus()).isTrue();
        // LoginActivity username field is still focused but window has no focus
        assertThat(loginActivity.getUsername().hasFocus()).isTrue();
        assertThat(loginActivity.hasWindowFocus()).isFalse();

        // Make LoginActivity to regain window focus and fill ui is expected to show
        tapViewAndExpectWindowEvent(loginActivity.getUsernameLabel());
        mUiBot.assertDatasets("The Dude");
        assertThat(emptyActivity.hasWindowFocus()).isFalse();

        // Tap on EmptyActivity and fill ui is gone.
        tapViewAndExpectWindowEvent(emptyActivity.getEmptyView());
        mUiBot.assertNoDatasets();
        assertThat(emptyActivity.hasWindowFocus()).isTrue();
        // LoginActivity username field is still focused but window has no focus
        assertThat(loginActivity.getUsername().hasFocus()).isTrue();
        assertThat(loginActivity.hasWindowFocus()).isFalse();
    }
}
