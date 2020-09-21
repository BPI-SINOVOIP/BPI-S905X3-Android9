/******************************************************************
*
*Copyright (C) 2012 Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.otaupgrade;

import android.app.Activity;
import android.app.AlertDialog;

import android.app.AlertDialog.Builder;

import android.app.Dialog;

import android.content.DialogInterface;
import android.content.Intent;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

import android.view.View.OnClickListener;

import android.view.ViewGroup;

import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.Toast;

import com.amlogic.update.OtaUpgradeUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import android.os.Build;

/**
 * @ClassName UpdateActivity
 * @Description TODO
 * @Date 2013-7-16
 * @Email
 * @Author
 * @Version V1.0
 */
public class MainActivity extends Activity implements OnClickListener {
    private static final String TAG = UpdateService.TAG;
    private static final String ENCODING = "UTF-8";
    private static final String VERSION_NAME = "version";
    private static final int queryReturnOk = 0;
    private static final int queryUpdateFile = 1;
    private static int UpdateMode = OtaUpgradeUtils.UPDATE_UPDATE;
    private Button mOnlineUpdateBtn;
    private Button mLocalUpdateBtn;
    private Button mBackupBtn;
    private Button mRestoreBtn;
    private Button mUpdateCertern;
    private CheckBox mWipeDate;
    private CheckBox mWipeMedia;
    public static final String RESULT_BACKUP = "com.droidlogic.backupresult";
    public static final String RESULT_RESTORE = "com.droidlogic.restoreresult";
    //private CheckBox mWipeCache;
    private TextView filepath;
    private TextView filename;
    private PrefUtils mPreference;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_view);
        mOnlineUpdateBtn = (Button) findViewById(R.id.updatebtn);
        mLocalUpdateBtn = (Button) findViewById(R.id.btn_update_locale);
        mUpdateCertern = (Button) findViewById(R.id.btn_locale_certern);
        mBackupBtn = (Button) findViewById(R.id.backup);
        mRestoreBtn = (Button) findViewById(R.id.restore);
        mWipeDate = (CheckBox) findViewById(R.id.wipedata);
        mWipeMedia = (CheckBox) findViewById(R.id.wipemedia);
        boolean recoveryUsd = getResources().getBoolean(R.bool.device_custom_recovery);

        if (!recoveryUsd || Build.VERSION.SDK_INT >= 28) {
            mWipeDate.setVisibility(View.GONE);
            mWipeMedia.setVisibility(View.GONE);
        }
        //mWipeCache = (CheckBox) findViewById(R.id.wipecache);
        filename = (TextView) findViewById(R.id.update_file_name);
        filepath = (TextView) findViewById(R.id.update_full_name);
        mPreference = new PrefUtils(this);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);

        MenuItem item = menu.findItem(R.id.settings);

        if (PrefUtils.getAutoCheck()) {
            item.setVisible(false);
            mPreference.setBoolean(PrefUtils.PREF_AUTO_CHECK, true);
        } else {
            if (mPreference.getBooleanVal(PrefUtils.PREF_AUTO_CHECK, false)) {
                item.setTitle(R.string.auto_check_close);
            } else {
                item.setTitle(R.string.auto_check);
            }
        }

        return true;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (data != null) {
            if ((requestCode == queryUpdateFile) &&
                    (resultCode == queryReturnOk)) {
                Bundle bundle = data.getExtras();
                String file = bundle.getString(FileSelector.FILE);

                if (file != null) {
                    filepath.setText(file);
                    filename.setText(file.substring(file.lastIndexOf("/") + 1,
                            file.length()));
                }
            }
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.about) {
            AlertDialog.Builder builder = new Builder(MainActivity.this);

            if (getResources().getConfiguration().locale.getCountry().equals("CN")) {
                builder.setMessage(getFromAssets(VERSION_NAME + "_cn"));
            } else {
                builder.setMessage(getFromAssets(VERSION_NAME));
            }

            builder.setTitle(R.string.version_info);
            builder.setPositiveButton(R.string.sure,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });
            builder.create().show();
        } else if (item.getItemId() == R.id.settings) {
            if (MainActivity.this.getResources().getString(R.string.auto_check)
                                     .equals(item.getTitle().toString())) {
                mPreference.setBoolean(PrefUtils.PREF_AUTO_CHECK, true);
                item.setTitle(R.string.auto_check_close);
            } else {
                mPreference.setBoolean(PrefUtils.PREF_AUTO_CHECK, false);
                item.setTitle(R.string.auto_check);
            }
        }

        return super.onOptionsItemSelected(item);
    }

    private void UpdateDialog(String filename) {
        final Dialog dlg = new Dialog(this, R.style.Theme_dialog);
        LayoutInflater inflater = LayoutInflater.from(this);
        InstallPackage dlgView = (InstallPackage) inflater.inflate(R.layout.install_ota,
                null, false);
        dlgView.setPackagePath(filename);
        dlgView.setParamter(UpdateMode);
        dlg.setContentView(dlgView);
        dlg.setCancelable(false);
        dlg.findViewById(R.id.confirm_cancel).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    dlg.dismiss();
                }
            });
        dlg.show();
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        super.onSaveInstanceState(savedInstanceState);

        if (filepath != null) {
            savedInstanceState.putString("filepath",
                filepath.getText().toString());
        }

        if (filename != null) {
            savedInstanceState.putString("filename",
                filename.getText().toString());
        }
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);

        String strPath = savedInstanceState.getString("filepath");
        String strName = savedInstanceState.getString("filename");

        if ((filepath != null) && (strPath != null)) {
            filepath.setText(strPath);
        }

        if ((filename != null) && (strName != null)) {
            filename.setText(strName);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mOnlineUpdateBtn.setOnClickListener(this);
        mLocalUpdateBtn.setOnClickListener(this);
        mBackupBtn.setOnClickListener(this);
        mRestoreBtn.setOnClickListener(this);
        mUpdateCertern.setOnClickListener(this);

        if (PrefUtils.isUserVer()) {
            ViewGroup vg = (ViewGroup) findViewById(R.id.update_locale_layer);

            if (vg != null) {
                vg.setVisibility(View.GONE);
            }
        }
        if (Build.VERSION.SDK_INT >= 28) {
            ViewGroup vp = (ViewGroup) findViewById(R.id.backup_layout);
            if (vp != null) {
                vp.setVisibility(View.GONE);
            }
            View view = findViewById(R.id.backuptitle);
            if (view != null) {
                view.setVisibility(View.GONE);
            }
        }
        String act = getIntent().getAction();
        if ( RESULT_BACKUP.equals ( act ) ) {
            Toast.makeText ( this, getString ( R.string.backup_result ), 5000 )
            .show();
        } else if ( RESULT_RESTORE.equals ( act ) ) {
            Toast.makeText ( this, getString ( R.string.restore_result ), 2000 )
            .show();
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
        case R.id.backup:

            Intent intent = new Intent(LoaderReceiver.BACKUPDATA);
            intent.setClass(this, BackupActivity.class);
            startActivity(intent);
            this.finish();

            break;

        case R.id.restore:

            Intent intent2 = new Intent(LoaderReceiver.RESTOREDATA);
            intent2.setClass(this, BackupActivity.class);
            startActivity(intent2);
            this.finish();

            break;

        case R.id.btn_update_locale:

            Intent intent0 = new Intent(this, FileSelector.class);
            Activity activity = (Activity) this;
            startActivityForResult(intent0, queryUpdateFile);

            break;

        case R.id.btn_locale_certern:
            String fullname = filepath.getText().toString();
            if ( fullname.lastIndexOf("/") > 0 && (filename != null) && (filename.length() > 0)) {
                if (mWipeDate == null)
                    UpdateMode = mPreference.createAmlScript(fullname, false, false);
                else
                    UpdateMode = mPreference.createAmlScript(fullname,mWipeDate.isChecked(),mWipeMedia.isChecked());

                if (UpdateMode == OtaUpgradeUtils.UPDATE_OTA && mPreference.inLocal(fullname)
                        &&(mWipeDate.isChecked() || mWipeMedia.isChecked())) {
                        mWipeDate.setChecked(false);
                        mWipeMedia.setChecked(false);
                        Toast.makeText(this, getString(R.string.reset_wipe), Toast.LENGTH_LONG).show();
                }

                UpdateDialog(fullname);
            } else {
                Toast.makeText(this, getString(R.string.file_not_exist), 2000).show();
            }
            break;

        case R.id.updatebtn:

            if (!checkInternet()) {
                Toast.makeText(this, getString(R.string.net_error), 2000).show();

                return;
            }

            Intent intent1 = new Intent();
            intent1.setAction(UpdateService.ACTION_CHECK);
            intent1.setClass(this, UpdateActivity.class);
            startActivity(intent1);
            this.finish();

            break;
        }
    }

    private boolean checkInternet() {
        ConnectivityManager cm = (ConnectivityManager) this.getSystemService(this.CONNECTIVITY_SERVICE);
        NetworkInfo info = cm.getActiveNetworkInfo();

        if ((info != null) && info.isConnected()) {
            return true;
        } else {
            Log.v(TAG, "It's can't connect the Internet!");

            return false;
        }
    }

    public static String getString(final byte[] data, final String charset) {
        if (data == null) {
            throw new IllegalArgumentException("Parameter may not be null");
        }
        try {
            return new String(data, 0, data.length, charset);
        } catch (UnsupportedEncodingException e) {
            return new String(data, 0, data.length);
        }
    }
    public String getFromAssets(String fileName) {
        String result = "";

        try {
            InputStream in = getResources().getAssets().open(fileName);
            int lenght = in.available();
            byte[] buffer = new byte[lenght];
            in.read(buffer);
            result = getString(buffer,"UTF-8");
        } catch (Exception e) {
            e.printStackTrace();
        }

        return result;
    }
}
