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
package android.os.cts.batterysaving;

import static android.os.cts.batterysaving.common.Values.getCommReceiver;

import android.os.cts.batterysaving.common.BatterySavingCtsCommon.Payload;

import com.android.compatibility.common.util.BroadcastRpcBase;

import com.google.protobuf.InvalidProtocolBufferException;

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

    public Payload sendRequest(String targetPackage,
            Payload testServiceRequest) throws Exception {
        return super.invoke(getCommReceiver(targetPackage), testServiceRequest);
    }
}
