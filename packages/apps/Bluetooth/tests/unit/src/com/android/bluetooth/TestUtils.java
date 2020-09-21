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
package com.android.bluetooth;

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.*;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ServiceTestRule;

import com.android.bluetooth.btservice.AdapterService;
import com.android.bluetooth.btservice.ProfileService;

import org.junit.Assert;
import org.mockito.ArgumentCaptor;
import org.mockito.internal.util.MockUtil;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * A set of methods useful in Bluetooth instrumentation tests
 */
public class TestUtils {
    private static final int SERVICE_TOGGLE_TIMEOUT_MS = 1000;    // 1s

    /**
     * Utility method to replace obj.fieldName with newValue where obj is of type c
     *
     * @param c type of obj
     * @param fieldName field name to be replaced
     * @param obj instance of type c whose fieldName is to be replaced, null for static fields
     * @param newValue object used to replace fieldName
     * @return the old value of fieldName that got replaced, caller is responsible for restoring
     *         it back to obj
     * @throws NoSuchFieldException when fieldName is not found in type c
     * @throws IllegalAccessException when fieldName cannot be accessed in type c
     */
    public static Object replaceField(final Class c, final String fieldName, final Object obj,
            final Object newValue) throws NoSuchFieldException, IllegalAccessException {
        Field field = c.getDeclaredField(fieldName);
        field.setAccessible(true);

        Object oldValue = field.get(obj);
        field.set(obj, newValue);
        return oldValue;
    }

    /**
     * Set the return value of {@link AdapterService#getAdapterService()} to a test specified value
     *
     * @param adapterService the designated {@link AdapterService} in test, must not be null, can
     * be mocked or spied
     * @throws NoSuchMethodException when setAdapterService method is not found
     * @throws IllegalAccessException when setAdapterService method cannot be accessed
     * @throws InvocationTargetException when setAdapterService method cannot be invoked, which
     * should never happen since setAdapterService is a static method
     */
    public static void setAdapterService(AdapterService adapterService)
            throws NoSuchMethodException, IllegalAccessException, InvocationTargetException {
        Assert.assertNull("AdapterService.getAdapterService() must be null before setting another"
                + " AdapterService", AdapterService.getAdapterService());
        Assert.assertNotNull(adapterService);
        // We cannot mock AdapterService.getAdapterService() with Mockito.
        // Hence we need to use reflection to call a private method to
        // initialize properly the AdapterService.sAdapterService field.
        Method method =
                AdapterService.class.getDeclaredMethod("setAdapterService", AdapterService.class);
        method.setAccessible(true);
        method.invoke(null, adapterService);
    }

    /**
     * Clear the return value of {@link AdapterService#getAdapterService()} to null
     *
     * @param adapterService the {@link AdapterService} used when calling
     * {@link TestUtils#setAdapterService(AdapterService)}
     * @throws NoSuchMethodException when clearAdapterService method is not found
     * @throws IllegalAccessException when clearAdapterService method cannot be accessed
     * @throws InvocationTargetException when clearAdappterService method cannot be invoked,
     * which should never happen since clearAdapterService is a static method
     */
    public static void clearAdapterService(AdapterService adapterService)
            throws NoSuchMethodException, IllegalAccessException, InvocationTargetException {
        Assert.assertSame("AdapterService.getAdapterService() must return the same object as the"
                        + " supplied adapterService in this method", adapterService,
                AdapterService.getAdapterService());
        Assert.assertNotNull(adapterService);
        Method method =
                AdapterService.class.getDeclaredMethod("clearAdapterService", AdapterService.class);
        method.setAccessible(true);
        method.invoke(null, adapterService);
    }

    /**
     * Start a profile service using the given {@link ServiceTestRule} and verify through
     * {@link AdapterService#getAdapterService()} that the service is actually started within
     * {@link TestUtils#SERVICE_TOGGLE_TIMEOUT_MS} milliseconds.
     * {@link #setAdapterService(AdapterService)} must be called with a mocked
     * {@link AdapterService} before calling this method
     *
     * @param serviceTestRule the {@link ServiceTestRule} used to execute the service start request
     * @param profileServiceClass a class from one of {@link ProfileService}'s child classes
     * @throws TimeoutException when service failed to start within either default timeout of
     * {@link ServiceTestRule#DEFAULT_TIMEOUT} (normally 5s) or user specified time when creating
     * {@link ServiceTestRule} through {@link ServiceTestRule#withTimeout(long, TimeUnit)} method
     */
    public static <T extends ProfileService> void startService(ServiceTestRule serviceTestRule,
            Class<T> profileServiceClass) throws TimeoutException {
        AdapterService adapterService = AdapterService.getAdapterService();
        Assert.assertNotNull(adapterService);
        Assert.assertTrue("AdapterService.getAdapterService() must return a mocked or spied object"
                + " before calling this method", MockUtil.isMock(adapterService));
        Intent startIntent =
                new Intent(InstrumentationRegistry.getTargetContext(), profileServiceClass);
        startIntent.putExtra(AdapterService.EXTRA_ACTION,
                AdapterService.ACTION_SERVICE_STATE_CHANGED);
        startIntent.putExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.STATE_ON);
        serviceTestRule.startService(startIntent);
        ArgumentCaptor<ProfileService> profile = ArgumentCaptor.forClass(profileServiceClass);
        verify(adapterService, timeout(SERVICE_TOGGLE_TIMEOUT_MS)).onProfileServiceStateChanged(
                profile.capture(), eq(BluetoothAdapter.STATE_ON));
        Assert.assertEquals(profileServiceClass.getName(), profile.getValue().getClass().getName());
    }

    /**
     * Stop a profile service using the given {@link ServiceTestRule} and verify through
     * {@link AdapterService#getAdapterService()} that the service is actually stopped within
     * {@link TestUtils#SERVICE_TOGGLE_TIMEOUT_MS} milliseconds.
     * {@link #setAdapterService(AdapterService)} must be called with a mocked
     * {@link AdapterService} before calling this method
     *
     * @param serviceTestRule the {@link ServiceTestRule} used to execute the service start request
     * @param profileServiceClass a class from one of {@link ProfileService}'s child classes
     * @throws TimeoutException when service failed to start within either default timeout of
     * {@link ServiceTestRule#DEFAULT_TIMEOUT} (normally 5s) or user specified time when creating
     * {@link ServiceTestRule} through {@link ServiceTestRule#withTimeout(long, TimeUnit)} method
     */
    public static <T extends ProfileService> void stopService(ServiceTestRule serviceTestRule,
            Class<T> profileServiceClass) throws TimeoutException {
        AdapterService adapterService = AdapterService.getAdapterService();
        Assert.assertNotNull(adapterService);
        Assert.assertTrue("AdapterService.getAdapterService() must return a mocked or spied object"
                + " before calling this method", MockUtil.isMock(adapterService));
        Intent stopIntent =
                new Intent(InstrumentationRegistry.getTargetContext(), profileServiceClass);
        stopIntent.putExtra(AdapterService.EXTRA_ACTION,
                AdapterService.ACTION_SERVICE_STATE_CHANGED);
        stopIntent.putExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.STATE_OFF);
        serviceTestRule.startService(stopIntent);
        ArgumentCaptor<ProfileService> profile = ArgumentCaptor.forClass(profileServiceClass);
        verify(adapterService, timeout(SERVICE_TOGGLE_TIMEOUT_MS)).onProfileServiceStateChanged(
                profile.capture(), eq(BluetoothAdapter.STATE_OFF));
        Assert.assertEquals(profileServiceClass.getName(), profile.getValue().getClass().getName());
    }

    /**
     * Create a test device.
     *
     * @param bluetoothAdapter the Bluetooth adapter to use
     * @param id the test device ID. It must be an integer in the interval [0, 0xFF].
     * @return {@link BluetoothDevice} test device for the device ID
     */
    public static BluetoothDevice getTestDevice(BluetoothAdapter bluetoothAdapter, int id) {
        Assert.assertTrue(id <= 0xFF);
        Assert.assertNotNull(bluetoothAdapter);
        BluetoothDevice testDevice =
                bluetoothAdapter.getRemoteDevice(String.format("00:01:02:03:04:%02X", id));
        Assert.assertNotNull(testDevice);
        return testDevice;
    }

    /**
     * Wait and verify that an intent has been received.
     *
     * @param timeoutMs the time (in milliseconds) to wait for the intent
     * @param queue the queue for the intent
     * @return the received intent
     */
    public static Intent waitForIntent(int timeoutMs, BlockingQueue<Intent> queue) {
        try {
            Intent intent = queue.poll(timeoutMs, TimeUnit.MILLISECONDS);
            Assert.assertNotNull(intent);
            return intent;
        } catch (InterruptedException e) {
            Assert.fail("Cannot obtain an Intent from the queue: " + e.getMessage());
        }
        return null;
    }

    /**
     * Wait and verify that no intent has been received.
     *
     * @param timeoutMs the time (in milliseconds) to wait and verify no intent
     * has been received
     * @param queue the queue for the intent
     * @return the received intent. Should be null under normal circumstances
     */
    public static Intent waitForNoIntent(int timeoutMs, BlockingQueue<Intent> queue) {
        try {
            Intent intent = queue.poll(timeoutMs, TimeUnit.MILLISECONDS);
            Assert.assertNull(intent);
            return intent;
        } catch (InterruptedException e) {
            Assert.fail("Cannot obtain an Intent from the queue: " + e.getMessage());
        }
        return null;
    }

    /**
     * Wait for looper to finish its current task and all tasks schedule before this
     *
     * @param looper looper of interest
     */
    public static void waitForLooperToFinishScheduledTask(Looper looper) {
        runOnLooperSync(looper, () -> {
            // do nothing, just need to make sure looper finishes current task
        });
    }

    /**
     * Run synchronously a runnable action on a looper.
     * The method will return after the action has been execution to completion.
     *
     * Example:
     * <pre>
     * {@code
     * TestUtils.runOnMainSync(new Runnable() {
     *       public void run() {
     *           Assert.assertTrue(mA2dpService.stop());
     *       }
     *   });
     * }
     * </pre>
     *
     * @param looper the looper used to run the action
     * @param action the action to run
     */
    public static void runOnLooperSync(Looper looper, Runnable action) {
        if (Looper.myLooper() == looper) {
            // requested thread is the same as the current thread. call directly.
            action.run();
        } else {
            Handler handler = new Handler(looper);
            SyncRunnable sr = new SyncRunnable(action);
            handler.post(sr);
            sr.waitForComplete();
        }
    }

    /**
     * Helper class used to run synchronously a runnable action on a looper.
     */
    private static final class SyncRunnable implements Runnable {
        private final Runnable mTarget;
        private volatile boolean mComplete = false;

        SyncRunnable(Runnable target) {
            mTarget = target;
        }

        @Override
        public void run() {
            mTarget.run();
            synchronized (this) {
                mComplete = true;
                notifyAll();
            }
        }

        public void waitForComplete() {
            synchronized (this) {
                while (!mComplete) {
                    try {
                        wait();
                    } catch (InterruptedException e) {
                    }
                }
            }
        }
    }
}
