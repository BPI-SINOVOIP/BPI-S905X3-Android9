package com.android.experimental.slicesapp;

import android.app.slice.Slice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

public class SlicesBroadcastReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        final String action = intent.getAction();
        if (SlicesProvider.SLICE_ACTION.equals(action)) {
            // A slice was clicked and we've got an action to handle
            Bundle bundle = intent.getExtras();
            String message = "";
            boolean newState = false;
            if (bundle != null) {
                message = bundle.getString(SlicesProvider.INTENT_ACTION_EXTRA);
                newState = bundle.getBoolean(Slice.EXTRA_TOGGLE_STATE);
            }
            String text = message + " new state: " + newState;
            Toast.makeText(context, text, Toast.LENGTH_SHORT).show();
        }
    }
}
