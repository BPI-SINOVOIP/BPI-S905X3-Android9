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
import android.os.AsyncTask;
import android.os.Looper;

import com.android.ims.IRcsPresenceListener;
import com.android.ims.RcsManager;
import com.android.ims.RcsPresence;
import com.android.ims.RcsException;
import com.android.ims.RcsManager.ResultCode;
import com.android.ims.internal.Logger;

import java.util.ArrayList;
import java.util.List;

public class PollingAction extends AsyncTask<Void, Integer, Integer> {
    private Logger logger = Logger.getLogger(this.getClass().getName());

    private Context mContext;
    private PollingTask mPollingTask;
    private int mResult;
    private int mRequestId = -1;

    private final Object mPollingSyncObj = new Object();
    private boolean mIsPolling = false;
    private boolean mFullUpdated = false;

    private IRcsPresenceListener mClientListener = new IRcsPresenceListener.Stub() {
        public void onSuccess(int reqId) {
            logger.print("onSuccess() is called. reqId=" + reqId);
        }

        public void onError(int reqId, int code) {
            logger.print("onError() is called, reqId=" + reqId + " error code: " + code);
            synchronized(mPollingSyncObj) {
                mResult = code;
                mIsPolling = false;
                mPollingSyncObj.notifyAll();
            }
        }

        public void onFinish(int reqId) {
            logger.print("onFinish() is called, reqId=" + reqId);
            if (reqId == mRequestId) {
                synchronized(mPollingSyncObj) {
                    mFullUpdated = true;
                    mIsPolling = false;
                    mPollingSyncObj.notifyAll();
                }
            }
        }

        public void onTimeout(int reqId) {
            logger.print("onTimeout() is called, reqId=" + reqId);
            if (reqId == mRequestId) {
                synchronized(mPollingSyncObj) {
                    mIsPolling = false;
                    mPollingSyncObj.notifyAll();
                }
            }
        }
    };

    public PollingAction(Context context, PollingTask task) {
        mContext = context;
        mPollingTask = task;
        logger.info("PollingAction(), task=" + mPollingTask);
    }

    @Override
    protected void onPreExecute() {
        super.onPreExecute();
        mPollingTask.onPreExecute();
    }

    @Override
    protected Integer doInBackground(Void... params) {
        logger.debug("doInBackground(), Thread = " + Thread.currentThread().getName());

        if (Looper.myLooper() == null) {
            Looper.prepare();
        }

        int requestExpiration = PresenceSetting.getCapabilityPollListSubscriptionExpiration();
        logger.print("getCapabilityPollListSubscriptionExpiration: " + requestExpiration);
        if (requestExpiration == -1) {
            requestExpiration = 30;
        }
        requestExpiration += 30;

        mResult = ResultCode.SUBSCRIBE_NOT_FOUND;
        ArrayList<String> uriList = new ArrayList<String>();
        uriList.clear();

        List<Contacts.Item> contacts = mPollingTask.mContacts;
        for (int i = 0; i < contacts.size(); i++) {
            Contacts.Item item = contacts.get(i);
            uriList.add(getCompleteUri(item.number()));
        }
        int size = uriList.size();
        if (size <= 0) {
            logger.debug("No contacts in polling task, no action.");
        } else {
            mResult = ResultCode.ERROR_SERVICE_NOT_AVAILABLE;
            synchronized(mPollingSyncObj) {
                RcsManager rcsManager = RcsManager.getInstance(mContext, 0);
                if (rcsManager == null) {
                    logger.debug("rcsManager == null");
                } else {
                    try {
                        RcsPresence rcsPresence = rcsManager.getRcsPresenceInterface();
                        if (rcsPresence == null) {
                            logger.debug("rcsPresence == null");
                        } else {
                            logger.print("call requestCapability: " + size);
                            // If ret > 0 then it is the request Id, or it is result code.
                            int ret = rcsPresence.requestCapability(uriList, mClientListener);
                            if (ret > 0) {
                                mRequestId = ret;
                                mResult = ResultCode.SUCCESS;
                            } else {
                                mRequestId = -1;
                                mResult = ret;
                            }
                        }
                    } catch (RcsException ex) {
                        logger.print("RcsException", ex);
                    }
                }

                if (mResult == ResultCode.SUCCESS) {
                    logger.print("Capability discovery success, RequestId = " + mRequestId);
                    mIsPolling = true;
                } else {
                    logger.info("Capability discovery failure result = " + mResult);
                    mIsPolling = false;
                }

                final long endTime = System.currentTimeMillis() + requestExpiration * 1000;
                while(true) {
                    if (!mIsPolling) {
                        break;
                    }

                    long delay = endTime - System.currentTimeMillis();
                    if (delay <= 0) {
                        break;
                    }

                    try {
                        mPollingSyncObj.wait(delay);
                    } catch (InterruptedException ex) {
                    }
                }
            }
        }

        logger.print("The action final result = " + mResult);
        mPollingTask.onPostExecute(mResult);
        if (ResultCode.SUBSCRIBE_TEMPORARY_ERROR == mResult) {
            mPollingTask.retry();
        } else {
            mPollingTask.finish(mFullUpdated);
        }

        return mResult;
    }

    private String getCompleteUri(String phone) {
        phone = "tel:" + phone;
        return phone;
    }

    @Override
    protected void onPostExecute(Integer result) {
        super.onPostExecute(result);
        logger.print("onPostExecute(), result = " + result);
    }
}

