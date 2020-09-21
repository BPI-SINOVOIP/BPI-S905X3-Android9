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
 * limitations under the License.
 */
package android.content.pm.cts.shortcutmanager.throttling;

import static com.android.server.pm.shortcutmanagertest.ShortcutManagerTestUtils.list;

import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ShortcutManager;
import android.content.pm.cts.shortcutmanager.common.Constants;
import android.content.pm.cts.shortcutmanager.common.ReplyUtil;
import android.os.IBinder;
import android.util.Log;

/**
 * Make sure that when only a bg service is running, shortcut manager calls are throttled.
 */
public class BgService extends Service {
    public static void start(Context context, String replyAction) {
        final Intent i =
                new Intent().setComponent(new ComponentName(context, BgService.class))
                        .putExtra(Constants.EXTRA_REPLY_ACTION, replyAction);
        context.startService(i);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        final Context context = getApplicationContext();
        final ShortcutManager manager = context.getSystemService(ShortcutManager.class);

        final String replyAction = intent.getStringExtra(Constants.EXTRA_REPLY_ACTION);

        Log.i(ThrottledTests.TAG, Constants.TEST_BG_SERVICE_THROTTLED);

        ReplyUtil.runTestAndReply(context, replyAction, () -> {
            ThrottledTests.assertThrottled(
                    context, () -> manager.setDynamicShortcuts(list()));
        });

        stopSelf();

        return Service.START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
