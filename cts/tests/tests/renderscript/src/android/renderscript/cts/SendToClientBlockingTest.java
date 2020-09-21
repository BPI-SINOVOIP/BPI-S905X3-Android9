/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.renderscript.cts;

import android.renderscript.RenderScript.RSMessageHandler;

import java.util.concurrent.Semaphore;
import java.util.Random;

public class SendToClientBlockingTest extends RSBaseCompute {
    private static final Semaphore mSync = new Semaphore(0);

    private ScriptC_sendToClientBlocking mScript;
    private Random random;

    private int resultId = 0;
    private int resultData = 0;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        random = new Random();
    }

    @Override
    protected void tearDown() throws Exception {
        if (mScript != null) {
            mScript.destroy();
        }
        super.tearDown();
    }

    RSMessageHandler mRsMessageForTest = new RSMessageHandler() {
        public void run() {
            switch (mID) {
            default:
                resultId = mID;
                resultData = mData[0];
                try {
                    mSync.release();
                } catch (Exception e) {
                    // TODO: handle exception
                }
                return;
            }
        }
    };

    public void testSendToClientBlocking1Params() {

        int id = random.nextInt(10);
        mRS.setMessageHandler(mRsMessageForTest);
        mScript = new ScriptC_sendToClientBlocking(mRS);
        mScript.set_ID(id);
        // Log.i("testSendToClientBlocking1Params", "==" + id);
        mScript.invoke_callBack1Params();
        try {
            mSync.acquire();
        } catch (InterruptedException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        assertTrue("testSendToClientBlocking1Params fail the result is:" + resultId,
                resultId == id);
    }

    public void testSendToClientBlocking3Params() {

        int id = random.nextInt(10);
        int data = random.nextInt();
        mRS.setMessageHandler(mRsMessageForTest);
        mScript = new ScriptC_sendToClientBlocking(mRS);
        mScript.set_ID(id);
        mScript.set_data(data);
        // Log.i("testSendToClientBlocking3Params", data + "==" + id);
        mScript.invoke_callBack3Params();
        try {
            mSync.acquire();
        } catch (InterruptedException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        assertTrue("testSendToClientBlocking3Params fail the ID is:" + resultId +
                "The data is:" + data, resultId == id && resultData == data);
    }

}
