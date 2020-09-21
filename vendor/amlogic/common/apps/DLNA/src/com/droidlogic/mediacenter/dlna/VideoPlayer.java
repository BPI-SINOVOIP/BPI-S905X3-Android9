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

import com.droidlogic.mediacenter.airplay.util.ApiHelper;

import android.app.ProgressDialog;
import android.app.Activity;
import android.app.Dialog;
import android.app.AlertDialog;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;

import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnInfoListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.PowerManager;
import android.provider.Settings;

import android.widget.Button;
import android.widget.SeekBar;
import android.widget.ProgressBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Window;
import android.view.WindowManager;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.WindowManager.LayoutParams;
import android.view.Display;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.lang.reflect.Method;
import java.lang.reflect.Field;
import org.amlogic.upnp.*; // for CyberLink

import org.cybergarage.util.Debug;
import org.cybergarage.upnp.*;
import org.cybergarage.upnp.device.*;

import com.droidlogic.mediacenter.R;

public class VideoPlayer extends Activity implements OnInfoListener// implements
    // OnPreparedListener,
    // OnErrorListener,
    // OnCompletionListener
{
        private static final String     TAG                 = "VideoPlayer";
        public static boolean    running             = false;

        private static final int VIDEO_START         = 0;
        private static final int GETINFO_FRESH       = 3;
        private static final int NEXT_VIDEO          = 5;
        private static final int SHOW_LOADING        = 6;
        private static final int HIDE_LOADING        = 7;
        private static final int FORE_VIDEO          = 8;
        private static final int STOP_BY_SEVER       = 9;
        private static final int STOPED_REMOTE = 10;
        private static final int REMOVE_VOLUME = 11;
        public static final int  STATE_PLAY          = 0;
        public static final int  STATE_PAUSE         = 1;
        public static final int  STATE_STOP          = 2;
        public static final int  DIALOG_VOLUME_ID    = 2;
        public static final int  DIALOG_EXIT_ID      = 3;
        public static final int  DIALOG_NEXT_ID      = 4;
        private static final int SHOW_STOP           = 1;
        private int              mCurIndex           = 0;
        private LoadingDialog    exitDlg;
        private static final int HNALDE_HIDE_LOADING = 4;
        private Dialog           dialog_volume;
        private ProgressBar      vol_bar;
        private boolean          mVolTouch           = false;
        private boolean          mVolChanged         = false;
        private boolean          mProgressTouch      = false;
        UPNPVideoView            mVideoView          = null;
        VideoController          mVideoController    = null;
        private LoadingDialog    nextDlg             = null;
        private String           currentURI          = null;
        private String           mediaType           = null;
        UPNPReceiver             mUPNPReceiver;
        private int              mDuration           = 0;
        private float            mTransitionAnimationScale;
        private int              mCurPos             = 0;
        private Handler          mBrocastProgress;
        private AudioManager     mAudioManager;
        private boolean          mPausedByTransientLossOfFocus;
        private int              play_state          = STATE_STOP;
        private boolean          readyForFinish      = false;
        private LoadingDialog    progressDialog      = null;
        private int              mLastState          = STATE_STOP;
        private int              volume_level        = 50;
        private boolean          mDisplayDMP         = false;
        private String           mVideoBuffer        = "0.0";
        private PowerManager.WakeLock mWakeLock;
        private boolean mHideStatusBar = false;
        private Timer mTimer;
        private TimerTask mTimerTask;
        private boolean seekComplete = true;
        private static final int SHOW_DIALOG_DELAY = 5000;
        private static final int HIDE_LOADING_DIALOG = 500;
        private static final int FRESH_POSITION = 2000;
        private static final int SWITCH_VIDEO_DELAY = 3000;
        public static final int SEEK_INTERVAL            = 9999;
        @Override
        public void onCreate ( Bundle savedInstanceState ) {
            super.onCreate ( savedInstanceState );
            getWindow().addFlags(LayoutParams.FLAG_DISMISS_KEYGUARD | LayoutParams.FLAG_TURN_SCREEN_ON);
            mTimer = new Timer();
            setContentView ( R.layout.video_view );
            mVideoController = new VideoController ( this );
            Intent intent = getIntent();
            currentURI = intent.getStringExtra ( AmlogicCP.EXTRA_MEDIA_URI );
            mediaType = intent.getStringExtra ( AmlogicCP.EXTRA_MEDIA_TYPE );
            mHideStatusBar = intent.getBooleanExtra ( "hideStatusBar", false );
            /*
             * if (!MediaRendererDevice.TYPE_VIDEO.equals(mediaType) || (currentURI
             * == null)) { finish(); return; }
             */
            if ( DeviceFileBrowser.TYPE_DMP.equals ( intent
                    .getStringExtra ( DeviceFileBrowser.DEV_TYPE ) ) ) {
                mDisplayDMP = true;
                mCurIndex = intent.getIntExtra ( DeviceFileBrowser.CURENT_POS, 0 );
            } else {
                mDisplayDMP = false;
            }
            mVideoView = ( UPNPVideoView ) findViewById ( R.id.videoView1 );
            mVideoView.setVideoController ( mVideoController );
            mVideoController.setExitListener ( new View.OnClickListener() {
                public void onClick ( View v ) {
                    ret2list();
                }
            } );
            mVideoController.setVolumeListener ( new View.OnClickListener() {
                public void onClick ( View v ) {
                    if ( mTimerTask != null ) {
                        mTimerTask.cancel();
                        mTimerTask = null;
                    }
                    showDialog ( DIALOG_VOLUME_ID );
                    mTimerTask = new VolumeHideTask();
                    mTimer.schedule ( mTimerTask, SHOW_DIALOG_DELAY );
                }
            } );
            if ( mDisplayDMP ) {
                mVideoController.setPrevNextListeners ( new View.OnClickListener() {
                    @Override
                    public void onClick ( View v ) {
                        handlerUI.sendEmptyMessageDelayed ( NEXT_VIDEO, SWITCH_VIDEO_DELAY );
                    }
                }, new View.OnClickListener() {
                    @Override
                    public void onClick ( View v ) {
                        handlerUI.sendEmptyMessageDelayed ( FORE_VIDEO, SWITCH_VIDEO_DELAY );
                    }
                } );
            } else {
                mVideoController.setPrevNextListeners ( null, null );
            }
            mBrocastProgress = new Handler();
            mAudioManager = ( AudioManager ) getSystemService ( Context.AUDIO_SERVICE );
            mVideoView.setOnCompletionListener ( new MediaPlayer.OnCompletionListener() {
                public void onCompletion ( MediaPlayer mp ) {
                    Intent intent = new Intent ( AmlogicCP.PLAY_POSTION_REFRESH );
                    intent.putExtra ( "curPosition", mDuration );
                    intent.putExtra ( "totalDuration", mDuration );
                    VideoPlayer.this.sendBroadcast ( intent );
                    Debug.d ( TAG, "##########onCompletion####################" );
                    mAudioManager.abandonAudioFocus ( mAudioFocusListener );
                    /*
                     * if(nextDlg == null){ showDialog(DIALOG_NEXT_ID); }
                     * nextDlg.show();
                     */
                    mCurPos = 0;
                    mDuration = 0;
                    if ( DeviceFileBrowser.playList.size() > 0
                    && mDisplayDMP ) {
                        handlerUI.sendEmptyMessageDelayed ( NEXT_VIDEO, SWITCH_VIDEO_DELAY);
                    } else {
                        stopExit();
                        handlerUI.sendEmptyMessageDelayed ( SHOW_STOP, SHOW_DIALOG_DELAY );
                    }
                    //sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_STOPPED );
                }
            } );
            mVideoView.setOnPreparedListener ( new MediaPlayer.OnPreparedListener() {
                public void onPrepared ( MediaPlayer mp ) {
                    Debug.d ( TAG, "##########onPrepared####################" +currentURI);
                    if ( isFinishing() )
                    { return; }
                    //sendPlayStateChangeBroadcast ( MedIaRendererDevice.PLAY_STATE_PLAYING );
                    mp.setOnInfoListener ( VideoPlayer.this );
                    play();
                    if ( nextDlg != null ) {
                        nextDlg.dismiss();
                    }
                    handlerUI.sendEmptyMessageDelayed ( HIDE_LOADING, HIDE_LOADING_DIALOG );
                }
            } );
            mVideoView.setOnErrorListener ( new MediaPlayer.OnErrorListener() {
                public boolean onError ( MediaPlayer mp, int framework_err,
                int impl_err ) {
                    Debug.d ( TAG, "##########onError####################" );
                    showDialog ( DIALOG_EXIT_ID );
                    return true;
                }
            } );
            mVideoView.setOnSeekCompleteListener ( new MediaPlayer.OnSeekCompleteListener () {
                public void onSeekComplete ( MediaPlayer mp ) {
                    Debug.d ( TAG, "#############onSeekComplete#####################"+mp.getCurrentPosition());
                    handlerUI.sendEmptyMessage ( HIDE_LOADING );
                    //handlerUI.removeMessages ( GETINFO_FRESH );
                    //handlerUI.sendEmptyMessage ( GETINFO_FRESH );
                    seekComplete = true;
                }
            } );
            mVideoView.setOnStateChangedListener ( new UPNPVideoView.OnStateChangedListener() {
                public void onStateChanged ( int state ) {
                    Debug.d ( TAG,
                              "##########onStateChanged####################"+state );
                    switch ( state ) {
                        case UPNPVideoView.STATE_PAUSED:
                            sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_PAUSED );
                            play_state = STATE_PAUSE;
                            break;
                        case UPNPVideoView.STATE_PLAYING:
                            handlerUI.sendEmptyMessage ( HIDE_LOADING );
                            sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_PLAYING );
                            play_state = STATE_PLAY;
                            readyForFinish = false;
                            break;
                        case UPNPVideoView.STATE_PLAYBACK_COMPLETED:
                            play_state = STATE_STOP;
                            sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_STOPPED );
                            break;
                        case UPNPVideoView.STATE_ERROR:
                        //case UPNPVideoView.STATE_IDLE:
                            sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_STOPPED );
                            /* if(handlerUI != null)
                                 handlerUI.removeMessages(GETINFO_FRESH);*/
                            play_state = STATE_STOP;
                            break;
                    }
                }
            } );
            start();
            mUPNPReceiver = new UPNPReceiver();
            Debug.d ( TAG, "##############################" );
            Debug.d ( TAG, "##############################" );
            Debug.d ( TAG, "onCreate: make running as TRUE" );
        }
        private Handler handlerUI = new Handler() {
            @Override
            public void handleMessage ( Message msg ) {
                switch ( msg.what ) {
                    case SHOW_STOP:
                        wait2Exit();
                        return;
                    case VIDEO_START:
                        start();
                        return;
                    case GETINFO_FRESH:
                        refressGetInfo();
                        return;
                    case HNALDE_HIDE_LOADING:
                        /*
                         * if (mVideoView != null &&
                         * mVideoView
                         * .getCurrentPosition() > 0) {
                         */
                        hideLoading();
                        mVideoController.show();
                        /*
                         * } else { handlerUI
                         * .sendEmptyMessageDelayed(
                         * HNALDE_HIDE_LOADING, 500); }
                         */
                        return;
                    case NEXT_VIDEO:
                        next();
                        return;
                    case SHOW_LOADING:
                        showLoading();
                        return;
                    case HIDE_LOADING:
                        hideLoading();
                        return;
                    case FORE_VIDEO:
                        before();
                        return;
                    case STOP_BY_SEVER:
                        if ( !running ) {
                            VideoPlayer.this.finish();
                        } else {
                            if ( handlerUI != null )
                            { handlerUI.sendEmptyMessageDelayed ( STOP_BY_SEVER, SHOW_DIALOG_DELAY ); }
                        }
                        return;
                    case STOPED_REMOTE:
                        sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_STOPPED );
                        return;
                }
            }
        };

        private void ret2list() {
            if (mDisplayDMP) {
                Intent intent = new Intent(this,DeviceFileBrowser.class);
                this.setResult ( mCurIndex, intent );
            }
            this.finish();
        }

        @Override
        protected void onResume() {
            Intent intent = getIntent();
            currentURI = intent.getStringExtra ( AmlogicCP.EXTRA_MEDIA_URI );
            mediaType = intent.getStringExtra ( AmlogicCP.EXTRA_MEDIA_TYPE );
            mHideStatusBar = intent.getBooleanExtra ( "hideStatusBar", false );
            super.onResume();
            if ( currentURI == null )
            { this.finish(); }
            if ( handlerUI != null ) {
                handlerUI.removeMessages ( STOPED_REMOTE );
            }
            running = true;
            int max = mAudioManager.getStreamMaxVolume ( AudioManager.STREAM_MUSIC );
            int vol = mAudioManager.getStreamVolume ( AudioManager.STREAM_MUSIC );
            mVideoBuffer = (String)PrefUtils.getProperties( "media.amplayer.buffertime","0.0");
            PrefUtils.setProperties("media.amplayer.buffertime", "6" );
            //PrefUtils.setProperties("media.amplayer.displast_frame", "true");
            if ( mLastState == STATE_PLAY ) {
                play();
            }
            /* enable backlight */
            PowerManager pm = ( PowerManager ) getSystemService ( Context.POWER_SERVICE );
            mWakeLock = pm.newWakeLock ( PowerManager.SCREEN_BRIGHT_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE, TAG );
            mWakeLock.acquire();
            IntentFilter filter = new IntentFilter();
            filter.addAction ( AmlogicCP.UPNP_STOP_ACTION );
            filter.addAction ( AmlogicCP.UPNP_PLAY_ACTION );
            filter.addAction ( AmlogicCP.UPNP_PAUSE_ACTION );
            filter.addAction ( AmlogicCP.UPNP_SETVOLUME_ACTION );
            filter.addAction ( AmlogicCP.UPNP_SETMUTE_ACTION );
            filter.addAction ( MediaRendererDevice.PLAY_STATE_SEEK );
            registerReceiver ( mUPNPReceiver, filter );
        }

        private void before() {
            if ( DeviceFileBrowser.playList.size() == 1 ) {
                mCurIndex = 0;
            } else if ( mCurIndex <= 0 ) {
                mCurIndex = ( DeviceFileBrowser.playList.size() - 1 );
            } else {
                mCurIndex--;
            }
            Map<String, Object> item = ( Map<String, Object> ) DeviceFileBrowser.playList
                                       .get ( mCurIndex );
            currentURI = ( String ) item.get ( "item_uri" );
            mCurPos = 0;
            start();
        }

        private void next() {
            if ( DeviceFileBrowser.playList.size() == 1 ) {
                mCurIndex = 0;
            } else if ( mCurIndex >= ( DeviceFileBrowser.playList.size() - 1 ) || mCurIndex < 0 ) {
                mCurIndex = 0;
            } else if ( mCurIndex < ( DeviceFileBrowser.playList.size() - 1 ) ) {
                mCurIndex++;
            } else {
                mCurIndex = 0;
            }
            Map<String, Object> item = ( Map<String, Object> ) DeviceFileBrowser.playList
                                       .get ( mCurIndex );
            currentURI = ( String ) item.get ( "item_uri" );
            mCurPos = 0;
            start();
        }

        @Override
        protected void onPause() {
            super.onPause();
            stopExit();
            mTransitionAnimationScale = Settings.System.getFloat (
                                            VideoPlayer.this.getContentResolver(),
                                            Settings.System.TRANSITION_ANIMATION_SCALE,
                                            mTransitionAnimationScale );

            try {
                Class service = Class.forName("android.view.IWindowManager.Stub");
                Method method = service.getMethod("setAnimationScale",int.class,float.class);
                method.invoke(service, 1, 0.0f );
            } catch ( Exception e ) {
            }
            mLastState = play_state;
            PrefUtils.setProperties ( "media.amplayer.buffertime", mVideoBuffer );
            if ( handlerUI != null ) {
                handlerUI.removeMessages ( GETINFO_FRESH );
            }
            mAudioManager.setStreamMute ( AudioManager.STREAM_MUSIC, false );
            mWakeLock.release();
            unregisterReceiver ( mUPNPReceiver );
            if ( running == true ) {
                running = false;
            }
            hideLoading();

            if ( mTimerTask != null ) {
                mTimerTask.cancel();
                mTimerTask = null;
            }
            hideVolume();
        }

        private void hideVolume() {
            if ( dialog_volume != null && dialog_volume.isShowing() ) {
                dialog_volume.dismiss();
            }
        }

        public void onStop() {
            super.onStop();
            stopPlayback();
            Debug.d ( TAG, "##############################" );
            Debug.d ( TAG, "##############################" );
            Debug.d ( TAG, "onStop: make running as FALSE" );
            //PrefUtils.setProperties("media.amplayer.displast_frame", "false");
            mCurPos = 0;
            mDuration = 0;
            currentURI = null;
            //handlerUI = null;
            //mBrocastProgress = null;
        }

        public void onDestory() {
            super.onDestroy();
            handlerUI = null;
            try {
                Class service = Class.forName("android.view.IWindowManager.Stub");
                Method method = service.getMethod("setAnimationScale",int.class,float.class);
                method.invoke(service, 1, mTransitionAnimationScale);
            } catch ( Exception e ) {
            }
        }

        private OnAudioFocusChangeListener mAudioFocusListener = new OnAudioFocusChangeListener() {
            public void onAudioFocusChange (
            int focusChange ) {
                Debug.d ( TAG,
                          "##########onAudioFocusChange####################" );
                switch ( focusChange ) {
                    case AudioManager.AUDIOFOCUS_LOSS:
                        mPausedByTransientLossOfFocus = false;
                        pause();
                        break;
                    case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                    case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                        if ( play_state == STATE_PLAY ) {
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
            }
        };
        private void refressGetInfo() {
            if ( play_state == STATE_STOP ) {
                return;
            }
            mDuration = mVideoView.getDuration();
            if ( mDuration > 0 ) {
                int curPos = mVideoView.getCurrentPosition();
                Intent intent = new Intent ( AmlogicCP.PLAY_POSTION_REFRESH );
                intent.putExtra ( "curPosition", curPos );
                intent.putExtra ( "totalDuration", mDuration );
                VideoPlayer.this.sendBroadcast ( intent );
                Debug.d ( TAG, "######sendBroadcast(progress)######" + curPos
                          + "/" + mDuration );
            }
            handlerUI.removeMessages ( GETINFO_FRESH );
            handlerUI.sendEmptyMessageDelayed ( GETINFO_FRESH, FRESH_POSITION );
            //mBrocastProgress.removeCallbacksAndMessages(null);
            //mBrocastProgress.postDelayed(new ProgressRefresher(), 500);
        }
        class ProgressRefresher implements Runnable {
                public void run() {
                    /*mDuration = mVideoView.getDuration();
                    Debug.d(TAG, "####=============##sendBroadcast(progress)######" + mDuration
                               + "/" + mVideoView.getCurrentPosition());
                    if ( mDuration > 0 ) {
                        int curPos = mVideoView.getCurrentPosition();
                        Intent intent = new Intent ( AmlogicCP.PLAY_POSTION_REFRESH );
                        intent.putExtra ( "curPosition", curPos );
                        intent.putExtra ( "totalDuration", mDuration );
                        sendBroadcast ( intent );
                        Debug.d ( TAG, "######sendBroadcast(progress)######" + curPos
                                  + "/" + mDuration );
                    }
                    mBrocastProgress.removeCallbacksAndMessages ( null );
                    mBrocastProgress.postDelayed(new ProgressRefresher(), 500);*/
                }
        }

        private void start() {
            // mAudioManager.requestAudioFocus(mAudioFocusListener,
            // AudioManager.STREAM_MUSIC,
            // AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
            seekComplete = true;
            Debug.d ( TAG, "*********************currentURI=" + currentURI );
            //showLoading();
            handlerUI.sendEmptyMessage ( SHOW_LOADING );
            //handlerUI.sendEmptyMessageDelayed(HIDE_LOADING,3000);
            handlerUI.removeMessages ( STOP_BY_SEVER );
            handlerUI.sendEmptyMessageDelayed ( STOP_BY_SEVER, SHOW_DIALOG_DELAY );
            //handlerUI.removeMessages(GETINFO_FRESH);
            //handlerUI.sendEmptyMessage ( GETINFO_FRESH );
            // mVideoView.start();
            // mVideoView.seekTo(mCurPos);
            // mBrocastProgress.postDelayed(new ProgressRefresher(), 200);
            // play_state = STATE_PLAY;
            // readyForFinish = false;
            // sendPlayStateChangeBroadcast(MediaRendererDevice.PLAY_STATE_PLAYING);
            // mVideoController.updatePausePlay();
            // mVideoController.show();
            mVideoView.setVideoPath ( currentURI );
        }

        private void play() {
            mAudioManager.requestAudioFocus ( mAudioFocusListener,
                                              AudioManager.STREAM_MUSIC,
                                              AudioManager.AUDIOFOCUS_GAIN_TRANSIENT );
            // mVideoView.setVideoPath(currentURI);
            // mVideoView.seekTo(mCurPos);
            if ( handlerUI != null ) {
                handlerUI.removeMessages (GETINFO_FRESH);
                handlerUI.sendEmptyMessage ( GETINFO_FRESH );
            }else{
                refressGetInfo();
            }
            mVideoController.dispatchKeyEvent ( new KeyEvent ( KeyEvent.ACTION_DOWN,
                                                KeyEvent.KEYCODE_MEDIA_PLAY ) );
        }

        private void pause() {
            mCurPos = mVideoView.getCurrentPosition();
            mVideoController.dispatchKeyEvent ( new KeyEvent ( KeyEvent.ACTION_DOWN,
                                                KeyEvent.KEYCODE_MEDIA_PAUSE ) );
        }

        private void stopPlayback() {
            mVideoView.stopPlayback();
            mCurPos = 0;
            mDuration = 0;
            mAudioManager.abandonAudioFocus ( mAudioFocusListener );
            // mVideoController.updatePausePlay();
            // mVideoController.show();
            mVideoController.dispatchKeyEvent ( new KeyEvent ( KeyEvent.ACTION_DOWN,
                                                KeyEvent.KEYCODE_MEDIA_STOP ) );
        }

        class UPNPReceiver extends BroadcastReceiver {
                public void onReceive ( Context context, Intent intent ) {
                    String action = intent.getAction();
                    if ( action == null )
                    { return; }
                    String mediaType = intent
                                       .getStringExtra ( AmlogicCP.EXTRA_MEDIA_TYPE );
                    // if(!MediaRendererDevice.TYPE_VIDEO.equals(mediaType))
                    // return;
                    Debug.d ( TAG, "#####VideoPlayer.UPNPReceiver: " + action
                              + ",  mediaType=" + mediaType );
                    readyForFinish = false;
                    mHideStatusBar = intent.getBooleanExtra ( "hideStatusBar", false );
                    if ( action.equals ( AmlogicCP.UPNP_PLAY_ACTION ) ) {
                        String uri = intent.getStringExtra ( AmlogicCP.EXTRA_MEDIA_URI );
                        seekComplete = true;
                        stopExit();
                        if ( DeviceFileBrowser.TYPE_DMP.equals ( intent
                                .getStringExtra ( DeviceFileBrowser.DEV_TYPE ) ) ) {
                            mDisplayDMP = true;
                            mCurIndex = intent.getIntExtra (
                                            DeviceFileBrowser.CURENT_POS, 0 );
                        } else {
                            mDisplayDMP = false;
                        }
                        Debug.d ( TAG, "show UpnpReceiver:" + mDisplayDMP );
                        if ( mDisplayDMP ) {
                            mVideoController.setPrevNextListeners ( new View.OnClickListener() {
                                @Override
                                public void onClick ( View v ) {
                                    handlerUI.sendEmptyMessageDelayed ( FORE_VIDEO, 3000 );
                                }
                            }, new View.OnClickListener() {
                                @Override
                                public void onClick ( View v ) {
                                    handlerUI.sendEmptyMessageDelayed ( NEXT_VIDEO, 3000 );
                                }
                            } );
                        } else {
                            mVideoController.setPrevNextListeners ( null, null );
                        }
                        if ( currentURI.equals ( uri ) && play_state == STATE_PLAY ) {
                            sendPlayStateChangeBroadcast ( MediaRendererDevice.PLAY_STATE_PLAYING );
                        }
                        if ( !currentURI.equals ( uri ) || ( play_state == STATE_STOP ) ) {
                            mCurPos = 0;
                            currentURI = uri;
                            // showLoading();
                            // mPlayerThread.start();
                            start();
                        } else {
                            play();
                        }
                    } else if ( action.equals ( AmlogicCP.UPNP_PAUSE_ACTION ) ) {
                        pause();
                    } else if ( action.equals ( AmlogicCP.UPNP_STOP_ACTION ) ) {
                        stopPlayback();
                        seekComplete = true;
                        stopExit();
                        handlerUI.sendEmptyMessageDelayed ( SHOW_STOP, 6000 );
                    } else if ( action.equals ( MediaRendererDevice.PLAY_STATE_SEEK ) ) {
                        String time = intent.getStringExtra ( "REL_TIME" );
                        if ( time == null || time.isEmpty() )
                            return;
                        int msecs = parseTimeStringToMSecs ( time );
                        if ( ( msecs < 0 ) || ( ( msecs > mDuration ) && ( mDuration > 0 ) ) )
                        { return; }
                        Debug.d ( TAG, "*********seek to: " + time + ",   msecs=" + msecs + "mDuration:"+ mDuration);
                        if ( seekComplete ) {
                            seekComplete = false;
                            if (( msecs - mVideoView.getCurrentPosition() ) > SEEK_INTERVAL || ( mVideoView.getCurrentPosition() - msecs ) > SEEK_INTERVAL ) {
                                mVideoView.seekTo ( msecs );
                                if ( play_state == STATE_PLAY ) {
                                    handlerUI.removeMessages ( GETINFO_FRESH );
                                }
                            }
                        }else{
                            seekComplete = true;
                        }
                        //handlerUI.sendEmptyMessageDelayed(3, 500);
                    } else if ( action.equals ( AmlogicCP.UPNP_SETVOLUME_ACTION ) ) {
                        int vol = intent.getIntExtra ( "DesiredVolume", 50 );
                        int max = mAudioManager
                                  .getStreamMaxVolume ( AudioManager.STREAM_MUSIC );
                        mAudioManager.setStreamVolume ( AudioManager.STREAM_MUSIC, vol
                                                        * max / 100, AudioManager.FLAG_PLAY_SOUND );
                        if ( mTimerTask != null ) {
                            mTimerTask.cancel();
                            mTimerTask = null;
                        }
                        if ( dialog_volume == null ) {
                            showDialog ( DIALOG_VOLUME_ID );
                        } else {
                            dialog_volume.show();
                        }
                        mTimerTask = new VolumeHideTask();
                        mTimer.schedule ( mTimerTask, 5000 );
                        if ( vol_bar != null )
                        { vol_bar.setProgress ( vol ); }
                    } else if ( action.equals ( AmlogicCP.UPNP_SETMUTE_ACTION ) ) {
                        Debug.d ( TAG, "*******setMuteAction" );
                        Boolean mute = ( Boolean ) intent.getBooleanExtra ( "DesiredMute", false );
                        // mAudioManager.setMasterMute(mute);
                        mAudioManager.setStreamMute ( AudioManager.STREAM_MUSIC, mute );
                    }
                }
        }
        class VolumeHideTask extends TimerTask {
                public void run() {
                    hideVolume();
                }
        }
        public static int parseTimeStringToMSecs ( String time ) {
            String[] str = time.split ( ":|\\." );
            if ( str.length < 3 )
            { return 0; }
            int hour = Integer.parseInt ( str[0] );
            int min = Integer.parseInt ( str[1] );
            int sec = Integer.parseInt ( str[2] );
            Debug.d(TAG,"***********parseTimeStringToInt: "+hour+":"+min+":"+sec);
            return ( hour * 3600 + min * 60 + sec ) * 1000;
        }

        private void showLoading() {
            if ( nextDlg != null ) {
                nextDlg.dismiss();
            }
            if ( progressDialog == null && running ) {
                progressDialog = new LoadingDialog ( this,
                                                     LoadingDialog.TYPE_LOADING, this.getResources().getString (
                                                             R.string.loading ) );
                progressDialog.show();
            } else if ( progressDialog != null && running ) {
                progressDialog.show();
            }
        }

        private void hideLoading() {
            if ( progressDialog != null ) {
                progressDialog.stopAnim();
                progressDialog.dismiss();
                progressDialog = null;
            }
        }

        /*
         * private void waitForExit(long delay) { Debug.d(TAG,
         * "********waitForExit"); handlerUI.removeMessages(1); readyForFinish =
         * true; handlerUI.sendEmptyMessageDelayed(1, delay); }
         */


        class PlayerThread extends Thread {
                private String mThreadName;

                public PlayerThread ( String threadName ) {
                    super ( threadName );
                    mThreadName = threadName;
                }

                public void run() {
                    Debug.d ( TAG, "*********************currentURI=" + currentURI );
                    // showLoading();
                    mVideoView.setVideoPath ( currentURI );
                }
        }

        private void sendPlayStateChangeBroadcast ( String stat ) {
            Intent intent = new Intent();
            intent.setAction ( stat );
            sendBroadcast ( intent );
            Debug.d ( TAG, "########sendPlayStateChangeBroadcast:  " + stat );
            return;
        }

        @Override
        protected Dialog onCreateDialog ( int id ) {
            LayoutInflater inflater = ( LayoutInflater ) VideoPlayer.this
                                      .getSystemService ( LAYOUT_INFLATER_SERVICE );
            switch ( id ) {
                case DIALOG_VOLUME_ID:
                    View layout_volume = inflater.inflate ( R.layout.volume_dialog,
                                                            ( ViewGroup ) findViewById ( R.id.layout_root_volume ) );
                    dialog_volume = new VolumeDialog ( this );
                    return dialog_volume;
                case DIALOG_EXIT_ID:
                    Dialog errDlg = new AlertDialog.Builder ( VideoPlayer.this )
                    .setTitle ( R.string.video_err_title )
                    .setMessage ( R.string.video_err_summary )
                    .setPositiveButton ( R.string.str_ok,
                    new DialogInterface.OnClickListener() {
                        public void onClick ( DialogInterface dialog,
                        int which ) {
                            ret2list();
                        }
                    } ).create();
                    errDlg.setOnDismissListener ( new DialogInterface.OnDismissListener() {
                        public void onDismiss ( DialogInterface dialog ) {
                            ret2list();
                        }
                    } );
                    return errDlg;
                case DIALOG_NEXT_ID:
                    nextDlg = new LoadingDialog ( this, LoadingDialog.TYPE_LOADING,
                                                  this.getResources().getString (
                                                      R.string.video_next_summary ) );
                    nextDlg.setOnCancelListener ( new DialogInterface.OnCancelListener() {
                        @Override
                        public void onCancel ( DialogInterface arg0 ) {
                            ret2list();
                        }
                    } );
                    return nextDlg;
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
                        volume_level = current * 100 / mmax;/*volume_level relative value*/
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
                                    Debug.d ( TAG, "vol_bar:onProgressChanged="
                                              + progress );
                                    if ( !fromuser )
                                    { return; }
                                    long now = SystemClock.elapsedRealtime();
                                    if ( ( now - mLastTime ) > 250 ) {
                                        mLastTime = now;
                                        // trackball event, allow progress updates
                                        if ( mVolTouch ) {
                                            Debug.d ( TAG, "***progress=" + progress );
                                            vol_bar.setProgress ( progress );
                                            volume_level = progress;
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
                                    }
                                }
                                public void onStopTrackingTouch ( SeekBar bar ) {
                                    Debug.d ( TAG, "vol_bar:onStopTrackingTouch: "
                                              + volume_level );
                                    mVolTouch = false;
                                }
                            } );
                        }
                        vol_bar.setMax ( 100 );
                        vol_bar.setProgress ( volume_level );
                        break;
                    }
            }
        }

        @Override
        public boolean onKeyDown ( int keyCode, KeyEvent event ) {
            if ( keyCode == KeyEvent.KEYCODE_BACK ) {
                hideLoading();
                if ( nextDlg != null ) {
                    nextDlg.dismiss();
                }
                ret2list();
                return true;
            } else if ( keyCode == KeyEvent.KEYCODE_VOLUME_MUTE ) {
                if ( MediaRendererDevice.mMuteState.equals ( "1" ) ) {
                    MediaRendererDevice.mMuteState = "0";
                    mAudioManager.setStreamMute ( AudioManager.STREAM_MUSIC, false );
                    Debug.d ( TAG, "input  keycode KeyEvent.KEYCODE_VOLUME_MUTE false" );
                } else {
                    MediaRendererDevice.mMuteState = "1";
                    mAudioManager.setStreamMute ( AudioManager.STREAM_MUSIC, true );
                    Debug.d ( TAG, "input  keycode KeyEvent.KEYCODE_VOLUME_MUTE true" );
                }
            } else if ( keyCode == KeyEvent.KEYCODE_VOLUME_UP ) {
                MediaRendererDevice.mMuteState = "0";
                mAudioManager.setStreamMute ( AudioManager.STREAM_MUSIC, false );
                Debug.d ( TAG, "input  keycode KeyEvent.KEYCODE_VOLUME_Up" );
            }
            return super.onKeyDown ( keyCode, event );
        }

        private class VolumeDialog extends Dialog {
                VolumeDialog ( Context context ) {
                    super ( context, R.style.theme_dialog );
                    setContentView ( R.layout.volume_dialog );
                    LayoutParams params = getWindow().getAttributes();
                    params.x = 190;
                    params.y = -200;
                    getWindow().setAttributes ( params );
                }

                @Override
                public boolean onKeyDown ( int keyCode, KeyEvent event ) {
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
                        if ( mTimerTask != null ) {
                            mTimerTask.cancel();
                            mTimerTask = null;
                        }
                        if ( dialog_volume == null ) {
                            showDialog ( DIALOG_VOLUME_ID );
                        } else {
                            dialog_volume.show();
                        }
                        mTimerTask = new VolumeHideTask();
                        mTimer.schedule ( mTimerTask, 5000 );
                        if ( vol_bar != null )
                        { vol_bar.setProgress ( volume_level ); }
                        return true;
                    } else if ( keyCode == KeyEvent.KEYCODE_BACK ) {
                        VolumeDialog.this.cancel();
                        return true;
                    }
                    return super.onKeyDown ( keyCode, event );
                }
        }

        private void stopExit() {
            handlerUI.removeMessages ( SHOW_STOP );
            if ( exitDlg != null ) {
                exitDlg.dismiss();
                exitDlg = null;
            }
        }

        public void wait2Exit() {
            Debug.d ( TAG, "wait2Exit......" + running );
            hideLoading();
            if ( !running ) {
                return;
            }
            if ( exitDlg == null ) {
                exitDlg = new LoadingDialog ( this, LoadingDialog.TYPE_EXIT_TIMER, "" );
                exitDlg.setCancelable ( true );
                exitDlg.setOnDismissListener ( new OnDismissListener() {
                    @Override
                    public void onDismiss ( DialogInterface arg0 ) {
                        if ( exitDlg != null && ( VideoPlayer.this.getClass().getName().equals ( exitDlg.getTopActivity ( VideoPlayer.this ) ) ||
                        exitDlg.getCountNum() == 0 ) ) {
                            ret2list();
                        }
                    }
                } );
                exitDlg.setCountNum ( 10 );
                exitDlg.show();
            } else {
                exitDlg.setCountNum ( 10 );
                exitDlg.show();
            }
        }

        @Override
        public boolean onInfo ( MediaPlayer mp, int what, int extra ) {
            if ( what == MediaPlayer.MEDIA_INFO_BUFFERING_START && mVideoView.getBufferPercentage() < 99 && running ) {
                handlerUI.sendEmptyMessageDelayed ( SHOW_LOADING, FRESH_POSITION );
                // showLoading();
            } else if ( what == MediaPlayer.MEDIA_INFO_BUFFERING_END ) {
                handlerUI.removeMessages ( SHOW_LOADING );
                handlerUI.sendEmptyMessage ( HIDE_LOADING );
               // Intent intent = new Intent ( MediaCenterService.DEVICE_STATUS_CHANGE );
               // intent.putExtra ( "status", "PLAYING" );
               // sendBroadcast ( intent );
                // hideLoading();
            }
            return false;
        }


}
