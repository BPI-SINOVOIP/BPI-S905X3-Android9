package js.kbars;

import android.content.Context;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnPreparedListener;
import android.net.Uri;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnSystemUiVisibilityChangeListener;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import android.widget.VideoView;

public final class MediaModeToggleButton extends Button {
    private static final int MEDIA_FLAGS = 1798;
    private final FrameLayout mFrame;
    private boolean mMediaMode;
    private VideoView mVideo;

    public MediaModeToggleButton(Context context, FrameLayout frame) {
        super(context);
        this.mFrame = frame;
        setText("Enter media mode");
        setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                MediaModeToggleButton.this.setSystemUiVisibility(MediaModeToggleButton.MEDIA_FLAGS);
            }
        });
        setOnSystemUiVisibilityChangeListener(new OnSystemUiVisibilityChangeListener() {
            public void onSystemUiVisibilityChange(int vis) {
                MediaModeToggleButton.this.updateSysUi(vis);
            }
        });
        initVideo();
        updateSysUi(0);
    }

    private void initVideo() {
        this.mVideo = new VideoView(getContext());
        this.mVideo.setVisibility(8);
        LayoutParams videoLP = new LayoutParams(-1, -1);
        videoLP.gravity = 17;
        this.mFrame.addView(this.mVideo, videoLP);
        this.mVideo.setOnPreparedListener(new OnPreparedListener() {
            public void onPrepared(MediaPlayer mp) {
                mp.setLooping(true);
            }
        });
        this.mVideo.setVideoURI(Uri.parse("android.resource://" + getContext().getPackageName() + "/" + R.raw.clipcanvas));
        this.mVideo.setBackgroundColor(0);
    }

    private void updateSysUi(int vis) {
        boolean requested;
        boolean hideStatus;
        boolean hideNav;
        boolean hideSomething;
        if (getSystemUiVisibility() == MEDIA_FLAGS) {
            requested = true;
        } else {
            requested = false;
        }
        if ((vis & 4) != 0) {
            hideStatus = true;
        } else {
            hideStatus = false;
        }
        if ((vis & 2) != 0) {
            hideNav = true;
        } else {
            hideNav = false;
        }
        if (hideStatus || hideNav) {
            hideSomething = true;
        } else {
            hideSomething = false;
        }
        Log.d(KBarsActivity.TAG, String.format("vis change hideStatus=%s hideNav=%s hideSomething=%s mMediaMode=%s", new Object[]{Boolean.valueOf(hideStatus), Boolean.valueOf(hideNav), Boolean.valueOf(hideSomething), Boolean.valueOf(this.mMediaMode)}));
        this.mMediaMode = false;
        if (requested && hideStatus && hideNav) {
            this.mMediaMode = true;
        } else {
            setSystemUiVisibility(KBarsActivity.BASE_FLAGS);
        }
        if (this.mMediaMode) {
            this.mVideo.setVisibility(0);
            this.mVideo.start();
            return;
        }
        this.mVideo.setVisibility(8);
        this.mVideo.stopPlayback();
    }
}
