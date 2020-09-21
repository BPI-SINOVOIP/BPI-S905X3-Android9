package com.droidlogic.fragment;

import android.app.Activity;
import android.app.ActionBar;
import android.app.Fragment;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.os.Build;
import android.widget.Toast;

import com.droidlogic.fragment.dialog.DialogManager;

import org.dtvkit.inputsource.R;
import org.droidlogic.dtvkit.DtvkitGlueClient;

public class ScanMainActivity extends Activity {
    private static final String TAG = "ScanMainActivity";
    private Button mDishSetup = null;
    private Button mScanChannel = null;
    private int mCurrentFragment = 0;
    private ScanFragmentManager mScanFragmentManager = null;
    private ParameterMananer mParameterMananer = null;
    private DialogManager mDialogManager = null;
    private DtvkitGlueClient mDtvkitGlueClient = null;

    public static final int REQUEST_CODE_START_SETUP_ACTIVITY = 1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_main);
        mScanFragmentManager = new ScanFragmentManager(this);
        //mScanFragmentManager.show(new PlaceholderFragment());
        mScanFragmentManager.show(new ScanDishSetupFragment());
        mCurrentFragment = ScanFragmentManager.INIT_FRAGMENT;
        mDtvkitGlueClient = DtvkitGlueClient.getInstance();
        mParameterMananer = new ParameterMananer(this, mDtvkitGlueClient);
        mDialogManager = new DialogManager(this, mParameterMananer);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mScanFragmentManager.removeRunnable();
    }

    public ParameterMananer getParameterMananer() {
        return mParameterMananer;
    }

    public DialogManager getDialogManager() {
        return mDialogManager;
    }

    public ScanFragmentManager getScanFragmentManager() {
        return mScanFragmentManager;
    }

    public DtvkitGlueClient getDtvkitGlueClient() {
        return mDtvkitGlueClient;
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyDown " + event);
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (mScanFragmentManager.isActive()) {
                mScanFragmentManager.popSideFragment();
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case REQUEST_CODE_START_SETUP_ACTIVITY:
                if (resultCode == RESULT_OK) {
                    setResult(RESULT_OK, data);
                    finish();
                } else {
                    setResult(RESULT_CANCELED);
                }
                break;
            default:
                // do nothing
                Log.d(TAG, "onActivityResult other request");
        }
    }
}
