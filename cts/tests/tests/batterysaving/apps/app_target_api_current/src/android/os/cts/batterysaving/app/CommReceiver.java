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
package android.os.cts.batterysaving.app;

import static android.os.cts.batterysaving.common.Values.KEY_REQUEST_FOREGROUND;
import static android.os.cts.batterysaving.common.Values.getTestService;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload;
import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload.TestServiceRequest.SetAlarmRequest;
import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload.TestServiceResponse;
import android.util.Log;

import com.android.compatibility.common.util.BroadcastRpcBase;

import com.google.protobuf.InvalidProtocolBufferException;

public class CommReceiver extends BroadcastRpcBase.ReceiverBase<Payload, Payload> {
    private static final String TAG = "CommReceiver";

    @Override
    protected Payload handleRequest(Context context, Payload request) {
        final Payload.Builder responseBuilder = Payload.newBuilder();
        if (request.hasTestServiceRequest()) {
            handleBatterySaverBgServiceRequest(context, request, responseBuilder);
        }
        return responseBuilder.build();
    }

    private void handleBatterySaverBgServiceRequest(Context context,
            Payload request, Payload.Builder responseBuilder) {
        final TestServiceResponse.Builder rb = TestServiceResponse.newBuilder();

        if (request.getTestServiceRequest().getClearLastIntent()) {
            // Request to clear the last intent to TestService.

            TestService.LastStartIntent.set(null);
            rb.setClearLastIntentAck(true);

        } else if (request.getTestServiceRequest().getGetLastIntent()) {
            // Request to return the last intent action that started TestService.

            final Intent intent = TestService.LastStartIntent.get();
            if (intent != null) {
                rb.setGetLastIntentAction(intent.getAction());
            }

        } else if (request.getTestServiceRequest().hasStartService()) {
            // Request to start TestService with a given action.

            final String action = request.getTestServiceRequest().getStartService().getAction();
            final boolean fg = request.getTestServiceRequest().getStartService().getForeground();

            final Intent intent = new Intent(action)
                    .setComponent(getTestService(context.getPackageName()))
                    .putExtra(KEY_REQUEST_FOREGROUND, fg);

            Log.d(TAG, "Starting service " + intent);

            if (fg) {
                context.startForegroundService(intent);
            } else {
                context.startService(intent);
            }
            rb.setStartServiceAck(true);

        } else if (request.getTestServiceRequest().hasSetAlarm()) {
            // Set an alarm with a given intent.

            final SetAlarmRequest req = request.getTestServiceRequest().getSetAlarm();

            final AlarmManager am = context.getSystemService(AlarmManager.class);

            final int type = req.getType();
            final long triggerTime = req.getTriggerTime();
            final long interval = req.getRepeatInterval();
            final boolean allowWhileIdle = req.getAllowWhileIdle();

            final PendingIntent alarmSender = PendingIntent.getBroadcast(context, 1,
                    new Intent(req.getIntentAction()), 0);

            Log.d(TAG, "Setting alarm: type=" + type + ", triggerTime=" + triggerTime
                    + ", interval=" + interval + ", allowWhileIdle=" + allowWhileIdle);
            if (interval > 0) {
                am.setRepeating(type, triggerTime, interval, alarmSender);
            } else if (allowWhileIdle) {
                am.setExactAndAllowWhileIdle(type, triggerTime, alarmSender);
            } else {
                am.setExact(type, triggerTime, alarmSender);
            }
            rb.setSetAlarmAck(true);
        }

        responseBuilder.setTestServiceResponse(rb);
    }

    @Override
    protected byte[] responseToBytes(Payload payload) {
        return payload.toByteArray();
    }

    @Override
    protected Payload bytesToRequest(byte[] bytes) {
        try {
            return Payload.parseFrom(bytes);
        } catch (InvalidProtocolBufferException e) {
            throw new RuntimeException("InvalidProtocolBufferException", e);
        }
    }
}
