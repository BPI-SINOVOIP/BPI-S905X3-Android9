/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
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
package com.droidlogic.mediacenter.dlna;

import java.io.ByteArrayOutputStream;
import java.io.BufferedInputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Map;

import org.amlogic.upnp.AmlogicCP;
import org.amlogic.upnp.MediaRendererDevice;
import org.cybergarage.util.Debug;

import com.droidlogic.mediacenter.R;
import com.droidlogic.mediacenter.dlna.ScalingUtilities.ScalingLogic;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.media.ThumbnailUtils;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.Process;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.Toast;
import android.graphics.BitmapFactory.Options;
/**
 * @ClassName ImageFromUrl
 * @Description TODO
 * @Date 2013-8-27
 * @Email
 * @Author
 * @Version V1.0
 */
public class ImageFromUrl extends Activity {
        private static final String  TAG                 = "ImageFromUrl";
        private PowerManager.WakeLock mWakeLock;
        private DecordUri            mCurUri;
        private ImageFromUrlReceiver imageFromUrlReceiver;
        private Handler              mHandler;
        private DecodeBitmapTask     mDecodeBitmapTask;
        private Bitmap               myBitmap;
        private ImageView            mShowView;
        private LoadingDialog        mLoadingDialog;
        private static final float   TOPSIZE             = 2048.0f;
        private static final int     LOADING_URL_IMAG    = 1;
        private static final int     SHOW_BITMAP_URL     = 2;
        private static final int     EXIT_IMAG           = 3;
        private static final int     SHOW_STOP           = 4;
        private static final int     SLID_SHOW           = 5;
        private static final int     SHOWPANEL           = 6;
        private static final int     HIDEPANEL           = 7;
        private static final int     STOP_BY_SEVER       = 8;
        private static final int     STOP_DELAY          = 6000;
        private static final int     STOP_SHOW_INTERVAL  = 5000;
        private static final int     SLIDE_SHOW_INTERVAL = 5000;
        private static final int     SLIDE_MIN_INTERVAL  = 2000;
        private static final int     SLIDE_SHOW_NEXT = 10000;
        public static boolean        isShowingForehand;
        private LoadingDialog        exitDlg;
        private static final int     SLIDE_UNSTATE       = 0;
        private static final int     SLIDE_START         = 1;
        private static final int     SLIDE_STOP          = 2;
        private static final int     TOP_LEVEL = 32;
        private int                  mSlideShow          = SLIDE_UNSTATE;
        //private int                  mSlideShowDirection = 0;
        private ImageButton         btnBack;
        private ImageButton            btnLeft;
        private ImageButton            btnRight;
        private ImageButton            btnPlay;
        private ImageButton            btnRoomIn;
        private ImageButton            btnRoomOut;
        private ViewGroup            mSlideView;
        private int                  mCurIndex           = 0;
        private double zoomCount = 1;
        private boolean              isBrowserMode;
        private boolean reg = false;
        private boolean stopZoom = false;
        private String mNextURI;
        @Override
        protected void onCreate ( Bundle arg0 ) {
            super.onCreate ( arg0 );
            setContentView ( R.layout.display_image_activity );
        }
        @Override
        protected void onStart() {
            super.onStart();
            mCurUri = new DecordUri();
            mDecodeBitmapTask = new DecodeBitmapTask ( mCurUri );
            mShowView = ( ImageView ) findViewById ( R.id.imageview );
            mHandler = new DecodeHandler ( mCurUri );
            mDecodeBitmapTask.start();
            Debug.d(TAG,"mCurUri:"+mCurUri);
            ViewGroup mainView = ( ViewGroup ) findViewById ( R.id.image_control );
            mainView.setOnClickListener ( new View.OnClickListener() {
                @Override
                public void onClick ( View v ) {
                    if ( isBrowserMode ) {
                        mHandler.sendEmptyMessage ( SHOWPANEL );
                        hideBar ( 3000 );
                    }
                }
            } );
            btnBack = ( ImageButton ) findViewById ( R.id.back );
            btnBack.setOnClickListener ( new View.OnClickListener() {
                @Override
                public void onClick ( View v ) {
                    ret2list();
                }
            } );
            btnLeft = ( ImageButton ) findViewById ( R.id.bt_left );
            btnLeft.setOnClickListener ( new View.OnClickListener() {
                public void onClick ( View v ) {
                    mSlideShow = SLIDE_UNSTATE;
                    hideBar ( 3000 );
                    prev();
                }
            } );
            btnRight = ( ImageButton ) findViewById ( R.id.bt_right );
            btnRight.setOnClickListener ( new View.OnClickListener() {
                public void onClick ( View v ) {
                    mSlideShow = SLIDE_UNSTATE;
                    hideBar ( 3000 );
                    next();
                }
            } );
            btnPlay = ( ImageButton ) findViewById ( R.id.bt_play_pause );
            /*mShowView.setFocusableInTouchMode(true);
            mShowView.requestFocusFromTouch();*/
            btnPlay.requestFocus();
            btnPlay.setOnClickListener ( new View.OnClickListener() {
                public void onClick ( View v ) {
                    if ( mSlideShow == SLIDE_START ) {
                        btnPlay.setImageResource ( R.drawable.play_play );
                        mSlideShow = SLIDE_STOP;
                        mCurUri.setUrl ( null );
                    } else {
                        btnPlay.setImageResource ( R.drawable.suspend_play );
                        mSlideShow = SLIDE_START;
                        showLoading();
                        mHandler.sendEmptyMessageDelayed ( SLID_SHOW,
                                                           SLIDE_MIN_INTERVAL );
                        hideBar ( 1000 );
                    }
                }
            } );
            btnRoomIn = ( ImageButton ) findViewById ( R.id.bt_roomin );
            btnRoomIn.setOnClickListener ( new View.OnClickListener() {
                @Override
                public void onClick ( View v ) {
                    zoomIn();
                }
            } );
            btnRoomOut = ( ImageButton ) findViewById ( R.id.bt_roomout );
            btnRoomOut.setOnClickListener ( new View.OnClickListener() {
                @Override
                public void onClick ( View v ) {
                    zoomOut();
                }
            } );
            mSlideView = ( ViewGroup ) findViewById ( R.id.split_layout );

            OnFocusChangeListener listener = new View.OnFocusChangeListener() {
                @Override
                public void onFocusChange ( View v, boolean hasFocus ) {
                    Debug.d ( TAG, "image View:" + hasFocus + "isBrowserMode:" + isBrowserMode );
                    if ( hasFocus && isBrowserMode ) {
                        mHandler.removeMessages ( HIDEPANEL );
                        mHandler.sendEmptyMessageDelayed ( HIDEPANEL, STOP_SHOW_INTERVAL );
                    }
                }
            };
            btnBack.setOnFocusChangeListener ( listener );
            btnRoomOut.setOnFocusChangeListener ( listener );
            btnRoomIn.setOnFocusChangeListener ( listener );
            btnPlay.setOnFocusChangeListener ( listener );
            btnRight.setOnFocusChangeListener ( listener );
            btnLeft.setOnFocusChangeListener ( listener );
            reg = false;
            Intent intent = getIntent();
            if ( intent != null ) {
                String url = intent.getStringExtra ( AmlogicCP.EXTRA_MEDIA_URI );
                Message msg = new Message();
                mNextURI= intent.getStringExtra(MediaRendererDevice.EXTRA_NEXT_URI);
                if ( DeviceFileBrowser.TYPE_DMP.equals ( intent
                        .getStringExtra ( DeviceFileBrowser.DEV_TYPE ) ) ) {
                    mCurIndex = intent.getIntExtra ( DeviceFileBrowser.CURENT_POS, 0 );
                    isBrowserMode = true;
                    mSlideView.setVisibility ( View.VISIBLE );
                } else {
                    mCurIndex = -1;
                    isBrowserMode = false;
                    mSlideView.setVisibility ( View.INVISIBLE );
                }
                msg.what = LOADING_URL_IMAG;
                msg.obj = url;
                mHandler.sendMessage ( msg );
            }
        }
        private void unregistRec() {
            if ( reg ) {
                unregisterReceiver ( imageFromUrlReceiver );
                reg = false;
            }
        }
        private void zoomIn() {
            Display display = getWindowManager().getDefaultDisplay();
            Rect rect = new Rect();
            display.getRectSize ( rect );
            if ( myBitmap == null ) {
                Toast.makeText ( getApplicationContext(),
                                 R.string.pic_unspport, Toast.LENGTH_SHORT ).show();
                return;
            }
            /*BitmapDrawable bitmapDrawable = (BitmapDrawable)mShowView.getDrawable();
            Bitmap bm = bitmapDrawable.getBitmap();*/
            double newCount = zoomCount;
            newCount = newCount * 2;
            int newWidth = ( int ) ( myBitmap.getWidth() * newCount );
            int newHeight = ( int ) ( myBitmap.getHeight() * newCount );
            if ( newCount < TOP_LEVEL || ( newWidth <= rect.width() && newHeight <= rect.height() ) ) {
                zoomCount = newCount;
                Bitmap scaledBitmap;
                if ( newWidth >= rect.width() || newHeight >= rect.height() ) {
                    newWidth = newWidth > rect.width() ? rect.width() : newWidth;
                    newHeight = newHeight > rect.height() ? rect.height() : newHeight;
                    scaledBitmap = ScalingUtilities.createScaledBitmap ( myBitmap, newWidth,
                                   newHeight, ScalingLogic.CROP );
                } else {
                    scaledBitmap = ScalingUtilities.createScaledBitmap ( myBitmap, newWidth,
                                   newHeight, ScalingLogic.FIT );
                }
                mShowView.setImageBitmap ( scaledBitmap );
                scaledBitmap = null;
            } else {
                Toast.makeText ( getApplicationContext(),
                                 R.string.pic_largest, Toast.LENGTH_SHORT ).show();
            }
        }
        private void ret2list() {
            if (isBrowserMode) {
                Intent intent = new Intent(this,DeviceFileBrowser.class);
                this.setResult ( mCurIndex, intent );
            }
            this.finish();
        }
        private void zoomOut() {
            Display display = getWindowManager().getDefaultDisplay();
            Rect rect = new Rect();
            display.getRectSize ( rect );
            /*BitmapDrawable bitmapDrawable = (BitmapDrawable)mShowView.getDrawable();
            Bitmap bm = bitmapDrawable.getBitmap();*/
            if ( myBitmap == null ) {
                Toast.makeText ( getApplicationContext(),
                                 R.string.pic_unspport, Toast.LENGTH_SHORT ).show();
                return;
            }
            Debug.d ( TAG, "zoomCount:" + zoomCount + " Top_level:" + ( 1.0 / TOP_LEVEL ) );
            double newCount = zoomCount / 2;
            int newWidth = ( int ) ( myBitmap.getWidth() * newCount );
            int newHeight = ( int ) ( myBitmap.getHeight() * newCount );
            if ( newCount > ( 1.0 / TOP_LEVEL ) || ( newWidth > 16 && newHeight > 16 ) ) {
                zoomCount = newCount;
                Bitmap scaledBitmap = null;
                if ( newCount == 1 ) {
                    mShowView.setImageBitmap ( myBitmap );
                } else {
                    if ( newWidth > rect.width() || newHeight > rect.height() ) {
                        newWidth = newWidth > rect.width() ? rect.width() : newWidth;
                        newHeight = newHeight > rect.height() ? rect.height() : newHeight;
                        scaledBitmap = ScalingUtilities.createScaledBitmap ( myBitmap, newWidth,
                                       newHeight, ScalingLogic.CROP );
                    } else {
                        scaledBitmap = ScalingUtilities.createScaledBitmap ( myBitmap, newWidth,
                                       newHeight, ScalingLogic.FIT );
                    }
                    mShowView.setImageBitmap ( scaledBitmap );
                }
                scaledBitmap = null;
            } else {
                Toast.makeText ( getApplicationContext(), R.string.pic_lest, Toast.LENGTH_SHORT ).show();
                return;
            }
        }

        private void onReg() {
            imageFromUrlReceiver = new ImageFromUrlReceiver();
            if ( !reg ) {
                IntentFilter filter = new IntentFilter();
                filter.addAction ( AmlogicCP.UPNP_STOP_ACTION );
                filter.addAction ( AmlogicCP.UPNP_PLAY_ACTION );
                registerReceiver ( imageFromUrlReceiver, filter );
                reg = true;
            }
        }
        public boolean onKeyDown ( int keyCode, KeyEvent event ) {
            Debug.d ( TAG, "******keycode=" + keyCode );
            if ( keyCode == KeyEvent.KEYCODE_BACK ) {
                if ( isBrowserMode && mSlideShow == SLIDE_START ) {
                    mHandler.removeMessages ( HIDEPANEL );
                    mSlideShow = SLIDE_STOP;
                    mCurUri.setUrl ( null );
                    mHandler.sendEmptyMessage ( SHOWPANEL );
                } else {
                    //mHandler = null;
                    unregistRec();
                    stopExit();
                    mSlideShow = SLIDE_UNSTATE;
                    ret2list();
                }
                return true;
            } else if ( !mSlideView.isShown()
                        && keyCode == KeyEvent.KEYCODE_DPAD_CENTER ) {
                if ( isBrowserMode ) {
                    mHandler.sendEmptyMessage ( SHOWPANEL );
                    hideBar ( 10000 );
                }
                return true;
            } else if ( ( keyCode == KeyEvent.KEYCODE_DPAD_UP || keyCode == KeyEvent.KEYCODE_DPAD_DOWN )
                        && ( mSlideShow == SLIDE_START ) ) {
                mHandler.sendEmptyMessage ( SHOWPANEL );
                hideBar ( 3000 );
                return true;
            }
            return super.onKeyDown ( keyCode, event );
        }

        private void next() {
            getCurIndex ( true );
            play();
        }

        @Override
        protected void onDestroy() {
            super.onDestroy();
        }

        private void getCurIndex ( boolean next ) {
            if ( mCurIndex > ( DeviceFileBrowser.playList.size() - 1 )
                    || mCurIndex < 0 )
            { mCurIndex = 0; }
            if ( !next ) {
                if ( mCurIndex > 0 ) {
                    mCurIndex--;
                } else {
                    mCurIndex = DeviceFileBrowser.playList.size() - 1;
                }
            } else {
                if ( mCurIndex < ( DeviceFileBrowser.playList.size() - 1 ) ) {
                    mCurIndex++;
                } else {
                    mCurIndex = 0;
                }
            }
        }

        private void prev() {
            getCurIndex ( false );
            play();
        }

        private void play() {
            Map<String, Object> item = ( Map<String, Object> ) DeviceFileBrowser.playList
                                       .get ( mCurIndex );
            Message msg = mHandler.obtainMessage();
            msg.what = LOADING_URL_IMAG;
            msg.obj = ( String ) item.get ( "item_uri" );
            msg.sendToTarget();
        }

        @Override
        protected void onResume() {
            super.onResume();
            getWindow().addFlags(LayoutParams.FLAG_DISMISS_KEYGUARD | LayoutParams.FLAG_TURN_SCREEN_ON);
            isShowingForehand = true;
            //mHandler.removeMessages ( STOP_BY_SEVER );
            //mHandler.sendEmptyMessageDelayed ( STOP_BY_SEVER, 5000 );
            if ( isBrowserMode ) {
                mHandler.sendEmptyMessageDelayed ( HIDEPANEL, STOP_SHOW_INTERVAL );
            }
            onReg();
            /* enable backlight */
            PowerManager pm = ( PowerManager ) getSystemService ( Context.POWER_SERVICE );
            mWakeLock = pm.newWakeLock ( PowerManager.SCREEN_BRIGHT_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE, TAG );
            mWakeLock.acquire();
        }

        private void hideBar ( int timeout ) {
            mHandler.removeMessages ( HIDEPANEL );
            mHandler.sendEmptyMessageDelayed ( HIDEPANEL, timeout );
        }


        @Override
        protected void onPause() {
            super.onPause();
            stopExit();
            isShowingForehand = false;
            hideLoading();
            mSlideShow = SLIDE_UNSTATE;
            if ( mCurUri != null ) {
                mCurUri.setUrl ( null );
            }
            unregistRec();
            mWakeLock.release();
            this.finish();
        }

        @Override
        protected void onStop() {
            super.onStop();
            Debug.d(TAG,"onStop");
            if ( mHandler != null ) {
                mHandler = null;
            }
            if ( mCurUri != null ) {
                mCurUri = null;
            }
            if ( mDecodeBitmapTask != null ) {
                mDecodeBitmapTask.interrupted();
                mDecodeBitmapTask.stopThread();
                mDecodeBitmapTask = null;
            }
        }

        class ImageFromUrlReceiver extends BroadcastReceiver {
                /*
                 * (non-Javadoc)
                 *
                 * @see
                 * android.content.BroadcastReceiver#onReceive(android.content.Context,
                 * android.content.Intent)
                 */
                @Override
                public void onReceive ( Context cxt, Intent intent ) {
                    String action = intent.getAction();
                    String mediaType = intent
                                       .getStringExtra ( AmlogicCP.EXTRA_MEDIA_TYPE );
                    Debug.d ( TAG, "getAction:" + action );
                    if ( !MediaRendererDevice.TYPE_IMAGE.equals ( mediaType ) )
                    { return; }
                    if ( action.equals ( AmlogicCP.UPNP_PLAY_ACTION ) ) {
                        hideLoading();
                        mHandler.removeMessages ( LOADING_URL_IMAG );
                        String url = intent.getStringExtra ( AmlogicCP.EXTRA_MEDIA_URI );
                        mNextURI= intent.getStringExtra(MediaRendererDevice.EXTRA_NEXT_URI);
                        Message msg = new Message();
                        if ( DeviceFileBrowser.TYPE_DMP.equals ( intent
                                .getStringExtra ( DeviceFileBrowser.DEV_TYPE ) ) ) {
                            mCurIndex = intent.getIntExtra (
                                            DeviceFileBrowser.CURENT_POS, 0 );
                            isBrowserMode = true;
                        } else {
                            isBrowserMode = false;
                        }
                        msg.what = LOADING_URL_IMAG;
                        msg.obj = url;
                        mHandler.removeMessages(LOADING_URL_IMAG);
                        mHandler.sendMessage ( msg );
                        return;
                    } else if ( action.equals ( AmlogicCP.UPNP_STOP_ACTION ) ) {
                        stopExit();
                        mHandler.sendEmptyMessageDelayed ( SHOW_STOP, STOP_DELAY );
                    }
                }
        }

        class DecodeBitmapTask extends Thread {
                private DecordUri mUrl;
                private boolean   stop = false;

                public DecodeBitmapTask ( DecordUri url ) {
                    mUrl = url;
                }

                public void stopThread() {
                    stop = true;
                }

                @Override
                public void run() {
                    String urlString = null;
                    stop = false;
                    Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_DISPLAY );
                    Debug.d(TAG,"startTime:"+System.currentTimeMillis());
                    while ( !stop ) {
                        urlString = mUrl.getUrl();
                        if ( urlString != null ) {
                            try {
                                URL url = new URL ( urlString );
                                HttpURLConnection connection = ( HttpURLConnection ) url.openConnection();
                                connection.setDoInput ( true );
                                connection.connect();
                                BufferedInputStream input = new BufferedInputStream ( connection.getInputStream() );
                                if ( input == null ) {
                                    Debug.e ( TAG, "***get Bitmap error!" );
                                    throw new RuntimeException ( "stream is null" );
                                } else {
                                    BitmapFactory.Options bmOptions = new BitmapFactory.Options();
                                    bmOptions.inSampleSize = 1;
                                    bmOptions.inPurgeable = true;
                                    bmOptions.inPreferredConfig = Bitmap.Config.RGB_565;
                                    bmOptions.inDither = false;
                                    myBitmap = BitmapFactory.decodeStream ( input, null, bmOptions );
                                    Debug.d(TAG,"myBitmap==null"+(myBitmap==null));
                                    Intent playIntent = new Intent(MediaRendererDevice.PLAY_STATE_PLAYING);
                                    ImageFromUrl.this.sendBroadcast(playIntent);
                                    if ( mHandler != null ) {
                                        mHandler.removeMessages(SHOW_BITMAP_URL);
                                        Message msg = new Message();
                                        msg.what = SHOW_BITMAP_URL;
                                        msg.obj = urlString;
                                        mHandler.sendMessage(msg);
                                        Debug.d(TAG,"SHOW_BITMAP_URL"+urlString);
                                    }
                                    connection.disconnect();
                                }
                            } catch ( Exception e ) {
                                // TODO Auto-generated catch block
                                Looper.prepare();
                                Toast.makeText ( getApplicationContext(),
                                                 R.string.disply_err, Toast.LENGTH_SHORT ).show();
                                e.printStackTrace();
                                Looper.loop();
                            }
                        } else {
                            try {
                                sleep ( 1000 );
                            } catch ( InterruptedException e ) {
                                // TODO Auto-generated catch block
                                e.printStackTrace();
                            }
                        }
                    }
                }
        }

        private void showLoading() {
            if ( mLoadingDialog == null ) {
                mLoadingDialog = new LoadingDialog ( this,
                                     LoadingDialog.TYPE_LOADING, this.getResources().getString (R.string.loading ) );
                mLoadingDialog.setCancelable ( true );
                mLoadingDialog.show();
            } else {
                mLoadingDialog.show();
            }
        }

        private void stopExit() {
            mHandler.removeMessages ( SHOW_STOP );
            if ( exitDlg != null ) {
                exitDlg.dismiss();
                exitDlg = null;
            }
        }

        /**
         * @Description TODO
         */
        public void wait2Exit() {
            Debug.d ( TAG, "wait2Exit......" );
            if ( !isShowingForehand ) {
                this.finish();
                return;
            }
            hideLoading();
            if ( exitDlg == null ) {
                exitDlg = new LoadingDialog ( this, LoadingDialog.TYPE_EXIT_TIMER, "" );
                exitDlg.setCancelable ( true );
                exitDlg.setOnDismissListener ( new OnDismissListener() {
                    @Override
                    public void onDismiss ( DialogInterface arg0 ) {
                        if ( exitDlg != null && ( ImageFromUrl.this.getClass().getName().equals ( exitDlg.getTopActivity ( ImageFromUrl.this ) ) ||
                        exitDlg.getCountNum() == 0 ) ) {
                            unregistRec();
                            ImageFromUrl.this.finish();
                        }
                    }
                } );
                exitDlg.show();
            } else {
                exitDlg.setCountNum ( 4 );
                exitDlg.show();
            }
        }

        private void hideLoading() {
            if ( mLoadingDialog != null ) {
                mLoadingDialog.stopAnim();
                mLoadingDialog.dismiss();
            }
        }

        class DecordUri {
                private String mDecodeUrl;

                public synchronized void setUrl ( String url ) {
                    mDecodeUrl = url;
                }

                public synchronized String getUrl() {
                    String url = mDecodeUrl;
                    mDecodeUrl = null;
                    return url;
                }
        }

        private void showImage() {
             Debug.d(TAG,"showImage showMap:"+mSlideShow);
            if ( mSlideShow == SLIDE_STOP ) {
                hideLoading();
                return;
            }
            Debug.d(TAG,"showImage showMap:"+System.currentTimeMillis()+"-"+(myBitmap != null));
            if ( myBitmap != null ) {
                zoomCount = 1;
                int height = myBitmap.getHeight();
                int width = myBitmap.getWidth();
                float reSize = 1.0f;
                Debug.d(TAG,"showImage showMap:"+height+"-"+width);
                if ( width > TOPSIZE || height > TOPSIZE ) {
                    if ( height > width ) {
                        reSize = TOPSIZE / height;
                    } else {
                        reSize = TOPSIZE / width;
                    }
                    Matrix matrix = new Matrix();
                    matrix.postScale ( reSize, reSize );
                    myBitmap = Bitmap.createBitmap ( myBitmap, 0, 0, width,
                                                     height, matrix, true );
                    Debug.d(TAG,"showImage showMap:"+(myBitmap==null));
                    mShowView.setImageBitmap ( myBitmap );
                } else {
                    Debug.d(TAG,"showImage showMa==========");
                    mShowView.setImageBitmap ( myBitmap );
                }
                //myBitmap = null;
            } else {
                mShowView.setImageResource ( R.drawable.ic_missing_thumbnail_picture );
                Toast.makeText ( getApplicationContext(), R.string.disply_err,
                                 Toast.LENGTH_SHORT ).show();
            }
            hideLoading();
            if (mNextURI != null && !mNextURI.isEmpty()) {
                String uri = mNextURI;
                Message msg = new Message();
                msg.what = LOADING_URL_IMAG;
                msg.obj = uri;
                msg.arg1 = 1;
                mHandler.sendMessageDelayed(msg,SLIDE_SHOW_NEXT);
                mNextURI = "";
            }
            if ( mSlideShow == SLIDE_START && null != mHandler ) {
                mHandler.sendEmptyMessageDelayed ( SLID_SHOW, SLIDE_SHOW_INTERVAL );
            }
        }

        private void slideShow() {
            if ( mSlideShow != SLIDE_START ) {
                return;
            }
            next();
        }

        class DecodeHandler extends Handler {
                private DecordUri mUrl;

                public DecodeHandler ( DecordUri url ) {
                    mUrl = url;
                }

                @Override
                public void handleMessage ( Message msg ) {
                    Debug.d (TAG,"handleMessage msg:"+msg.what);
                    switch ( msg.what ) {
                        case LOADING_URL_IMAG:
                            stopExit();
                            showLoading();
                            Debug.d ( TAG, "(String)msg.obj:" + ( String ) msg.obj );
                            mUrl.setUrl ( ( String ) msg.obj );
                            if ( mDecodeBitmapTask == null ) {
                                mDecodeBitmapTask = new DecodeBitmapTask ( mCurUri );
                                mDecodeBitmapTask.start();
                            }
                            break;
                        case SHOW_BITMAP_URL:
                            //hideLoading();
                            showImage();
                            break;
                        case SHOW_STOP:
                            wait2Exit();
                            break;
                        case SLID_SHOW:
                            slideShow();
                            break;
                        case SHOWPANEL:
                            if ( mSlideView != null ) {
                                if ( btnPlay != null ) {
                                    btnPlay.requestFocus();
                                    if ( mSlideShow == SLIDE_START ) {
                                        //btnPlay.setImageDrawable(ImageFromUrl.this.getResources().getDrawable(R.drawable.suspend_play));
                                        btnPlay.setImageResource ( R.drawable.suspend_play );
                                    } else {
                                        //btnPlay.setImageDrawable(ImageFromUrl.this.getResources().getDrawable(R.drawable.play_play));
                                        btnPlay.setImageResource ( R.drawable.play_play );
                                    }
                                }
                                mSlideView.setVisibility ( View.VISIBLE );
                            }
                            break;
                        case HIDEPANEL:
                            if ( mSlideView != null ) {
                                mSlideView.setVisibility ( View.INVISIBLE );
                            }
                            break;
                        case STOP_BY_SEVER:
                            /*if ( !isShowingForehand ) {
                                ImageFromUrl.this.finish();
                            } else {
                                if ( null != mHandler ) {
                                    mHandler.sendEmptyMessageDelayed ( STOP_BY_SEVER, 5000 );
                                }
                            }*/
                            break;
                    }
                }
        }
}
