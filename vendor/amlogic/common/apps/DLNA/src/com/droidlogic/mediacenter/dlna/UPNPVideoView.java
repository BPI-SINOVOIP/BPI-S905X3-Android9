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

/**
 * @ClassName UPNPVideoView
 * @Description TODO
 * @Date 2013-8-30
 * @Email
 * @Author
 * @Version V1.0
 */
import com.droidlogic.mediacenter.airplay.util.ApiHelper;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Resources;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnSeekCompleteListener;
import android.net.Uri;
import android.util.AttributeSet;
import org.cybergarage.util.Debug;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.graphics.PixelFormat;
import android.graphics.Canvas;

import java.io.IOException;
import java.util.Map;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
/**
 * Displays a video file.  The UPNPVideoView class
 * can load images from various sources (such as resources or content
 * providers), takes care of computing its measurement from the video so that
 * it can be used in any layout manager, and provides various display options
 * such as scaling and tinting.
 */
public class UPNPVideoView extends SurfaceView implements VideoController.MediaPlayerControl,
        View.OnLayoutChangeListener {
        private String TAG = "UPNPVideoView";
        // settable by the client
        private Uri         mUri;
        private Map<String, String> mHeaders;
        private int         mDuration;

        // all possible internal states
        public static final int STATE_ERROR              = -1;
        public static final int STATE_IDLE               = 0;
        public static final int STATE_PREPARING          = 1;
        public static final int STATE_PREPARED           = 2;
        public static final int STATE_PLAYING            = 3;
        public static final int STATE_PAUSED             = 4;
        public static final int STATE_PLAYBACK_COMPLETED = 5;

        // mCurrentState is a UPNPVideoView object's current state.
        // mTargetState is the state that a method caller intends to reach.
        // For instance, regardless the UPNPVideoView object's current state,
        // calling pause() intends to bring the object to a target state
        // of STATE_PAUSED.
        private int mCurrentState = STATE_IDLE;
        private int mTargetState  = STATE_IDLE;
        private Context mContext;
        // All the stuff we need for playing and showing a video
        private SurfaceHolder mSurfaceHolder = null;
        private MediaPlayer mMediaPlayer = null;
        private int         mVideoWidth;
        private int         mVideoHeight;
        private int         mSurfaceWidth;
        private int         mSurfaceHeight;
        private VideoController mVideoController;
        private OnCompletionListener mOnCompletionListener;
        private MediaPlayer.OnPreparedListener mOnPreparedListener;
        private int         mCurrentBufferPercentage;
        private OnErrorListener mOnErrorListener;
        private OnSeekCompleteListener mSeekCompleteListener;
        private int         mSeekWhenPrepared;  // recording the seek position while preparing
        private boolean     mCanPause;
        private boolean     mCanSeekBack;
        private boolean     mCanSeekForward;
        private boolean     mWaitDisplay;
        public UPNPVideoView ( Context context ) {
            super ( context );
            mContext = context;
            initVideoView();
        }

        public UPNPVideoView ( Context context, AttributeSet attrs ) {
            this ( context, attrs, 0 );
            mContext = context;
            initVideoView();
        }

        public UPNPVideoView ( Context context, AttributeSet attrs, int defStyle ) {
            super ( context, attrs, defStyle );
            mContext = context;
            initVideoView();
        }

        /** get current hdmi display mode*/
        private static final String MODE_PATH = "/sys/class/display/mode";
        private boolean chkIfHdmiMode() {
            String modeStr = null;
            File file = new File ( MODE_PATH );
            if ( !file.exists() ) {
                return false;
            }
            try {
                BufferedReader reader = new BufferedReader ( new FileReader ( MODE_PATH ), 32 );
                try {
                    modeStr = reader.readLine();
                } finally {
                    reader.close();
                }
                if ( modeStr == null )
                { return false; }
            } catch ( IOException e ) {
                Debug.e ( "VideoController", "IO Exception when read: " + MODE_PATH, e );
                return false;
            }
            if ( modeStr.equals ( "panel" ) )
            { return false; }
            else
            { return true; }
        }

        @Override
        protected void onMeasure ( int widthMeasureSpec, int heightMeasureSpec ) {
            //Debug.d("@@@@", "onMeasure");
            int width = getDefaultSize ( mVideoWidth, widthMeasureSpec );
            int height = getDefaultSize ( mVideoHeight, heightMeasureSpec );
            if ( mVideoWidth > 0 && mVideoHeight > 0 ) {
                //    if( mMediaPlayer!=null &&
                //      mMediaPlayer.getIntParameter(MediaPlayer.KEY_PARAMETER_AML_PLAYER_VIDEO_OUT_TYPE)==MediaPlayer.VIDEO_OUT_HARDWARE &&
                //      chkIfHdmiMode()){
                //      /*is hardware mode,we used max size layout now
                //      always used the max full size.
                //      if not HDMI,we may play in windows
                //      */
                //    }else{
                if ( mVideoWidth * height  > width * mVideoHeight ) {
                    //Debug.d("@@@", "image too tall, correcting");
                    height = width * mVideoHeight / mVideoWidth;
                } else if ( mVideoWidth * height  < width * mVideoHeight ) {
                    //Debug.d("@@@", "image too wide, correcting");
                    width = height * mVideoWidth / mVideoHeight;
                } else {
                    //Debug.d("@@@", "aspect ratio is correct: " +
                    //width+"/"+height+"="+
                    //mVideoWidth+"/"+mVideoHeight);
                }
                //      }
            }
            //Debug.d("@@@@@@@@@@", "setting size: " + width + 'x' + height);
            setMeasuredDimension ( width, height );
        }

        public int resolveAdjustedSize ( int desiredSize, int measureSpec ) {
            int result = desiredSize;
            int specMode = MeasureSpec.getMode ( measureSpec );
            int specSize =  MeasureSpec.getSize ( measureSpec );
            switch ( specMode ) {
                case MeasureSpec.UNSPECIFIED:
                    /* Parent says we can be as big as we want. Just don't be larger
                     * than max size imposed on ourselves.
                     */
                    result = desiredSize;
                    break;
                case MeasureSpec.AT_MOST:
                    /* Parent says we can be as big as we want, up to specSize.
                     * Don't be larger than specSize, and don't be larger than
                     * the max size imposed on ourselves.
                     */
                    result = Math.min ( desiredSize, specSize );
                    break;
                case MeasureSpec.EXACTLY:
                    // No choice. Do what we are told.
                    result = specSize;
                    break;
            }
            return result;
        }

        private void initVideoView() {
            mVideoWidth = 0;
            mVideoHeight = 0;
            getHolder().addCallback ( mSHCallback );
            getHolder().setFormat ( 5 ); //PixelFormat.VIDEO_HOLE which is not in pdk
            getHolder().setType ( SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS );
            addOnLayoutChangeListener ( this );
            setFocusable ( true );
            setFocusableInTouchMode ( true );
            requestFocus();
            mCurrentState = STATE_IDLE;
            mTargetState  = STATE_IDLE;
        }

        public void setVideoPath ( String path ) {
            String realpath = path;
            if (realpath.contains("?formatID=")) {
               realpath = path.substring(0,path.indexOf("?formatID"));
            }
            setVideoURI ( Uri.parse ( realpath ) );
        }

        public void setVideoURI ( Uri uri ) {
            setVideoURI ( uri, null );
        }

        /**
         * @hide
         */
        public void setVideoURI ( Uri uri, Map<String, String> headers ) {
            mUri = uri;
            mHeaders = headers;
            mSeekWhenPrepared = 0;
            if ( mSurfaceHolder == null ) {
                mWaitDisplay = true;
            }else {
                openVideo();
                mWaitDisplay = false;
            }

            requestLayout();
            invalidate();
        }

        public void stopPlayback() {
            if ( mMediaPlayer != null ) {
                mMediaPlayer.stop();
                mMediaPlayer.reset();
                //mMediaPlayer.release();
                //mMediaPlayer = null;
                mCurrentState = STATE_IDLE;
                mTargetState  = STATE_IDLE;
                if ( mOnStateChangedListener != null ) {
                    mOnStateChangedListener.onStateChanged ( mCurrentState );
                }
            }
        }

        private void openVideo() {
            // Tell the music playback service to pause
            // TODO: these constants need to be published somewhere in the framework.
            Intent i = new Intent ( "com.android.music.musicservicecommand" );
            i.putExtra ( "command", "pause" );
            mContext.sendBroadcast ( i );
            // we shouldn't clear the target state, because somebody might have
            // called start() previously
            release ( false );
            try {
                mMediaPlayer = new MediaPlayer();
                mMediaPlayer.setOnPreparedListener ( mPreparedListener );
                mMediaPlayer.setOnVideoSizeChangedListener ( mSizeChangedListener );
                mMediaPlayer.setOnSeekCompleteListener ( mSeekCompleteListener );
                mDuration = -1;
                mMediaPlayer.setOnCompletionListener ( mCompletionListener );
                mMediaPlayer.setOnErrorListener ( mErrorListener );
                mMediaPlayer.setOnBufferingUpdateListener ( mBufferingUpdateListener );
                mCurrentBufferPercentage = 0;
                mMediaPlayer.setDataSource ( mContext, mUri, mHeaders );
                mMediaPlayer.setDisplay( mSurfaceHolder );
                mMediaPlayer.setScreenOnWhilePlaying ( true );
                mMediaPlayer.setAudioStreamType ( AudioManager.STREAM_MUSIC );
               // mMediaPlayer.prepare();
                mMediaPlayer.prepareAsync();
                // we don't set the target state here either, but preserve the
                // target state that was there before.
                mCurrentState = STATE_PREPARING;
                if ( mOnStateChangedListener != null ) {
                    mOnStateChangedListener.onStateChanged ( mCurrentState );
                }
                attachVideoController();
            } catch ( IOException ex ) {
                Debug.e ( TAG, "Unable to open content: " + mUri, ex );
                mCurrentState = STATE_ERROR;
                mTargetState = STATE_ERROR;
                if ( mOnStateChangedListener != null ) {
                    mOnStateChangedListener.onStateChanged ( mCurrentState );
                }
                mErrorListener.onError ( mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0 );
                return;
            } catch ( IllegalArgumentException ex ) {
                Debug.e ( TAG, "Unable to open content: " + mUri, ex );
                mCurrentState = STATE_ERROR;
                mTargetState = STATE_ERROR;
                if ( mOnStateChangedListener != null ) {
                    mOnStateChangedListener.onStateChanged ( mCurrentState );
                }
                mErrorListener.onError ( mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0 );
                return;
            }
        }

        public void setVideoController ( VideoController controller ) {
            if ( mVideoController != null ) {
                mVideoController.hide();
            }
            mVideoController = controller;
            attachVideoController();
        }

        private void attachVideoController() {
            if ( mMediaPlayer != null && mVideoController != null ) {
                mVideoController.setMediaPlayer ( this );
                View anchorView = this.getParent() instanceof View ?
                                  ( View ) this.getParent() : this;
                mVideoController.setAnchorView ( anchorView );
                mVideoController.setEnabled ( isInPlaybackState() );
            }
        }

        MediaPlayer.OnVideoSizeChangedListener mSizeChangedListener =
        new MediaPlayer.OnVideoSizeChangedListener() {
            public void onVideoSizeChanged ( MediaPlayer mp, int width, int height ) {
                mVideoWidth = mp.getVideoWidth();
                mVideoHeight = mp.getVideoHeight();
                Debug.d ( TAG, "***onVideoSizeChanged: (" + mVideoWidth + ", " + mVideoHeight + ")" );
                if ( mVideoWidth != 0 && mVideoHeight != 0 ) {
                    getHolder().setFixedSize ( mVideoWidth, mVideoHeight );
                }
            }
        };

        MediaPlayer.OnPreparedListener mPreparedListener = new MediaPlayer.OnPreparedListener() {
            public void onPrepared ( MediaPlayer mp ) {
                mCurrentState = STATE_PREPARED;
                if ( mOnStateChangedListener != null ) {
                    mOnStateChangedListener.onStateChanged ( mCurrentState );
                }
                /*Debug.d(TAG,"MediaPlayer:"+
                    mp.getStringParameter(MediaPlayer.KEY_PARAMETER_AML_PLAYER_TYPE_STR)
                    +" Prepared");*/
                // Get the capabilities of the player for this stream
                mCanPause = mCanSeekBack = mCanSeekForward = true;
                if ( mOnPreparedListener != null ) {
                    mOnPreparedListener.onPrepared ( mMediaPlayer );
                }
                if ( mVideoController != null ) {
                    mVideoController.setEnabled ( true );
                }
                mVideoWidth = mp.getVideoWidth();
                mVideoHeight = mp.getVideoHeight();
                /*int seekToPosition = mSeekWhenPrepared;  // mSeekWhenPrepared may be changed after seekTo() call
                if ( seekToPosition != 0 ) {
                    seekTo ( seekToPosition );
                }*/
                if ( mVideoWidth != 0 && mVideoHeight != 0 ) {
                    //Debug.d("@@@@", "video size: " + mVideoWidth +"/"+ mVideoHeight);
                    getHolder().setFixedSize ( mVideoWidth, mVideoHeight );
                    if ( mSurfaceWidth == mVideoWidth && mSurfaceHeight == mVideoHeight ) {
                        // We didn't actually change the size (it was already at the size
                        // we need), so we won't get a "surface changed" callback, so
                        // start the video here instead of in the callback.
                        if ( mTargetState == STATE_PLAYING ) {
                            start();
                            /*if (mVideoController != null) {
                                mVideoController.show();
                            }*/
                        } else if ( !isPlaying() &&
                        ( /*seekToPosition != 0 ||*/ getCurrentPosition() > 0 ) ) {
                            if ( mVideoController != null ) {
                                // Show the media controls when we're paused into a video and make 'em stick.
                                mVideoController.show ( 0 );
                            }
                        }
                    }
                } else {
                    // We don't know the video size yet, but should start anyway.
                    // The video size might be reported to us later.
                    if ( mTargetState == STATE_PLAYING ) {
                        start();
                    }
                }
            }
        };

        private MediaPlayer.OnCompletionListener mCompletionListener =
        new MediaPlayer.OnCompletionListener() {
            public void onCompletion ( MediaPlayer mp ) {
                mCurrentState = STATE_PLAYBACK_COMPLETED;
                mTargetState = STATE_PLAYBACK_COMPLETED;
                if ( mVideoController != null ) {
                    mVideoController.hide();
                }
                if ( mOnCompletionListener != null ) {
                    mOnCompletionListener.onCompletion ( mMediaPlayer );
                }
                if ( mOnStateChangedListener != null ) {
                    mOnStateChangedListener.onStateChanged ( mCurrentState );
                }
            }
        };

        private MediaPlayer.OnErrorListener mErrorListener =
        new MediaPlayer.OnErrorListener() {
            public boolean onError ( MediaPlayer mp, int framework_err, int impl_err ) {
                Debug.d ( TAG, "Error: " + framework_err + "," + impl_err );
                mCurrentState = STATE_ERROR;
                mTargetState = STATE_ERROR;
                if ( mOnStateChangedListener != null ) {
                    mOnStateChangedListener.onStateChanged ( mCurrentState );
                }
                if ( mVideoController != null ) {
                    mVideoController.hide();
                }
                /* If an error handler has been supplied, use it and finish. */
                if ( mOnErrorListener != null ) {
                    if ( mOnErrorListener.onError ( mMediaPlayer, framework_err, impl_err ) ) {
                        return true;
                    }
                }
                if ( mSeekCompleteListener != null ) {
                    mSeekCompleteListener.onSeekComplete ( mMediaPlayer );
                }
                /* Otherwise, pop up an error dialog so the user knows that
                 * something bad has happened. Only try and pop up the dialog
                 * if we're attached to a window. When we're going away and no
                 * longer have a window, don't bother showing the user an error.
                 */
                if ( getWindowToken() != null ) {
                    Resources r = mContext.getResources();
                    int messageId;
                    if ( framework_err == MediaPlayer.MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK ) {
                        messageId = (int)PrefUtils.getResource("com.android.internal.R.string","VideoView_error_text_invalid_progressive_playback");
                    } else {
                        messageId = (int)PrefUtils.getResource("com.android.internal.R.string","VideoView_error_text_unknown");
                    }
                    new AlertDialog.Builder ( mContext )
                    .setTitle ( (int)PrefUtils.getResource("com.android.internal.R.string","VideoView_error_title") )
                    .setMessage ( messageId )
                    .setPositiveButton (  (int)PrefUtils.getResource("com.android.internal.R.string","VideoView_error_button"),
                    new DialogInterface.OnClickListener() {
                        public void onClick ( DialogInterface dialog, int whichButton ) {
                            /* If we get here, there is no onError listener, so
                             * at least inform them that the video is over.
                             */
                            if ( mOnCompletionListener != null ) {
                                mOnCompletionListener.onCompletion ( mMediaPlayer );
                            }
                        }
                    } )
                    //.setCancelable(false)
                    .show();
                }
                return true;
            }
        };

        private MediaPlayer.OnBufferingUpdateListener mBufferingUpdateListener =
        new MediaPlayer.OnBufferingUpdateListener() {
            public void onBufferingUpdate ( MediaPlayer mp, int percent ) {
                mCurrentBufferPercentage = percent;
            }
        };

        /**
         * Register a callback to be invoked when the media file
         * is loaded and ready to go.
         *
         * @param l The callback that will be run
         */
        public void setOnPreparedListener ( MediaPlayer.OnPreparedListener l ) {
            mOnPreparedListener = l;
        }

        /**
         * Register a callback to be invoked when the end of a media file
         * has been reached during playback.
         *
         * @param l The callback that will be run
         */
        public void setOnCompletionListener ( OnCompletionListener l ) {
            mOnCompletionListener = l;
        }

        /**
         * Register a callback to be invoked when an error occurs
         * during playback or setup.  If no listener is specified,
         * or if the listener returned false, VideoView will inform
         * the user of any errors.
         *
         * @param l The callback that will be run
         */
        public void setOnErrorListener ( OnErrorListener l ) {
            mOnErrorListener = l;
        }

        public void setOnSeekCompleteListener ( OnSeekCompleteListener l ) {
            mSeekCompleteListener = l;
        }
        /**
         * Interface definition for a callback to be invoked when the media
         * play state is changed.
         */
        public interface OnStateChangedListener {
            /**
             * Called when the media play state is changed.
             *
             * @param state the MediaPlayer state
             */
            void onStateChanged ( int state );
        }

        /**
         * Register a callback to be invoked when the media
         * play state is changed.
         *
         * @param listener the callback that will be run
         */
        public void setOnStateChangedListener ( OnStateChangedListener listener ) {
            mOnStateChangedListener = listener;
        }

        private OnStateChangedListener mOnStateChangedListener;


        /**
         * Interface definition for a callback to be invoked when the media
         * play progress is changed.
         */
        public interface OnProgressListener {
            /**
             * Called when the media play progress is changed.
             *
             * @param cur the current play time
             *
             * @param total the total duration
             */
            void onProgress ( int cur, int total );
        }

        /**
         * Register a callback to be invoked when the media
         * play progress is changed.
         *
         * @param listener the callback that will be run
         */
        public void setOnProgressListener ( OnProgressListener listener ) {
            mOnProgressListener = listener;
        }

        private OnProgressListener mOnProgressListener;

        SurfaceHolder.Callback mSHCallback = new SurfaceHolder.Callback() {
            private void initSurface ( SurfaceHolder h ) {
                // for TV platform
                // for mbx, there may be some video enlarge, and the subtitle is only shown half
                /*  Canvas c = null;
                  try {
                    Debug.d(TAG, "initSurface");
                    c = h.lockCanvas();
                  } finally {
                    if (c != null)
                      h.unlockCanvasAndPost(c);
                  }        */
            }
            public void surfaceChanged ( SurfaceHolder holder, int format,
            int w, int h ) {
                mSurfaceHolder = holder;
                /*mSurfaceWidth = w;
                mSurfaceHeight = h;
                boolean isValidState =  ( mTargetState == STATE_PLAYING );
                boolean hasValidSize = ( mVideoWidth == w && mVideoHeight == h );
                if ( mMediaPlayer != null && isValidState && hasValidSize ) {
                    if ( mSeekWhenPrepared != 0 ) {
                        seekTo ( mSeekWhenPrepared );
                    }
                    start();
                }*/
                initSurface ( holder );
            }
            public void surfaceCreated ( SurfaceHolder holder ) {
                mSurfaceHolder = holder;
                if (mWaitDisplay) {
                    openVideo();
                    mWaitDisplay = false;
                }
                initSurface ( holder );
            }
            public void surfaceDestroyed ( SurfaceHolder holder ) {
                // after we return from this we can't use the surface any more
                mSurfaceHolder = null;
                if ( mVideoController != null ) { mVideoController.hide(); }
                stopPlayback();
                release ( true );
            }
        };

        /*
         * release the media player in any state
         */
        private void release ( boolean cleartargetstate ) {
            if ( mMediaPlayer != null ) {
                mMediaPlayer.reset();
                mMediaPlayer.release();
                mMediaPlayer = null;
                mCurrentState = STATE_IDLE;
                if ( mOnStateChangedListener != null ) {
                    mOnStateChangedListener.onStateChanged ( mCurrentState );
                }
                if ( cleartargetstate ) {
                    mTargetState  = STATE_IDLE;
                }
            }
        }
        @Override
        public void onLayoutChange ( View v, int left, int top, int right, int bottom,
                                     int oldLeft, int oldTop, int oldRight, int oldBottom ) {
            int Rotation = 0;
            Debug.d ( TAG, "Layout changed,left=" + left + " top=" + top + " right=" + right + " bottom=" + bottom );
            Debug.d ( TAG, "Layout changed,oldLeft=" + oldLeft + " oldTop=" + oldTop + " oldRight=" + oldRight + " oldBottom=" + oldBottom );
            if ( mMediaPlayer != null ) {
                StringBuilder builder = new StringBuilder();;
                builder.append ( ".left=" + left );
                builder.append ( ".top=" + top );
                builder.append ( ".right=" + right );
                builder.append ( ".bottom=" + bottom );
                builder.append ( ".oldLeft=" + oldLeft );
                builder.append ( ".oldTop=" + oldTop );
                builder.append ( ".oldRight=" + oldRight );
                builder.append ( ".oldBottom=" + oldBottom );
                builder.append ( ".Rotation=" + Rotation );
                Debug.d ( TAG, builder.toString() );
                //mMediaPlayer.setParameter(MediaPlayer.KEY_PARAMETER_AML_VIDEO_POSITION_INFO,builder.toString());
            }
        }
        @Override
        public boolean onTouchEvent ( MotionEvent ev ) {
            if ( isInPlaybackState() && mVideoController != null ) {
                toggleMediaControlsVisiblity();
            }
            return false;
        }
        private void showSystemUi ( boolean visible ) {
            if ( !ApiHelper.HAS_VIEW_SYSTEM_UI_FLAG_LAYOUT_STABLE )
            { return; }
            int flag = View.SYSTEM_UI_FLAG_VISIBLE;
            if ( !visible ) {
                // We used the deprecated "STATUS_BAR_HIDDEN" for unbundling
                flag |= View.STATUS_BAR_HIDDEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
            }
            this.setSystemUiVisibility ( flag );
        }


        @Override
        public boolean onTrackballEvent ( MotionEvent ev ) {
            if ( isInPlaybackState() && mVideoController != null ) {
                toggleMediaControlsVisiblity();
            }
            return false;
        }

        @Override
        public boolean onKeyDown ( int keyCode, KeyEvent event ) {
            boolean isKeyCodeSupported = keyCode != KeyEvent.KEYCODE_BACK &&
                                         keyCode != KeyEvent.KEYCODE_VOLUME_UP &&
                                         keyCode != KeyEvent.KEYCODE_VOLUME_DOWN &&
                                         keyCode != KeyEvent.KEYCODE_VOLUME_MUTE &&
                                         keyCode != KeyEvent.KEYCODE_MENU &&
                                         keyCode != KeyEvent.KEYCODE_CALL &&
                                         keyCode != KeyEvent.KEYCODE_ENDCALL;
            if ( isInPlaybackState() && isKeyCodeSupported && mVideoController != null ) {
                if ( keyCode == KeyEvent.KEYCODE_HEADSETHOOK ||
                        keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE ) {
                    if ( mMediaPlayer.isPlaying() ) {
                        pause();
                        mVideoController.show();
                    } else {
                        start();
                        mVideoController.hide();
                    }
                    return true;
                } else if ( keyCode == KeyEvent.KEYCODE_MEDIA_PLAY ) {
                    if ( !mMediaPlayer.isPlaying() ) {
                        start();
                        mVideoController.hide();
                    }
                    return true;
                } else if ( keyCode == KeyEvent.KEYCODE_MEDIA_STOP ) {
                    if ( isInPlaybackState() ) {
                        mMediaPlayer.seekTo ( 0 );
                        if ( mMediaPlayer.isPlaying() ) {
                            pause();
                        }
                        mVideoController.show();
                    }
                    return true;
                } else if ( keyCode == KeyEvent.KEYCODE_MEDIA_PAUSE ) {
                    if ( isPlaying() ) {
                        pause();
                        mVideoController.show();
                    }
                    return true;
                } else {
                    toggleMediaControlsVisiblity();
                }
            }
            return super.onKeyDown ( keyCode, event );
        }

        private void toggleMediaControlsVisiblity() {
            if ( mVideoController.isShowing() ) {
                mVideoController.hide();
            } else {
                mVideoController.show();
            }
        }

        public void start() {
            if ( isInPlaybackState() ) {
                mMediaPlayer.start();
                mCurrentState = STATE_PLAYING;
                if ( mOnStateChangedListener != null ) {
                    mOnStateChangedListener.onStateChanged ( mCurrentState );
                }
            }
            mTargetState = STATE_PLAYING;
        }

        public void pause() {
            if ( isInPlaybackState() ) {
                if ( mMediaPlayer.isPlaying() ) {
                    mMediaPlayer.pause();
                    mCurrentState = STATE_PAUSED;
                    if ( mOnStateChangedListener != null ) {
                        mOnStateChangedListener.onStateChanged ( mCurrentState );
                    }
                }
            }
            mTargetState = STATE_PAUSED;
        }

        public void suspend() {
            release ( false );
        }

        public void resume() {
            openVideo();
        }

        // cache duration as mDuration for faster access
        public int getDuration() {
            if ( isInPlaybackState() ) {
                if ( mDuration > 0 ) {
                    return mDuration;
                }
                mDuration = mMediaPlayer.getDuration();
                return mDuration;
            }
            mDuration = -1;
            return mDuration;
        }

        public int getCurrentPosition() {
            if ( isInPlaybackState() ) {
                return mMediaPlayer.getCurrentPosition();
            }
            return 0;
        }

        public void seekTo ( int msec ) {
            if ( isInPlaybackState() ) {
                mMediaPlayer.seekTo ( msec );
                //mSeekWhenPrepared = 0;
            }/* else {
                mSeekWhenPrepared = msec;
            }*/
        }

        public boolean isPlaying() {
            return isInPlaybackState() && mMediaPlayer.isPlaying();
        }

        public int getBufferPercentage() {
            if ( mMediaPlayer != null ) {
                return mCurrentBufferPercentage;
            }
            return 0;
        }

        public boolean isInPlaybackState() {
            return ( mMediaPlayer != null &&
                     mCurrentState != STATE_ERROR &&
                     mCurrentState != STATE_IDLE &&
                     mCurrentState != STATE_PREPARING );
        }

        public boolean canPause() {
            return mCanPause;
        }

        public boolean canSeekBackward() {
            return mCanSeekBack;
        }

        public boolean canSeekForward() {
            return mCanSeekForward;
        }
}
