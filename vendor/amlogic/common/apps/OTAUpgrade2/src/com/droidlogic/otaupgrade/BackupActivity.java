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

import android.content.Intent;
import android.os.Handler;
import android.os.Bundle;
import android.os.Environment;
import android.widget.Toast;
import com.amlogic.update.Backup;
import com.amlogic.update.Backup.IBackupConfirmListener;
import java.io.File;


public class BackupActivity extends Activity implements IBackupConfirmListener{
    public static String BACKUP_FILE = "/data/data/com.droidlogic.otaupgrade/BACKUP";

    public static final String SdcardDir = "/storage/external_storage/sdcard1/";
    public static final int FUNCBACKUP = 1;
    public static final int FUNCRESTORE = 2;
    public static int func = 0;
    public Handler mHandler;

    /*private static void getBackUpFileName() {
        File devDir = new File(PrefUtils.DEV_PATH);
        File[] devs = devDir.listFiles();

        for (File dev : devs) {
            if (dev.isDirectory() && dev.canWrite()) {
                BACKUP_FILE = dev.getAbsolutePath();
                BACKUP_FILE += "/BACKUP";

                break;
            }
        }
    }*/

    @Override
    protected void onCreate(Bundle icicle) {
        //getBackUpFileName();

        super.onCreate(icicle);
        mHandler = new Handler();
        boolean flag = false;
        String act = getIntent().getAction();

        if (act.equals(LoaderReceiver.BACKUPDATA)) {
            func = FUNCBACKUP;

            if (!OnSDcardStatus()) {
                flag = true;

                Intent intent0 = new Intent(this, BadMovedSDcard.class);
                Activity activity = (Activity) this;
                startActivityForResult(intent0, 1);
            } else {
                Backup();
            }
        } else if (act.equals(LoaderReceiver.RESTOREDATA)) {
            func = FUNCRESTORE;

            if (!OnSDcardStatus()) {
                flag = true;

                Intent intent0 = new Intent(this, BadMovedSDcard.class);
                Activity activity = (Activity) this;
                startActivityForResult(intent0, 1);
            } else {
                if (!new File(BACKUP_FILE).exists()) {
                    Toast.makeText ( BackupActivity.this.getApplicationContext(), getString ( R.string.prepare_waitting ), 5000 )
                        .show();
                }
                Restore();
            }
        }

        if (!flag) {
            Intent intent = new Intent();
            setResult(1, intent);
            finish();
        }
    }
    public void onBackupComplete() {
        if ( android.os.Build.VERSION.SDK_INT > 20 ) {
            if (mHandler != null) {
                mHandler.post(new Runnable(){
                    public void run(){
                        Toast.makeText ( BackupActivity.this.getApplicationContext(), getString ( R.string.remove_hit ), 5000 )
                        .show();
                    }
                });
            }
            PrefUtils util = new PrefUtils(this);
            util.copyBKFile();
        }
        Intent intent = new Intent ( MainActivity.RESULT_BACKUP );
        intent.setClass ( this, MainActivity.class );
        startActivity ( intent );
    }

    public void onRestoreComplete() {
        Intent intent = new Intent ( MainActivity.RESULT_RESTORE );
        intent.setClass ( this, MainActivity.class );
        startActivity ( intent );
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if ((data != null) && (requestCode == 1)) {
            if (resultCode == BadMovedSDcard.SDCANCEL) {
                this.finish();
            } else if (resultCode == BadMovedSDcard.SDOK) {
                if (func == FUNCBACKUP) {
                    Backup();
                } else {
                    Restore();
                }

                Intent intent = new Intent();
                setResult(1, intent);
                finish();
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    private boolean OnSDcardStatus() {
        File file = new File(BACKUP_FILE);

        return file.getParentFile().canWrite();

        //return Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorage2State());
    }

    private void Backup() {
        new Thread() {
                final String[] args = {
                        BACKUP_FILE, "backup", "-apk", "-system","-widget","-compress", "-noshared"
                    };

                public void run() {
                    Backup mBackup = new Backup(BackupActivity.this);
                    mBackup.setConfirmListener ( BackupActivity.this );
                    mBackup.main(args);
                }
            }.start();
    }

    private void Restore() {
        new Thread() {
                final String[] args = {
                        BACKUP_FILE, "restore", "-apk", "-system","-widget","-compress", "-noshared"
                    };

                public void run() {
                    if (!new File(BACKUP_FILE).exists()) {
                        PrefUtils pref = new PrefUtils(BackupActivity.this);
                        pref.copyBackup(false);
                    }
                    Backup mBackup = new Backup(BackupActivity.this);
                    mBackup.setConfirmListener ( BackupActivity.this );
                    mBackup.main(args);
                }
            }.start();
    }
}
