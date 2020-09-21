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

package com.googlecode.android_scripting.service;

import android.app.Service;
import android.content.Intent;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Messenger;

import com.googlecode.android_scripting.facade.FacadeConfiguration;
import com.googlecode.android_scripting.facade.FacadeManagerFactory;
import com.googlecode.android_scripting.jsonrpc.RpcReceiverManagerFactory;

/**
 * FacadeService exposes SL4A's Facade methods through a {@link Service} so
 * they can be invoked from third-party apps.
 * <p>
 * Example binding code:<br>
 * {@code
 *   Intent intent = new Intent();
 *   intent.setPackage("com.googlecode.android_scripting");
 *   intent.setAction("com.googlecode.android_scripting.service.FacadeService.ACTION_BIND");
 *   sl4aService = bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
 * }
 * Example using the service:<br>
 * {@code
 *   Bundle sl4aBundle = new Bundle;
 *   bundle.putString{"sl4aMethod", "makeToast"};
 *   bundle.putString{"message", "Hello World!"};
 *   Message msg = Message.obtain(null, SL4A_ACTION);
 *   msg.setData(sl4aBundle);
 *   msg.replyTo = myReplyHandler; // Set a Messenger if you need the response
 *   mSl4aService.send(msg);
 * }
 * <p>
 * For more info on binding a {@link Service} using a {@link Messenger} please
 * refer to Android's public developer documentation.
 */
public class FacadeService extends Service {

    private RpcReceiverManagerFactory rpcReceiverManagerFactory;

    @Override
    public IBinder onBind(Intent intent) {
        if (rpcReceiverManagerFactory == null) {
            rpcReceiverManagerFactory =
                    new FacadeManagerFactory(FacadeConfiguration.getSdkLevel(), this, null,
                            FacadeConfiguration.getFacadeClasses());
        }
        HandlerThread handlerThread = new HandlerThread("MessageHandlerThread");
        handlerThread.start();
        Messenger aMessenger = new Messenger(new MessageHandler(handlerThread,
                rpcReceiverManagerFactory));
        return aMessenger.getBinder();
    }
}
