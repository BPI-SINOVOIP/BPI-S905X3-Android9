/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.telephony.embms.cts;

import android.telephony.MbmsStreamingSession;
import android.telephony.cts.embmstestapp.CtsStreamingService;
import android.telephony.mbms.MbmsErrors;
import android.telephony.mbms.StreamingServiceInfo;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class MbmsStreamingSessionTest extends MbmsStreamingTestBase {
    public void testDuplicateSession() throws Exception {
        try {
            MbmsStreamingSession failure = MbmsStreamingSession.create(
                    mContext, mCallbackExecutor, mCallback);
            fail("Duplicate create should've thrown an exception");
        } catch (IllegalStateException e) {
            // Succeed
        }
    }

    public void testRequestUpdateStreamingServices() throws Exception {
        List<String> testClasses = Arrays.asList("class1", "class2");
        mStreamingSession.requestUpdateStreamingServices(testClasses);

        // Make sure we got the streaming services
        List<StreamingServiceInfo> serviceInfos =
                (List<StreamingServiceInfo>) mCallback.waitOnStreamingServicesUpdated().arg1;
        assertEquals(CtsStreamingService.STREAMING_SERVICE_INFO, serviceInfos.get(0));
        assertEquals(0, mCallback.getNumErrorCalls());

        // Make sure the middleware got the call with the right args
        List<List<Object>> requestStreamingServicesCalls =
                getMiddlewareCalls(CtsStreamingService.METHOD_REQUEST_UPDATE_STREAMING_SERVICES);
        assertEquals(1, requestStreamingServicesCalls.size());
        assertEquals(3, requestStreamingServicesCalls.get(0).size());
        List<String> middlewareReceivedServiceClasses =
                (List<String>) requestStreamingServicesCalls.get(0).get(2);
        assertEquals(testClasses.size(), middlewareReceivedServiceClasses.size());
        for (int i = 0; i < testClasses.size(); i++) {
            assertEquals(testClasses.get(i), middlewareReceivedServiceClasses.get(i));
        }
    }

    public void testClose() throws Exception {
        mStreamingSession.close();

        // Make sure we can't use it anymore
        try {
            mStreamingSession.requestUpdateStreamingServices(Collections.emptyList());
            fail("Streaming session should not be usable after close");
        } catch (IllegalStateException e) {
            // Succeed
        }

        // Make sure that the middleware got the call to close
        List<List<Object>> closeCalls = getMiddlewareCalls(CtsStreamingService.METHOD_CLOSE);
        assertEquals(1, closeCalls.size());
    }

    public void testErrorDelivery() throws Exception {
        mMiddlewareControl.forceErrorCode(
                MbmsErrors.GeneralErrors.ERROR_MIDDLEWARE_TEMPORARILY_UNAVAILABLE);
        mStreamingSession.requestUpdateStreamingServices(Collections.emptyList());
        assertEquals(MbmsErrors.GeneralErrors.ERROR_MIDDLEWARE_TEMPORARILY_UNAVAILABLE,
                mCallback.waitOnError().arg1);
    }
}
