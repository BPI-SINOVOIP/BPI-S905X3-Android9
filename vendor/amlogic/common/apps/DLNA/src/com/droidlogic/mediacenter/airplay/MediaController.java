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
package com.droidlogic.mediacenter.airplay;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.PixelFormat;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.SeekBar.OnSeekBarChangeListener;

//import com.android.internal.policy.PolicyManager;

import java.lang.reflect.Method;
import java.lang.reflect.Constructor;
import java.util.Formatter;
import java.util.Locale;

import com.droidlogic.mediacenter.R;

/**
 * A view containing controls for a MediaPlayer. Typically contains the buttons
 * like "Play/Pause", "Rewind", "Fast Forward" and a progress slider. It takes
 * care of synchronizing the controls with the state of the MediaPlayer.
 * <p>
 * The way to use this class is to instantiate it programatically. The
 * MediaController will create a default set of controls and put them in a
 * window floating above your application. Specifically, the controls will float
 * above the view specified with setAnchorView(). The window will disappear if
 * left idle for three seconds and reappear when the user touches the anchor
 * view.
 * <p>
 * Functions like show() and hide() have no effect when MediaController is
 * created in an xml layout.
 *
 * MediaController will hide and show the buttons according to these rules:
 * <ul>
 * <li>The "previous" and "next" buttons are hidden until setPrevNextListeners()
 * has been called
 * <li>The "previous" and "next" buttons are visible but disabled if
 * setPrevNextListeners() was called with null listeners
 * <li>The "rewind" and "fastforward" buttons are shown unless requested
 * otherwise by using the MediaController(Context, boolean) constructor with the
 * boolean set to false
 * </ul>
 */
public class MediaController extends FrameLayout {

        private MediaPlayerControl mPlayer;
        private Context mContext;
        private View mAnchor;
        private View mRoot;
        private WindowManager mWindowManager;
        private Window mWindow;
        private View mDecor;
        private WindowManager.LayoutParams mDecorLayoutParams;
        // private ProgressBar mProgress;
        private SeekBar mProgress;
        private TextView mEndTime, mCurrentTime;
        private boolean mShowing;
        private boolean mDragging;
        private static final int sDefaultTimeout = 5000;
        private static final int FADE_OUT = 1;
        private static final int SHOW_PROGRESS = 2;
        private boolean mUseFastForward;
        private boolean mFromXml;
        private boolean mListenersSet;
        private View.OnClickListener mNextListener, mPrevListener, mExitListener, mVolumeListener;
        StringBuilder mFormatBuilder;
        Formatter mFormatter;
        private ImageButton mPauseButton;
        private ImageButton mFfwdButton;
        private ImageButton mRewButton;
        private ImageButton mNextButton;
        private ImageButton mPrevButton;
        private ImageButton mExitButton;
        private ImageButton mVolumeButton;
        public MediaController ( Context context, AttributeSet attrs ) {
            super ( context, attrs );
            mRoot = this;
            mContext = context;
            mUseFastForward = true;
            mFromXml = true;
        }

        @Override
        public void onFinishInflate() {
            if ( mRoot != null ) {
                initControllerView ( mRoot );
            }
        }

        public MediaController ( Context context, boolean useFastForward ) {
            super ( context );
            mContext = context;
            mUseFastForward = useFastForward;
            initFloatingWindowLayout();
            initFloatingWindow();
        }

        public MediaController ( Context context ) {
            this ( context, true );
        }

        public void setVolumeListener ( View.OnClickListener volume ) {
            mVolumeListener = volume;
            Log.d ( "tt", "mVolumeButton != null?" + ( mVolumeButton != null ) );
            if ( mVolumeButton != null ) {
                mVolumeButton.setOnClickListener ( mVolumeListener );
            }
        }

        private void initFloatingWindow() {
            mWindowManager = ( WindowManager ) mContext
                             .getSystemService ( Context.WINDOW_SERVICE );
            try {
                Class policyClass;
                if ( android.os.Build.VERSION.SDK_INT >= 23 ) {
                    policyClass = Class.forName ( "com.android.internal.policy.PhoneWindow" );
                    if (policyClass != null) {
                        Constructor create = policyClass.getConstructor(Context.class);
                        if (create != null) {
                            mWindow = (Window)create.newInstance(mContext);
                         }
                    }
                }else{
                    policyClass = Class.forName ( "com.android.internal.policy.PolicyManager" );
                    Method meths[] = policyClass.getMethods();
                    Method makenewwindow = null;
                    // Method makenewwindow = policyClass.getMethod("makeNewWindow");
                    for ( int i = 0; i < meths.length; i++ ) {
                        if ( meths[i].getName().endsWith ( "makeNewWindow" ) ) {
                            makenewwindow = meths[i];
                        }
                    }
                    mWindow = ( Window ) makenewwindow.invoke ( null, mContext );
                }

            } catch ( Exception e ) {
                e.printStackTrace();
            }
            if ( mWindow != null ) {
                mWindow.setWindowManager ( mWindowManager, null, null );
                mWindow.requestFeature ( Window.FEATURE_NO_TITLE );
                mDecor = mWindow.getDecorView();
                mDecor.setOnTouchListener ( mTouchListener );
                mWindow.setContentView ( this );
                mWindow.setBackgroundDrawableResource ( android.R.color.transparent );
                // While the media controller is up, the volume control keys should
                // affect the media stream type
                mWindow.setVolumeControlStream ( AudioManager.STREAM_MUSIC );
            }
            setFocusable ( true );
            setFocusableInTouchMode ( true );
            setDescendantFocusability ( ViewGroup.FOCUS_AFTER_DESCENDANTS );
            requestFocus();
        }

        // Allocate and initialize the static parts of mDecorLayoutParams. Must
        // also call updateFloatingWindowLayout() to fill in the dynamic parts
        // (y and width) before mDecorLayoutParams can be used.
        private void initFloatingWindowLayout() {
            Log.d ( "tt", "initFloatingWindowLayout...................." );
            mDecorLayoutParams = new WindowManager.LayoutParams();
            WindowManager.LayoutParams p = mDecorLayoutParams;
            p.gravity = Gravity.TOP;
            p.height = LayoutParams.WRAP_CONTENT;
            p.x = 0;
            p.format = PixelFormat.TRANSLUCENT;
            p.type = WindowManager.LayoutParams.TYPE_APPLICATION_PANEL;
            p.flags |= WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM
                       | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                       | WindowManager.LayoutParams.FLAG_SPLIT_TOUCH;
            p.token = null;
            p.windowAnimations = 0; // android.R.style.DropDownAnimationDown;
        }

        // Update the dynamic parts of mDecorLayoutParams
        // Must be called with mAnchor != NULL.
        private void updateFloatingWindowLayout() {
            int[] anchorPos = new int[2];
            mAnchor.getLocationOnScreen ( anchorPos );
            WindowManager.LayoutParams p = mDecorLayoutParams;
            p.width = mAnchor.getWidth();
            p.y = anchorPos[1] + mAnchor.getHeight();
        }

        // This is called whenever mAnchor's layout bound changes
        private OnLayoutChangeListener mLayoutChangeListener = new OnLayoutChangeListener() {
            public void onLayoutChange ( View v, int left, int top, int right,
            int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom ) {
                updateFloatingWindowLayout();
                if ( mShowing ) {
                    mWindowManager.updateViewLayout ( mDecor, mDecorLayoutParams );
                }
            }
        };

        private OnTouchListener mTouchListener = new OnTouchListener() {
            public boolean onTouch ( View v, MotionEvent event ) {
                if ( event.getAction() == MotionEvent.ACTION_DOWN ) {
                    if ( mShowing ) {
                        hide();
                    }
                }
                return false;
            }
        };

        public void setMediaPlayer ( MediaPlayerControl player ) {
            mPlayer = player;
            updatePausePlay();
        }
        public void setExitListener ( View.OnClickListener listen ) {
            mExitListener = listen;
            if ( ( mExitButton != null ) && ( listen != null ) ) {
                mExitButton.setOnClickListener ( listen );
                mExitButton.setVisibility ( View.VISIBLE );
                mExitButton.setEnabled ( true );
            }
        }
        /**
         * Set the view that acts as the anchor for the control view. This can for
         * example be a VideoView, or your Activity's main view.
         *
         * @param view
         *            The view to which to anchor the controller when it is visible.
         */
        public void setAnchorView ( View view ) {
            if ( mAnchor != null ) {
                mAnchor.removeOnLayoutChangeListener ( mLayoutChangeListener );
            }
            mAnchor = view;
            if ( mAnchor != null ) {
                mAnchor.addOnLayoutChangeListener ( mLayoutChangeListener );
            }
            FrameLayout.LayoutParams frameParams = new FrameLayout.LayoutParams (
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT );
            removeAllViews();
            View v = makeControllerView();
            addView ( v, frameParams );
        }

        /**
         * Create the view that holds the widgets that control playback. Derived
         * classes can override this to create their own.
         *
         * @return The controller view.
         * @hide This doesn't work as advertised
         */
        protected View makeControllerView() {
            LayoutInflater inflate = ( LayoutInflater ) mContext
                                     .getSystemService ( Context.LAYOUT_INFLATER_SERVICE );
            mRoot = inflate.inflate ( R.layout.media_controller, null );
            Log.d ( "tt", "makeControllerView...................." );
            initControllerView ( mRoot );
            return mRoot;
        }

        private void initControllerView ( View v ) {
            Log.d ( "tt", "initControllerView" );
            mPauseButton = ( ImageButton ) v.findViewById ( R.id.pause );
            if ( mPauseButton != null ) {
                mPauseButton.requestFocus();
                mPauseButton.setOnClickListener ( mPauseListener );
            }
            mFfwdButton = ( ImageButton ) v.findViewById ( R.id.ffwd );
            if ( mFfwdButton != null ) {
                mFfwdButton.setOnClickListener ( mFfwdListener );
                if ( !mFromXml ) {
                    mFfwdButton.setVisibility ( mUseFastForward ? View.VISIBLE
                                                : View.GONE );
                }
            }
            mRewButton = ( ImageButton ) v.findViewById ( R.id.rew );
            if ( mRewButton != null ) {
                mRewButton.setOnClickListener ( mRewListener );
                if ( !mFromXml ) {
                    mRewButton.setVisibility ( mUseFastForward ? View.VISIBLE
                                               : View.GONE );
                }
            }
            // By default these are hidden. They will be enabled when
            // setPrevNextListeners() is called
            mNextButton = ( ImageButton ) v.findViewById ( R.id.next );
            if ( mNextButton != null && !mFromXml && !mListenersSet ) {
                mNextButton.setVisibility ( View.GONE );
            }
            mPrevButton = ( ImageButton ) v.findViewById ( R.id.prev );
            if ( mPrevButton != null && !mFromXml && !mListenersSet ) {
                mPrevButton.setVisibility ( View.GONE );
            }
            mProgress = ( SeekBar ) v.findViewById ( R.id.mediacontroller_progress );
            if ( mProgress != null ) {
                mProgress.setOnSeekBarChangeListener ( mSeekListener );
                mProgress.setMax ( 1000 );
            }
            mEndTime = ( TextView ) v.findViewById ( R.id.totalTime );
            mCurrentTime = ( TextView ) v.findViewById ( R.id.curTime );
            mFormatBuilder = new StringBuilder();
            mFormatter = new Formatter ( mFormatBuilder, Locale.getDefault() );
            mExitButton = ( ImageButton ) v.findViewById ( R.id.back );
            mVolumeButton = ( ImageButton ) v.findViewById ( R.id.VolumeBtn );
            if ( mExitButton != null ) {
                mExitButton.setOnClickListener ( mExitListener );
            }
            if ( mVolumeButton != null ) {
                mVolumeButton.setOnClickListener ( mVolumeListener );
            }
            installPrevNextListeners();
        }

        /**
         * Show the controller on screen. It will go away automatically after 3
         * seconds of inactivity.
         */
        public void show() {
            show ( sDefaultTimeout );
        }

        /**
         * Disable pause or seek buttons if the stream cannot be paused or seeked.
         * This requires the control interface to be a MediaPlayerControlExt
         */
        private void disableUnsupportedButtons() {
            try {
                if ( mPauseButton != null && !mPlayer.canPause() ) {
                    mPauseButton.setEnabled ( false );
                }
                if ( mRewButton != null && !mPlayer.canSeekBackward() ) {
                    mRewButton.setEnabled ( false );
                }
                if ( mFfwdButton != null && !mPlayer.canSeekForward() ) {
                    mFfwdButton.setEnabled ( false );
                }
            } catch ( IncompatibleClassChangeError ex ) {
                // We were given an old version of the interface, that doesn't have
                // the canPause/canSeekXYZ methods. This is OK, it just means we
                // assume the media can be paused and seeked, and so we don't
                // disable
                // the buttons.
            }
        }

        /**
         * Show the controller on screen. It will go away automatically after
         * 'timeout' milliseconds of inactivity.
         *
         * @param timeout
         *            The timeout in milliseconds. Use 0 to show the controller
         *            until hide() is called.
         */
        public void show ( int timeout ) {
            if ( !mShowing && mAnchor != null ) {
                setProgress();
                if ( mPauseButton != null ) {
                    mPauseButton.requestFocus();
                }
                disableUnsupportedButtons();
                updateFloatingWindowLayout();
                mWindowManager.addView ( mDecor, mDecorLayoutParams );
                mShowing = true;
            }
            updatePausePlay();
            // cause the progress bar to be updated even if mShowing
            // was already true. This happens, for example, if we're
            // paused with the progress bar showing the user hits play.
            mHandler.removeMessages(SHOW_PROGRESS);
            mHandler.sendEmptyMessage ( SHOW_PROGRESS );
            Message msg = mHandler.obtainMessage ( FADE_OUT );
            if ( timeout != 0 ) {
                mHandler.removeMessages ( FADE_OUT );
                mHandler.sendMessageDelayed ( msg, timeout );
            }
        }

        public boolean isShowing() {
            return mShowing;
        }

        /**
         * Remove the controller from the screen.
         */
        public void hide() {
            if ( mAnchor == null ) {
                return;
            }
            if ( mShowing ) {
                try {
                    mHandler.removeMessages ( SHOW_PROGRESS );
                    mWindowManager.removeView ( mDecor );
                } catch ( IllegalArgumentException ex ) {
                    Log.i ( "MediaController", "already removed" );
                }
                mShowing = false;
            }
        }

        private Handler mHandler = new Handler() {
            @Override
            public void handleMessage ( Message msg ) {
                int pos;
                switch ( msg.what ) {
                    case FADE_OUT:
                        hide();
                        break;
                    case SHOW_PROGRESS:
                        pos = setProgress();
                        if ( !mDragging && mShowing && mPlayer.isPlaying() ) {
                            msg = obtainMessage ( SHOW_PROGRESS );
                            removeMessages(SHOW_PROGRESS);
                            sendMessageDelayed ( msg, 1000);
                        }
                        break;
                }
            }
        };

        /*
         * private String stringForTime(int timeMs) { int totalSeconds = timeMs /
         * 1000;
         *
         * int seconds = totalSeconds % 60; int minutes = (totalSeconds / 60) % 60;
         * int hours = totalSeconds / 3600;
         *
         * mFormatBuilder.setLength(0); if (hours > 0) { return hours + ":" +
         * minutes + ":" +seconds; //return mFormatter.format("%d:%02d:%02d", hours,
         * minutes, seconds).toString(); } else { // return
         * mFormatter.format("%02d:%02d", minutes, seconds).toString(); return
         * minutes + ":" +seconds; } }
         */
        private String stringForTime ( int tt ) {
            int t = tt / 1000;
            StringBuffer sb = new StringBuffer();
            int hour = t / 3600;
            int min = ( t / 60 ) % 60;
            int sec = t % 60;
            // sb.delete( 0, sb.length() );
            if ( hour > 0 ) {
                if ( hour >= 10 ) {
                    sb.append ( String.valueOf ( hour ) );
                } else {
                    sb.append ( "0" + String.valueOf ( hour ) );
                }
                sb.append ( ":" );
            }
            if ( min >= 10 ) {
                sb.append ( String.valueOf ( min ) );
            } else {
                sb.append ( "0" + String.valueOf ( min ) );
            }
            sb.append ( ":" );
            if ( sec >= 10 ) {
                sb.append ( String.valueOf ( sec ) );
            } else {
                sb.append ( "0" + String.valueOf ( sec ) );
            }
            return sb.toString();
        }

        private int setProgress() {
            if ( mPlayer == null || mDragging ) {
                return 0;
            }
            int position = mPlayer.getCurrentPosition();
            int duration = mPlayer.getDuration();
            if ( mProgress != null ) {
                if ( duration > 0 ) {
                    // use long to avoid overflow
                    long pos = 1000L * position / duration;
                    mProgress.setProgress ( ( int ) pos );
                }
                int percent = mPlayer.getBufferPercentage();
                mProgress.setSecondaryProgress ( percent * 10 );
            }
            if ( mEndTime != null ) {
                if ( duration == 0 ) {
                    mEndTime.setText ( "--:--" );
                } else {
                    mEndTime.setText ( stringForTime ( duration ) );
                }
            }
            if ( mCurrentTime != null ) {
                mCurrentTime.setText ( stringForTime ( position ) );
            }
            return position;
        }

        @Override
        public boolean onTouchEvent ( MotionEvent event ) {
            show ( sDefaultTimeout );
            return true;
        }

        @Override
        public boolean onTrackballEvent ( MotionEvent ev ) {
            show ( sDefaultTimeout );
            return false;
        }

        @Override
        public boolean dispatchKeyEvent ( KeyEvent event ) {
            int keyCode = event.getKeyCode();
            final boolean uniqueDown = event.getRepeatCount() == 0
                                       && event.getAction() == KeyEvent.ACTION_DOWN;
            if ( keyCode == KeyEvent.KEYCODE_HEADSETHOOK
                    || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE
                    || keyCode == KeyEvent.KEYCODE_SPACE ) {
                if ( uniqueDown ) {
                    doPauseResume();
                    show ( sDefaultTimeout );
                    if ( mPauseButton != null ) {
                        mPauseButton.requestFocus();
                    }
                }
                return true;
            }/* else if (keyCode == KeyEvent.KEYCODE_ENTER) {
            if (uniqueDown && !mPlayer.isPlaying()) {
                mPlayer.start();
                updatePausePlay();
                show(sDefaultTimeout);
            } else if (uniqueDown && mPlayer.isPlaying()) {
                mPlayer.pause();
                updatePausePlay();
                show(sDefaultTimeout);
            }
            return true;
        } */else if ( keyCode == KeyEvent.KEYCODE_MEDIA_PLAY ) {
                if ( uniqueDown && !mPlayer.isPlaying() ) {
                    // Intent i = new Intent(mainConst.PLAY);
                    // mContext.sendBroadcast(i);
                    mPlayer.start();
                    updatePausePlay();
                    show ( sDefaultTimeout );
                }
                return true;
            } else if ( keyCode == KeyEvent.KEYCODE_MEDIA_STOP
                        || keyCode == KeyEvent.KEYCODE_MEDIA_PAUSE ) {
                if ( uniqueDown && mPlayer.isPlaying() ) {
                    mPlayer.pause();
                    updatePausePlay();
                    show ( sDefaultTimeout );
                }
                return true;
            } else if ( keyCode == KeyEvent.KEYCODE_VOLUME_DOWN
                        || keyCode == KeyEvent.KEYCODE_VOLUME_UP
                        || keyCode == KeyEvent.KEYCODE_VOLUME_MUTE ) {
                // don't show the controls for volume adjustment
                return super.dispatchKeyEvent ( event );
            } else if ( keyCode == KeyEvent.KEYCODE_BACK
                        || keyCode == KeyEvent.KEYCODE_MENU ) {
                if ( uniqueDown ) {
                    hide();
                }
                return true;
            } /*else if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
            int pos = mPlayer.getCurrentPosition();
            pos += 5000; // milliseconds
            mPlayer.seekTo(pos);
            setProgress();

            show(sDefaultTimeout);
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
            int pos = mPlayer.getCurrentPosition();
            pos -= 5000; // milliseconds
            if (pos < 0)
                pos = 0;
            mPlayer.seekTo(pos);
            setProgress();

            show(sDefaultTimeout);
            return true;
        }*/
            show ( sDefaultTimeout );
            return super.dispatchKeyEvent ( event );
        }

        private View.OnClickListener mPauseListener = new View.OnClickListener() {
            public void onClick ( View v ) {
                Log.d ( "tt", "clicked pauseListener" );
                doPauseResume();
                show ( sDefaultTimeout );
            }
        };

        private void updatePausePlay() {
            if ( mRoot == null || mPauseButton == null ) {
                return;
            }
            if ( mPlayer.isPlaying() ) {
                mPauseButton.setImageResource ( R.drawable.suspend_play );
            } else {
                mPauseButton.setImageResource ( R.drawable.play_play );
            }
        }

        private void doPauseResume() {
            if ( mPlayer.isPlaying() ) {
                mPlayer.pause();
            } else {
                mPlayer.start();
            }
            updatePausePlay();
        }

        // There are two scenarios that can trigger the seekbar listener to trigger:
        //
        // The first is the user using the touchpad to adjust the posititon of the
        // seekbar's thumb. In this case onStartTrackingTouch is called followed by
        // a number of onProgressChanged notifications, concluded by
        // onStopTrackingTouch.
        // We're setting the field "mDragging" to true for the duration of the
        // dragging
        // session to avoid jumps in the position in case of ongoing playback.
        //
        // The second scenario involves the user operating the scroll ball, in this
        // case there WON'T BE onStartTrackingTouch/onStopTrackingTouch
        // notifications,
        // we will simply apply the updated position without suspending regular
        // updates.
        private OnSeekBarChangeListener mSeekListener = new OnSeekBarChangeListener() {
            public void onStartTrackingTouch ( SeekBar bar ) {
                show ( 3600000 );
                mDragging = true;
                // By removing these pending progress messages we make sure
                // that a) we won't update the progress while the user adjusts
                // the seekbar and b) once the user is done dragging the thumb
                // we will post one of these messages to the queue again and
                // this ensures that there will be exactly one message queued up.
                mHandler.removeMessages ( SHOW_PROGRESS );
            }
            public void onProgressChanged ( SeekBar bar, int progress,
            boolean fromuser ) {
                if ( !fromuser ) {
                    // We're not interested in programmatically generated changes to
                    // the progress bar's position.
                    return;
                }
                long duration = mPlayer.getDuration();
                long newposition = ( duration * progress ) / 1000L;
                mPlayer.seekTo ( ( int ) newposition );
                if ( mCurrentTime != null ) {
                    mCurrentTime.setText ( stringForTime ( ( int ) newposition ) );
                }
            }
            public void onStopTrackingTouch ( SeekBar bar ) {
                mDragging = false;
                setProgress();
                updatePausePlay();
                show ( sDefaultTimeout );
                // Ensure that progress is properly updated in the future,
                // the call to show() does not guarantee this because it is a
                // no-op if we are already showing.
                mHandler.removeMessages(SHOW_PROGRESS);
                mHandler.sendEmptyMessage ( SHOW_PROGRESS );
            }
        };

        @Override
        public void setEnabled ( boolean enabled ) {
            if ( mPauseButton != null ) {
                mPauseButton.setEnabled ( enabled );
            }
            if ( mFfwdButton != null ) {
                mFfwdButton.setEnabled ( enabled );
            }
            if ( mRewButton != null ) {
                mRewButton.setEnabled ( enabled );
            }
            if ( mNextButton != null ) {
                mNextButton.setEnabled ( enabled && mNextListener != null );
            }
            if ( mPrevButton != null ) {
                mPrevButton.setEnabled ( enabled && mPrevListener != null );
            }
            if ( mProgress != null ) {
                mProgress.setEnabled ( enabled );
            }
            disableUnsupportedButtons();
            super.setEnabled ( enabled );
        }

        private View.OnClickListener mRewListener = new View.OnClickListener() {
            public void onClick ( View v ) {
                int pos = mPlayer.getCurrentPosition();
                pos -= 15000; // milliseconds
                if ( pos < 0 ) {
                    pos = 0;
                }
                mPlayer.seekTo ( pos );
                setProgress();
                show ( sDefaultTimeout );
            }
        };

        private View.OnClickListener mFfwdListener = new View.OnClickListener() {
            public void onClick ( View v ) {
                int pos = mPlayer.getCurrentPosition();
                pos += 15000; // milliseconds
                mPlayer.seekTo ( pos );
                setProgress();
                show ( sDefaultTimeout );
            }
        };

        private void installPrevNextListeners() {
            if ( mNextButton != null ) {
                mNextButton.setOnClickListener ( mNextListener );
                mNextButton.setEnabled ( mNextListener != null );
            }
            if ( mPrevButton != null ) {
                mPrevButton.setOnClickListener ( mPrevListener );
                mPrevButton.setEnabled ( mPrevListener != null );
            }
        }

        public void setPrevNextListeners ( View.OnClickListener next,
                                           View.OnClickListener prev ) {
            mNextListener = next;
            mPrevListener = prev;
            mListenersSet = true;
            if ( mRoot != null ) {
                installPrevNextListeners();
                if ( mNextButton != null && !mFromXml ) {
                    mNextButton.setVisibility ( View.VISIBLE );
                }
                if ( mPrevButton != null && !mFromXml ) {
                    mPrevButton.setVisibility ( View.VISIBLE );
                }
            }
        }

        public interface MediaPlayerControl {
            void start();

            void pause();

            int getDuration();

            int getCurrentPosition();

            void seekTo ( int pos );

            boolean isPlaying();

            int getBufferPercentage();

            boolean canPause();

            boolean canSeekBackward();

            boolean canSeekForward();
        }
}
