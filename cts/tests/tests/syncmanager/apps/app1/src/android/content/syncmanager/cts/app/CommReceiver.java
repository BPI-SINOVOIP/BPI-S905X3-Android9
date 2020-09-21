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
package android.content.syncmanager.cts.app;

import android.content.Context;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Response.SyncInvocations;

import com.android.compatibility.common.util.BroadcastRpcBase;

import com.google.protobuf.InvalidProtocolBufferException;

public class CommReceiver extends BroadcastRpcBase.ReceiverBase<Payload, Payload> {
    @Override
    protected Payload handleRequest(Context context, Payload request) throws Exception {
        ContextHolder.sContext = context;

        final Request req = request.getRequest();
        final Payload.Response.Builder res = Payload.Response.newBuilder();

        if (req.hasAddAccount()) {
            SyncManagerCtsAuthenticator.ensureTestAccount(req.getAddAccount().getName());

        } else if (req.hasRemoveAllAccounts()) {
            SyncManagerCtsAuthenticator.removeAllAccounts();

        } else if (req.hasClearSyncInvocations()) {
            SyncManagerCtsSyncAdapter.clearSyncInvocations();

        } else if (req.hasGetSyncInvocations()) {
            res.setSyncInvocations(SyncInvocations.newBuilder()
                    .addAllSyncInvocations(SyncManagerCtsSyncAdapter.getSyncInvocations()));

        } else if (req.hasSetResult()) {
            SyncManagerCtsSyncAdapter.setResult(req.getSetResult());

        }

        return Payload.newBuilder().setResponse(res).build();
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
