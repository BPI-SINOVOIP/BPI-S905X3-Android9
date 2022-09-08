
package com.example.SubtitleSimpleDemo;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;

import android.app.ListActivity;
import android.content.Intent;
import android.graphics.Color;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;


public class FileBrowserActivity extends ListActivity {
    public static final String EXTRA_FILE_PATH = "filebrowser";

    private static final String TAG = "FileBrowserActivity";
    private final int MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE = 0;
    private ArrayAdapter<File> mAdapter;
    private File mCurrentDir;
    //request code for permission check
    private boolean mPermissionGranted = false;
    private boolean doubleBackToExitPressedOnce = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_file_browser);


        mAdapter = new ArrayAdapter<File>(this, android.R.layout.simple_list_item_1) {
            @Override
            public View getView(int position, View convertView, ViewGroup parent) {
                View v = super.getView(position, convertView, parent);
                TextView textView = (TextView) v.findViewById(android.R.id.text1);
                File f = getItem(position);
                textView.setText(f.getName());
                if (f.isDirectory()) {
                    textView.setTextColor(Color.parseColor("#8d2800"));
                } else {
                    textView.setTextColor(Color.BLACK);
                }

                return v;
            }
        };
        setListAdapter(mAdapter);

        mCurrentDir = Environment.getExternalStorageDirectory();
        Log.d(TAG, "Start Path" + mCurrentDir.getPath());


        if (PackageManager.PERMISSION_DENIED == ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)) {
            Log.d(TAG, "[onCreate]requestPermissions");
            ActivityCompat.requestPermissions(FileBrowserActivity.this,
                    new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                    MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE);
        } else {
            mPermissionGranted = true;
            updatePath(mCurrentDir);
        }
    }

    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE:
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.i(TAG, "user granted the permission!");
                    mPermissionGranted = true;
                    updatePath(mCurrentDir);
                } else {
                    Log.i(TAG, "user denied the permission!");
                    mPermissionGranted = false;
                    finish();
                }
                return;
        }
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        File file = mAdapter.getItem(position);
        if (file.isDirectory()) {
            updatePath(file);
        } else {
            String message = "You have select file:" + file + "\n Please click start to play!";
            Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
            Intent intent = new Intent(this, PlayerActivity.class);
            intent.putExtra("filePath", file.getAbsolutePath());
            startActivity(intent);
        }
        super.onListItemClick(l, v, position, id);
    }

    @Override
    public void onBackPressed() {
        if (doubleBackToExitPressedOnce) {
            super.onBackPressed();
            return;
        }

        this.doubleBackToExitPressedOnce = true;
        String message = "hohey";
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
        back();

        new Handler().postDelayed(new Runnable() {

            @Override
            public void run() {
                doubleBackToExitPressedOnce = false;
            }
        }, 1000);
    }


    private void back() {
        // when pressed back button id possible
        // change dir to parent dir
        File parentDir = mCurrentDir.getParentFile();
        if (parentDir != null) {
            updatePath(parentDir);

        }
    }

    private void updatePath(File file) {
        if (file.isDirectory()) {
            // if selected file is a directory open.
            mCurrentDir = file;
            File[] files = file.listFiles();
            mAdapter.clear();
            for (File f : files) {
                if (f.isDirectory() && (f.list().length == 0)) {
                    continue;
                }
                mAdapter.add(f);
            }
        } else {
            // id selected file is a file return file path as intent's result
            Intent resultIntent = new Intent();
            resultIntent.putExtra(EXTRA_FILE_PATH, file.getAbsolutePath());
            setResult(RESULT_OK, resultIntent);
            finish();

        }
    }


}
