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

import android.content.Context;

import com.android.ims.internal.Logger;

import java.util.ArrayList;
import java.util.List;

public class PollingsQueue {
    private Logger logger = Logger.getLogger(this.getClass().getName());

    private Context mContext;
    private CapabilityPolling mCapabilityPolling;
    private List<PollingTask> mPollingTasks = new ArrayList<PollingTask>();
    private boolean mAskVerifyResult = false;
    private int mVerifyCounts = 0;

    private static PollingsQueue sInstance = null;
    public static synchronized PollingsQueue getInstance(Context context) {
        if ((sInstance == null) && (context != null)) {
            sInstance = new PollingsQueue(context);
        }

        return sInstance;
    }

    private PollingsQueue(Context context) {
        mContext = context;
        mPollingTasks.clear();
    }

    public synchronized void setCapabilityPolling(CapabilityPolling cp) {
        mCapabilityPolling = cp;
    }

    public CapabilityPolling getCapabilityPolling() {
        return mCapabilityPolling;
    }

    public Context getContext() {
        return mContext;
    }

    public synchronized void clear() {
        mPollingTasks.clear();
    }

    public synchronized void add(int type, List<Contacts.Item> list) {
        if (list.size() <= 0) {
            return;
        }

        List<Contacts.Item> contacts = new ArrayList<Contacts.Item>();
        contacts.clear();
        for(int i = 0; i < list.size(); i++) {
            Contacts.Item item = list.get(i);

            boolean bExist = false;
            for(int j = 0; j < contacts.size(); j++) {
                Contacts.Item item0 = contacts.get(j);
                if (item.equals(item0)) {
                    bExist = true;
                    break;
                }
            }

            for (int j = 0; j < mPollingTasks.size(); j++) {
                if (bExist) {
                    break;
                }
                PollingTask task = mPollingTasks.get(j);
                for(int k = 0; k < task.mContacts.size(); k++) {
                    Contacts.Item item0 = task.mContacts.get(k);
                    if (item.equals(item0)) {
                        bExist = true;
                        break;
                    }
                }
            }
            if (bExist) {
                continue;
            }

            contacts.add(item);
            if (type == CapabilityPolling.ACTION_POLLING_NORMAL) {
                if (item.lastUpdateTime() == 0) {
                    type = CapabilityPolling.ACTION_POLLING_NEW_CONTACTS;
                }
            }
        }

        PollingTask taskCancelled = null;
        int nTasks = mPollingTasks.size();
        logger.print("Before add(), the existing tasks number: " + nTasks);

        if (contacts.size() <= 0) {
            logger.debug("Empty/duplicated list, no request added.");
            return;
        }

        int maxEntriesInRequest = PresenceSetting.getMaxNumberOfEntriesInRequestContainedList();
        logger.print("getMaxNumberOfEntriesInRequestContainedList: " + maxEntriesInRequest);
        if (maxEntriesInRequest == -1) {
            maxEntriesInRequest = 100;
        }

        int noOfIterations = contacts.size() / maxEntriesInRequest;
        for (int loop = 0; loop <= noOfIterations; loop++) {
            int entriesInRequest = maxEntriesInRequest;
            if (loop == noOfIterations) {
                entriesInRequest = contacts.size() % maxEntriesInRequest;
            }

            List<Contacts.Item> cl = new ArrayList<Contacts.Item>();
            cl.clear();
            int pos = loop * maxEntriesInRequest;
            for (int i = 0; i < entriesInRequest; i++) {
                Contacts.Item item = contacts.get(pos);
                cl.add(item);
                pos++;
            }

            if (cl.size() > 0) {
                PollingTask task = new PollingTask(type, cl);
                logger.debug("One new polling task added: " + task);

                boolean bInserted = false;
                for (int i = 0; i < mPollingTasks.size(); i++) {
                    PollingTask task0 = mPollingTasks.get(i);
                    if (task.mType > task0.mType) {
                        bInserted = true;
                        mPollingTasks.add(i, task);
                        if ((i == 0) && (taskCancelled == null)) {
                            taskCancelled = task0;
                        }
                        break;
                    }
                }
                if (!bInserted) {
                    mPollingTasks.add(task);
                }
            }
        }

        logger.print("After add(), the total tasks number: " + mPollingTasks.size());
        if (nTasks <= 0) {
            if (mPollingTasks.size() > 0) {
                PollingTask task = mPollingTasks.get(0);
                task.execute();
            }
        } else {
            if (taskCancelled != null) {
                taskCancelled.cancel();
            }
        }
    }

    public synchronized void remove(PollingTask task) {
        int nTasks = mPollingTasks.size();
        if (nTasks <= 0) {
            return;
        }

        logger.debug("Remove polling task: " + task);
        if (task != null) {
            for (int i = 0; i < nTasks; i++) {
                PollingTask task0 = mPollingTasks.get(i);
                if (task0.equals(task)) {
                    if (task.isCompleted()) {
                        mVerifyCounts = 0;
                    } else {
                        mAskVerifyResult = true;
                    }
                    mPollingTasks.remove(i);
                    break;
                }
            }
        }

        if (mPollingTasks.size() > 0) {
            PollingTask task0 = mPollingTasks.get(0);
            task0.execute();
        } else {
            if (mAskVerifyResult) {
                mAskVerifyResult = false;
                mVerifyCounts++;
                if (mCapabilityPolling != null) {
                    mCapabilityPolling.enqueueVerifyPollingResult(mVerifyCounts);
                }
            }
        }
    }

    public synchronized void retry(long id) {
        int nTasks = mPollingTasks.size();
        if (nTasks <= 0) {
            return;
        }

        PollingTask task0 = null;
        for (int i = 0; i < nTasks; i++) {
            PollingTask task = mPollingTasks.get(i);
            if (task.mId == id) {
                task0 = task;
                break;
            }
        }

        if (task0 == null) {
            logger.debug("Trigger wrong retry: " + id);
            task0 = mPollingTasks.get(0);
        }

        task0.execute();
    }
}

