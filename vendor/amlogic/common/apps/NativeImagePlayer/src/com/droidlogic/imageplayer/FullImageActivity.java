/******************************************************************
 *
 *Copyright (C) 2012  Amlogic, Inc.
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
package com.droidlogic.imageplayer;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.text.TextUtils;
import android.util.Log;
import android.util.Size;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.droidlogic.app.SystemControlManager;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.text.DecimalFormat;
import java.util.ArrayList;

/**
 * show the image in video layer
 */
public class FullImageActivity extends Activity implements View.OnClickListener, View.OnFocusChangeListener {
    private static final String TAG = "FullImageActivity";

    public static final int DISPLAY_MENU_TIME = 5000;
    private static final int DISPLAY_PROGRESSBAR = 1000;

    private static final boolean DEBUG = true;

    public static final String ACTION_REVIEW = "com.android.camera.action.REVIEW";
    public static final String KEY_GET_CONTENT = "get-content";
    public static final String KEY_GET_ALBUM = "get-album";
    public static final String KEY_TYPE_BITS = "type-bits";
    public static final String KEY_MEDIA_TYPES = "mediaTypes";
    public static final String KEY_DISMISS_KEYGUARD = "dismiss-keyguard";

    public static final String VIDE_AXIS_NODE = "/sys/class/video/axis";
    public static final String WINDOW_AXIS_NODE = "/sys/class/graphics/fb0/window_axis";

    private static final int DISMISS_PROGRESSBAR = 0;
    private static final int DISMISS_MENU = 1;
    private static final int NOT_DISPLAY = 2;
    private static final int ROTATE_L = 3;
    private static final int ROTATE_R = 4;
    private static final int SCALE_UP = 5;
    private static final int SCALE_DOWN = 6;

    private static final float SCALE_RATIO = 0.2f;
    private static final float SCALE_ORI = 1.0f;
    private static final float SCALE_MAX = 2.0f;
    private static final float SCALE_MIN = 0.2f;
    private static final float SCALE_ERR = 0.01f;

    private static final float ROTATION_DEGREE = 90;

    private static final long maxlenth = 7340032;//gif max lenth 7MB

    private boolean mPlayPicture;
    private RelativeLayout mMenu;
    private ImagePlayer mImageplayer;
    private SurfaceView mSurfaceView;
    private Animation mOutAnimation;
    private Animation mLeftInAnimation;
    private Animation mMenuInAnimation;
    private ProgressBar mLoadingProgress;
    private LinearLayout mRotateLLay;
    private LinearLayout mRotateRlay;
    private LinearLayout mScaleUp;
    private LinearLayout mScaleDown;

    private float mScale = SCALE_ORI;

    private String mCurPicPath;

    private ArrayList<Uri> mImageList = new ArrayList<Uri>();
    private ArrayList<String> mPathList = new ArrayList<String>();


    private SystemControlManager mSystemControl;
    private String mCurrenAXIS;

    private int mSlideIndex;
    private int mDegress;

    private LoadImageTask mLoadImageTask;
    private Uri mUri;

    private Handler mUIHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case DISMISS_PROGRESSBAR:
                    break;
                case DISMISS_MENU:
                    displayMenu(false);
                    break;
                case NOT_DISPLAY:
                    Toast.makeText(FullImageActivity.this, R.string.not_display, Toast.LENGTH_LONG).show();
                    break;
                case ROTATE_R:
                    rotate(true);
                    break;
                case ROTATE_L:
                    rotate(false);
                    break;
                case SCALE_UP:
                    scale(true);
                    break;
                case SCALE_DOWN:
                    scale(false);
                    break;
            }
        }
    };

    private void runAndShow() {
        mCurPicPath = getPathByUri(mUri);
        Log.d(TAG, "runAndShow mCurPicPath " + mCurPicPath);
        if (TextUtils.isEmpty(mCurPicPath)) {
            Log.e(TAG, "runAndShow pic path empty!");
            return;
        }

        if (!isSupportSuchSize(mCurPicPath)) {
            finish();
            return;
        }

        int ret = mImageplayer.showImage(mCurPicPath);
        Log.d(TAG,"runAndShow return"+ret);
        if (ret != 0) {
           mUIHandler.sendEmptyMessage(NOT_DISPLAY);
        }

    }

    public final BroadcastReceiver mUsbScanner = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            final Uri uri = intent.getData();
            if (Intent.ACTION_MEDIA_EJECT.equals(action)) {
                if (uri != null && "file".equals(uri.getScheme())) {
                    String path = uri.getPath();
                    try {
                        path = new File(path).getCanonicalPath();
                    } catch (IOException e) {
                        Log.e(TAG, "couldn't canonicalize " + path);
                        return;
                    }
                    Log.d(TAG, "action: " + action + " path: " + path);
                    if (mCurPicPath == null)
                        return;
                    if (mCurPicPath.startsWith(path)) {
                        Toast.makeText(context, R.string.usb_eject, Toast.LENGTH_SHORT).show();
                        finish();
                    }
                }
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initViews();

        PermissionUtils.verifyStoragePermissions(this);

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
        intentFilter.addDataScheme("file");
        registerReceiver(mUsbScanner, intentFilter);

        mSystemControl = SystemControlManager.getInstance();

        Intent intent = getIntent();
        if (intent.getBooleanExtra(KEY_DISMISS_KEYGUARD, false)) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
        }

        mUri = intent.getData();

        if (mUri == null) {
            Log.e(TAG, "onCreate uri null! ");
            finish();
            return;
        }

        Log.d(TAG, "onCreate uri " + mUri + " scheme " + mUri.getScheme() + " path " + mUri.getPath());
    }

    public String getPathByUri(Uri uri) {
        String uriStr = uri.toString();
        String scheme = uri.getScheme();
        if ("http".equals(scheme) || "https".equals(scheme)) {
            return uriStr;
        }
        if ("file".equals(scheme)) {
            return uri.getPath();
        }
        if (DocumentsContract.isDocumentUri(this, uri)) {
            String path = getDocumentPath(this, uri);
            return path;
        }
        String[] proj = {MediaStore.Images.Media.DATA};
        Cursor cursor = managedQuery(uri, proj, null, null, null);
        if (cursor != null) {
            int column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
            cursor.moveToFirst();
            return cursor.getString(column_index);
        }
        return uri.getPath();
    }

    public String getDocumentPath(final Context context, final Uri uri) {
        if (DocumentsContract.isDocumentUri(context, uri)) {
            if ("com.android.externalstorage.documents".equals(uri.getAuthority())) {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];
                if ("primary".equalsIgnoreCase(type)) {
                    return Environment.getExternalStorageDirectory() + "/" + split[1];
                } else {
                    return "/storage/" + type + "/" + split[1];
                }

            } else if ("com.android.providers.downloads.documents".equals(uri.getAuthority())) {
                final String id = DocumentsContract.getDocumentId(uri);
                final Uri contentUri = ContentUris.withAppendedId(Uri.parse("content://downloads/public_downloads"), Long.valueOf(id));
                return getDataColumn(context, contentUri, null, null);
            } else if ("com.android.providers.media.documents".equals(uri.getAuthority())) {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                Log.d(TAG, "docId:" + docId);
                Uri contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                final String selection = "_id=?";
                final String[] selectionArgs = new String[]{split[1]};
                return getDataColumn(context, contentUri, selection, selectionArgs);
            }
        }
        return null;
    }

    private String getDataColumn(Context context, Uri uri, String selection, String[] selectionArgs) {
        Cursor cursor = null;
        final String[] projection = {
                "_data"
        };
        try {
            cursor = getContentResolver().query(uri, projection, selection, selectionArgs, null);
            if (cursor != null && cursor.moveToFirst()) {
                final int index = cursor.getColumnIndexOrThrow("_data");
                return cursor.getString(index);
            }
        } catch (Exception e) {
            Log.e(TAG, "getDataColumn fail! " + e);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return null;
    }

    private boolean isSupportSuchSize(String path) {
        boolean ret = true;
        String endstr1 = ".gif";
        String endstr2 = ".gif?";
        if ((mCurPicPath != null && mCurPicPath.length() >= 0) && (mCurPicPath.endsWith(endstr1) || mCurPicPath.endsWith(endstr2))) {
            File f = new File(mCurPicPath);
            long lenth = f.length();
            Log.d(TAG, "this picture size is :" + lenth);
            if (lenth > maxlenth) {
                //Toast.makeText(this, "This picture size " + ((new DecimalFormat("#.00")).format((double) lenth / 1048576) + "MB") + " > 7.0MB not support!", Toast.LENGTH_LONG).show();
                //ret = false;
            }
        }
        return ret;
    }

    private void writeAxisNode() {
        Log.d(TAG,"mSystemControl"+(mSystemControl != null));
        if (mSystemControl != null) {
            mCurrenAXIS = mSystemControl.readSysFs(VIDE_AXIS_NODE);
            String targetAXIS = mSystemControl.readSysFs(WINDOW_AXIS_NODE);
            Size size = getWH(mCurrenAXIS,targetAXIS);
            mImageplayer.setSampleSurfaceSize(1, size.getWidth()+1,size.getHeight()+1);
            int startIndex = targetAXIS.indexOf("[");
            int endIndex = targetAXIS.indexOf("]");
            String newAxis;
            if (startIndex < endIndex) {
                newAxis = targetAXIS.substring(startIndex + 1, endIndex);
            } else {
                newAxis = targetAXIS;
            }
            mSystemControl.writeSysFs(VIDE_AXIS_NODE, newAxis);
        }
    }

    private void initViews() {
        mRotateLLay = (LinearLayout) findViewById(R.id.ll_rotate_r);
        mRotateRlay = (LinearLayout) findViewById(R.id.ll_rotate_l);
        mScaleUp = (LinearLayout) findViewById(R.id.ll_scale_up);
        mScaleDown = (LinearLayout) findViewById(R.id.ll_scale_down);

        mRotateRlay.setOnClickListener(this);
        mRotateLLay.setOnClickListener(this);
        mScaleUp.setOnClickListener(this);
        mScaleDown.setOnClickListener(this);

        mRotateRlay.setOnFocusChangeListener(this);
        mRotateLLay.setOnFocusChangeListener(this);
        mScaleUp.setOnFocusChangeListener(this);
        mScaleDown.setOnFocusChangeListener(this);

        mSurfaceView = (SurfaceView) findViewById(R.id.surfaceview_show_picture);
        mSurfaceView.getHolder().addCallback(new SurfaceCallback());
        mSurfaceView.getHolder().setKeepScreenOn(true);
        mSurfaceView.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        mSurfaceView.getHolder().setFormat(258);
        mSurfaceView.setFocusable(true);
        mSurfaceView.setFocusableInTouchMode(true);

        mMenu = (RelativeLayout) findViewById(R.id.menu_layout);
        mMenu.setVisibility(View.GONE);
        mLoadingProgress = (ProgressBar) findViewById(R.id.loading_image);
        mLoadingProgress.setVisibility(View.GONE);
        mOutAnimation = AnimationUtils.loadAnimation(this,
                R.anim.menu_and_left_out);
        mLeftInAnimation = AnimationUtils.loadAnimation(this, R.anim.left_in);
        mMenuInAnimation = AnimationUtils.loadAnimation(this, R.anim.menu_in);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (DEBUG) {
            Log.d(TAG, "onKeyDown: " + keyCode);
        }

        if (event.getRepeatCount() == 0 && (keyCode == KeyEvent.KEYCODE_MENU || keyCode == KeyEvent.KEYCODE_DPAD_CENTER)) {
            if (mMenu.getVisibility() == View.VISIBLE) {
                mUIHandler.removeMessages(DISMISS_MENU);
                mUIHandler.sendEmptyMessage(DISMISS_MENU);
            } else {
                displayMenu(true);
            }

            return true;
        } else if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (mMenu.getVisibility() == View.VISIBLE) {
                mUIHandler.removeMessages(DISMISS_MENU);
                mUIHandler.sendEmptyMessage(DISMISS_MENU);
            } else {
                FullImageActivity.this.finish();
            }

            return true;
        }

        return super.onKeyDown(keyCode, event);
    }

    private void displayMenu(boolean show) {
        if (!show) {
            mMenu.startAnimation(mLeftInAnimation);
            mMenu.setVisibility(View.GONE);
        } else {
            Log.d(TAG, "displayMenu set menu visible");
            mRotateRlay.requestFocus();
            mMenu.startAnimation(mOutAnimation);
            mMenu.setVisibility(View.VISIBLE);

            mUIHandler.sendEmptyMessageDelayed(DISMISS_MENU, DISPLAY_MENU_TIME);
        }
    }

    private void rotate(boolean right) {
        if (null == mImageplayer) {
            Log.e(TAG, "rotateRight imageplayer null");
            return;
        }

        if (right) {
            mDegress += ROTATION_DEGREE;
        } else {
            mDegress -= ROTATION_DEGREE;
        }
        Log.d(TAG, "rotate degree " + mDegress);
        if (mScale != SCALE_ORI) {
            Log.d(TAG, "rotate has scale with " + mScale);
            mImageplayer.setRotateScale(mDegress % 360, mScale, mScale, 0);
        } else {
            mImageplayer.setRotate(mDegress % 360, 1);
        }
    }

    private void scale(boolean scaleUp) {
        if (null == mImageplayer) {
            Log.e(TAG, "scale imageplayer null");
            return;
        }

        if (scaleUp) {
            mScale += SCALE_RATIO;
        } else {
            mScale -= SCALE_RATIO;
        }
        Log.d(TAG, "scale " + mScale);

        // value like 1.999999 could continue to be enlarged or else
        if ((SCALE_MAX - SCALE_ERR) <= mScale) {
            Toast.makeText(this, R.string.scale_to_maximized, Toast.LENGTH_SHORT).show();
            Log.e(TAG, "scale is max " + mScale);
            mScale = SCALE_MAX - SCALE_RATIO;
            return;
        }
        if ((SCALE_MIN + SCALE_ERR) >= mScale) {
            Toast.makeText(this, R.string.scale_to_minimized, Toast.LENGTH_SHORT).show();
            Log.e(TAG, "scale is min " + mScale);
            mScale = SCALE_MIN + SCALE_RATIO;
            return;
        }

        if (mImageplayer != null) {
            if (mDegress % 360 != 0) {
                Log.d(TAG, "scale has rotation with " + mDegress);
                mImageplayer.setRotateScale(mDegress % 360, mScale, mScale, 0);
            } else {
                mImageplayer.setScale(mScale, mScale, 0);
            }

        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.d(TAG, "onStart");
        if (null == mImageplayer) {
            mImageplayer = new ImagePlayer();
        }
        writeAxisNode();

        mLoadImageTask = new LoadImageTask();
        mLoadImageTask.execute();

        mUIHandler.sendEmptyMessageDelayed(DISMISS_MENU, DISPLAY_MENU_TIME);
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.d(TAG, "onStop");
        mLoadImageTask = null;

        if (mImageplayer != null) {
            Log.d(TAG, "onStop imageplayer release");
            mImageplayer.release();
            mImageplayer = null;
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy()");

        unregisterReceiver(mUsbScanner);
    }

    /* (non-Javadoc)
     * @see android.view.View.OnClickListener#onClick(android.view.View)
     */
    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.ll_rotate_r:
                mUIHandler.removeMessages(ROTATE_R);
                mUIHandler.sendEmptyMessage(ROTATE_R);
                break;
            case R.id.ll_rotate_l:
                mUIHandler.removeMessages(ROTATE_L);
                mUIHandler.sendEmptyMessage(ROTATE_L);
                break;
            case R.id.ll_scale_up:
                mUIHandler.removeMessages(SCALE_UP);
                mUIHandler.sendEmptyMessage(SCALE_UP);
                break;
            case R.id.ll_scale_down:
                mUIHandler.removeMessages(SCALE_DOWN);
                mUIHandler.sendEmptyMessage(SCALE_DOWN);
                break;
            default:
                break;
        }

        mUIHandler.removeMessages(DISMISS_MENU);
        mUIHandler.sendEmptyMessageDelayed(DISMISS_MENU, DISPLAY_MENU_TIME);
    }

    /* (non-Javadoc)
     * @see android.view.View.OnFocusChangeListener#onFocusChange(android.view.View, boolean)
     */
    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        if (hasFocus) {
            mUIHandler.removeMessages(DISMISS_MENU);
            mUIHandler.sendEmptyMessageDelayed(DISMISS_MENU, DISPLAY_MENU_TIME);
        }
    }

    private class SurfaceCallback implements SurfaceHolder.Callback {
        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            Log.v(TAG, "surfaceChanged");
            if (mImageplayer != null) {
                mImageplayer.setSampleSurfaceSize(1,width,height);
            }
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            Log.v(TAG, "surfaceCreated");

            if (mImageplayer != null) {
                Log.d(TAG, "surfaceCreated setDisplay");
                mImageplayer.setDisplay(holder);
            }
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            Log.v(TAG, "surfaceDestroyed");
        }
    }

    private final class LoadImageTask extends AsyncTask<Void, Void, Void> {
        @Override
        protected void onPreExecute() {
            // We don't want to show the spinner every time we load images, because that would be
            // annoying; instead, only start showing the spinner if loading the image has taken
            // longer than 1 sec (ie 1000 ms)
            mLoadingProgress.postDelayed(new Runnable() {
                public void run() {
                    if (getStatus() != AsyncTask.Status.FINISHED && mLoadingProgress.getVisibility() == View.GONE) {
                        mLoadingProgress.setVisibility(View.VISIBLE);
                    }
                }
            }, DISPLAY_PROGRESSBAR);
        }

        @Override
        protected Void doInBackground(Void... args) {
            Log.d(TAG, "doInBackground show image by image player service");
            runAndShow();
            return null;
        }

        @Override
        protected void onPostExecute(Void arg) {
            if (mLoadingProgress.getVisibility() == View.VISIBLE) {
                mLoadingProgress.setVisibility(View.GONE);
            }
        }

        @Override
        protected void onCancelled() {
            if (mLoadingProgress.getVisibility() == View.VISIBLE) {
                mLoadingProgress.setVisibility(View.GONE);
            }
        }
    }
    private static int[] getSplitStr(String originalStr,String splitStr){
        String[] vals = originalStr.split(splitStr);
        if (vals == null) return null;
        int[] intval = new int[vals.length];
        for (int i=0; i<vals.length; i++) {
            intval[i] = Integer.valueOf(vals[i]);
        }
        return intval;
    }
    private static Size getWH(String videoaxis,String windowaxis) {
        int[] videos = getSplitStr(videoaxis, "\\s+");
        if (videos.length == 4) {
            if (videos[2] == -1 && videos[3] == -1) {
                videos = getSplitStr(windowaxis, "\\s+");
            }
            Size wh = new Size(videos[2], videos[3]);
            return wh;
        }
        return new Size(3840,2160);
    }
}
