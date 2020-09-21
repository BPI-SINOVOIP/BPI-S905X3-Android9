package com.android.cts.managedprofile;

import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import android.test.AndroidTestCase;

/**
 * Test wipeDataWithReason() has indeed shown the notification.
 * The function wipeDataWithReason() is called and executed in another test.
 */
public class WipeDataWithReasonVerificationTest extends AndroidTestCase {

    private static final String WIPE_DATA_TITLE = "Work profile deleted";
    // This reason string should be aligned with the one in WipeDataTest as this is a followup test.
    private static final String TEST_WIPE_DATA_REASON = "cts test for WipeDataWithReason";
    private static final long UI_TIMEOUT_MILLI = 5000;

    public void testWipeDataWithReasonVerification() throws UiObjectNotFoundException,
            InterruptedException {
        // The function wipeDataWithReason() is called and executed in another test.
        // The data should be wiped and notification should be sent before.
        UiDevice uiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        uiDevice.openNotification();
        Boolean wipeDataTitleExist = uiDevice.wait(Until.hasObject(By.text(WIPE_DATA_TITLE)),
                UI_TIMEOUT_MILLI);
        Boolean wipeDataReasonExist = uiDevice.wait(Until.hasObject(By.text(TEST_WIPE_DATA_REASON)),
                UI_TIMEOUT_MILLI);

        // Verify the notification is there.
        assertEquals("Wipe notification title not found", Boolean.TRUE, wipeDataTitleExist);
        assertEquals("Wipe notification content not found",
                Boolean.TRUE, wipeDataReasonExist);
    }
}
