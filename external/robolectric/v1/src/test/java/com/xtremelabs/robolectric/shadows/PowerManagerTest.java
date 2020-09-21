package com.xtremelabs.robolectric.shadows;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertTrue;

import com.xtremelabs.robolectric.Robolectric;
import com.xtremelabs.robolectric.WithTestDefaultsRunner;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import android.content.Context;
import android.os.PowerManager;

@RunWith(WithTestDefaultsRunner.class)
public class PowerManagerTest {

    PowerManager powerManager;
    ShadowPowerManager shadowPowerManager;

    @Before
    public void before() {
        powerManager = (PowerManager) Robolectric.application.getSystemService(Context.POWER_SERVICE);
        shadowPowerManager = Robolectric.shadowOf(powerManager);
    }

    @Test
    public void testIsScreenOn() {
        assertTrue(powerManager.isScreenOn());
        shadowPowerManager.setIsScreenOn(false);
        assertFalse(powerManager.isScreenOn());
    }

    @Test
    public void shouldCreateWakeLock() throws Exception {
        assertNotNull(powerManager.newWakeLock(0, "TAG"));
    }

    @Test
    public void shouldAcquireAndReleaseReferenceCountedLock() throws Exception {
        PowerManager.WakeLock lock = powerManager.newWakeLock(0, "TAG");
        assertFalse(lock.isHeld());
        lock.acquire();
        assertTrue(lock.isHeld());
        lock.acquire();

        assertTrue(lock.isHeld());
        lock.release();

        assertTrue(lock.isHeld());
        lock.release();
        assertFalse(lock.isHeld());
    }

    @Test
    public void shouldAcquireAndReleaseNonReferenceCountedLock() throws Exception {
        PowerManager.WakeLock lock = powerManager.newWakeLock(0, "TAG");
        lock.setReferenceCounted(false);

        assertFalse(lock.isHeld());
        lock.acquire();
        assertTrue(lock.isHeld());
        lock.acquire();
        assertTrue(lock.isHeld());

        lock.release();

        assertFalse(lock.isHeld());
    }

    @Test(expected = RuntimeException.class)
    public void shouldThrowRuntimeExceptionIfLockisUnderlocked() throws Exception {
        PowerManager.WakeLock lock = powerManager.newWakeLock(0, "TAG");
        lock.release();
    }
}