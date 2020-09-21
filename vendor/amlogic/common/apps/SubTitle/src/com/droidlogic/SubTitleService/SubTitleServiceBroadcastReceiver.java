package com.droidlogic.SubTitleService;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.os.ServiceManager;

public class SubTitleServiceBroadcastReceiver extends BroadcastReceiver {
        private static final String TAG = "SubTitleServiceBroadcastReceiver";
        private SubtitleMiddleClientManager subtitleService = null;

        @Override
        public void onReceive (Context context, Intent intent) {
            String action = intent.getAction();
            Log.i (TAG, "[onReceive]action:" + action + ", subtitleService:" + subtitleService);
            if (Intent.ACTION_BOOT_COMPLETED.equals (action)) {
                new SubtitleMiddleClientManager (context);
            }
        }
}

