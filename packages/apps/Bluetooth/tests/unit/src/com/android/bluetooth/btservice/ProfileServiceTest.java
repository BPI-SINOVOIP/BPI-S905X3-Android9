/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.bluetooth.btservice;

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.*;

import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.os.Looper;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ServiceTestRule;
import android.support.test.runner.AndroidJUnit4;

import com.android.bluetooth.TestUtils;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.lang.reflect.InvocationTargetException;
import java.util.List;
import java.util.concurrent.TimeoutException;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class ProfileServiceTest {
    private static final int PROFILE_START_MILLIS = 1250;
    private static final int NUM_REPEATS = 5;

    @Rule public final ServiceTestRule mServiceTestRule = new ServiceTestRule();

    private void setProfileState(Class profile, int state) throws TimeoutException {
        Intent startIntent = new Intent(InstrumentationRegistry.getTargetContext(), profile);
        startIntent.putExtra(AdapterService.EXTRA_ACTION,
                AdapterService.ACTION_SERVICE_STATE_CHANGED);
        startIntent.putExtra(BluetoothAdapter.EXTRA_STATE, state);
        mServiceTestRule.startService(startIntent);
    }

    private void setAllProfilesState(int state, int invocationNumber) throws TimeoutException {
        for (Class profile : mProfiles) {
            setProfileState(profile, state);
        }
        ArgumentCaptor<ProfileService> argument = ArgumentCaptor.forClass(ProfileService.class);
        verify(mMockAdapterService, timeout(PROFILE_START_MILLIS).times(
                mProfiles.length * invocationNumber)).onProfileServiceStateChanged(
                argument.capture(), eq(state));
        List<ProfileService> argumentProfiles = argument.getAllValues();
        for (Class profile : mProfiles) {
            int matches = 0;
            for (ProfileService arg : argumentProfiles) {
                if (arg.getClass().getName().equals(profile.getName())) {
                    matches += 1;
                }
            }
            Assert.assertEquals(invocationNumber, matches);
        }
    }

    private @Mock AdapterService mMockAdapterService;

    private Class[] mProfiles;

    @Before
    public void setUp()
            throws NoSuchMethodException, InvocationTargetException, IllegalAccessException {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
        Assert.assertNotNull(Looper.myLooper());

        MockitoAnnotations.initMocks(this);

        mProfiles = Config.getSupportedProfiles();

        mMockAdapterService.initNative();

        TestUtils.setAdapterService(mMockAdapterService);

        Assert.assertNotNull(AdapterService.getAdapterService());
    }

    @After
    public void tearDown()
            throws NoSuchMethodException, InvocationTargetException, IllegalAccessException {
        mMockAdapterService.cleanupNative();
        TestUtils.clearAdapterService(mMockAdapterService);
        mMockAdapterService = null;
        mProfiles = null;
    }

    /**
     * Test: Start the Bluetooth services that are configured.
     * Verify that the same services start.
     */
    @Test
    public void testEnableDisable() throws TimeoutException {
        setAllProfilesState(BluetoothAdapter.STATE_ON, 1);
        setAllProfilesState(BluetoothAdapter.STATE_OFF, 1);
    }

    /**
     * Test: Start the Bluetooth services that are configured twice.
     * Verify that the services start.
     */
    @Test
    public void testEnableDisableTwice() throws TimeoutException {
        setAllProfilesState(BluetoothAdapter.STATE_ON, 1);
        setAllProfilesState(BluetoothAdapter.STATE_OFF, 1);
        setAllProfilesState(BluetoothAdapter.STATE_ON, 2);
        setAllProfilesState(BluetoothAdapter.STATE_OFF, 2);
    }

    /**
     * Test: Start the Bluetooth services that are configured.
     * Verify that each profile starts and stops.
     */
    @Test
    public void testEnableDisableInterleaved() throws TimeoutException {
        for (Class profile : mProfiles) {
            setProfileState(profile, BluetoothAdapter.STATE_ON);
            setProfileState(profile, BluetoothAdapter.STATE_OFF);
        }
        ArgumentCaptor<ProfileService> starts = ArgumentCaptor.forClass(ProfileService.class);
        ArgumentCaptor<ProfileService> stops = ArgumentCaptor.forClass(ProfileService.class);
        int invocationNumber = mProfiles.length;
        verify(mMockAdapterService,
                timeout(PROFILE_START_MILLIS).times(invocationNumber)).onProfileServiceStateChanged(
                starts.capture(), eq(BluetoothAdapter.STATE_ON));
        verify(mMockAdapterService,
                timeout(PROFILE_START_MILLIS).times(invocationNumber)).onProfileServiceStateChanged(
                stops.capture(), eq(BluetoothAdapter.STATE_OFF));

        List<ProfileService> startedArguments = starts.getAllValues();
        List<ProfileService> stoppedArguments = stops.getAllValues();
        Assert.assertEquals(startedArguments.size(), stoppedArguments.size());
        for (ProfileService service : startedArguments) {
            Assert.assertTrue(stoppedArguments.contains(service));
            stoppedArguments.remove(service);
            Assert.assertFalse(stoppedArguments.contains(service));
        }
    }

    /**
     * Test: Start and stop a single profile repeatedly.
     * Verify that the profiles start and stop.
     */
    @Test
    public void testRepeatedEnableDisableSingly() throws TimeoutException {
        int profileNumber = 0;
        for (Class profile : mProfiles) {
            for (int i = 0; i < NUM_REPEATS; i++) {
                setProfileState(profile, BluetoothAdapter.STATE_ON);
                ArgumentCaptor<ProfileService> start =
                        ArgumentCaptor.forClass(ProfileService.class);
                verify(mMockAdapterService, timeout(PROFILE_START_MILLIS).times(
                        NUM_REPEATS * profileNumber + i + 1)).onProfileServiceStateChanged(
                        start.capture(), eq(BluetoothAdapter.STATE_ON));
                setProfileState(profile, BluetoothAdapter.STATE_OFF);
                ArgumentCaptor<ProfileService> stop = ArgumentCaptor.forClass(ProfileService.class);
                verify(mMockAdapterService, timeout(PROFILE_START_MILLIS).times(
                        NUM_REPEATS * profileNumber + i + 1)).onProfileServiceStateChanged(
                        stop.capture(), eq(BluetoothAdapter.STATE_OFF));
                Assert.assertEquals(start.getValue(), stop.getValue());
            }
            profileNumber += 1;
        }
    }

    /**
     * Test: Start and stop a single profile repeatedly and verify that the profile services are
     * registered and unregistered accordingly.
     */
    @Test
    public void testProfileServiceRegisterUnregister() throws TimeoutException {
        int profileNumber = 0;
        for (Class profile : mProfiles) {
            for (int i = 0; i < NUM_REPEATS; i++) {
                setProfileState(profile, BluetoothAdapter.STATE_ON);
                ArgumentCaptor<ProfileService> start =
                        ArgumentCaptor.forClass(ProfileService.class);
                verify(mMockAdapterService, timeout(PROFILE_START_MILLIS).times(
                        NUM_REPEATS * profileNumber + i + 1)).addProfile(
                        start.capture());
                setProfileState(profile, BluetoothAdapter.STATE_OFF);
                ArgumentCaptor<ProfileService> stop = ArgumentCaptor.forClass(ProfileService.class);
                verify(mMockAdapterService, timeout(PROFILE_START_MILLIS).times(
                        NUM_REPEATS * profileNumber + i + 1)).removeProfile(
                        stop.capture());
                Assert.assertEquals(start.getValue(), stop.getValue());
            }
            profileNumber += 1;
        }
    }
}
