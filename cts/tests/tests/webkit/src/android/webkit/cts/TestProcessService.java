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
 * limitations under the License.
 */

package android.webkit.cts;


import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

import junit.framework.Assert;
import junit.framework.AssertionFailedError;

// Subclasses are the ones that get actually used, so make this abstract
abstract class TestProcessService extends Service {
    static final int MSG_RUN_TEST = 0;
    static final int MSG_EXIT_PROCESS = 1;
    static final String TEST_CLASS_KEY = "class";

    static final int REPLY_OK = 0;
    static final int REPLY_EXCEPTION = 1;
    static final String REPLY_EXCEPTION_KEY = "exception";

    @Override
    public IBinder onBind(Intent intent) {
        return mMessenger.getBinder();
    }

    final Messenger mMessenger = new Messenger(new IncomingHandler());

    private class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == MSG_EXIT_PROCESS) {
                System.exit(0);
            }

            try {
                if (msg.what != MSG_RUN_TEST) {
                    throw new AssertionFailedError("Unknown service message " + msg.what);
                }

                String testClassName = msg.getData().getString(TEST_CLASS_KEY);
                Class testClass = Class.forName(testClassName);
                TestProcessClient.TestRunnable test =
                        (TestProcessClient.TestRunnable) testClass.newInstance();
                test.run(TestProcessService.this);
            } catch (Throwable t) {
                try {
                    Message m = Message.obtain(null, REPLY_EXCEPTION);
                    m.getData().putSerializable(REPLY_EXCEPTION_KEY, t);
                    msg.replyTo.send(m);
                } catch (RemoteException e) {
                    throw new RuntimeException(e);
                }
                return;
            }

            try {
                msg.replyTo.send(Message.obtain(null, REPLY_OK));
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }
    }
}
