/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.cts.deviceowner;


import android.app.admin.DevicePolicyManager;
import android.app.admin.FreezePeriod;
import android.app.admin.SystemUpdatePolicy;
import android.app.admin.SystemUpdatePolicy.ValidationFailedException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.icu.util.Calendar;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.util.Pair;

import java.time.LocalDate;
import java.time.MonthDay;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Test {@link SystemUpdatePolicy}, {@link DevicePolicyManager#setSystemUpdatePolicy} and
 * {@link DevicePolicyManager#getSystemUpdatePolicy}
 */
public class SystemUpdatePolicyTest extends BaseDeviceOwnerTest {

    private static final int TIMEOUT_MS = 20_000;

    private final Semaphore mPolicyChangedSemaphore = new Semaphore(0);
    private final Semaphore mTimeChangedSemaphore = new Semaphore(0);
    private final BroadcastReceiver policyChangedReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (DevicePolicyManager.ACTION_SYSTEM_UPDATE_POLICY_CHANGED.equals(action)) {
                mPolicyChangedSemaphore.release();
            } else if (Intent.ACTION_TIME_CHANGED.equals(action)) {
                mTimeChangedSemaphore.release();
            }
        }
    };

    private int mSavedAutoTimeConfig;
    private LocalDate mSavedSystemDate;
    private boolean mRestoreDate;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        IntentFilter filter = new IntentFilter();
        filter.addAction(DevicePolicyManager.ACTION_SYSTEM_UPDATE_POLICY_CHANGED);
        filter.addAction(Intent.ACTION_TIME_CHANGED);
        mContext.registerReceiver(policyChangedReceiver, filter);
        clearFreezeRecord();
        mSavedAutoTimeConfig = Settings.Global.getInt(mContext.getContentResolver(),
                Global.AUTO_TIME, 0);
        executeShellCommand("settings put global auto_time 0");
        mSavedSystemDate = LocalDate.now();
        mRestoreDate = false;
    }

    @Override
    protected void tearDown() throws Exception {
        mDevicePolicyManager.setSystemUpdatePolicy(getWho(), null);
        clearFreezeRecord();
        if (mRestoreDate) {
            setSystemDate(mSavedSystemDate);
        }
        executeShellCommand("settings put global auto_time",
                Integer.toString(mSavedAutoTimeConfig));
        // This needs to happen last since setSystemDate() relies on the receiver for
        // synchronization.
        mContext.unregisterReceiver(policyChangedReceiver);
        super.tearDown();
    }

    public void testSetEmptytInstallPolicy() {
        testPolicy(null);
    }

    public void testSetAutomaticInstallPolicy() {
        testPolicy(SystemUpdatePolicy.createAutomaticInstallPolicy());
    }

    public void testSetWindowedInstallPolicy() {
        testPolicy(SystemUpdatePolicy.createWindowedInstallPolicy(0, 720));
    }

    public void testSetPostponeInstallPolicy() {
        testPolicy(SystemUpdatePolicy.createPostponeInstallPolicy());
    }

    public void testShouldFailInvalidWindowPolicy() throws Exception {
        try {
            SystemUpdatePolicy.createWindowedInstallPolicy(24 * 60 + 1, 720);
            fail("Invalid window start should not be accepted.");
        } catch (IllegalArgumentException expected) { }
        try {
            SystemUpdatePolicy.createWindowedInstallPolicy(-1, 720);
            fail("Invalid window start should not be accepted.");
        } catch (IllegalArgumentException expected) { }
        try {
            SystemUpdatePolicy.createWindowedInstallPolicy(0, 24 * 60 + 1);
            fail("Invalid window end should not be accepted.");
        } catch (IllegalArgumentException expected) { }
        try {
            SystemUpdatePolicy.createWindowedInstallPolicy(0, -1);
            fail("Invalid window end should not be accepted.");
        } catch (IllegalArgumentException expected) { }
    }

    public void testFreezePeriodValidation() {
        // Dates are in MM-DD format
        validateFreezePeriodsSucceeds("01-01", "01-02");
        validateFreezePeriodsSucceeds("01-31", "01-31");
        validateFreezePeriodsSucceeds("11-01", "01-15");
        validateFreezePeriodsSucceeds("02-01", "02-29");
        validateFreezePeriodsSucceeds("03-01", "03-31", "09-01", "09-30");
        validateFreezePeriodsSucceeds("10-01", "10-31", "12-31", "01-31");
        validateFreezePeriodsSucceeds("01-01", "02-28", "05-01", "06-30", "09-01", "10-31");
        validateFreezePeriodsSucceeds("11-02", "01-15", "03-18", "04-30", "08-01", "08-30");

        // full overlap
        validateFreezePeriodsFailsOverlap("12-01", "01-31", "12-25", "01-15");
        // partial overlap
        validateFreezePeriodsFailsOverlap("03-01", "03-31", "03-15", "01-01");
        // touching interval
        validateFreezePeriodsFailsOverlap("01-31", "01-31", "02-01", "02-01");
        validateFreezePeriodsFailsOverlap("12-01", "12-31", "04-01", "04-01", "01-01", "01-30");

        // entire year
        validateFreezePeriodsFailsTooLong("01-01", "12-31");
        // Regular long period
        validateFreezePeriodsSucceeds("01-01", "03-31", "06-01", "08-29");
        validateFreezePeriodsFailsTooLong("01-01", "03-31", "06-01", "08-30");
        // long period spanning across year end
        validateFreezePeriodsSucceeds("11-01", "01-29");
        validateFreezePeriodsFailsTooLong("11-01", "01-30");
        // Leap year handling
        validateFreezePeriodsSucceeds("12-01", "02-28");
        validateFreezePeriodsFailsTooLong("12-01", "03-01");

        // Regular short separation
        validateFreezePeriodsFailsTooClose( "01-01", "01-01", "01-03", "01-03");
        // Short interval spans across end of year
        validateFreezePeriodsSucceeds("01-31", "03-01", "11-01", "12-01");
        validateFreezePeriodsFailsTooClose("01-30", "03-01", "11-01", "12-01");
        // Short separation is after wrapped period
        validateFreezePeriodsSucceeds("03-03", "03-31", "12-31", "01-01");
        validateFreezePeriodsFailsTooClose("03-02", "03-31", "12-31", "01-01");
        // Short separation including Feb 29
        validateFreezePeriodsSucceeds("12-01", "01-15", "03-17", "04-01");
        validateFreezePeriodsFailsTooClose("12-01", "01-15", "03-16", "04-01");
        // Short separation including Feb 29
        validateFreezePeriodsSucceeds("01-01", "02-28", "04-30", "06-01");
        validateFreezePeriodsSucceeds("01-01", "02-29", "04-30", "06-01");
        validateFreezePeriodsFailsTooClose("01-01", "03-01", "04-30", "06-01");
    }

    public void testFreezePeriodCanBeSetAndChanged() throws Exception {
        setPolicyWithFreezePeriod("11-02", "01-15", "03-18", "04-30");
        // Set to a different period should work
        setPolicyWithFreezePeriod("08-01", "08-30");
        // Clear freeze period should work
        setPolicyWithFreezePeriod();
        // Set to the original period should work
        setPolicyWithFreezePeriod("11-02", "01-15", "03-18", "04-30");
    }

    public void testFreezePeriodCannotSetIfTooCloseToPrevious() throws Exception {
        setSystemDate(LocalDate.of(2018, 2, 28));
        setPolicyWithFreezePeriod("01-01", "03-01", "06-01", "06-30");
        // Clear policy
        mDevicePolicyManager.setSystemUpdatePolicy(getWho(), null);
        // Set to a conflict period (too close with previous period [2-28, 2-28]) should fail,
        // despite the previous policy was cleared from the system just now.
        try {
            setPolicyWithFreezePeriod("04-29", "04-30");
            fail("Did no flag invalid period");
        } catch (ValidationFailedException e) {
            assertEquals(e.getMessage(),
                    ValidationFailedException.ERROR_COMBINED_FREEZE_PERIOD_TOO_CLOSE,
                    e.getErrorCode());
        }
        // This should succeed as the new freeze period is exactly 60 days away.
        setPolicyWithFreezePeriod("04-30", "04-30");
    }

    public void testFreezePeriodCannotSetIfTooLongWhenCombinedWithPrevious() throws Exception {
        setSystemDate(LocalDate.of(2012, 4, 1));
        setPolicyWithFreezePeriod("03-01", "05-01");
        setSystemDate(LocalDate.of(2012, 4, 30));
        // Despite the wait for broadcast in setSystemDate(), TIME_CHANGED broadcast is asynchronous
        // so give DevicePolicyManagerService more time to receive TIME_CHANGED and to update the
        // freeze period record.
        Thread.sleep(5000);
        // Set to a conflict period (too long when combined with previous period [04-01, 04-30])
        // should fail
        try {
            setPolicyWithFreezePeriod("04-30", "06-30");
            fail("Did no flag invalid period");
        } catch (SystemUpdatePolicy.ValidationFailedException e) {
            assertEquals(e.getMessage(),
                    ValidationFailedException.ERROR_COMBINED_FREEZE_PERIOD_TOO_LONG,
                    e.getErrorCode());
        }
        // This should succeed as the combined length (59 days) is just below threshold (90 days).
        setPolicyWithFreezePeriod("05-01", "06-29");
    }

    public void testFreezePeriodForOneYear() throws Exception {
        // Set a normal period every day for 365 days
        for (int i = 1; i <= 365; i++) {
            // Add two days so the test date range wraps around year-end
            setSystemDate(LocalDate.ofYearDay(2019, i).plusDays(2));
            testFreezePeriodCanBeSetAndChanged();
        }
    }

    private void testPolicy(SystemUpdatePolicy policy) {
        mDevicePolicyManager.setSystemUpdatePolicy(getWho(), policy);
        waitForPolicyChangedBroadcast();
        SystemUpdatePolicy newPolicy = mDevicePolicyManager.getSystemUpdatePolicy();
        if (policy == null) {
            assertNull(newPolicy);
        } else {
            assertNotNull(newPolicy);
            assertEquals(policy.toString(), newPolicy.toString());
            assertEquals(policy.getPolicyType(), newPolicy.getPolicyType());
            if (policy.getPolicyType() == SystemUpdatePolicy.TYPE_INSTALL_WINDOWED) {
                assertEquals(policy.getInstallWindowStart(), newPolicy.getInstallWindowStart());
                assertEquals(policy.getInstallWindowEnd(), newPolicy.getInstallWindowEnd());
            }
        }
    }

    private void setPolicyWithFreezePeriod(String...dates) {
        SystemUpdatePolicy policy = SystemUpdatePolicy.createPostponeInstallPolicy();
        setFreezePeriods(policy, dates);
        mDevicePolicyManager.setSystemUpdatePolicy(getWho(), policy);

        List<FreezePeriod> loadedFreezePeriods = mDevicePolicyManager
                .getSystemUpdatePolicy().getFreezePeriods();
        assertEquals(dates.length / 2, loadedFreezePeriods.size());
        for (int i = 0; i < dates.length; i += 2) {
            assertEquals(parseMonthDay(dates[i]), loadedFreezePeriods.get(i / 2).getStart());
            assertEquals(parseMonthDay(dates[i + 1]), loadedFreezePeriods.get(i / 2).getEnd());
        }
    }

    private void validateFreezePeriodsSucceeds(String...dates)  {
        SystemUpdatePolicy p = SystemUpdatePolicy.createPostponeInstallPolicy();
        setFreezePeriods(p, dates);
    }

    private void validateFreezePeriodsFails(int errorCode, String... dates)  {
        SystemUpdatePolicy p = SystemUpdatePolicy.createPostponeInstallPolicy();
        try {
            setFreezePeriods(p, dates);
            fail("Exception not thrown for dates: " + String.join(" ", dates));
        } catch (SystemUpdatePolicy.ValidationFailedException e) {
            assertEquals("Exception not expected: " + e.getMessage(),
                    errorCode,e.getErrorCode());
        }
    }

    private void validateFreezePeriodsFailsOverlap(String... dates)  {
        validateFreezePeriodsFails(ValidationFailedException.ERROR_DUPLICATE_OR_OVERLAP, dates);
    }

    private void validateFreezePeriodsFailsTooLong(String... dates)  {
        validateFreezePeriodsFails(ValidationFailedException.ERROR_NEW_FREEZE_PERIOD_TOO_LONG,
                dates);
    }

    private void validateFreezePeriodsFailsTooClose(String... dates)  {
        validateFreezePeriodsFails(ValidationFailedException.ERROR_NEW_FREEZE_PERIOD_TOO_CLOSE,
                dates);
    }

    //dates are in MM-DD format
    private void setFreezePeriods(SystemUpdatePolicy policy, String... dates) {
        List<FreezePeriod> periods = new ArrayList<>();
        for (int i = 0; i < dates.length; i+= 2) {
            periods.add(new FreezePeriod(parseMonthDay(dates[i]), parseMonthDay(dates[i + 1])));
        }
        policy.setFreezePeriods(periods);
    }

    private MonthDay parseMonthDay(String date) {
        return MonthDay.of(Integer.parseInt(date.substring(0, 2)),
                Integer.parseInt(date.substring(3, 5)));
    }

    private void clearFreezeRecord() throws Exception {
        executeShellCommand("dpm", "clear-freeze-period-record");
    }

    private void setSystemDate(LocalDate date) throws Exception {
        mRestoreDate = true;
        Calendar c = Calendar.getInstance();
        c.set(Calendar.YEAR, date.getYear());
        c.set(Calendar.MONTH, date.getMonthValue() - 1);
        c.set(Calendar.DAY_OF_MONTH, date.getDayOfMonth());
        mDevicePolicyManager.setTime(getWho(), c.getTimeInMillis());
        waitForTimeChangedBroadcast();
    }

    private void waitForPolicyChangedBroadcast() {
        try {
            assertTrue("Timeout while waiting for broadcast.",
                    mPolicyChangedSemaphore.tryAcquire(TIMEOUT_MS, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail("Interrupted while waiting for broadcast.");
        }
    }

    private void waitForTimeChangedBroadcast() {
        try {
            assertTrue("Timeout while waiting for broadcast.",
                    mTimeChangedSemaphore.tryAcquire(TIMEOUT_MS, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail("Interrupted while waiting for broadcast.");
        }
    }
}
