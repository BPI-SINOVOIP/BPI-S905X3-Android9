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

package com.android.car;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertTrue;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.hardware.CarPropertyValue;
import android.car.hardware.CarSensorEvent;
import android.car.hardware.CarSensorManager;
import android.car.hardware.property.CarPropertyEvent;
import android.car.hardware.property.ICarPropertyEventListener;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationManager;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import com.android.internal.util.ArrayUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.stream.Collectors;

/**
 * This class contains unit tests for the {@link CarLocationService}.
 * It tests that {@link LocationManager}'s last known location is stored in and loaded from a JSON
 * file upon appropriate system events.
 *
 * The following mocks are used:
 * 1. {@link Context} provides files and a mocked {@link LocationManager}.
 * 2. {@link LocationManager} provides dummy {@link Location}s.
 * 3. {@link CarSensorService} registers a handler for sensor events and sends ignition-off events.
 */
@RunWith(AndroidJUnit4.class)
public class CarLocationServiceTest {
    private static String TAG = "CarLocationServiceTest";
    private static String TEST_FILENAME = "location_cache_test.json";
    private CarLocationService mCarLocationService;
    private Context mContext;
    private CountDownLatch mLatch;
    @Mock private Context mMockContext;
    @Mock private LocationManager mMockLocationManager;
    @Mock private CarPropertyService mMockCarPropertyService;
    @Mock private CarPowerManagementService mMockCarPowerManagementService;

    /**
     * Initialize all of the objects with the @Mock annotation.
     */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mContext = InstrumentationRegistry.getTargetContext();
        mLatch = new CountDownLatch(1);
        mCarLocationService = new CarLocationService(mMockContext, mMockCarPowerManagementService,
                mMockCarPropertyService) {
            @Override
            void asyncOperation(Runnable operation) {
                super.asyncOperation(() -> {
                    operation.run();
                    mLatch.countDown();
                });
            }
        };
    }

    @After
    public void tearDown() throws Exception {
        if (mCarLocationService != null) {
            mCarLocationService.release();
        }
        mContext.deleteFile(TEST_FILENAME);
    }

    /**
     * Test that the {@link CarLocationService} has the permissions necessary to call the
     * {@link LocationManager} injectLocation API.
     *
     * Note that this test will never fail even if the relevant permissions are removed from the
     * manifest since {@link CarService} runs in a system process.
     */
    @Test
    public void testCarLocationServiceShouldHavePermissions() {
        int fineLocationCheck =
                mContext.checkSelfPermission(android.Manifest.permission.ACCESS_FINE_LOCATION);
        int locationHardwareCheck =
                mContext.checkSelfPermission(android.Manifest.permission.LOCATION_HARDWARE);
        assertEquals(PackageManager.PERMISSION_GRANTED, fineLocationCheck);
        assertEquals(PackageManager.PERMISSION_GRANTED, locationHardwareCheck);
    }

    /**
     * Test that the {@link CarLocationService} registers to receive the locked boot completed
     * intent and ignition sensor events upon initialization.
     */
    @Test
    public void testRegistersToReceiveEvents() {
        ArgumentCaptor<IntentFilter> argument = ArgumentCaptor.forClass(IntentFilter.class);
        mCarLocationService.init();
        verify(mMockCarPowerManagementService).registerPowerEventProcessingHandler(
                mCarLocationService);
        verify(mMockContext).registerReceiver(eq(mCarLocationService), argument.capture());
        IntentFilter intentFilter = argument.getValue();
        assertEquals(3, intentFilter.countActions());
        String[] actions = {intentFilter.getAction(0), intentFilter.getAction(1),
                intentFilter.getAction(2)};
        assertTrue(ArrayUtils.contains(actions, Intent.ACTION_LOCKED_BOOT_COMPLETED));
        assertTrue(ArrayUtils.contains(actions, LocationManager.MODE_CHANGED_ACTION));
        assertTrue(ArrayUtils.contains(actions, LocationManager.GPS_ENABLED_CHANGE_ACTION));
        verify(mMockCarPropertyService).registerListener(
                eq(CarSensorManager.SENSOR_TYPE_IGNITION_STATE), eq(0.0f), any());
    }

    /**
     * Test that the {@link CarLocationService} unregisters its event receivers.
     */
    @Test
    public void testUnregistersEventReceivers() {
        mCarLocationService.release();
        verify(mMockContext).unregisterReceiver(mCarLocationService);
        verify(mMockCarPropertyService).unregisterListener(
                eq(CarSensorManager.SENSOR_TYPE_IGNITION_STATE), any());
    }

    /**
     * Test that the {@link CarLocationService} parses a location from a JSON serialization and then
     * injects it into the {@link LocationManager} upon boot complete.
     */
    @Test
    public void testLoadsLocation() throws IOException, InterruptedException {
        long currentTime = System.currentTimeMillis();
        long elapsedTime = SystemClock.elapsedRealtimeNanos();
        long pastTime = currentTime - 60000;
        writeCacheFile("{\"provider\": \"gps\", \"latitude\": 16.7666, \"longitude\": 3.0026,"
                + "\"accuracy\":12.3, \"captureTime\": " + pastTime + "}");
        ArgumentCaptor<Location> argument = ArgumentCaptor.forClass(Location.class);
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.injectLocation(argument.capture())).thenReturn(true);
        when(mMockContext.getFileStreamPath("location_cache.json"))
                .thenReturn(mContext.getFileStreamPath(TEST_FILENAME));

        mCarLocationService.onReceive(mMockContext,
                new Intent(Intent.ACTION_LOCKED_BOOT_COMPLETED));
        mLatch.await();

        Location location = argument.getValue();
        assertEquals("gps", location.getProvider());
        assertEquals(16.7666, location.getLatitude());
        assertEquals(3.0026, location.getLongitude());
        assertEquals(12.3f, location.getAccuracy());
        assertTrue(location.getTime() >= currentTime);
        assertTrue(location.getElapsedRealtimeNanos() >= elapsedTime);
    }

    /**
     * Test that the {@link CarLocationService} does not inject a location if there is no location
     * cache file.
     */
    @Test
    public void testDoesNotLoadLocationWhenNoFileExists()
            throws FileNotFoundException, InterruptedException {
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER))
                .thenReturn(null);
        when(mMockContext.getFileStreamPath("location_cache.json"))
                .thenReturn(mContext.getFileStreamPath(TEST_FILENAME));
        mCarLocationService.onReceive(mMockContext,
                new Intent(Intent.ACTION_LOCKED_BOOT_COMPLETED));
        mLatch.await();
        verify(mMockLocationManager, never()).injectLocation(any());
    }

    /**
     * Test that the {@link CarLocationService} handles an incomplete JSON file gracefully.
     */
    @Test
    public void testDoesNotLoadLocationFromIncompleteFile() throws IOException,
            InterruptedException {
        writeCacheFile("{\"provider\": \"gps\", \"latitude\": 16.7666, \"longitude\": 3.0026,");
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER))
                .thenReturn(null);
        when(mMockContext.getFileStreamPath("location_cache.json"))
                .thenReturn(mContext.getFileStreamPath(TEST_FILENAME));
        mCarLocationService.onReceive(mMockContext,
                new Intent(Intent.ACTION_LOCKED_BOOT_COMPLETED));
        mLatch.await();
        verify(mMockLocationManager, never()).injectLocation(any());
    }

    /**
     * Test that the {@link CarLocationService} handles a corrupt JSON file gracefully.
     */
    @Test
    public void testDoesNotLoadLocationFromCorruptFile() throws IOException, InterruptedException {
        writeCacheFile("{\"provider\":\"latitude\":16.7666,\"longitude\": \"accuracy\":1.0}");
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER))
                .thenReturn(null);
        when(mMockContext.getFileStreamPath("location_cache.json"))
                .thenReturn(mContext.getFileStreamPath(TEST_FILENAME));
        mCarLocationService.onReceive(mMockContext,
                new Intent(Intent.ACTION_LOCKED_BOOT_COMPLETED));
        mLatch.await();
        verify(mMockLocationManager, never()).injectLocation(any());
    }

    /**
     * Test that the {@link CarLocationService} does not inject a location that is missing
     * accuracy.
     */
    @Test
    public void testDoesNotLoadIncompleteLocation() throws IOException, InterruptedException {
        writeCacheFile("{\"provider\": \"gps\", \"latitude\": 16.7666, \"longitude\": 3.0026}");
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER))
                .thenReturn(null);
        when(mMockContext.getFileStreamPath("location_cache.json"))
                .thenReturn(mContext.getFileStreamPath(TEST_FILENAME));
        mCarLocationService.onReceive(mMockContext,
                new Intent(Intent.ACTION_LOCKED_BOOT_COMPLETED));
        mLatch.await();
        verify(mMockLocationManager, never()).injectLocation(any());
    }

    /**
     * Test that the {@link CarLocationService} does not inject a location that is older than
     * thirty days.
     */
    @Test
    public void testDoesNotLoadOldLocation() throws IOException, InterruptedException {
        long thirtyThreeDaysMs = 33 * 24 * 60 * 60 * 1000L;
        long oldTime = System.currentTimeMillis() - thirtyThreeDaysMs;
        writeCacheFile("{\"provider\": \"gps\", \"latitude\": 16.7666, \"longitude\": 3.0026,"
                + "\"accuracy\":12.3, \"captureTime\": " + oldTime + "}");
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER))
                .thenReturn(null);
        when(mMockContext.getFileStreamPath("location_cache.json"))
                .thenReturn(mContext.getFileStreamPath(TEST_FILENAME));
        mCarLocationService.onReceive(mMockContext,
                new Intent(Intent.ACTION_LOCKED_BOOT_COMPLETED));
        mLatch.await();
        verify(mMockLocationManager, never()).injectLocation(any());
    }

    /**
     * Test that the {@link CarLocationService} stores the {@link LocationManager}'s last known
     * location in a JSON file upon ignition-off events.
     */
    @Test
    public void testStoresLocationUponIgnitionOff()
            throws IOException, RemoteException, InterruptedException {
        long currentTime = System.currentTimeMillis();
        long elapsedTime = SystemClock.elapsedRealtimeNanos();
        Location timbuktu = new Location(LocationManager.GPS_PROVIDER);
        timbuktu.setLatitude(16.7666);
        timbuktu.setLongitude(3.0026);
        timbuktu.setAccuracy(13.75f);
        timbuktu.setTime(currentTime);
        timbuktu.setElapsedRealtimeNanos(elapsedTime);
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER))
                .thenReturn(timbuktu);
        when(mMockContext.getFileStreamPath("location_cache.json"))
                .thenReturn(mContext.getFileStreamPath(TEST_FILENAME));
        sendIgnitionOffEvent();
        mLatch.await();
        verify(mMockLocationManager).getLastKnownLocation(LocationManager.GPS_PROVIDER);
        String actualContents = readCacheFile();
        long oneDayMs = 24 * 60 * 60 * 1000;
        long granularCurrentTime = (currentTime / oneDayMs) * oneDayMs;
        String expectedContents = "{\"provider\":\"gps\",\"latitude\":16.7666,\"longitude\":"
                + "3.0026,\"accuracy\":13.75,\"captureTime\":" + granularCurrentTime + "}";
        assertEquals(expectedContents, actualContents);
    }

    /**
     * Test that the {@link CarLocationService} stores the {@link LocationManager}'s last known
     * location upon prepare-shutdown events.
     */
    @Test
    public void testStoresLocationUponPrepareShutdown()
            throws IOException, RemoteException, InterruptedException {
        long currentTime = System.currentTimeMillis();
        long elapsedTime = SystemClock.elapsedRealtimeNanos();
        Location timbuktu = new Location(LocationManager.GPS_PROVIDER);
        timbuktu.setLatitude(16.7666);
        timbuktu.setLongitude(3.0026);
        timbuktu.setAccuracy(13.75f);
        timbuktu.setTime(currentTime);
        timbuktu.setElapsedRealtimeNanos(elapsedTime);
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER))
                .thenReturn(timbuktu);
        when(mMockContext.getFileStreamPath("location_cache.json"))
                .thenReturn(mContext.getFileStreamPath(TEST_FILENAME));
        mCarLocationService.onPrepareShutdown(true);
        mLatch.await();
        verify(mMockLocationManager).getLastKnownLocation(LocationManager.GPS_PROVIDER);
        String actualContents = readCacheFile();
        long oneDayMs = 24 * 60 * 60 * 1000;
        long granularCurrentTime = (currentTime / oneDayMs) * oneDayMs;
        String expectedContents = "{\"provider\":\"gps\",\"latitude\":16.7666,\"longitude\":"
                + "3.0026,\"accuracy\":13.75,\"captureTime\":" + granularCurrentTime + "}";
        assertEquals(expectedContents, actualContents);
    }

    /**
     * Test that the {@link CarLocationService} does not store null locations.
     */
    @Test
    public void testDoesNotStoreNullLocation() throws Exception {
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER))
                .thenReturn(null);
        when(mMockContext.getFileStreamPath("location_cache.json"))
                .thenReturn(mContext.getFileStreamPath(TEST_FILENAME));
        sendIgnitionOffEvent();
        mLatch.await();
        verify(mMockLocationManager).getLastKnownLocation(LocationManager.GPS_PROVIDER);
        verify(mMockContext).deleteFile("location_cache.json");
    }

    /**
     * Test that the {@link CarLocationService} deletes location_cache.json when location is
     * disabled.
     */
    @Test
    public void testDeletesCacheFileWhenLocationIsDisabled() throws Exception {
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.isLocationEnabled()).thenReturn(false);
        mCarLocationService.init();
        mCarLocationService.onReceive(mMockContext,
                new Intent(LocationManager.MODE_CHANGED_ACTION));
        mLatch.await();
        verify(mMockLocationManager, times(1)).isLocationEnabled();
        verify(mMockContext).deleteFile("location_cache.json");
    }

    /**
     * Test that the {@link CarLocationService} deletes location_cache.json when the GPS location
     * provider is disabled.
     */
    @Test
    public void testDeletesCacheFileWhenTheGPSProviderIsDisabled() throws Exception {
        when(mMockContext.getSystemService(Context.LOCATION_SERVICE))
                .thenReturn(mMockLocationManager);
        when(mMockLocationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)).thenReturn(
                false);
        mCarLocationService.init();
        mCarLocationService.onReceive(mMockContext,
                new Intent(LocationManager.GPS_ENABLED_CHANGE_ACTION));
        mLatch.await();
        verify(mMockLocationManager, times(1))
                .isProviderEnabled(LocationManager.GPS_PROVIDER);
        verify(mMockContext).deleteFile("location_cache.json");
    }

    private void writeCacheFile(String json) throws IOException {
        FileOutputStream fos = mContext.openFileOutput(TEST_FILENAME, Context.MODE_PRIVATE);
        fos.write(json.getBytes());
        fos.close();
    }

    private String readCacheFile() throws IOException {
        FileInputStream fis = mContext.openFileInput(TEST_FILENAME);
        String json = new BufferedReader(new InputStreamReader(fis)).lines()
                .parallel().collect(Collectors.joining("\n"));
        fis.close();
        return json;
    }

    private void sendIgnitionOffEvent() throws RemoteException {
        mCarLocationService.init();
        ArgumentCaptor<ICarPropertyEventListener> argument =
                ArgumentCaptor.forClass(ICarPropertyEventListener.class);
        verify(mMockCarPropertyService).registerListener(
                eq(CarSensorManager.SENSOR_TYPE_IGNITION_STATE), eq(0.0f), argument.capture());
        ICarPropertyEventListener carPropertyEventListener = argument.getValue();
        int intValues = CarSensorEvent.IGNITION_STATE_OFF;
        CarPropertyValue ignitionOff = new CarPropertyValue(
                CarSensorManager.SENSOR_TYPE_IGNITION_STATE, 0, 0,
                System.currentTimeMillis(), intValues);
        CarPropertyEvent event = new CarPropertyEvent(0, ignitionOff);
        List<CarPropertyEvent> events = new ArrayList<>(Arrays.asList(event));
        carPropertyEventListener.onEvent(events);
    }
}
