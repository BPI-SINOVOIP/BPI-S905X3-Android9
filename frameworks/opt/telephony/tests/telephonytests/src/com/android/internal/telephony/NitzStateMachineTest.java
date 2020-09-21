/*
 * Copyright 2017 The Android Open Source Project
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

package com.android.internal.telephony;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import android.icu.util.Calendar;
import android.icu.util.GregorianCalendar;
import android.icu.util.TimeZone;

import com.android.internal.telephony.TimeZoneLookupHelper.CountryResult;
import com.android.internal.telephony.TimeZoneLookupHelper.OffsetResult;
import com.android.internal.telephony.util.TimeStampedValue;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;

public class NitzStateMachineTest extends TelephonyTest {

    @Mock
    private NitzStateMachine.DeviceState mDeviceState;

    @Mock
    private TimeServiceHelper mTimeServiceHelper;

    private TimeZoneLookupHelper mRealTimeZoneLookupHelper;

    private NitzStateMachine mNitzStateMachine;

    @Before
    public void setUp() throws Exception {
        logd("NitzStateMachineTest +Setup!");
        super.setUp("NitzStateMachineTest");

        // In tests we use the real TimeZoneLookupHelper.
        mRealTimeZoneLookupHelper = new TimeZoneLookupHelper();
        mNitzStateMachine = new NitzStateMachine(
                mPhone, mTimeServiceHelper, mDeviceState, mRealTimeZoneLookupHelper);

        logd("ServiceStateTrackerTest -Setup!");
    }

    @After
    public void tearDown() throws Exception {
        checkNoUnverifiedSetOperations(mTimeServiceHelper);

        super.tearDown();
    }

    // A country that has multiple zones, but there is only one matching time zone at the time :
    // the zone cannot be guessed from the country alone, but can be guessed from the country +
    // NITZ.
    private static final Scenario UNIQUE_US_ZONE_SCENARIO = new Scenario.Builder()
            .setInitialDeviceSystemClockUtc(1977, 1, 1, 12, 0, 0)
            .setInitialDeviceRealtimeMillis(123456789L)
            .setTimeZone("America/Los_Angeles")
            .setActualTimeUtc(2018, 1, 1, 12, 0, 0)
            .setCountryIso("us")
            .build();

    @Test
    public void test_uniqueUsZone_Assumptions() {
        // Check we'll get the expected behavior from TimeZoneLookupHelper.

        // allZonesHaveSameOffset == false, so we shouldn't pick an arbitrary zone.
        CountryResult expectedCountryLookupResult = new CountryResult(
                "America/New_York", false /* allZonesHaveSameOffset */,
                UNIQUE_US_ZONE_SCENARIO.getInitialSystemClockMillis());
        CountryResult actualCountryLookupResult =
                mRealTimeZoneLookupHelper.lookupByCountry(
                        UNIQUE_US_ZONE_SCENARIO.getNetworkCountryIsoCode(),
                        UNIQUE_US_ZONE_SCENARIO.getInitialSystemClockMillis());
        assertEquals(expectedCountryLookupResult, actualCountryLookupResult);

        // isOnlyMatch == true, so the combination of country + NITZ should be enough.
        OffsetResult expectedLookupResult =
                new OffsetResult("America/Los_Angeles", true /* isOnlyMatch */);
        OffsetResult actualLookupResult = mRealTimeZoneLookupHelper.lookupByNitzCountry(
                UNIQUE_US_ZONE_SCENARIO.getNitzSignal().mValue,
                UNIQUE_US_ZONE_SCENARIO.getNetworkCountryIsoCode());
        assertEquals(expectedLookupResult, actualLookupResult);
    }

    // A country with a single zone : the zone can be guessed from the country.
    private static final Scenario UNITED_KINGDOM_SCENARIO = new Scenario.Builder()
            .setInitialDeviceSystemClockUtc(1977, 1, 1, 12, 0, 0)
            .setInitialDeviceRealtimeMillis(123456789L)
            .setTimeZone("Europe/London")
            .setActualTimeUtc(2018, 1, 1, 12, 0, 0)
            .setCountryIso("gb")
            .build();

    @Test
    public void test_unitedKingdom_Assumptions() {
        // Check we'll get the expected behavior from TimeZoneLookupHelper.

        // allZonesHaveSameOffset == true (not only that, there is only one zone), so we can pick
        // the zone knowing only the country.
        CountryResult expectedCountryLookupResult = new CountryResult(
                "Europe/London", true /* allZonesHaveSameOffset */,
                UNITED_KINGDOM_SCENARIO.getInitialSystemClockMillis());
        CountryResult actualCountryLookupResult =
                mRealTimeZoneLookupHelper.lookupByCountry(
                        UNITED_KINGDOM_SCENARIO.getNetworkCountryIsoCode(),
                        UNITED_KINGDOM_SCENARIO.getInitialSystemClockMillis());
        assertEquals(expectedCountryLookupResult, actualCountryLookupResult);

        OffsetResult expectedLookupResult =
                new OffsetResult("Europe/London", true /* isOnlyMatch */);
        OffsetResult actualLookupResult = mRealTimeZoneLookupHelper.lookupByNitzCountry(
                UNITED_KINGDOM_SCENARIO.getNitzSignal().mValue,
                UNITED_KINGDOM_SCENARIO.getNetworkCountryIsoCode());
        assertEquals(expectedLookupResult, actualLookupResult);
    }

    @Test
    public void test_uniqueUsZone_timeEnabledTimeZoneEnabled_countryThenNitz() throws Exception {
        Scenario scenario = UNIQUE_US_ZONE_SCENARIO;
        Device device = new DeviceBuilder()
                .setClocksFromScenario(scenario)
                .setTimeDetectionEnabled(true)
                .setTimeZoneDetectionEnabled(true)
                .setTimeZoneSettingInitialized(false)
                .initialize();
        Script script = new Script(device);

        int clockIncrement = 1250;
        long expectedTimeMillis = scenario.getActualTimeMillis() + clockIncrement;
        script.countryReceived(scenario.getNetworkCountryIsoCode())
                // Country won't be enough for time zone detection.
                .verifyNothingWasSetAndReset()
                // Increment the clock so we can tell the time was adjusted correctly when set.
                .incrementClocks(clockIncrement)
                .nitzReceived(scenario.getNitzSignal())
                // Country + NITZ is enough for both time + time zone detection.
                .verifyTimeAndZoneSetAndReset(expectedTimeMillis, scenario.getTimeZoneId());

        // Check NitzStateMachine state.
        assertTrue(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertEquals(scenario.getTimeZoneId(), mNitzStateMachine.getSavedTimeZoneId());
    }

    @Test
    public void test_unitedKingdom_timeEnabledTimeZoneEnabled_countryThenNitz() throws Exception {
        Scenario scenario = UNITED_KINGDOM_SCENARIO;
        Device device = new DeviceBuilder()
                .setClocksFromScenario(scenario)
                .setTimeDetectionEnabled(true)
                .setTimeZoneDetectionEnabled(true)
                .setTimeZoneSettingInitialized(false)
                .initialize();
        Script script = new Script(device);

        int clockIncrement = 1250;
        long expectedTimeMillis = scenario.getActualTimeMillis() + clockIncrement;
        script.countryReceived(scenario.getNetworkCountryIsoCode())
                // Country alone is enough to guess the time zone.
                .verifyOnlyTimeZoneWasSetAndReset(scenario.getTimeZoneId())
                // Increment the clock so we can tell the time was adjusted correctly when set.
                .incrementClocks(clockIncrement)
                .nitzReceived(scenario.getNitzSignal())
                // Country + NITZ is enough for both time + time zone detection.
                .verifyTimeAndZoneSetAndReset(expectedTimeMillis, scenario.getTimeZoneId());

        // Check NitzStateMachine state.
        assertTrue(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertEquals(scenario.getTimeZoneId(), mNitzStateMachine.getSavedTimeZoneId());
    }

    @Test
    public void test_uniqueUsZone_timeEnabledTimeZoneDisabled_countryThenNitz() throws Exception {
        Scenario scenario = UNIQUE_US_ZONE_SCENARIO;
        Device device = new DeviceBuilder()
                .setClocksFromScenario(scenario)
                .setTimeDetectionEnabled(true)
                .setTimeZoneDetectionEnabled(false)
                .setTimeZoneSettingInitialized(false)
                .initialize();
        Script script = new Script(device);

        int clockIncrement = 1250;
        script.countryReceived(scenario.getNetworkCountryIsoCode())
                // Country is not enough to guess the time zone and time zone detection is disabled.
                .verifyNothingWasSetAndReset()
                // Increment the clock so we can tell the time was adjusted correctly when set.
                .incrementClocks(clockIncrement)
                .nitzReceived(scenario.getNitzSignal())
                // Time zone detection is disabled, but time should be set from NITZ.
                .verifyOnlyTimeWasSetAndReset(scenario.getActualTimeMillis() + clockIncrement);

        // Check NitzStateMachine state.
        assertTrue(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertEquals(scenario.getTimeZoneId(), mNitzStateMachine.getSavedTimeZoneId());
    }

    @Test
    public void test_unitedKingdom_timeEnabledTimeZoneDisabled_countryThenNitz()
            throws Exception {
        Scenario scenario = UNITED_KINGDOM_SCENARIO;
        Device device = new DeviceBuilder()
                .setClocksFromScenario(scenario)
                .setTimeDetectionEnabled(true)
                .setTimeZoneDetectionEnabled(false)
                .setTimeZoneSettingInitialized(false)
                .initialize();
        Script script = new Script(device);

        int clockIncrement = 1250;
        script.countryReceived(scenario.getNetworkCountryIsoCode())
                // Country alone would be enough for time zone detection, but it's disabled.
                .verifyNothingWasSetAndReset()
                // Increment the clock so we can tell the time was adjusted correctly when set.
                .incrementClocks(clockIncrement)
                .nitzReceived(scenario.getNitzSignal())
                // Time zone detection is disabled, but time should be set from NITZ.
                .verifyOnlyTimeWasSetAndReset(scenario.getActualTimeMillis() + clockIncrement);

        // Check NitzStateMachine state.
        assertTrue(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertEquals(scenario.getTimeZoneId(), mNitzStateMachine.getSavedTimeZoneId());
    }

    @Test
    public void test_uniqueUsZone_timeDisabledTimeZoneEnabled_countryThenNitz() throws Exception {
        Scenario scenario = UNIQUE_US_ZONE_SCENARIO;
        Device device = new DeviceBuilder()
                .setClocksFromScenario(scenario)
                .setTimeDetectionEnabled(false)
                .setTimeZoneDetectionEnabled(true)
                .setTimeZoneSettingInitialized(false)
                .initialize();
        Script script = new Script(device);

        script.countryReceived(scenario.getNetworkCountryIsoCode())
                // Country won't be enough for time zone detection.
                .verifyNothingWasSetAndReset()
                .nitzReceived(scenario.getNitzSignal())
                // Time detection is disabled, but time zone should be detected from country + NITZ.
                .verifyOnlyTimeZoneWasSetAndReset(scenario.getTimeZoneId());

        // Check NitzStateMachine state.
        assertTrue(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertEquals(scenario.getTimeZoneId(), mNitzStateMachine.getSavedTimeZoneId());
    }

    @Test
    public void test_unitedKingdom_timeDisabledTimeZoneEnabled_countryThenNitz() throws Exception {
        Scenario scenario = UNITED_KINGDOM_SCENARIO;
        Device device = new DeviceBuilder()
                .setClocksFromScenario(scenario)
                .setTimeDetectionEnabled(false)
                .setTimeZoneDetectionEnabled(true)
                .setTimeZoneSettingInitialized(false)
                .initialize();
        Script script = new Script(device);

        script.countryReceived(scenario.getNetworkCountryIsoCode())
                // Country alone is enough to detect time zone.
                .verifyOnlyTimeZoneWasSetAndReset(scenario.getTimeZoneId())
                .nitzReceived(scenario.getNitzSignal())
                // Time detection is disabled, so we don't set the clock from NITZ.
                .verifyOnlyTimeZoneWasSetAndReset(scenario.getTimeZoneId());

        // Check NitzStateMachine state.
        assertTrue(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertEquals(scenario.getTimeZoneId(), mNitzStateMachine.getSavedTimeZoneId());
    }

    @Test
    public void test_uniqueUsZone_timeDisabledTimeZoneDisabled_countryThenNitz() throws Exception {
        Scenario scenario = UNIQUE_US_ZONE_SCENARIO;
        Device device = new DeviceBuilder()
                .setClocksFromScenario(scenario)
                .setTimeDetectionEnabled(false)
                .setTimeZoneDetectionEnabled(false)
                .setTimeZoneSettingInitialized(false)
                .initialize();
        Script script = new Script(device);

        script.countryReceived(scenario.getNetworkCountryIsoCode())
                // Time and time zone detection is disabled.
                .verifyNothingWasSetAndReset()
                .nitzReceived(scenario.getNitzSignal())
                // Time and time zone detection is disabled.
                .verifyNothingWasSetAndReset();

        // Check NitzStateMachine state.
        assertTrue(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertEquals(scenario.getTimeZoneId(), mNitzStateMachine.getSavedTimeZoneId());
    }

    @Test
    public void test_unitedKingdom_timeDisabledTimeZoneDisabled_countryThenNitz() throws Exception {
        Scenario scenario = UNITED_KINGDOM_SCENARIO;
        Device device = new DeviceBuilder()
                .setClocksFromScenario(scenario)
                .setTimeDetectionEnabled(false)
                .setTimeZoneDetectionEnabled(false)
                .setTimeZoneSettingInitialized(false)
                .initialize();
        Script script = new Script(device);

        script.countryReceived(scenario.getNetworkCountryIsoCode())
                // Time and time zone detection is disabled.
                .verifyNothingWasSetAndReset()
                .nitzReceived(scenario.getNitzSignal())
                // Time and time zone detection is disabled.
                .verifyNothingWasSetAndReset();

        // Check NitzStateMachine state.
        assertTrue(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertEquals(scenario.getTimeZoneId(), mNitzStateMachine.getSavedTimeZoneId());
    }

    @Test
    public void test_uniqueUsZone_timeDisabledTimeZoneEnabled_nitzThenCountry() throws Exception {
        Scenario scenario = UNIQUE_US_ZONE_SCENARIO;
        Device device = new DeviceBuilder()
                .setClocksFromScenario(scenario)
                .setTimeDetectionEnabled(false)
                .setTimeZoneDetectionEnabled(true)
                .setTimeZoneSettingInitialized(false)
                .initialize();
        Script script = new Script(device);

        // Simulate receiving an NITZ signal.
        script.nitzReceived(scenario.getNitzSignal())
                // The NITZ alone isn't enough to detect a time zone.
                .verifyNothingWasSetAndReset();

        // Check NitzStateMachine state.
        assertFalse(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertNull(mNitzStateMachine.getSavedTimeZoneId());

        // Simulate the country code becoming known.
        script.countryReceived(scenario.getNetworkCountryIsoCode())
                // The NITZ + country is enough to detect the time zone.
                .verifyOnlyTimeZoneWasSetAndReset(scenario.getTimeZoneId());

        // Check NitzStateMachine state.
        // TODO(nfuller): The following line should probably be assertTrue but the logic under test
        // may be buggy. Look at whether it needs to change.
        assertFalse(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertEquals(scenario.getTimeZoneId(), mNitzStateMachine.getSavedTimeZoneId());
    }

    @Test
    public void test_unitedKingdom_timeDisabledTimeZoneEnabled_nitzThenCountry() throws Exception {
        Scenario scenario = UNITED_KINGDOM_SCENARIO;
        Device device = new DeviceBuilder()
                .setClocksFromScenario(scenario)
                .setTimeDetectionEnabled(false)
                .setTimeZoneDetectionEnabled(true)
                .setTimeZoneSettingInitialized(false)
                .initialize();
        Script script = new Script(device);

        // Simulate receiving an NITZ signal.
        script.nitzReceived(scenario.getNitzSignal())
                // The NITZ alone isn't enough to detect a time zone.
                .verifyNothingWasSetAndReset();

        // Check NitzStateMachine state.
        assertFalse(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertNull(mNitzStateMachine.getSavedTimeZoneId());

        // Simulate the country code becoming known.
        script.countryReceived(scenario.getNetworkCountryIsoCode());

        // The NITZ + country is enough to detect the time zone.
        // NOTE: setting the timezone happens twice because of a quirk in NitzStateMachine: it
        // handles the country lookup / set, then combines the country with the NITZ state and does
        // another lookup / set. We shouldn't require it is set twice but we do for simplicity.
        script.verifyOnlyTimeZoneWasSetAndReset(scenario.getTimeZoneId(), 2 /* times */);

        // Check NitzStateMachine state.
        // TODO(nfuller): The following line should probably be assertTrue but the logic under test
        // may be buggy. Look at whether it needs to change.
        assertFalse(mNitzStateMachine.getNitzTimeZoneDetectionSuccessful());
        assertEquals(scenario.getNitzSignal().mValue, mNitzStateMachine.getCachedNitzData());
        assertEquals(scenario.getTimeZoneId(), mNitzStateMachine.getSavedTimeZoneId());
    }

    private static long createUtcTime(int year, int monthInYear, int day, int hourOfDay, int minute,
            int second) {
        Calendar cal = new GregorianCalendar(TimeZone.getTimeZone("Etc/UTC"));
        cal.clear();
        cal.set(year, monthInYear - 1, day, hourOfDay, minute, second);
        return cal.getTimeInMillis();
    }

    /**
     * A helper class for common test operations involving a device.
     */
    class Script {
        private final Device mDevice;

        Script(Device device) {
            this.mDevice = device;
        }

        Script countryReceived(String countryIsoCode) {
            mDevice.networkCountryKnown(countryIsoCode);
            return this;
        }

        Script nitzReceived(TimeStampedValue<NitzData> nitzSignal) {
            mDevice.nitzSignalReceived(nitzSignal);
            return this;
        }

        Script incrementClocks(int clockIncrement) {
            mDevice.incrementClocks(clockIncrement);
            return this;
        }

        Script verifyNothingWasSetAndReset() {
            mDevice.verifyTimeZoneWasNotSet();
            mDevice.verifyTimeWasNotSet();
            mDevice.checkNoUnverifiedSetOperations();
            mDevice.resetInvocations();
            return this;
        }

        Script verifyOnlyTimeZoneWasSetAndReset(String timeZoneId, int times) {
            mDevice.verifyTimeZoneWasSet(timeZoneId, times);
            mDevice.verifyTimeWasNotSet();
            mDevice.checkNoUnverifiedSetOperations();
            mDevice.resetInvocations();
            return this;
        }

        Script verifyOnlyTimeZoneWasSetAndReset(String timeZoneId) {
            return verifyOnlyTimeZoneWasSetAndReset(timeZoneId, 1);
        }

        Script verifyOnlyTimeWasSetAndReset(long expectedTimeMillis) {
            mDevice.verifyTimeZoneWasNotSet();
            mDevice.verifyTimeWasSet(expectedTimeMillis);
            mDevice.checkNoUnverifiedSetOperations();
            mDevice.resetInvocations();
            return this;
        }

        Script verifyTimeAndZoneSetAndReset(long expectedTimeMillis, String timeZoneId) {
            mDevice.verifyTimeZoneWasSet(timeZoneId);
            mDevice.verifyTimeWasSet(expectedTimeMillis);
            mDevice.checkNoUnverifiedSetOperations();
            mDevice.resetInvocations();
            return this;
        }

        Script reset() {
            mDevice.checkNoUnverifiedSetOperations();
            mDevice.resetInvocations();
            return this;
        }
    }

    /**
     * An abstraction of a device for use in telephony time zone detection tests. It can be used to
     * retrieve device state, modify device state and verify changes.
     */
    class Device {

        private final long mInitialSystemClockMillis;
        private final long mInitialRealtimeMillis;
        private final boolean mTimeDetectionEnabled;
        private final boolean mTimeZoneDetectionEnabled;
        private final boolean mTimeZoneSettingInitialized;

        Device(long initialSystemClockMillis, long initialRealtimeMillis,
                boolean timeDetectionEnabled, boolean timeZoneDetectionEnabled,
                boolean timeZoneSettingInitialized) {
            mInitialSystemClockMillis = initialSystemClockMillis;
            mInitialRealtimeMillis = initialRealtimeMillis;
            mTimeDetectionEnabled = timeDetectionEnabled;
            mTimeZoneDetectionEnabled = timeZoneDetectionEnabled;
            mTimeZoneSettingInitialized = timeZoneSettingInitialized;
        }

        void initialize() {
            // Set initial configuration.
            when(mDeviceState.getIgnoreNitz()).thenReturn(false);
            when(mDeviceState.getNitzUpdateDiffMillis()).thenReturn(2000);
            when(mDeviceState.getNitzUpdateSpacingMillis()).thenReturn(1000 * 60 * 10);

            // Simulate the country not being known.
            when(mDeviceState.getNetworkCountryIsoForPhone()).thenReturn("");

            when(mTimeServiceHelper.elapsedRealtime()).thenReturn(mInitialRealtimeMillis);
            when(mTimeServiceHelper.currentTimeMillis()).thenReturn(mInitialSystemClockMillis);
            when(mTimeServiceHelper.isTimeDetectionEnabled()).thenReturn(mTimeDetectionEnabled);
            when(mTimeServiceHelper.isTimeZoneDetectionEnabled())
                    .thenReturn(mTimeZoneDetectionEnabled);
            when(mTimeServiceHelper.isTimeZoneSettingInitialized())
                    .thenReturn(mTimeZoneSettingInitialized);
        }

        void networkCountryKnown(String countryIsoCode) {
            when(mDeviceState.getNetworkCountryIsoForPhone()).thenReturn(countryIsoCode);
            mNitzStateMachine.handleNetworkCountryCodeSet(true);
        }

        void incrementClocks(int millis) {
            long currentElapsedRealtime = mTimeServiceHelper.elapsedRealtime();
            when(mTimeServiceHelper.elapsedRealtime()).thenReturn(currentElapsedRealtime + millis);
            long currentTimeMillis = mTimeServiceHelper.currentTimeMillis();
            when(mTimeServiceHelper.currentTimeMillis()).thenReturn(currentTimeMillis + millis);
        }

        void nitzSignalReceived(TimeStampedValue<NitzData> nitzSignal) {
            mNitzStateMachine.handleNitzReceived(nitzSignal);
        }

        void verifyTimeZoneWasNotSet() {
            verify(mTimeServiceHelper, times(0)).setDeviceTimeZone(any(String.class));
        }

        void verifyTimeZoneWasSet(String timeZoneId) {
            verifyTimeZoneWasSet(timeZoneId, 1 /* times */);
        }

        void verifyTimeZoneWasSet(String timeZoneId, int times) {
            verify(mTimeServiceHelper, times(times)).setDeviceTimeZone(timeZoneId);
        }

        void verifyTimeWasNotSet() {
            verify(mTimeServiceHelper, times(0)).setDeviceTime(anyLong());
        }

        void verifyTimeWasSet(long expectedTimeMillis) {
            ArgumentCaptor<Long> timeServiceTimeCaptor = ArgumentCaptor.forClass(Long.TYPE);
            verify(mTimeServiceHelper, times(1)).setDeviceTime(timeServiceTimeCaptor.capture());
            assertEquals(expectedTimeMillis, (long) timeServiceTimeCaptor.getValue());
        }

        /**
         * Used after calling verify... methods to reset expectations.
         */
        void resetInvocations() {
            clearInvocations(mTimeServiceHelper);
        }

        void checkNoUnverifiedSetOperations() {
            NitzStateMachineTest.checkNoUnverifiedSetOperations(mTimeServiceHelper);
        }
    }

    /** A class used to construct a Device. */
    class DeviceBuilder {

        private long mInitialSystemClock;
        private long mInitialRealtimeMillis;
        private boolean mTimeDetectionEnabled;
        private boolean mTimeZoneDetectionEnabled;
        private boolean mTimeZoneSettingInitialized;

        Device initialize() {
            Device device = new Device(mInitialSystemClock, mInitialRealtimeMillis,
                    mTimeDetectionEnabled, mTimeZoneDetectionEnabled, mTimeZoneSettingInitialized);
            device.initialize();
            return device;
        }

        DeviceBuilder setTimeDetectionEnabled(boolean enabled) {
            mTimeDetectionEnabled = enabled;
            return this;
        }

        DeviceBuilder setTimeZoneDetectionEnabled(boolean enabled) {
            mTimeZoneDetectionEnabled = enabled;
            return this;
        }

        DeviceBuilder setTimeZoneSettingInitialized(boolean initialized) {
            mTimeZoneSettingInitialized = initialized;
            return this;
        }

        DeviceBuilder setClocksFromScenario(Scenario scenario) {
            mInitialRealtimeMillis = scenario.getInitialRealTimeMillis();
            mInitialSystemClock = scenario.getInitialSystemClockMillis();
            return this;
        }
    }

    /**
     * A scenario used during tests. Describes a fictional reality.
     */
    static class Scenario {

        private final long mInitialDeviceSystemClockMillis;
        private final long mInitialDeviceRealtimeMillis;
        private final long mActualTimeMillis;
        private final TimeZone mZone;
        private final String mNetworkCountryIsoCode;

        private TimeStampedValue<NitzData> mNitzSignal;

        Scenario(long initialDeviceSystemClock, long elapsedRealtime, long timeMillis,
                String zoneId, String countryIsoCode) {
            mInitialDeviceSystemClockMillis = initialDeviceSystemClock;
            mActualTimeMillis = timeMillis;
            mInitialDeviceRealtimeMillis = elapsedRealtime;
            mZone = TimeZone.getTimeZone(zoneId);
            mNetworkCountryIsoCode = countryIsoCode;
        }

        TimeStampedValue<NitzData> getNitzSignal() {
            if (mNitzSignal == null) {
                int[] offsets = new int[2];
                mZone.getOffset(mActualTimeMillis, false /* local */, offsets);
                int zoneOffsetMillis = offsets[0] + offsets[1];
                NitzData nitzData = NitzData
                        .createForTests(zoneOffsetMillis, offsets[1], mActualTimeMillis, null);
                mNitzSignal = new TimeStampedValue<>(nitzData, mInitialDeviceRealtimeMillis);
            }
            return mNitzSignal;
        }

        long getInitialRealTimeMillis() {
            return mInitialDeviceRealtimeMillis;
        }

        long getInitialSystemClockMillis() {
            return mInitialDeviceSystemClockMillis;
        }

        String getNetworkCountryIsoCode() {
            return mNetworkCountryIsoCode;
        }

        String getTimeZoneId() {
            return mZone.getID();
        }

        long getActualTimeMillis() {
            return mActualTimeMillis;
        }

        static class Builder {

            private long mInitialDeviceSystemClockMillis;
            private long mInitialDeviceRealtimeMillis;
            private long mActualTimeMillis;
            private String mZoneId;
            private String mCountryIsoCode;

            Builder setInitialDeviceSystemClockUtc(int year, int monthInYear, int day,
                    int hourOfDay, int minute, int second) {
                mInitialDeviceSystemClockMillis = createUtcTime(year, monthInYear, day, hourOfDay,
                        minute, second);
                return this;
            }

            Builder setInitialDeviceRealtimeMillis(long realtimeMillis) {
                mInitialDeviceRealtimeMillis = realtimeMillis;
                return this;
            }

            Builder setActualTimeUtc(int year, int monthInYear, int day, int hourOfDay,
                    int minute, int second) {
                mActualTimeMillis = createUtcTime(year, monthInYear, day, hourOfDay, minute,
                        second);
                return this;
            }

            Builder setTimeZone(String zoneId) {
                mZoneId = zoneId;
                return this;
            }

            Builder setCountryIso(String isoCode) {
                mCountryIsoCode = isoCode;
                return this;
            }

            Scenario build() {
                return new Scenario(mInitialDeviceSystemClockMillis, mInitialDeviceRealtimeMillis,
                        mActualTimeMillis, mZoneId, mCountryIsoCode);
            }
        }
    }

    /**
     * Confirms all mTimeServiceHelper side effects were verified.
     */
    private static void checkNoUnverifiedSetOperations(TimeServiceHelper mTimeServiceHelper) {
        // We don't care about current auto time / time zone state retrievals / listening so we can
        // use "at least 0" times to indicate they don't matter.
        verify(mTimeServiceHelper, atLeast(0)).setListener(any());
        verify(mTimeServiceHelper, atLeast(0)).isTimeDetectionEnabled();
        verify(mTimeServiceHelper, atLeast(0)).isTimeZoneDetectionEnabled();
        verify(mTimeServiceHelper, atLeast(0)).isTimeZoneSettingInitialized();
        verify(mTimeServiceHelper, atLeast(0)).elapsedRealtime();
        verify(mTimeServiceHelper, atLeast(0)).currentTimeMillis();
        verifyNoMoreInteractions(mTimeServiceHelper);
    }
}
