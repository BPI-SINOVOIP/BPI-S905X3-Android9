package android.server.am.lifecycle;

import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_STOP;

import android.app.Activity;
import android.content.Intent;
import android.platform.test.annotations.Presubmit;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityLifecycleKeyguardTests
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
@Presubmit
public class ActivityLifecycleKeyguardTests extends ActivityLifecycleClientTestBase {

    @Test
    public void testSingleLaunch() throws Exception {
        try (final LockScreenSession lockScreenSession = new LockScreenSession()) {
            lockScreenSession.setLockCredential().gotoKeyguard();

            final Activity activity = mFirstActivityTestRule.launchActivity(new Intent());
            waitAndAssertActivityStates(state(activity, ON_STOP));

            LifecycleVerifier.assertLaunchAndStopSequence(FirstActivity.class, getLifecycleLog());
        }
    }
}
