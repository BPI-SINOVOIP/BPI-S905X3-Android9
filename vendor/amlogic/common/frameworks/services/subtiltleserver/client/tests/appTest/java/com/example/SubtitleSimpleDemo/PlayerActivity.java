package com.example.SubtitleSimpleDemo;

import android.media.AudioManager;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;


import android.app.Activity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.os.*;

import com.droidlogic.app.MediaPlayerExt;

import android.media.MediaPlayer;
import android.media.MediaPlayer.OnBufferingUpdateListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaPlayer.OnVideoSizeChangedListener;

import java.io.UnsupportedEncodingException;
import java.lang.Thread.UncaughtExceptionHandler;

public class PlayerActivity extends Activity implements
        OnBufferingUpdateListener, OnCompletionListener,
        OnPreparedListener, OnVideoSizeChangedListener, SurfaceHolder.Callback {
    private static final String TAG = "PlayerActivity";
    private String mFilePath;

    private MediaPlayerExt mMediaPlayer;
    MediaPlayer.TrackInfo[] mTrackInfo;

    private SurfaceView mVideoView;
    private SurfaceHolder mHolder;
    private int mVideoWidth;
    private int mVideoHeight;

    private Button mStartBtn;
    private Button mShowBtn;
    private Button mHideBtn;
    private Button mSetRegionBtn;
    private EditText mStartXET;
    private EditText mStartYET;
    private EditText mWidthET;
    private EditText mHeightET;
    private SubtitleAPI mApiWrapper;

    //Teletext related:
    private EditText mTTPageIdET;
    private Button mTTGoPageBtn;
    private Button mTTNextPageBtn;
    private Button mTTNextSubPageBtn;
    private Button mTTGoHomeBtn;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_player);
        mFilePath = getIntent().getStringExtra("filePath");

        // init video view!
        mVideoView = (SurfaceView) findViewById(R.id.videoView);
        mHolder = mVideoView.getHolder();
        mHolder.addCallback(this);
        mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

        ///// Init UI
        mStartBtn = (Button) findViewById(R.id.start_play);
        mShowBtn = (Button) findViewById(R.id.show_btn);
        mHideBtn = (Button) findViewById(R.id.hide_btn);
        mSetRegionBtn = (Button) findViewById(R.id.set_region);
        mStartBtn.setOnClickListener(mBtnOnClickListener);
        mShowBtn.setOnClickListener(mBtnOnClickListener);
        mHideBtn.setOnClickListener(mBtnOnClickListener);
        mStartXET = (EditText) findViewById(R.id.startX);
        mStartYET = (EditText) findViewById(R.id.startY);
        mWidthET = (EditText) findViewById(R.id.width);
        mHeightET = (EditText) findViewById(R.id.height);
        mSetRegionBtn.setOnClickListener(mBtnOnClickListener);

        //TeleText
        mTTPageIdET = (EditText) findViewById(R.id.teletext_page);
        mTTGoPageBtn = (Button) findViewById(R.id.goto_page);
        mTTNextPageBtn = (Button) findViewById(R.id.next_page);
        mTTNextSubPageBtn = (Button) findViewById(R.id.next_sub_page);
        mTTGoHomeBtn = (Button) findViewById(R.id.tt_go_home);
        mTTGoPageBtn.setOnClickListener(mBtnOnClickListener);
        mTTNextSubPageBtn.setOnClickListener(mBtnOnClickListener);
        mTTNextPageBtn.setOnClickListener(mBtnOnClickListener);
        mTTGoHomeBtn.setOnClickListener(mBtnOnClickListener);
        mApiWrapper = new SubtitleAPI();
        mStartBtn.requestFocus();

        mMediaPlayer = new MediaPlayerExt();
        mApiWrapper.subtitleCreate();
    }

    @Override
    protected void onStop() {
        super.onStop();

        // exit, then we stop the play.
        mApiWrapper.subtitleClose();
        mApiWrapper.subtitleDestroy();
    }

    private View.OnClickListener mBtnOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (v == mStartBtn) {
                Log.d(TAG, "here, start play");

                playVideo();
            } else if (v == mShowBtn) {
                Log.d(TAG, "here, show subtitle");
                mApiWrapper.show();
            } else if (v == mHideBtn) {
                Log.d(TAG, "here, hide subtitle");
                mApiWrapper.hide();
            } else if (v == mSetRegionBtn) {
                mApiWrapper.setDisplayRect(0, 0, 800, 600);
            } else if (v == mTTGoPageBtn) {
                int page = Integer.parseInt(mTTPageIdET.getText().toString());
                mApiWrapper.teletextGotoPage(page, 0);
            } else if (v == mTTNextPageBtn) {
                Log.d(TAG, "mTTNextPageBtn button pressed!");
                mApiWrapper.teletextNextPage(true);
            } else if (v == mTTNextSubPageBtn) {
                Log.d(TAG, "mTTNextSubPageBtn button pressed!");
                mApiWrapper.teletextNextSubPage(true);
            } else if (v == mTTGoHomeBtn) {
                Log.d(TAG, "GoHome button pressed!");
                mApiWrapper.teletextGoHome();
            }
        }
    };


    private void startSubtitlePlay(){
        // Here, we simply use MediaPlayer to play, it use softdemux.
        // currently, softdemux need start subtitle first.
        // Middleware hardware demux no such limitation.
        mApiWrapper.subtitleDvbSubs(
                SubtitleAPI.E_SUBTITLE_FMQ, // softdemux ust fast message queue. middleware typically use E_SUBTITLE_DEMUX
                9, // 5 means DVB subtitle. middlware must provide, FMQ ignore this, no need, any value is ok.
                0, // only hwdemux need subtitle pid from ts stream
                0, // only hwdemux need anc id from ts stream
                0 // only hwdemux need comp id from ts stream
        );
        mApiWrapper.show();
    }

    private void stopSubtitlePlay() {
        mApiWrapper.hide();
        mApiWrapper.subtitleClose();
    }


    private void playVideo() {
        try {
            // Create a new media player and set the listeners
            mMediaPlayer.setDataSource (this, Uri.parse(mFilePath), null);
            mMediaPlayer.setDisplay(mHolder);

            mMediaPlayer.setUseLocalExtractor(mMediaPlayer);
            mMediaPlayer.prepare();
            mMediaPlayer.setOnBufferingUpdateListener(this);
            mMediaPlayer.setOnCompletionListener(this);
            mMediaPlayer.setOnPreparedListener(this);
            mMediaPlayer.setOnVideoSizeChangedListener(this);
            //mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        } catch (Exception e) {
            Log.e(TAG, "error: " + e.getMessage(), e);
        }
    }

    @Override
    public void onBufferingUpdate(MediaPlayer mp, int percent) {
        Log.d(TAG, "onBufferingUpdate called");
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        Log.d(TAG, "onCompletion called");
        mMediaPlayer.stop();
        mMediaPlayer.reset();
        Log.d(TAG, "onCompletion called stopped");
        stopSubtitlePlay();

        try {
            mMediaPlayer.setDataSource (this, Uri.parse(mFilePath), null);
            mMediaPlayer.setUseLocalExtractor(mMediaPlayer);
            mMediaPlayer.prepare();
        } catch (Exception ex) {
            Log.d(TAG, "Unable to open  ex:", ex);
        }
    }

    @Override
    public void onPrepared(MediaPlayer mp) {
        Log.d(TAG, "onPrepared called mVideoView=" + mVideoView);
        mTrackInfo = mp.getTrackInfo();
        if (mTrackInfo != null) {
            for (int j = 0; j < mTrackInfo.length; j++) {
                if (mTrackInfo[j] != null) {
                    int trackType = mTrackInfo[j].getTrackType();
                    Log.d(TAG, "get Track, track info="+mTrackInfo[j]);
                    if (trackType == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_SUBTITLE || trackType == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT) {
                        Log.d(TAG, "get  subtitle Track, track info="+mTrackInfo[j]);
                    }
                }

            }
        }

        mHolder.setFixedSize(mVideoWidth, mVideoHeight);
        mMediaPlayer.start();
        startSubtitlePlay();


        mStartBtn.requestFocus();
    }

    @Override
    public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
        Log.d(TAG, "onVideoSizeChanged called width="+width +", height="+height);
        mVideoWidth = width;
        mVideoHeight = height;

        mApiWrapper.setDisplayRect(mVideoView.getLeft(), mVideoView.getTop(),
                mVideoView.getRight()-mVideoView.getLeft(), mVideoView.getBottom()-mVideoView.getTop());

        // also update the default container.
        mStartXET.setText(Integer.toString(mVideoView.getLeft()));
        mStartYET.setText(Integer.toString(mVideoView.getTop()));
        mWidthET.setText(Integer.toString(mVideoView.getRight()-mVideoView.getLeft()));
        mHeightET.setText(Integer.toString(mVideoView.getBottom()-mVideoView.getTop()));
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surfaceCreated called");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "surfaceChanged called");
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "surfaceDestroyed called");
    }
}
