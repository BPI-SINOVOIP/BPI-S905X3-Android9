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
 * limitations under the License
 */

package com.android.server.telecom.callfiltering;

import android.content.Context;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.provider.BlockedNumberContract;
import android.telecom.Log;
import android.telecom.Logging.Session;
import android.telecom.TelecomManager;

import com.android.internal.telephony.CallerInfo;
import com.android.server.telecom.Call;
import com.android.server.telecom.CallerInfoLookupHelper;
import com.android.server.telecom.LogUtils;
import com.android.server.telecom.settings.BlockedNumbersUtil;

/**
 * An {@link AsyncTask} that checks if a call needs to be blocked.
 * <p> An {@link AsyncTask} is used to perform the block check to avoid blocking the main thread.
 * The block check itself is performed in the {@link AsyncTask#doInBackground(Object[])}.
 */
public class AsyncBlockCheckFilter extends AsyncTask<String, Void, Boolean>
        implements IncomingCallFilter.CallFilter {
    private final Context mContext;
    private final BlockCheckerAdapter mBlockCheckerAdapter;
    private Call mIncomingCall;
    private Session mBackgroundTaskSubsession;
    private Session mPostExecuteSubsession;
    private CallFilterResultCallback mCallback;
    private CallerInfoLookupHelper mCallerInfoLookupHelper;

    public AsyncBlockCheckFilter(Context context, BlockCheckerAdapter blockCheckerAdapter,
            CallerInfoLookupHelper callerInfoLookupHelper) {
        mContext = context;
        mBlockCheckerAdapter = blockCheckerAdapter;
        mCallerInfoLookupHelper = callerInfoLookupHelper;
    }

    @Override
    public void startFilterLookup(Call call, CallFilterResultCallback callback) {
        mCallback = callback;
        mIncomingCall = call;
        String number = call.getHandle() == null ?
                null : call.getHandle().getSchemeSpecificPart();
        if (BlockedNumbersUtil.isEnhancedCallBlockingEnabledByPlatform(mContext)) {
            int presentation = mIncomingCall.getHandlePresentation();
            if (presentation == TelecomManager.PRESENTATION_ALLOWED) {
                mCallerInfoLookupHelper.startLookup(call.getHandle(),
                        new CallerInfoLookupHelper.OnQueryCompleteListener() {
                            @Override
                            public void onCallerInfoQueryComplete(Uri handle, CallerInfo info) {
                                boolean contactExists = info == null ? false : info.contactExists;
                                execute(number, String.valueOf(presentation),
                                        String.valueOf(contactExists));
                            }

                            @Override
                            public void onContactPhotoQueryComplete(Uri handle, CallerInfo info) {
                                // ignore
                            }
                        });
            } else {
                this.execute(number, String.valueOf(presentation));
            }
        } else {
            this.execute(number);
        }
    }

    @Override
    protected void onPreExecute() {
        mBackgroundTaskSubsession = Log.createSubsession();
        mPostExecuteSubsession = Log.createSubsession();
    }

    @Override
    protected Boolean doInBackground(String... params) {
        try {
            Log.continueSession(mBackgroundTaskSubsession, "ABCF.dIB");
            Log.addEvent(mIncomingCall, LogUtils.Events.BLOCK_CHECK_INITIATED);
            Bundle extras = new Bundle();
            if (params.length > 1) {
                extras.putInt(BlockedNumberContract.EXTRA_CALL_PRESENTATION,
                        Integer.valueOf(params[1]));
            }
            if (params.length > 2) {
                extras.putBoolean(BlockedNumberContract.EXTRA_CONTACT_EXIST,
                        Boolean.valueOf(params[2]));
            }
            return mBlockCheckerAdapter.isBlocked(mContext, params[0], extras);
        } finally {
            Log.endSession();
        }
    }

    @Override
    protected void onPostExecute(Boolean isBlocked) {
        Log.continueSession(mPostExecuteSubsession, "ABCF.oPE");
        try {
            CallFilteringResult result;
            if (isBlocked) {
                result = new CallFilteringResult(
                        false, // shouldAllowCall
                        true, //shouldReject
                        false, //shouldAddToCallLog
                        false // shouldShowNotification
                );
            } else {
                result = new CallFilteringResult(
                        true, // shouldAllowCall
                        false, // shouldReject
                        true, // shouldAddToCallLog
                        true // shouldShowNotification
                );
            }
            Log.addEvent(mIncomingCall, LogUtils.Events.BLOCK_CHECK_FINISHED, result);
            mCallback.onCallFilteringComplete(mIncomingCall, result);
        } finally {
            Log.endSession();
        }
    }
}
