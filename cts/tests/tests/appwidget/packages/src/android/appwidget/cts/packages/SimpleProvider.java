package android.appwidget.cts.packages;

import android.app.Activity;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.appwidget.cts.common.Constants;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;

public class SimpleProvider extends AppWidgetProvider {

    @Override
    public void onReceive(Context context, Intent intent) {
        super.onReceive(context, intent);

        if (Constants.ACTION_APPLY_OVERRIDE.equals(intent.getAction())) {
            String request = intent.getStringExtra(Constants.EXTRA_REQUEST);
            AppWidgetManager.getInstance(context).updateAppWidgetProviderInfo(
                    new ComponentName(context, SimpleProvider.class),
                    request);
            setResultCode(Activity.RESULT_OK);
        }
    }
}
