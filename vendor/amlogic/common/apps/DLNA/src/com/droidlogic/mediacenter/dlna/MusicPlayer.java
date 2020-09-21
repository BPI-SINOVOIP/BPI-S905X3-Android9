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

import java.lang.Integer;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.util.Timer;
import java.util.TimerTask;
import java.util.Random;
import java.io.IOException;
import java.lang.Math;
import java.lang.Thread.UncaughtExceptionHandler;

import android.os.Handler;
import android.os.Message;
import android.os.Bundle;
import android.os.SystemClock;
import android.os.PowerManager;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.widget.Toast;
import android.content.DialogInterface.OnDismissListener;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff.Mode;
import android.graphics.PorterDuffXfermode;
import android.graphics.drawable.Drawable;


import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.view.Display;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;

import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.SeekBar;
import android.widget.ProgressBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.LinearLayout;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;

import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnInfoListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.net.Uri;
import org.amlogic.upnp.*; // for CyberLink

import org.cybergarage.upnp.*;
import org.cybergarage.upnp.device.*;
import org.cybergarage.util.Debug;

import com.droidlogic.mediacenter.R;

public class MusicPlayer extends Activity implements OnPreparedListener,
        OnCompletionListener, OnInfoListener {
        private static final boolean DEBUG_PLAYER = true;
        private static final String TAG                    = "MusicPlayer";
        public static final String  TIME_START             = "00:00:00";
        public static final int     MSG_GET_POSITION_TIMER = 0xABCDDEAD;
        public static final int     STATE_PLAY             = 0;
        public static final int     STATE_PAUSE            = 1;
        public static final int     STATE_STOP             = 2;
        private static final int    DIALOG_VOLUME_ID       = 2;
        private static final int    DIALOG_MODE_ID         = 3;


        private static final int    SHOW_START             = 2;
        private static final int    SHOW_STOP              = 3;
        private static final int    STOP_AND_START         = 4;
        private static final int    SHOW_LOADING           = 5;
        private static final int    HIDE_LOADING           = 6;
        private static final int    STOP_BY_SEVER          = 7;
        private static final int    VOLUME_HIDE            = 8;
        private static final int    STOP_DEVICE               = 9;
        private LoadingDialog exitDlg;
        private Dialog              dialog_volume;
        private TextView            mFileName;
        private ImageView           mMediaType;
        private MarqueeTextView     mPath;
        private ImageView           mMediaThumb;
        private TextView            mCurTime;
        private TextView            mTotalTime;
        // private SeekBar mSeekBar;
        private ProgressBar         mProgress;
        private ImageButton         btn_back;
        // private ImageButton btn_stop;
        private ImageButton         btn_prev;
        private ImageButton         btn_backward;
        private ImageButton         btn_play;
        private ImageButton         btn_forward;
        private ImageButton         btn_next;
        private ImageButton         btn_volume;
        private ImageButton         btn_mode;
        private ProgressBar         vol_bar;
        private PreviewPlayer       mPlayer;
        private AudioManager        mAudioManager;
        private Handler             mProgressRefresher;
        private boolean             mPausedByTransientLossOfFocus;
        private String              cur_uri;
        private String              media_type;
        private String              file_name              = null;
        private boolean             isBrowserMode;
        private int mCurIndex = 0;
        private int                 play_state             = STATE_PLAY;
        private int                 volume_level           = 50;
        private boolean             mVolTouch              = false;
        private boolean             mVolChanged            = false;
        private boolean             mProgressTouch         = false;
        private int                 MODE_ALL_LOOP          = 0;
        private int                 MODE_SINGLE_LOOP       = 1;
        private int                 MODE_RANDOM            = 2;
        private int                 mode                   = 0;
        private Dialog              mode_dialog;
        // total time in ms
        private int                 mTotalDuration;
        private boolean             readyForFinish         = false;
        private LoadingDialog       progressDialog         = null;
        private UPNPReceiver        mUPNPReceiver;
        //private boolean mVisualizerRelease = false;
        public static boolean       isShowingForehand;
        //private ViewGroup mShowVisual;
        private List<Map<Integer, Boolean>> modeData;
        private PowerManager.WakeLock mWakeLock;

        private static final int DIALOG_SHOW_DELAY = 5000;
        private static final int PROGRESS_TIME_DELAY = 1000;
        private static final int BUFFER_INTERVAL = 1000;
        /*send to Device info:AVT_STOP_STATUS*/
        private static final int STOP_PLAY_BROADCAST = 6000;
        private boolean setMute = false;
        private String mNextURI = null;
        private String mNextFileName = null;
        private String mCurrentMeta = null;
        private static final int SEEK_STEP = 10000;//10 s
        public void onCreate ( Bundle savedInstanceState ) {
            super.onCreate ( savedInstanceState );
            setContentView ( R.layout.music_activity );
            getWindow().addFlags(LayoutParams.FLAG_DISMISS_KEYGUARD | LayoutParams.FLAG_TURN_SCREEN_ON);
            LayoutParams params = getWindow().getAttributes();
            //params.type = WindowManager.LayoutParams.TYPE_SYSTEM_ALERT;
            //| WindowManager.LayoutParams.TYPE_SYSTEM_OVERLAY;
            //params.flags = LayoutParams.FLAG_NOT_TOUCH_MODAL;
            //params.alpha = 1.0f;
            //params.dimAmount = 0.0f;
            //        params.height = WindowManager.LayoutParams.WRAP_CONTENT;
            params.width = WindowManager.LayoutParams.MATCH_PARENT;
            //params.gravity=Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;
            getWindow().setAttributes ( params );
            /*getWindow().setFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND,
                    WindowManager.LayoutParams.FLAG_BLUR_BEHIND);*/
            getWindow().setGravity ( Gravity.BOTTOM | Gravity.FILL_HORIZONTAL );
            // mMediaType = (ImageView) findViewById(R.id.media_type);
            // mPath = (MarqueeTextView) findViewById(R.id.path);
            // mMediaThumb = (ImageView) findViewById(R.id.media_thumb);
            //mVisualizerView = (VisualizerView)findViewById(R.id.showvisual);
            /*mVisualizerView = new VisualizerView(this);
            mVisualizerView.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));
            mShowVisual.addView(mVisualizerView);*/
            mCurTime = ( TextView ) findViewById ( R.id.currenttime );
            mTotalTime = ( TextView ) findViewById ( R.id.totaltime );
            mProgress = ( ProgressBar ) findViewById ( R.id.progress );
            btn_back = ( ImageButton ) findViewById ( R.id.back );
            // btn_stop = (ImageButton) findViewById(R.id.stop);
            btn_prev = ( ImageButton ) findViewById ( R.id.prev );
            btn_backward = ( ImageButton ) findViewById ( R.id.backward );
            btn_play = ( ImageButton ) findViewById ( R.id.pause );
            btn_forward = ( ImageButton ) findViewById ( R.id.forward );
            btn_next = ( ImageButton ) findViewById ( R.id.next );
            btn_volume = ( ImageButton ) findViewById ( R.id.volume );
            btn_mode = ( ImageButton ) findViewById ( R.id.play_mode );
            Intent intent = getIntent();
            media_type = intent.getStringExtra ( AmlogicCP.EXTRA_MEDIA_TYPE );
            cur_uri = intent.getStringExtra ( AmlogicCP.EXTRA_MEDIA_URI );
            mNextURI= intent.getStringExtra(MediaRendererDevice.EXTRA_NEXT_URI);
            mCurrentMeta = intent.getStringExtra(MediaRendererDevice.EXTRA_META_DATA);
            file_name = intent.getStringExtra ( AmlogicCP.EXTRA_FILE_NAME );
            String type = intent.getStringExtra ( DeviceFileBrowser.DEV_TYPE );
            if ( DeviceFileBrowser.TYPE_DMP.equals ( type ) ) {
                mCurIndex = intent.getIntExtra ( DeviceFileBrowser.CURENT_POS, 0 );
                isBrowserMode = true;
            } else {
                isBrowserMode = false;
            }
            mFileName = ( TextView ) findViewById ( R.id.tx_music_name );
            if ( file_name == null ) {
                file_name = cur_uri.substring ( cur_uri.lastIndexOf ( '/' ) + 1, cur_uri.length() );
                //file_name = getResources().getString(R.string.str_unknown);
            }
            /*mFileName.setText(file_name + "                                 "
                    + file_name + "                                 " + file_name
                    + "                 ");*/
            mFileName.setText ( file_name );
            if ( !MediaRendererDevice.TYPE_AUDIO.equals ( media_type ) || ( cur_uri == null ) ) {
                finish();
                return;
            }
            //mVisualizerRelease = true;
            Debug.d ( TAG, "********onCreat: " + cur_uri );
            mProgressRefresher = new Handler();
            mAudioManager = ( AudioManager ) getSystemService ( Context.AUDIO_SERVICE );
            /*mAudioManager.requestAudioFocus(mAudioFocusListener,
                    AudioManager.STREAM_MUSIC,
                    AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);*/
            setVolumeControlStream ( AudioManager.STREAM_MUSIC );
            btn_prev.requestFocus();
            // btn_prev.setFocusableInTouchMode(true);
            // btn_prev.requestFocusFromTouch();
            if ( mProgress instanceof SeekBar ) {
                SeekBar seeker = ( SeekBar ) mProgress;
                // seeker.setOnSeekBarChangeListener(mSeekListener);
                seeker.setOnSeekBarChangeListener ( new OnSeekBarChangeListener() {
                    private long mLastTime = 0;
                    public void onStartTrackingTouch ( SeekBar bar ) {
                        if ( DEBUG_PLAYER )
                        { Debug.d ( TAG, "mProgress:onStartTrackingTouch" ); }
                        mLastTime = 0;
                        mProgressTouch = true;
                    }
                    public void onProgressChanged ( SeekBar bar, int progress,
                    boolean fromuser ) {
                        if ( DEBUG_PLAYER )
                        { Debug.d ( TAG, "mProgress:onProgressChanged=" + progress ); }
                        if ( !fromuser )
                        { return; }
                        mLastTime = 0;
                        mProgressTouch = true;
                        long now = SystemClock.elapsedRealtime();
                        if ( ( now - mLastTime ) > 250 ) {
                            mLastTime = now;
                            // trackball event, allow progress updates
                            if ( mProgressTouch && ( mPlayer != null )
                            && ( mTotalDuration > 0 ) ) {
                                Debug.d ( TAG, "***progress=" + progress );
                                mProgress.setProgress ( progress );
                                mPlayer.seekTo ( progress );
                                mCurTime.setText ( timeFormatToString ( progress ) );
                                Intent intent = new Intent (
                                    AmlogicCP.PLAY_POSTION_REFRESH );
                                intent.putExtra ( "curPosition", progress );
                                intent.putExtra ( "totalDuration", mTotalDuration );
                    intent.putExtra ("currentURI",cur_uri);
                    intent.putExtra ("currentMeta",mCurrentMeta);
                                sendBroadcast ( intent );
                                //Debug.d(TAG, "######sendBroadcast(seekto)######" + progress + "/" + mTotalDuration);
                            }
                        }
                        mProgressTouch = false;
                    }
                    public void onStopTrackingTouch ( SeekBar bar ) {
                        Debug.d ( TAG, "mProgress:onStopTrackingTouch: " + volume_level );
                        mProgressTouch = false;
                    }
                } );
            }
            mProgress.setMax ( 1000 );
            btn_play.setOnClickListener ( new OnClickListener() {
                public void onClick ( View v ) {
                    Debug.d ( TAG, "btn_play.OnClick" );
                    if ( mPlayer == null ) {
                        start();
                    } else if ( mPlayer.isPlaying() ) {
                        pause();
                        Debug.d ( TAG, ">>>>>>>>>playPauseClicked , pause" );
                    } else {
                        play();
                        Debug.d ( TAG, ">>>>>>>>>playPauseClicked , playing" );
                    }
                }
            } );
            btn_backward.setOnClickListener ( new OnClickListener() {
                public void onClick ( View v ) {
                    Debug.d ( TAG, "btn_backward.OnClick" );
                    if ( mPlayer == null )
                    { return; }
                    int curPos = mPlayer.getCurrentPosition();
                    curPos -= SEEK_STEP;
                    if ( curPos < 0 ) {
                        curPos = 0;
                    }
                    mPlayer.seekTo ( curPos );
                    mProgress.setProgress ( curPos );
                    mCurTime.setText ( timeFormatToString ( curPos ) );
                }
            } );
            btn_forward.setOnClickListener ( new OnClickListener() {
                public void onClick ( View v ) {
                    Debug.d ( TAG, "btn_forward.OnClick" );
                    if ( mPlayer == null )
                    { return; }
                    int curPos = mPlayer.getCurrentPosition();
                    int duration = mPlayer.getDuration();
                    curPos += SEEK_STEP;
                    if ( curPos > duration ) {
                        curPos = duration;
                    }
                    mPlayer.seekTo ( curPos );
                    mProgress.setProgress ( curPos );
                    mCurTime.setText ( timeFormatToString ( curPos ) );
                }
            } );
            btn_prev.setOnClickListener ( new OnClickListener() {
                public void onClick ( View v ) {
                    Debug.d ( TAG, "btn_prev.OnClick" );
                    if ( mode == MODE_RANDOM ) {
                        random_play();
                    } else {
                        prev();
                    }
                }
            } );
            btn_next.setOnClickListener ( new OnClickListener() {
                public void onClick ( View v ) {
                    Debug.d ( TAG, "btn_next.OnClick" );
                    if ( mode == MODE_RANDOM ) {
                        random_play();
                    } else {
                        next();
                    }
                }
            } );
            /*
             * btn_stop.setOnClickListener(new OnClickListener() { public void
             * onClick(View v) { Debug.d(TAG, "btn_stop.OnClick"); stopPlayback(); }
             * });
             */
            btn_back.setOnClickListener ( new OnClickListener() {
                public void onClick ( View v ) {
                    Debug.d ( TAG, "btn_back.OnClick" );
                    sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_STOPPED );
                    ret2list();
                }
            } );
            btn_volume.setOnClickListener ( new OnClickListener() {
                public void onClick ( View v ) {
                    Debug.d ( TAG, "btn_volume.OnClick:" + volume_level );
                    showDialog ( DIALOG_VOLUME_ID );
                }
            } );
            if ( mode == MODE_ALL_LOOP ) {
                btn_mode.setImageResource ( R.drawable.order_play );
            } else if ( mode == MODE_SINGLE_LOOP ) {
                btn_mode.setImageResource ( R.drawable.single_play );
            } else {
                btn_mode.setImageResource ( R.drawable.random_play );
            }
            //mode_dialog = buildDialog(this);
            btn_mode.setOnClickListener ( new OnClickListener() {
                public void onClick ( View v ) {
                    //mode_dialog.show();
                    album_list_box();
                    //showDialog(DIALOG_MODE_ID);
                }
            } );
            initDisplayView();
            mUPNPReceiver = new UPNPReceiver();
        }

        void initDisplayView() {
            if ( !isBrowserMode ) {
                btn_prev.setVisibility ( View.GONE );
                btn_next.setVisibility ( View.GONE );
                btn_mode.setVisibility ( View.GONE );
            }
            // mMediaThumb.setImageURI(cur_uri);
            mProgress.setProgress ( 0 );
            mCurTime.setText ( TIME_START );
            mTotalTime.setText ( TIME_START );
        }

        private void showLoading() {
            if ( progressDialog == null && isShowingForehand ) {
                playDisable();
                progressDialog = new LoadingDialog ( this,
                                                     LoadingDialog.TYPE_LOADING, this.getResources().getString (
                                                             R.string.str_loading ) );
                progressDialog.show();
            }
        }

        private void hideLoading() {
            if ( progressDialog != null ) {
                playEnable();
                progressDialog.stopAnim();
                progressDialog.dismiss();
                progressDialog = null;
            }
        }

        @Override
        protected void onPause() {
            super.onPause();
            hideLoading();
            if ( exitDlg != null ) {
                exitDlg.dismiss();
                exitDlg = null;
            }
            isShowingForehand = false;
            /*if(mVisualizerView!=null&& mPlayer!=null){
                mVisualizerView.release();
             }*/
            if ( mPlayer != null ) {
                mPlayer.pause();
            }
            /*resume mute status when pause*/
            if (setMute) {
                mAudioManager.setStreamMute ( AudioManager.STREAM_MUSIC, false );
            }
            unregisterReceiver ( mUPNPReceiver );
            mWakeLock.release();

            // sendPlayStateChangeBroadcast(MediaRendererDevice.PLAY_STATE_PAUSED);
        }

        @Override
        public void onStop() {
            super.onStop();
            if ( mPlayer != null ) {
                mPlayer.pause();
                stopPlayback();
            }
            hideLoading();
            Debug.d ( TAG, "##########onStop####################" );
        }

        public boolean onKeyDown ( int keyCode, KeyEvent event ) {
            Debug.d ( TAG, "******keycode=" + keyCode );
            if ( keyCode == 89 ) { // rewind
                int progress = mProgress.getProgress() - 10000;
                if ( getCurrentFocus() == mProgress && ( progress > 0 ) ) {
                    mProgress.setProgress ( progress );
                    mPlayer.seekTo ( progress );
                }
                return true;
            }
            if ( keyCode == KeyEvent.KEYCODE_BACK ) {
                //stopPlayback();
                hideLoading();
                ret2list();
                return true;
            }
            if ( keyCode == 90 ) { // fast_forward
                int progress = mProgress.getProgress() + 10000;
                if ( getCurrentFocus() == mProgress
                        && ( progress < mProgress.getMax() ) ) {
                    mProgress.setProgress ( progress );
                    mPlayer.seekTo ( progress );
                }
                return true;
            }
            /*if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
                if (getCurrentFocus() == btn_next || getCurrentFocus() == btn_prev
                        || getCurrentFocus() == btn_play
                        || getCurrentFocus() == btn_mode
                        || getCurrentFocus() == btn_volume) {
                    // sendPlayStateChangeBroadcast(MediaRendererDevice.PLAY_STATE_STOPPED);
                   MusicPlayer.this.finish();
                }
                return true;
            }*/
            return super.onKeyDown ( keyCode, event );
        }

        @Override
        public Object onRetainNonConfigurationInstance() {
            PreviewPlayer player = mPlayer;
            mPlayer = null;
            return player;
        }


        /*@Override
        public void onUserLeaveHint() {
            stopPlayback();
            finish();
            super.onUserLeaveHint();
        }*/

        public void onPrepared ( MediaPlayer mp ) {
            Debug.d ( TAG, "##########onPrepared####################" );
            if ( isFinishing() )
            { return; }
            mPlayer = ( PreviewPlayer ) mp;
            mp = null;
            hideLoading();
            play();
            mTotalDuration = mPlayer.getDuration();
            if ( mTotalDuration > 0 ) {
                mProgress.setMax ( mTotalDuration );
                mProgress.setVisibility ( View.VISIBLE );
                mTotalTime.setText ( timeFormatToString ( mTotalDuration ) );
            }
            mPlayer.setOnInfoListener ( this );
        }

        private OnAudioFocusChangeListener mAudioFocusListener = new OnAudioFocusChangeListener() {
            public void onAudioFocusChange (
            int focusChange ) {
                Debug.d ( TAG,
                          "##########onAudioFocusChange####################" );
                if ( mPlayer == null ) {
                    mAudioManager
                    .abandonAudioFocus ( this );
                    return;
                }
                switch ( focusChange ) {
                    case AudioManager.AUDIOFOCUS_LOSS:
                        mPausedByTransientLossOfFocus = false;
                        pause();
                        break;
                    case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                    case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                        if ( mPlayer.isPlaying() ) {
                            mPausedByTransientLossOfFocus = true;
                            pause();
                        }
                        break;
                    case AudioManager.AUDIOFOCUS_GAIN:
                        if ( mPausedByTransientLossOfFocus ) {
                            mPausedByTransientLossOfFocus = false;
                            play();
                        }
                        break;
                }
                updatePlayPause();
            }
        };

        public boolean onError ( MediaPlayer mp, int what, int extra ) {
            Debug.d ( TAG, "##########onError####################" );
            Toast.makeText ( getApplicationContext(), R.string.error_info, Toast.LENGTH_SHORT ).show();
            next();
            return true;
        }

        public void onCompletion ( MediaPlayer mp ) {
            if (play_state != STATE_STOP) {
                Debug.d ( TAG, "##########onCompletion####################" +mNextURI);
                Intent intent = new Intent (AmlogicCP.PLAY_POSTION_REFRESH );
                intent.putExtra ( "curPosition", mTotalDuration );
                intent.putExtra ( "totalDuration", mTotalDuration );
                sendBroadcast ( intent );
                mProgress.setProgress ( mTotalDuration );
                mProgressRefresher.removeCallbacksAndMessages ( null );
                play_state = STATE_STOP;
                updatePlayPause();
                if (mNextURI != null&&!mNextURI.isEmpty()) {
                    stopExit();
                    handlerUI.removeMessages ( SHOW_STOP );
                    next();
                    return;
                }
                if ( isBrowserMode ) {
                    stopExit();
                    handlerUI.removeMessages ( SHOW_STOP );
                    if ( mode == MODE_ALL_LOOP ) {
                        next();
                    } else if ( mode == MODE_SINGLE_LOOP ) {
                        change_music();
                    } else {
                        random_play();
                    }
                } else {
                    handlerUI.sendEmptyMessageDelayed ( STOP_DEVICE, STOP_PLAY_BROADCAST );
                    handlerUI.removeMessages ( SHOW_STOP );
                    handlerUI.sendEmptyMessageDelayed ( SHOW_STOP, DIALOG_SHOW_DELAY );
                }
            }
        }
        private void stopExit() {
            handlerUI.removeMessages ( SHOW_STOP );
            if ( exitDlg != null ) {
                exitDlg.dismiss();
                exitDlg = null;
            }
        }
        public void exitNow() {
            Debug.d ( TAG, "ExitNow..." + isShowingForehand );
            if ( !isShowingForehand ) {
                MusicPlayer.this.finish();
                return;
            }
            hideLoading();
            if ( exitDlg == null ) {
                exitDlg = new LoadingDialog ( this, LoadingDialog.TYPE_ERROR, MusicPlayer.this.getResources().getString ( R.string.error_exit ) );
                exitDlg.setCancelable ( true );
                exitDlg.setOnDismissListener ( new OnDismissListener() {
                    @Override
                    public void onDismiss ( DialogInterface arg0 ) {
                        if ( exitDlg != null && ( MusicPlayer.this.getClass().getName().equals ( exitDlg.getTopActivity ( MusicPlayer.this ) ) ||
                        exitDlg.getCountNum() == 0 ) ) {
                            //mAudioManager.abandonAudioFocus(mAudioFocusListener);
                            MusicPlayer.this.finish();
                        }
                    }
                } );
                exitDlg.show();
            } else {
                exitDlg.setCountNum ( 2 );
                MusicPlayer.this.finish();
            }
        }
        /**
         * @Description TODO
         */
        public void wait2Exit() {
            Debug.d ( TAG, "wait2Exit......" );
            if ( !isShowingForehand ) {
                MusicPlayer.this.finish();
                return;
            }
            hideLoading();
            if ( exitDlg == null ) {
                exitDlg = new LoadingDialog ( this, LoadingDialog.TYPE_EXIT_TIMER, "" );
                exitDlg.setCancelable ( true );
                exitDlg.setOnDismissListener ( new OnDismissListener() {
                    @Override
                    public void onDismiss ( DialogInterface arg0 ) {
                        if ( exitDlg != null && ( MusicPlayer.this.getClass().getName().equals ( exitDlg.getTopActivity ( MusicPlayer.this ) ) ||
                        exitDlg.getCountNum() == 0 ) ) {
                            //mAudioManager.abandonAudioFocus(mAudioFocusListener);
                            MusicPlayer.this.finish();
                        }
                    }
                } );
                exitDlg.show();
            } else {
                exitDlg.setCountNum ( 4 );
                exitDlg.show();
            }
        }
        private String timeFormatToString ( int relTime ) {
            int time;
            StringBuffer timeBuf = new StringBuffer();
            relTime = ( int ) relTime / 1000;
            time = relTime / 3600;
            if ( time >= 10 ) {
                timeBuf.append ( time );
            } else {
                timeBuf.append ( "0" ).append ( time );
            }
            relTime = relTime % 3600;
            time = relTime / 60;
            if ( time >= 10 ) {
                timeBuf.append ( ":" ).append ( time );
            } else {
                timeBuf.append ( ":0" ).append ( time );
            }
            time = relTime % 60;
            if ( time >= 10 ) {
                timeBuf.append ( ":" ).append ( time );
            } else {
                timeBuf.append ( ":0" ).append ( time );
            }
            return timeBuf.toString();
        }

        class ProgressRefresher implements Runnable {
                public void run() {
                    if ( mPlayer == null ) {
                        mProgressRefresher.removeCallbacksAndMessages ( null );
                        return;
                    } else if ( mTotalDuration > 0 ) {
                        int curPos = mPlayer.getCurrentPosition();
                        mProgress.setProgress ( curPos );
                        mCurTime.setText ( timeFormatToString ( curPos ) );
                        Intent intent = new Intent ( AmlogicCP.PLAY_POSTION_REFRESH );
                        intent.putExtra ( "curPosition", curPos );
                        intent.putExtra ( "totalDuration", mTotalDuration );
                        sendBroadcast ( intent );
                        if ( DEBUG_PLAYER )
                        { Debug.d ( TAG, "######sendBroadcast(progress)######" + curPos + "/" + mTotalDuration ); }
                    } else {
                        mTotalDuration = mPlayer.getDuration();
                        if ( mTotalDuration > 0 ) {
                            mProgress.setMax ( mTotalDuration );
                            mProgress.setVisibility ( View.VISIBLE );
                            mTotalTime.setText ( timeFormatToString ( mTotalDuration ) );
                        }
                        if ( DEBUG_PLAYER )
                            Debug.d ( TAG, "****ProgressRefresher: mTotalDuration="
                                      + mTotalDuration + ",   player=" + mPlayer );
                    }
                    Debug.d ( TAG, "--mPlayer.getDuration()" + mPlayer.getDuration() + "/" + mPlayer.getCurrentPosition() );
                    /*if (timeFormatToString(mPlayer.getDuration()).equals(timeFormatToString(mPlayer.getCurrentPosition()))){
                        mVisualizerView.release();
                        Debug.d(TAG, "mPlayer.getDuration()"+mPlayer.getDuration() + "/"+ mPlayer.getCurrentPosition());
                    }*/
                    if ( mVolChanged ) {
                        Intent intent = new Intent();
                        intent.setAction ( MediaRendererDevice.PLAY_STATE_SETVOLUME );
                        intent.putExtra ( "VOLUME", volume_level );
                        sendBroadcast ( intent );
                        mVolChanged = false;
                    }
                    mProgressRefresher.removeCallbacksAndMessages ( null );
                    mProgressRefresher.postDelayed ( new ProgressRefresher(), PROGRESS_TIME_DELAY );
                }
        }


        private void start() {
            handlerUI.sendEmptyMessage ( SHOW_LOADING );
            handlerUI.removeMessages ( STOP_BY_SEVER );
            handlerUI.sendEmptyMessageDelayed ( STOP_BY_SEVER, DIALOG_SHOW_DELAY );
            // mPath.setText(file_name==null?cur_uri:file_name);
            PreviewPlayer player = ( PreviewPlayer ) getLastNonConfigurationInstance();
            if ( player == null ) {
                if ( DEBUG_PLAYER )
                { Debug.d ( TAG, "*********************not get player" +cur_uri); }
                mPlayer = null;
                mPlayer = new PreviewPlayer();
                mPlayer.setActivity ( this );
                try {
                    mPlayer.setDataSourceAndPrepare ( Uri.parse ( cur_uri ) );
                } catch ( Exception ex ) {
                    // catch generic Exception, since we may be called with a media
                    // content URI, another content provider's URI, a file URI,
                    // an http URI, and there are different exceptions associated
                    // with failure to open each of those.
                    ex.printStackTrace();
                    //finish();
                    exitNow();
                    return;
                }
            } else {
                Debug.d ( TAG, "*********************get player" );
                mPlayer = player;
                try {
                    //player.release();
                    player = null;
                } catch ( Exception ex ) {
                } finally {
                    mPlayer.setActivity ( this );

                    if ( mPlayer.isPrepared() ) {
                        play();
                        mTotalDuration = mPlayer.getDuration();
                        if ( mTotalDuration > 0 ) {
                            mProgress.setMax ( mTotalDuration );
                            mProgress.setVisibility ( View.VISIBLE );
                            mTotalTime.setText ( timeFormatToString ( mTotalDuration ) );
                        }
                    }
                }
            }
        }

        private void play() {
            mAudioManager.requestAudioFocus ( mAudioFocusListener,
                                              AudioManager.STREAM_MUSIC,
                                              AudioManager.AUDIOFOCUS_GAIN_TRANSIENT );
            mPlayer.start();
            play_state = STATE_PLAY;
            Intent intent = new Intent( MediaRendererDevice.PLAY_STATE_PLAYING );
            intent.putExtra("currentURI",cur_uri);
            intent.putExtra("currentMeta",mCurrentMeta);
            sendBroadcast ( intent );
            mProgressRefresher.postDelayed ( new ProgressRefresher(), PROGRESS_TIME_DELAY );
            readyForFinish = false;
            updatePlayPause();
        }

        private void pause() {
            mPlayer.pause();
            play_state = STATE_PAUSE;
            sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_PAUSED );
            mProgressRefresher.removeCallbacksAndMessages ( null );
            readyForFinish = false;
            updatePlayPause();
        }

        private void next() {
            if ( isBrowserMode ) {
                if ( mCurIndex < ( DeviceFileBrowser.playList.size() - 1 ) ) {
                    mCurIndex++;
                } else {
                    mCurIndex = 0;
                }
                change_music();
                if ((cur_uri == null) || (cur_uri.isEmpty())) {
                    next();
                }
            }else if(mNextURI != null && !mNextURI.isEmpty()) {
                cur_uri = mNextURI;
                if (mNextFileName == null || mNextFileName.isEmpty()) {
                   file_name = cur_uri.substring ( cur_uri.lastIndexOf ( '/' ) + 1, cur_uri.length() );
                   mNextFileName = file_name;
                }
                if (mFileName != null) {
                    mFileName.setText(file_name);
                }
               mNextURI = null;
               start();
            }
        }

        private void prev() {
            if ( isBrowserMode ) {
                if ( mCurIndex > 0 ) {
                    mCurIndex--;
                } else {
                    mCurIndex = DeviceFileBrowser.playList.size() - 1;;
                }
                change_music();
            }
        }

        private void random_play() {
            Random random = new Random();
            mCurIndex = Math.abs ( random.nextInt() ) % ( DeviceFileBrowser.playList.size() - 1 );
            change_music();
        }

        private void change_music() {
            Map<String, Object> item = ( Map<String, Object> ) DeviceFileBrowser.playList.get ( mCurIndex );
            cur_uri = ( String ) item.get ( "item_uri" );
            if ((cur_uri == null) || (cur_uri.isEmpty())) {
                return;
            }
            file_name = ( String ) item.get ( "item_name" );
            //mFileName.setText(file_name +"                                 " + file_name +"                                 " + file_name +"                 ");
            mFileName.setText ( file_name );
            handlerUI.sendEmptyMessageDelayed ( STOP_AND_START, PROGRESS_TIME_DELAY );
            handlerUI.sendEmptyMessage ( SHOW_LOADING );
            //showLoading();
        }

        private void stopPlayback() {
            if ( mProgressRefresher != null ) {
                mProgressRefresher.removeCallbacksAndMessages ( null );
            }
            readyForFinish = true;
            //sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_STOPPED );
            updatePlayPause();
            mProgressRefresher.removeCallbacksAndMessages ( null );
            mCurTime.setText ( TIME_START );
            mProgress.setProgress ( 0 );
            if ( mPlayer != null ) {
                //mPlayer.pause();
                try {
                    //mPlayer.setDataSource(this,null);
                    play_state = STATE_STOP;
                    mPlayer.release();
                } catch ( Exception ex ) {}
                finally {
                    mPlayer = null;
                }
                mAudioManager.abandonAudioFocus ( mAudioFocusListener );
            }
        }

        private void updatePlayPause() {
            if ( btn_play != null ) {
                if ( ( mPlayer != null ) && mPlayer.isPlaying() ) {
                    btn_play.setImageResource ( R.drawable.suspend_play );
                } else {
                    btn_play.setImageResource ( R.drawable.play_play );
                }
            }
        }

        /** Dialog */
        @Override
        protected Dialog onCreateDialog ( int id ) {
            LayoutInflater inflater = ( LayoutInflater ) MusicPlayer.this
                                      .getSystemService ( LAYOUT_INFLATER_SERVICE );
            switch ( id ) {
                case DIALOG_VOLUME_ID:
                    View layout_volume = inflater.inflate ( R.layout.volume_dialog,
                                                            ( ViewGroup ) findViewById ( R.id.layout_root_volume ) );
                    dialog_volume = new VolumeDialog ( this );
                    dialog_volume.setOnShowListener( new DialogInterface.OnShowListener( ) {
                        public void onShow( DialogInterface dialog ) {
                            handlerUI.sendEmptyMessageDelayed(VOLUME_HIDE, DIALOG_SHOW_DELAY);
                        }
                    });
                    return dialog_volume;
                    /*case DIALOG_MODE_ID:
                        mode_dialog = new ModeDialog(this);
                        return mode_dialog; */
            }
            return null;
        }

        @Override
        protected void onPrepareDialog ( int id, Dialog dialog ) {
            WindowManager wm = getWindowManager();
            Display display = wm.getDefaultDisplay();
            LayoutParams lp = dialog.getWindow().getAttributes();
            switch ( id ) {
                case DIALOG_VOLUME_ID: {
                    if ( display.getHeight() > display.getWidth() ) {
                        lp.width = ( int ) ( display.getWidth() * 1.0 );
                    } else {
                        lp.width = ( int ) ( display.getWidth() * 0.5 );
                    }
                    dialog.getWindow().setAttributes ( lp );
                    vol_bar = ( ProgressBar ) dialog_volume.getWindow().findViewById (
                                  android.R.id.progress );
                    int mmax = mAudioManager
                               .getStreamMaxVolume ( AudioManager.STREAM_MUSIC );
                    int current = mAudioManager
                                  .getStreamVolume ( AudioManager.STREAM_MUSIC );
                    volume_level = current * 100 / mmax;
                    if ( vol_bar instanceof SeekBar ) {
                        SeekBar seeker = ( SeekBar ) vol_bar;
                        seeker.setOnSeekBarChangeListener ( new OnSeekBarChangeListener() {
                            private long mLastTime = 0;
                            public void onStartTrackingTouch ( SeekBar bar ) {
                                Debug.d ( TAG, "vol_bar:onStartTrackingTouch" );
                                mLastTime = 0;
                                mVolTouch = true;
                            }
                            public void onProgressChanged ( SeekBar bar,
                            int progress, boolean fromuser ) {
                                Debug.d ( TAG, "vol_bar:onProgressChanged=" + progress );
                                if ( !fromuser )
                                { return; }
                                long now = SystemClock.elapsedRealtime();
                                volume_level = progress;
                                if ( ( now - mLastTime ) > 250 ) {
                                    mLastTime = now;
                                    // trackball event, allow progress updates
                                    if ( mVolTouch ) {
                                        Debug.d ( TAG, "***progress=" + progress );
                                        vol_bar.setProgress ( volume_level );
                                        int max = mAudioManager
                                                  .getStreamMaxVolume ( AudioManager.STREAM_MUSIC );
                                        mAudioManager.setStreamVolume (
                                            AudioManager.STREAM_MUSIC,
                                            volume_level * max / 100, 0 );
                                        Intent intent = new Intent();
                                        intent.setAction ( MediaRendererDevice.PLAY_STATE_SETVOLUME );
                                        intent.putExtra ( "VOLUME", volume_level );
                                        sendBroadcast ( intent );
                                        handlerUI.removeMessages(VOLUME_HIDE);
                                        handlerUI.sendEmptyMessageDelayed(VOLUME_HIDE, DIALOG_SHOW_DELAY);
                                    }
                                }
                            }
                            public void onStopTrackingTouch ( SeekBar bar ) {
                                Debug.d ( TAG, "vol_bar:onStopTrackingTouch: "
                                          + volume_level );
                                mVolTouch = false;
                                vol_bar.setProgress ( volume_level );
                                int max = mAudioManager
                                          .getStreamMaxVolume ( AudioManager.STREAM_MUSIC );
                                mAudioManager.setStreamVolume (
                                    AudioManager.STREAM_MUSIC,
                                    volume_level * max / 100, 0 );
                                Intent intent = new Intent();
                                intent.setAction ( MediaRendererDevice.PLAY_STATE_SETVOLUME );
                                intent.putExtra ( "VOLUME", volume_level );
                                sendBroadcast ( intent );
                            }
                        } );
                    }
                    vol_bar.setMax ( 100 );
                    vol_bar.setProgress ( volume_level );
                    break;
                }
            }
        }

        /*
         * Wrapper class to help with handing off the MediaPlayer to the next
         * instance of the activity in case of orientation change, without losing
         * any state.
         */
        private static class PreviewPlayer extends MediaPlayer implements
                OnPreparedListener , OnErrorListener {
                MusicPlayer mActivity;
                boolean     mIsPrepared = false;

                public void setActivity ( MusicPlayer activity ) {
                    mActivity = activity;
                    setOnPreparedListener ( this );
                    setOnErrorListener ( this );
                    setOnCompletionListener ( mActivity );
                }

                public void setDataSourceAndPrepare ( Uri uri )
                throws IllegalArgumentException, SecurityException,
                IllegalStateException, IOException {
                    setDataSource ( mActivity, uri );
                    prepareAsync();
                }

                @Override
                public boolean onError ( MediaPlayer mp, int what, int extra ) {
                    if ( mIsPrepared ) {
                        mActivity.onError ( mp,  what,  extra );
                    }
                    return true;
                }

                /*
                 * (non-Javadoc)
                 *
                 * @see
                 * android.media.MediaPlayer.OnPreparedListener#onPrepared(android.media
                 * .MediaPlayer)
                 */
                @Override
                public void onPrepared ( MediaPlayer mp ) {
                    mIsPrepared = true;
                    mActivity.onPrepared ( mp );
                }

                boolean isPrepared() {
                    return mIsPrepared;
                }
        }

        public static int parseTimeStringToMSecs ( String time ) {
            String[] str = time.split ( ":|\\." );
            if ( str.length < 3 )
            { return 0; }
            int hour = Integer.parseInt ( str[0] );
            int min = Integer.parseInt ( str[1] );
            int sec = Integer.parseInt ( str[2] );
            // Debug.d(TAG, "***********parseTimeStringToInt: "+hour+":"+min+":"+sec);
            return ( hour * 3600 + min * 60 + sec ) * 1000;
        }

        class UPNPReceiver extends BroadcastReceiver {
                public void onReceive ( Context context, Intent intent ) {
                    String action = intent.getAction();
                    if ( action == null )
                    { return; }
                    Debug.d ( TAG, "#####MusicPlayer.UPNPReceiver: " + action );
                    String mediaType = intent
                                       .getStringExtra ( AmlogicCP.EXTRA_MEDIA_TYPE );
                    if ( !MediaRendererDevice.TYPE_AUDIO.equals ( mediaType ) )
                    { return; }
                    Debug.d ( TAG, "#####MusicPlayer.UPNPReceiver: " + action
                              + ",  mediaType=" + mediaType );
                    readyForFinish = false;
                    if ( action.equals ( AmlogicCP.UPNP_PLAY_ACTION ) ) {
                        handlerUI.removeMessages(STOP_DEVICE);
                        stopExit();
                        String uri = intent.getStringExtra ( AmlogicCP.EXTRA_MEDIA_URI );
                        mNextURI= intent.getStringExtra(MediaRendererDevice.EXTRA_NEXT_URI);
                        mCurrentMeta = intent.getStringExtra(MediaRendererDevice.EXTRA_META_DATA);
                        Debug.d(TAG,"cur_uri:"+cur_uri+"uri"+uri+"=?"+cur_uri.equals ( uri )+"mNextURI:"+mNextURI);
                        if ( !cur_uri.equals ( uri ) ) {
                            if ( play_state != STATE_STOP ) {
                                stopPlayback();
                            }
                            cur_uri = uri;
                            file_name = intent
                                        .getStringExtra ( AmlogicCP.EXTRA_FILE_NAME );
                            if ( file_name == null ) {
                                file_name = cur_uri.substring ( cur_uri.lastIndexOf ( '/' ) + 1, cur_uri.length() );
                            }
                            if ( mFileName != null ) {
                                /*mFileName.setText(file_name
                                        + "                                 "
                                        + file_name
                                        + "                                 "
                                        + file_name + "                 ");*/
                                mFileName.setText ( file_name );
                            }
                            if ( DeviceFileBrowser.TYPE_DMP.equals ( intent.getStringExtra ( DeviceFileBrowser.DEV_TYPE ) ) ) {
                                isBrowserMode = true;
                                mCurIndex = intent.getIntExtra ( DeviceFileBrowser.CURENT_POS, 0 );
                            } else {
                                isBrowserMode = false;
                            }
                            if ( !isBrowserMode ) {
                                btn_prev.setVisibility ( View.GONE );
                                btn_next.setVisibility ( View.GONE );
                                btn_mode.setVisibility ( View.GONE );
                            } else {
                                btn_prev.setVisibility ( View.VISIBLE );
                                btn_next.setVisibility ( View.VISIBLE );
                                btn_mode.setVisibility ( View.VISIBLE );
                            }
                            start();
                        }  else if ( mPlayer == null || play_state == STATE_STOP ) {
                            start();
                        }else {
                            play();
                        }
                    } else if ( action.equals ( AmlogicCP.UPNP_PAUSE_ACTION ) ) {
                        pause();
                    } else if ( action.equals ( AmlogicCP.UPNP_STOP_ACTION ) ) {
                        stopPlayback();
                        stopExit();
                        handlerUI.sendEmptyMessageDelayed ( SHOW_STOP, DIALOG_SHOW_DELAY );
                    } else if ( action.equals ( MediaRendererDevice.PLAY_STATE_SEEK ) ) {
                        if ( mPlayer != null ) {
                            String time = intent.getStringExtra ( "REL_TIME" );
                            int msecs = parseTimeStringToMSecs ( time );
                            if ( msecs < 0 )
                            { msecs = 0; }
                            else if ( ( msecs > mTotalDuration ) && ( mTotalDuration > 0 ) )
                            { msecs = mTotalDuration; }
                            Debug.d ( TAG, "*********seek to: " + time + ",   msecs="
                                      + msecs );
                            mPlayer.seekTo ( msecs );
                            if ( mTotalDuration > 0 )
                            { mProgress.setProgress ( msecs ); }
                            mCurTime.setText ( timeFormatToString ( msecs ) );
                        }
                    } else if ( action.equals ( AmlogicCP.UPNP_SETVOLUME_ACTION ) ) {
                        volume_level = intent.getIntExtra ( "DesiredVolume", 50 );
                        int max = mAudioManager
                                  .getStreamMaxVolume ( AudioManager.STREAM_MUSIC );
                        mAudioManager.setStreamVolume ( AudioManager.STREAM_MUSIC,
                                                        volume_level * max / 100, AudioManager.FLAG_SHOW_UI );
                        mVolChanged = true;
                        if ( vol_bar != null )
                        { vol_bar.setProgress ( volume_level ); }
                    } else if ( action.equals ( AmlogicCP.UPNP_SETMUTE_ACTION ) ) {
                        Debug.d ( TAG, "*******setMuteAction" );
                        Boolean mute = ( Boolean ) intent.getBooleanExtra ( "DesiredMute", false );
                        Debug.d ( "mute", "*******setMuteAction=" + mute );
                        // mAudioManager.setMasterMute(mute);
                        if ( mute ) {
                            setMute = true;
                        }else{
                            setMute = false;
                        }
                        mAudioManager.setStreamMute ( AudioManager.STREAM_MUSIC, mute );
                    }
                }
        }

        private void sendPlayStateChangeBroadcast ( String stat ) {
            Intent intent = new Intent();
            intent.setAction ( stat );
            sendBroadcast ( intent );
            Debug.d ( TAG, "########music: sendPlayStateChangeBroadcast:  " + stat );
            return;
        }

        private void waitForExit ( long delay ) {
            Debug.d ( TAG, "********waitForExit" );
            handlerUI.removeMessages ( 1 );
            readyForFinish = true;
            handlerUI.sendEmptyMessageDelayed ( 1, delay );
        }

        private Handler handlerUI = new Handler() {
            @Override
            public void handleMessage ( Message msg ) {
                switch ( msg.what ) {
                    case 1:
                        if ( readyForFinish )
                        { finish(); }
                        return;
                    case SHOW_START:
                        start();
                        return;
                    case SHOW_STOP:
                        wait2Exit();
                        break;
                    case STOP_AND_START:
                        stopAndStart();
                        break;
                    case SHOW_LOADING:
                        showLoading();
                        break;
                    case HIDE_LOADING:
                        hideLoading();
                        break;
                    case STOP_BY_SEVER:
                        if ( !isShowingForehand ) {
                           ret2list();
                        } else {
                            handlerUI.sendEmptyMessageDelayed ( STOP_BY_SEVER, DIALOG_SHOW_DELAY );
                        }
                        break;
                    case VOLUME_HIDE:
                        dialog_volume.dismiss();
                        break;
                    case STOP_DEVICE:
                        sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_STOPPED );
                        break;
                }
            }
        };
        private void ret2list() {
            if (isBrowserMode) {
                Intent intent = new Intent(MusicPlayer.this,DeviceFileBrowser.class);
                this.setResult ( mCurIndex, intent );
            }
            MusicPlayer.this.finish();
        }
        /*private class MyAdapter extends BaseAdapter {
            private Context                  context;
            private String[]                 music_mode;
            private LayoutInflater           inflater;

            public List<Map<Integer,Boolean>> musicList;
            public MyAdapter(Context context) {
                super();
                this.context = context;
                musicList = new ArrayList<Map<Integer,Boolean>>();
                music_mode = context.getResources().getStringArray(
                        R.array.music_mode);
                inflater = LayoutInflater.from(context);
                for (int i = 0; i < music_mode.length; i++) {
                    HashMap<Integer, Boolean> map = new HashMap<Integer, Boolean>();
                    map.put(i, false);
                    musicList.add(map);
                }
            }
            public List<Map<Integer,Boolean>> getData(){
                return musicList;
            }
            public int getCount() {
                // TODO Auto-generated method stub
                return music_mode.length;
            }

            public Object getItem(int position) {
                // TODO Auto-generated method stub
                return musicList.get(position);
            }

            public long getItemId(int position) {
                // TODO Auto-generated method stub
                return position;
            }

            public View getView(int position, View view, ViewGroup parent) {
                // TODO Auto-generated method stub
                view = inflater.inflate(R.layout.list_item_spinner, null);
                TextView title = (TextView) view.findViewById(R.id.ItemText);
                ImageView image = (ImageView) view.findViewById(R.id.ItemImage);
                title.setText(music_mode[position]);
                image.setImageDrawable(getResources().getDrawable(R.drawable.tick));
                HashMap<Integer, Boolean> map = (HashMap<Integer, Boolean>) musicList.get(position);
                if (map.get(position) == true) {
                    image.setVisibility(View.VISIBLE);
                } else {
                    image.setVisibility(View.GONE);
                }
                if (mode == position) {
                    image.setVisibility(View.VISIBLE);
                }
                return view;
            }
        }*/

        /*private Dialog buildDialog(Context context) {
            mode_dialog = new Dialog(this, R.style.theme_dialog);
            mode_dialog.setContentView(R.layout.music_mode);
            LayoutParams params = mode_dialog.getWindow().getAttributes();
            params.x = 120;
            params.y = -120;
            mode_dialog.getWindow().setAttributes(params);
            ListView lv = (ListView) mode_dialog.findViewById(R.id.mList);
            final MyAdapter mAdapter = new MyAdapter(this);
            modeData = mAdapter.getData();
            lv.setAdapter(mAdapter);

            lv.setOnItemClickListener(new OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> arg0, View arg1,
                        int position, long arg3) {
                    Log.d(TAG,"onItemClick");
                    //mode_dialog.dismiss();
                    HashMap<Integer,Boolean> map = (HashMap<Integer, Boolean>) modeData.get(position);
                    map.put(mode, false);
                    map.put(position, true);
                    mode = position;
                    mAdapter.notifyDataSetChanged();
                    if (mode == MODE_ALL_LOOP) {
                        btn_mode.setImageResource(R.drawable.order_play);
                    } else if (mode == MODE_SINGLE_LOOP) {
                        btn_mode.setImageResource(R.drawable.single_play);
                    } else {
                        btn_mode.setImageResource(R.drawable.random_play);
                    }
                }
            });
            return mode_dialog;
        }*/

        private void stopAndStart() {
            if ( mPlayer != null ) {
                mPlayer.pause();
                stopPlayback();
            }
            start();
        }

        private void album_list_box() {
            Dialog dialog = new Dialog ( this, R.style.theme_dialog );
            AlertDialog.Builder builder = new AlertDialog.Builder ( this );
            builder.setTitle ( R.string.change_mode );
            final String[] music_mode = getResources().getStringArray (
                                            R.array.music_mode );
            ListView modeList = new ListView ( this );
            final List<Map<String, Object>> listItem = new ArrayList<Map<String, Object>>();
            for ( int i = 0; i < music_mode.length; i++ ) {
                HashMap<String, Object> map = new HashMap<String, Object>();
                if ( i == 0 ) {
                    if ( mode == MODE_ALL_LOOP ) {
                        map.put ( "selected", R.drawable.tick );
                    }
                    map.put ( "icons", R.drawable.order_play );
                } else if ( i == 1 ) {
                    if ( mode == MODE_SINGLE_LOOP ) {
                        map.put ( "selected", R.drawable.tick );
                    }
                    map.put ( "icons", R.drawable.single_play );
                } else {
                    if ( mode == MODE_RANDOM ) {
                        map.put ( "selected", R.drawable.tick );
                    }
                    map.put ( "icons", R.drawable.random_play );
                }
                map.put ( "mode", music_mode[i] );
                listItem.add ( map );
            }
            final SimpleAdapter listItemAdapter = new SimpleAdapter ( this, listItem, R.layout.musicmode, new String[] {"icons", "mode", "selected"}, new int[] {R.id.icons, R.id.title, R.id.selected} );
            modeList.setAdapter ( listItemAdapter );
            modeList.setOnItemClickListener ( new OnItemClickListener() {
                @Override
                public void onItemClick ( AdapterView<?> arg0, View arg1,
                int position, long arg3 ) {
                    for ( int i = 0; i < music_mode.length; i++ ) {
                        HashMap<String, Object> map = ( HashMap<String, Object> ) listItem.get ( i );
                        if ( i == position ) {
                            map.put ( "selected", R.drawable.tick );
                        } else {
                            map.put ( "selected", null );
                        }
                    }
                    listItemAdapter.notifyDataSetChanged();
                    mode = position;
                    if ( mode == MODE_ALL_LOOP ) {
                        btn_mode.setImageResource ( R.drawable.order_play );
                    } else if ( mode == MODE_SINGLE_LOOP ) {
                        btn_mode.setImageResource ( R.drawable.single_play );
                    } else {
                        btn_mode.setImageResource ( R.drawable.random_play );
                    }
                }
            } );
            builder.setView ( modeList );
            dialog = builder.create();
            dialog.show();
        }
        private class VolumeDialog extends Dialog {
                VolumeDialog ( Context context ) {
                    super ( context, R.style.theme_dialog );
                    setContentView ( R.layout.volume_dialog );
                    LayoutParams params = getWindow().getAttributes();
                    params.x = 120;
                    params.y = -120;
                    getWindow().setAttributes ( params );
                }

                @Override
                public boolean onKeyDown ( int keyCode, KeyEvent event ) {
                    Debug.d ( TAG, "******keycode=" + keyCode );
                    if ( keyCode == KeyEvent.KEYCODE_BACK ) {
                        hideLoading();
                        this.dismiss();
                        return true;
                    }
                    if ( keyCode == 24 || keyCode == 25
                            || keyCode == KeyEvent.KEYCODE_DPAD_LEFT
                            || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT ) {
                        if ( keyCode == 24 || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT ) {
                            volume_level = vol_bar.getProgress() + 5;
                            if ( volume_level < vol_bar.getMax() ) {
                                vol_bar.setProgress ( volume_level );
                            } else {
                                volume_level = vol_bar.getMax();
                                vol_bar.setProgress ( volume_level );
                            }
                        } else if ( keyCode == 25
                                    || keyCode == KeyEvent.KEYCODE_DPAD_LEFT ) {
                            volume_level = vol_bar.getProgress() - 5;
                            if ( volume_level > 0 ) {
                                vol_bar.setProgress ( volume_level );
                            } else {
                                volume_level = 0;
                                vol_bar.setProgress ( volume_level );
                            }
                        }
                        int max = mAudioManager
                                  .getStreamMaxVolume ( AudioManager.STREAM_MUSIC );
                        mAudioManager.setStreamVolume ( AudioManager.STREAM_MUSIC,
                                                        volume_level * max / 100, 0 );
                        Intent intent = new Intent();
                        intent.setAction ( MediaRendererDevice.PLAY_STATE_SETVOLUME );
                        intent.putExtra ( "VOLUME", volume_level );
                        sendBroadcast ( intent );
                        handlerUI.removeMessages(VOLUME_HIDE);
                        handlerUI.sendEmptyMessageDelayed(VOLUME_HIDE, DIALOG_SHOW_DELAY);
                        return true;
                    }
                    return super.onKeyDown ( keyCode, event );
                }
        }

        @Override
        protected void onResume() {
            super.onResume();
            isShowingForehand = true;
            showLoading();
            IntentFilter filter = new IntentFilter();
            filter.addAction ( AmlogicCP.UPNP_PLAY_ACTION );
            filter.addAction ( AmlogicCP.UPNP_PAUSE_ACTION );
            filter.addAction ( AmlogicCP.UPNP_STOP_ACTION );
            filter.addAction ( MediaRendererDevice.PLAY_STATE_SEEK );
            filter.addAction ( AmlogicCP.UPNP_SETVOLUME_ACTION );
            filter.addAction ( AmlogicCP.UPNP_SETMUTE_ACTION );
            registerReceiver ( mUPNPReceiver, filter );
            /*start*/
            handlerUI.removeMessages ( SHOW_START );
            handlerUI.sendEmptyMessageDelayed ( SHOW_START, BUFFER_INTERVAL );
            /* enable backlight */
            PowerManager pm = ( PowerManager ) getSystemService ( Context.POWER_SERVICE );
            mWakeLock = pm.newWakeLock ( PowerManager.SCREEN_BRIGHT_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE, TAG );
            mWakeLock.acquire();
        }

        @Override
        public boolean onInfo ( MediaPlayer mp, int what, int extra ) {
            if ( what == MediaPlayer.MEDIA_INFO_BUFFERING_START ) {
                handlerUI.sendEmptyMessageDelayed ( SHOW_LOADING, BUFFER_INTERVAL );
                // showLoading();
            } else if ( what == MediaPlayer.MEDIA_INFO_BUFFERING_END ) {
                handlerUI.removeMessages ( SHOW_LOADING );
                handlerUI.sendEmptyMessage ( HIDE_LOADING );
                // hideLoading();
                Intent intent = new Intent ( MediaCenterService.DEVICE_STATUS_CHANGE );
                intent.putExtra ( "status", "PLAYING" );
                sendBroadcast ( intent );
            }
            return false;
        }

        private void playEnable() {
            if ( btn_play != null )
            { btn_play.setEnabled ( true ); }
            if ( btn_forward != null )
            { btn_forward.setEnabled ( true ); }
            if ( btn_backward != null )
            { btn_backward.setEnabled ( true ); }
            if ( mProgress != null )
            { mProgress.setEnabled ( true ); }
        }

        private void playDisable() {
            if ( btn_play != null )
            { btn_play.setEnabled ( false ); }
            if ( btn_forward != null )
            { btn_forward.setEnabled ( false ); }
            if ( btn_backward != null )
            { btn_backward.setEnabled ( false ); }
            if ( mProgress != null )
            { mProgress.setEnabled ( false ); }
        }

}
