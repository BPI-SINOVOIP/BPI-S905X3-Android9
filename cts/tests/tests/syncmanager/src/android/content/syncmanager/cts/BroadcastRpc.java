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
package android.content.syncmanager.cts;

import static android.content.syncmanager.cts.common.Values.getCommReceiver;

import android.content.syncmanager.cts.SyncManagerCtsProto.Payload;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request.Builder;

import com.android.compatibility.common.util.BroadcastRpcBase;

import com.google.protobuf.InvalidProtocolBufferException;

import java.util.function.Consumer;

public class BroadcastRpc extends BroadcastRpcBase<Payload, Payload> {
    @Override
    protected byte[] requestToBytes(Payload testServiceRequest) {
        return testServiceRequest.toByteArray();
    }

    @Override
    protected Payload bytesToResponse(byte[] bytes) {
        try {
            return Payload.parseFrom(bytes);
        } catch (InvalidProtocolBufferException e) {
            throw new RuntimeException("InvalidProtocolBufferException", e);
        }
    }

    public Payload.Response invoke(String targetPackage, Consumer<Builder> c) throws Exception {
        final Builder rb =  Request.newBuilder();
        c.accept(rb);
        final Request req = rb.build();
        return super.invoke(getCommReceiver(targetPackage),
                Payload.newBuilder().setRequest(req).build()).getResponse();
    }
}
