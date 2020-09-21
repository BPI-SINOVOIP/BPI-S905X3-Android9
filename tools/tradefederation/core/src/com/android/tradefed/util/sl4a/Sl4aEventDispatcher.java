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

import com.android.tradefed.log.LogUtil.CLog;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.function.Predicate;

/**
 * Event dispatcher polls for event and queue them by name to be queried.
 * TODO: add support for event handlers.
 */
public class Sl4aEventDispatcher extends Thread {

    private Sl4aClient mClient;
    private long mTimeout;

    public static final String SHUTDOWN_EVENT = "EventDispatcherShutdown";

    // The queues of events
    private Map<String, LinkedList<EventSl4aObject>> mEventQueue = new HashMap<>();

    private boolean mCanceled = false;

    public Sl4aEventDispatcher(Sl4aClient client, long timeout) {
        this.setName(getClass().getCanonicalName());
        this.setDaemon(true);
        mClient = client;
        mTimeout = timeout;
    }

    /**
     * Continuously poll events and cache them.
     */
    private void pollEvents() {
        while (!mCanceled) {
            if (!internalPolling()) {
                break;
            }
        }
    }

    /**
     * Internal polling of events, should not be called.
     * Exposed for testing.
     */
    protected boolean internalPolling() {
        try {
            Object response = mClient.rpcCall("eventWait", mTimeout);
            if (response == null) {
                return true;
            }
            EventSl4aObject event = new EventSl4aObject(new JSONObject(response.toString()));
            if (SHUTDOWN_EVENT.equals(event.getName())) {
                mCanceled = true;
                return false;
            }
            synchronized(mEventQueue) {
                if (mEventQueue.containsKey(event.getName())) {
                    mEventQueue.get(event.getName()).add(event);
                } else {
                    LinkedList<EventSl4aObject> queue = new LinkedList<>();
                    queue.add(event);
                    mEventQueue.put(event.getName(), queue);
                }
            }
        } catch (IOException e) {
            CLog.w("Error '%s' when polling the events.", e);
        } catch (JSONException e) {
            CLog.e(e);
        }
        return true;
    }

    @Override
    public void run() {
        pollEvents();
    }

    /**
     * Stop the thread execution and clean up all the events.
     */
    public void cancel() {
        mCanceled = true;
        this.interrupt();
        clearAllEvents();
    }

    /**
     * Poll for one event by name
     *
     * @param name the name of the event.
     * @param timeout the timeout in millisecond for the pop event to return.
     * @return the {@link EventSl4aObject} or null if no event found.
     */
    public EventSl4aObject popEvent(String name, long timeout) {
        long deadline = System.currentTimeMillis() + timeout;
        while (System.currentTimeMillis() < deadline){
            synchronized (mEventQueue) {
                if (mEventQueue.get(name) != null) {
                    // find a remove the first element of the list, null if empty.
                    EventSl4aObject res = mEventQueue.get(name).poll();
                    if (res != null) {
                        return res;
                    }
                }
            }
        }
        CLog.e("Timeout after waiting %sms for event '%s'", timeout, name);
        return null;
    }

    /**
     * Poll for a particular event that match the name and predicate.
     *
     * @param name the name of the event.
     * @param predicate the predicate the event needs to pass.
     * @param timeout timeout the timeout in millisecond for the pop event to return.
     * @return the {@link EventSl4aObject} or null if no event found.
     */
    public EventSl4aObject waitForEvent(String name, Predicate<EventSl4aObject> predicate,
            long timeout) {
        long deadline = System.currentTimeMillis() + timeout;
        while (System.currentTimeMillis() < deadline){
            synchronized (mEventQueue) {
                if (mEventQueue.get(name) != null) {
                    // find a remove the first element of the list, null if empty.
                    EventSl4aObject res = mEventQueue.get(name).poll();
                    if (res != null && predicate.test(res)) {
                        return res;
                    }
                }
            }
        }
        CLog.e("Timeout after waiting %sms for event '%s'", timeout, name);
        return null;
    }

    /**
     * Return all the events of one type, or empty list if no event.
     */
    public List<EventSl4aObject> popAllEvents(String name) {
        synchronized (mEventQueue) {
            if (mEventQueue.get(name) != null) {
                List<EventSl4aObject> results = new LinkedList<EventSl4aObject>();
                results.addAll(mEventQueue.get(name));
                mEventQueue.get(name).clear();
                return results;
            }
            return new LinkedList<EventSl4aObject>();
        }
    }

    /**
     * Clear all the events for one event name.
     */
    public void clearEvents(String name) {
        synchronized (mEventQueue) {
            if (mEventQueue.get(name) != null) {
                mEventQueue.get(name).clear();
            }
        }
    }

    /**
     * clear all the events
     */
    public void clearAllEvents() {
        synchronized (mEventQueue) {
            for (String key : mEventQueue.keySet()) {
                mEventQueue.get(key).clear();
            }
            mEventQueue.clear();
        }
    }

    /**
     * Object returned by the event poller.
     */
    public class EventSl4aObject {
        private String mName = null;
        private String mData = null;
        private long mTime = 0L;

        public EventSl4aObject(JSONObject response) throws JSONException {
            mName = response.getString("name");
            mData = response.get("data").toString();
            mTime = response.getLong("time");
        }

        public String getName() {
            return mName;
        }

        public String getData() {
            return mData;
        }

        public long getTime() {
            return mTime;
        }

        @Override
        public String toString() {
            return "EventSl4aObject [mName=" + mName + ", mData=" + mData + ", mTime=" + mTime
                    + "]";
        }
    }
}
