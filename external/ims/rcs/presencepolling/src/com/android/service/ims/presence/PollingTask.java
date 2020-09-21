/*
 * Copyright (c) 2015, Motorola Mobility LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     - Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     - Neither the name of Motorola Mobility nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY LLC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

package com.android.service.ims.presence;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.SystemClock;
import android.text.format.Time;

import com.android.ims.internal.Logger;

import java.util.ArrayList;
import java.util.List;

public class PollingTask {
    private Logger logger = Logger.getLogger(this.getClass().getName());
    private Context mContext = null;

    private static long sMaxId = 0;
    public static final String ACTION_POLLING_RETRY_ALARM =
            "com.android.service.ims.presence.capability_polling_retry";
    private PendingIntent mRetryAlarmIntent = null;

    public long mId;
    public int mType;
    public List<Contacts.Item> mContacts = new ArrayList<Contacts.Item>();

    private long mTimeUnit;
    private int mTotalRetry;
    private int mCurrentRetry;
    private long mLastUpdateTime;

    private boolean mCancelled = false;
    private boolean mCompleted = false;

    public PollingTask(int type, List<Contacts.Item> list) {
        mId = sMaxId++;
        mType = type;

        mContacts.clear();
        for(int i = 0; i < list.size(); i++) {
            Contacts.Item item = list.get(i);
            mContacts.add(item);
        }

        mCurrentRetry = 0;
        mTotalRetry = 5;
        mTimeUnit = 1800; // 1800s = 30 minutes
        if (CapabilityPolling.ACTION_POLLING_NEW_CONTACTS == mType) {
            mTotalRetry = 4;
            mTimeUnit = 60; // 60s = 1 minute
        }
        mLastUpdateTime = 0;
    }

    public void execute() {
        PollingsQueue queue = PollingsQueue.getInstance(null);
        if (queue == null) {
            return;
        }

        PollingAction action = new PollingAction(queue.getContext(), this);
        action.execute();
    }

    public void finish(boolean fullUpdated) {
        cancelRetryAlarm();

        PollingsQueue queue = PollingsQueue.getInstance(null);
        if (queue == null) {
            return;
        }

        mCompleted = fullUpdated ? true : false;
        queue.remove(this);
    }

    public void retry() {
        mCurrentRetry += 1;
        logger.print("retry mCurrentRetry=" + mCurrentRetry);
        if (mCurrentRetry > mTotalRetry) {
            logger.print("Retry finished for task: " + this);
            updateTimeStampToCurrent();
            finish(false);
            return;
        }

        if (mCancelled) {
            logger.print("Task is cancelled: " + this);
            finish(false);
            return;
        }

        long delay = mTimeUnit;
        for (int i = 0; i < mCurrentRetry - 1; i++) {
            delay *= 2;
        }

        logger.print("retry delay=" + delay);

        scheduleRetry(delay * 1000);
    }

    public void cancel() {
        mCancelled = true;
        logger.print("Cancel this task: " + this);

        if (mRetryAlarmIntent != null) {
            cancelRetryAlarm();
            finish(false);
        }
    }

    public void onPreExecute() {
        logger.print("onPreExecute(), id = " + mId);
        cancelRetryAlarm();
    }

    public void onPostExecute(int result) {
        logger.print("onPostExecute(), result = " + result);
        mLastUpdateTime = System.currentTimeMillis();
    }

    private void updateTimeStampToCurrent() {
        if (mContext == null) {
            return;
        }

        EABContactManager contactManager = new EABContactManager(
                mContext.getContentResolver(), mContext.getPackageName());
        for (int i = 0; i < mContacts.size(); i++) {
            Contacts.Item item = mContacts.get(i);

            String number = item.number();
            long current = System.currentTimeMillis();
            EABContactManager.Request request = new EABContactManager.Request(number)
                    .setLastUpdatedTimeStamp(current);
            int result = contactManager.update(request);
            if (result <= 0) {
                logger.debug("failed to update timestamp for contact: ");
            }
        }
    }

    private void cancelRetryAlarm() {
        if (mRetryAlarmIntent != null) {
            if (mContext != null) {
                AlarmManager alarmManager = (AlarmManager)
                        mContext.getSystemService(Context.ALARM_SERVICE);
                alarmManager.cancel(mRetryAlarmIntent);
            }
            mRetryAlarmIntent = null;
        }
    }

    private void scheduleRetry(long msec) {
        logger.print("scheduleRetry msec=" + msec);

        cancelRetryAlarm();

        if (mContext == null) {
            PollingsQueue queue = PollingsQueue.getInstance(null);
            if (queue != null) {
                mContext = queue.getContext();
            }
        }

        if (mContext != null) {
            Intent intent = new Intent(ACTION_POLLING_RETRY_ALARM);
            intent.setClass(mContext, AlarmBroadcastReceiver.class);
            intent.putExtra("pollingTaskId", mId);

            mRetryAlarmIntent = PendingIntent.getBroadcast(mContext, 0, intent,
                    PendingIntent.FLAG_UPDATE_CURRENT);

            AlarmManager alarmManager = (AlarmManager)
                    mContext.getSystemService(Context.ALARM_SERVICE);
            alarmManager.setExact(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                    SystemClock.elapsedRealtime() + msec, mRetryAlarmIntent);
        }
    }

    private String getTimeString(long time) {
        if (time <= 0) {
            time = System.currentTimeMillis();
        }

        Time tobj = new Time();
        tobj.set(time);
        return String.format("%s.%s", tobj.format("%m-%d %H:%M:%S"), time % 1000);
    }

    public boolean isCompleted() {
        return mCompleted;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof PollingTask)) return false;

        PollingTask that = (PollingTask)o;
        return (this.mId == that.mId) && (this.mType == that.mType);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("PollingTask { ");
        sb.append("\nId: " + mId);
        sb.append("\nType: " + mType);
        sb.append("\nCurrentRetry: " + mCurrentRetry);
        sb.append("\nTotalRetry: " + mTotalRetry);
        sb.append("\nTimeUnit: " + mTimeUnit);
        sb.append("\nLast update time: " + mLastUpdateTime + "(" +
                getTimeString(mLastUpdateTime) + ")");
        sb.append("\nContacts: " + mContacts.size());
        for (int i = 0; i < mContacts.size(); i++) {
            sb.append("\n");
            Contacts.Item item = mContacts.get(i);
            sb.append("Number " + i + ": " + Logger.hidePhoneNumberPii(item.number()));
        }
        sb.append("\nCompleted: " + mCompleted);
        sb.append(" }");
        return sb.toString();
    }
}

