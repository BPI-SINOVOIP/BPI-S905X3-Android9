/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.tradefed.command.remote;

import org.junit.Test;

/**
 * Unit tests for {@link RemoteOperation}.
 */
public class RemoteOperationTest {

    /**
     * Test that exception is thrown when incoming data has a different than expected protocol
     * version.
     */
    @Test(expected = RemoteException.class)
    public void testProtocolVersionMismatch() throws RemoteException {
        CloseOp o = new CloseOp();
        String data = o.pack(0);
        RemoteOperation.createRemoteOpFromString(data);
    }
}
