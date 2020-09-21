/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.util.sl4a;

import static org.mockito.Mockito.doReturn;

import com.android.tradefed.util.sl4a.Sl4aEventDispatcher.EventSl4aObject;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mockito;

import java.io.IOException;
import java.util.List;
import java.util.function.Predicate;

/**
 * Test class for {@link Sl4aEventDispatcher}.
 */
public class Sl4aEventDispatcherTest {

    private static final long FAKE_TIMEOUT_MS = 500;
    private static final long SHORT_TIMEOUT_MS = 100;
    private Sl4aEventDispatcher mEventDispatcher;
    private Sl4aClient mClient;

    @Before
    public void setUp() {
        mClient = Mockito.mock(Sl4aClient.class);
        mEventDispatcher = new Sl4aEventDispatcher(mClient, FAKE_TIMEOUT_MS);
    }

    @After
    public void tearDown() {
        if (mEventDispatcher != null) {
            mEventDispatcher.clearAllEvents();
        }
    }

    /**
     * Test the {@link Sl4aEventDispatcher#internalPolling()} polling of events.
     */
    @Test
    public void testPollEvents() throws IOException {
        doReturn("{\"name\":\"BluetoothStateChangedOff\","
                + "\"data\":{\"State\":\"OFF\"},\"time\":29519495823}")
                .when(mClient).rpcCall("eventWait", FAKE_TIMEOUT_MS);
        Assert.assertTrue(mEventDispatcher.internalPolling());
    }

    /**
     * Test the {@link Sl4aEventDispatcher#internalPolling()} when it receives a shutdown.
     */
    @Test
    public void testPollEvents_shutdown() throws IOException {
        doReturn("{\"name\":\""+ Sl4aEventDispatcher.SHUTDOWN_EVENT + "\","
                + "\"data\":null,\"time\":29519495823}")
                .when(mClient).rpcCall("eventWait", FAKE_TIMEOUT_MS);
        Assert.assertFalse(mEventDispatcher.internalPolling());
    }

    /**
     * Test {@link Sl4aEventDispatcher#popEvent(String, long)} for an existing event.
     */
    @Test
    public void testPopEvent() throws IOException {
        // Add an event to our cache.
        doReturn("{\"name\":\"BluetoothStateChangedOff\","
                + "\"data\":{\"State\":\"OFF\"},\"time\":29519495823}")
                .when(mClient).rpcCall("eventWait", FAKE_TIMEOUT_MS);
        Assert.assertTrue(mEventDispatcher.internalPolling());
        EventSl4aObject event =
                mEventDispatcher.popEvent("BluetoothStateChangedOff", FAKE_TIMEOUT_MS);
        Assert.assertEquals(29519495823L, event.getTime());
        Assert.assertEquals("BluetoothStateChangedOff", event.getName());
        Assert.assertEquals("{\"State\":\"OFF\"}", event.getData());
    }

    /**
     * Test {@link Sl4aEventDispatcher#popEvent(String, long)} for a non existing event.
     */
    @Test
    public void testPopEvent_notExisting() {
        EventSl4aObject event =
                mEventDispatcher.popEvent("BluetoothStateChangedOff", SHORT_TIMEOUT_MS);
        Assert.assertNull(event);
    }

    /**
     * Test {@link Sl4aEventDispatcher#popAllEvents(String)} for several cached events.
     */
    @Test
    public void testPopAllEvents() throws IOException {
        // Add 2 events to our cache.
        doReturn("{\"name\":\"BluetoothStateChangedOff\","
                + "\"data\":{\"State\":\"OFF\"},\"time\":29519495823}")
                .when(mClient).rpcCall("eventWait", FAKE_TIMEOUT_MS);
        Assert.assertTrue(mEventDispatcher.internalPolling());
        Assert.assertTrue(mEventDispatcher.internalPolling());
        List<EventSl4aObject> result = mEventDispatcher.popAllEvents("BluetoothStateChangedOff");
        Assert.assertEquals(2, result.size());
    }

    /**
     * Test {@link Sl4aEventDispatcher#popAllEvents(String)} for no cached events.
     */
    @Test
    public void testPopAllEvents_noEvent() {
        List<EventSl4aObject> result = mEventDispatcher.popAllEvents("BluetoothStateChangedOff");
        Assert.assertEquals(0, result.size());
    }

    /**
     * Test {@link Sl4aEventDispatcher#clearEvents(String)}.
     */
    @Test
    public void testClearEvents() throws IOException {
        // Add 2 events to our cache.
        doReturn("{\"name\":\"BluetoothStateChangedOff\","
                + "\"data\":{\"State\":\"OFF\"},\"time\":29519495823}")
                .when(mClient).rpcCall("eventWait", FAKE_TIMEOUT_MS);
        Assert.assertTrue(mEventDispatcher.internalPolling());
        Assert.assertTrue(mEventDispatcher.internalPolling());
        doReturn("{\"name\":\"NotCleared\","
                + "\"data\":{\"State\":\"OFF\"},\"time\":29519495823}")
                .when(mClient).rpcCall("eventWait", FAKE_TIMEOUT_MS);
        Assert.assertTrue(mEventDispatcher.internalPolling());
        // clear the events by name
        mEventDispatcher.clearEvents("BluetoothStateChangedOff");
        List<EventSl4aObject> result = mEventDispatcher.popAllEvents("BluetoothStateChangedOff");
        // it should be 0 for the event cleared
        Assert.assertEquals(0, result.size());
        result = mEventDispatcher.popAllEvents("NotCleared");
        // it should be 1 for the event not cleared
        Assert.assertEquals(1, result.size());
    }

    /**
     * Test {@link Sl4aEventDispatcher#waitForEvent(String, Predicate, long)} for an existing event
     * matching the predicate.
     */
    @Test
    public void testWaitForEvent() throws IOException {
        Predicate<EventSl4aObject> testPredicate = new Predicate<EventSl4aObject>() {
            @Override
            public boolean test(EventSl4aObject t) {
                return t.getData().contains("OFF");
            }
        };
        // Add an event to our cache.
        doReturn("{\"name\":\"BluetoothStateChangedOff\","
                + "\"data\":{\"State\":\"OFF\"},\"time\":29519495823}")
                .when(mClient).rpcCall("eventWait", FAKE_TIMEOUT_MS);
        Assert.assertTrue(mEventDispatcher.internalPolling());
        EventSl4aObject event = mEventDispatcher.waitForEvent("BluetoothStateChangedOff",
                testPredicate, FAKE_TIMEOUT_MS);
        Assert.assertNotNull(event);
        Assert.assertEquals(29519495823L, event.getTime());
        Assert.assertEquals("BluetoothStateChangedOff", event.getName());
        Assert.assertEquals("{\"State\":\"OFF\"}", event.getData());
    }

    /**
     * Test {@link Sl4aEventDispatcher#waitForEvent(String, Predicate, long)} for an existing event
     * not matching the predicate.
     */
    @Test
    public void testWaitForEvent_notMatching() throws IOException {
        Predicate<EventSl4aObject> testPredicate = new Predicate<EventSl4aObject>() {
            @Override
            public boolean test(EventSl4aObject t) {
                return t.getData().contains("ON");
            }
        };
        // Add an event to our cache.
        doReturn("{\"name\":\"BluetoothStateChangedOff\","
                + "\"data\":{\"State\":\"OFF\"},\"time\":29519495823}")
                .when(mClient).rpcCall("eventWait", FAKE_TIMEOUT_MS);
        Assert.assertTrue(mEventDispatcher.internalPolling());
        EventSl4aObject event = mEventDispatcher.waitForEvent("BluetoothStateChangedOff",
                testPredicate, SHORT_TIMEOUT_MS);
        Assert.assertNull(event);
    }
}