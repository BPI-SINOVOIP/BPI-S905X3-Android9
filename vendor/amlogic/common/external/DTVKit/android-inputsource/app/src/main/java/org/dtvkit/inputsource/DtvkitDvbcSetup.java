package org.dtvkit.inputsource;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TextView;

import org.dtvkit.companionlibrary.EpgSyncJobService;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.droidlogic.dtvkit.DtvkitGlueClient;

import java.util.Locale;

public class DtvkitDvbcSetup extends Activity {
    private static final String TAG = "DtvkitDvbcSetup";

    private final DtvkitGlueClient.SignalHandler mHandler = new DtvkitGlueClient.SignalHandler() {
        @Override
        public void onSignal(String signal, JSONObject data) {
            if (signal.equals("DvbcStatusChanged")) {
                int progress = 0;
                try {
                    progress = data.getInt("progress");
                } catch (JSONException ignore) {
                }

                if (progress < 100) {
                    getProgressBar().setProgress(progress);
                    setSearchStatus(String.format(Locale.ENGLISH, "Searching (%d%%)", progress));
                } else {
                    onSearchFinished();
                }
            }
        }
    };

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, final Intent intent) {
            String status = intent.getStringExtra(EpgSyncJobService.SYNC_STATUS);
            if (status.equals(EpgSyncJobService.SYNC_FINISHED)) {
                setSearchStatus("Finished");
                finish();
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.cabsetup);

        final View startSearch = findViewById(R.id.cablestartsearch);
        final View stopSearch = findViewById(R.id.cablestopsearch);

        startSearch.setEnabled(true);
        startSearch.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                startSearch.setEnabled(false);
                stopSearch.setEnabled(true);
                stopSearch.requestFocus();
                startSearch();
            }
        });

        stopSearch.setEnabled(false);
        stopSearch.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                onSearchFinished();
            }
        });
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        stopMonitoringSearch();
        stopMonitoringSync();
    }

    private void startSearch() {
        setSearchStatus("Searching");
        getProgressBar().setIndeterminate(false);
        startMonitoringSearch();

        CheckBox network = (CheckBox)findViewById(R.id.network);
        EditText frequency = (EditText)findViewById(R.id.frequency);
        EditText symbolrate = (EditText)findViewById(R.id.symbolrate);
        Spinner modulation = (Spinner)findViewById(R.id.modulation);

        try {
            JSONArray args = new JSONArray();
            args.put(network.isChecked());
            args.put(Integer.parseInt(frequency.getText().toString()));
            args.put(modulation.getSelectedItem().toString());
            args.put(Integer.parseInt(symbolrate.getText().toString()));
            args.put(true); // retune

            Log.i(TAG, args.toString());

            DtvkitGlueClient.getInstance().request("Dvbc.startManualSearch", args);
        } catch (Exception e) {
            stopMonitoringSearch();
            setSearchStatus(e.getMessage());
        }
    }

    private void onSearchFinished() {
        disableStopSearchButton();
        setSearchStatus("Finishing search");
        getProgressBar().setIndeterminate(true);
        stopMonitoringSearch();
        try {
            JSONArray args = new JSONArray();
            args.put(true); // Commit
            DtvkitGlueClient.getInstance().request("Dvbc.finishSearch", args);
        } catch (Exception e) {
            stopMonitoringSearch();
            setSearchStatus(e.getMessage());
            return;
        }

        setSearchStatus("Updating guide");
        startMonitoringSync();
        // By default, gets all channels and 1 hour of programs (DEFAULT_IMMEDIATE_EPG_DURATION_MILLIS)
        EpgSyncJobService.cancelAllSyncRequests(this);

        // If the intent that started this activity is from Live Channels app
        String inputId = this.getIntent().getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        Log.i(TAG, String.format("inputId: %s", inputId));
        EpgSyncJobService.requestImmediateSync(this, inputId, true, new ComponentName(this, DtvkitEpgSync.class)); // 12 hours
    }

    private void startMonitoringSearch() {
        DtvkitGlueClient.getInstance().registerSignalHandler(mHandler);
    }

    private void stopMonitoringSearch() {
        DtvkitGlueClient.getInstance().unregisterSignalHandler(mHandler);
    }

    private void startMonitoringSync() {
        LocalBroadcastManager.getInstance(this).registerReceiver(mReceiver,
                new IntentFilter(EpgSyncJobService.ACTION_SYNC_STATUS_CHANGED));
    }

    private void stopMonitoringSync() {
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mReceiver);
    }

    private void setSearchStatus(String status) {
        Log.i(TAG, String.format("Search status \"%s\"", status));
        final TextView text = (TextView) findViewById(R.id.searchstatus);
        text.setText(status);
    }

    private ProgressBar getProgressBar() {
        return (ProgressBar) findViewById(R.id.searchprogress);
    }

    private void disableStopSearchButton() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                findViewById(R.id.cablestopsearch).setEnabled(false);
            }
        });
    }
}
