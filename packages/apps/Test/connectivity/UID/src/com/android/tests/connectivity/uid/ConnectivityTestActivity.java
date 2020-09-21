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

package com.android.tests.connectivity.uid;

import android.app.Activity;
import android.app.DownloadManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.StrictMode;
import android.util.Log;
import android.webkit.MimeTypeMap;
import android.webkit.URLUtil;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;


public class ConnectivityTestActivity extends Activity {

    Context mContext;
    ConnectivityManager mConnectivityManager;
    DownloadManager mDownloadManager;

    public static final String TAG = "ConnectivityUIDTest";
    private static final String RESULT = "result";
    private static final String URL = "url";

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        StrictMode.ThreadPolicy policy =
                new StrictMode.ThreadPolicy.Builder().permitAll().build();
        StrictMode.setThreadPolicy(policy);

        mContext = this.getApplicationContext();
        mConnectivityManager = (ConnectivityManager)
                mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        mDownloadManager = (DownloadManager)
                mContext.getSystemService(Context.DOWNLOAD_SERVICE);
    }

    public void onResume() {
        super.onResume();
        boolean conn = checkNow();
        Intent returnIntent = new Intent();
        returnIntent.putExtra(RESULT, conn);
        setResult(RESULT_OK, returnIntent);
        Bundle extras = getIntent().getExtras();
        downloadData(extras);
        finish();
    }

    private boolean checkNow() {
        try{
            NetworkInfo netInfo = mConnectivityManager.getActiveNetworkInfo();
            return netInfo.isConnected() && httpRequest();
        } catch(Exception e) {
            Log.e(TAG, "CheckConnectivity exception: ", e);
        }

        return false;
    }

    private boolean httpRequest() {
        HttpURLConnection urlConnection = null;
        try {
            URL targetURL = new URL("http://www.google.com/generate_204");
            urlConnection = (HttpURLConnection) targetURL.openConnection();
            urlConnection.connect();
            int respCode = urlConnection.getResponseCode();
            return (respCode == 204);
        } catch (Exception e) {
            Log.e(TAG, "Checkconnectivity exception: ", e);
        }
        return false;
    }

    private void downloadData(Bundle extras) {
        if(extras == null)
            return;
        String url = extras.getString(URL);
        Log.d(TAG, "URL IS: " + url);
        DownloadManager.Request request =
                new DownloadManager.Request(Uri.parse(url));
        request.setNotificationVisibility(
                DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
        String name = URLUtil.guessFileName(
                url, null, MimeTypeMap.getFileExtensionFromUrl(url));
        request.setDestinationInExternalPublicDir(
                Environment.DIRECTORY_DOWNLOADS, name);
        mDownloadManager.enqueue(request);
    }
}
