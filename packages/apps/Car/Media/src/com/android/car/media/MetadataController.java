package com.android.car.media;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.media.session.PlaybackState;
import android.support.v4.media.session.PlaybackStateCompat;
import android.view.View;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;

import com.android.car.media.common.MediaItemMetadata;
import com.android.car.media.common.PlaybackModel;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
 * Common controller for displaying current track's metadata.
 */
public class MetadataController {

    private static final DateFormat TIME_FORMAT = new SimpleDateFormat("m:ss", Locale.US);

    @NonNull
    private final TextView mTitle;
    @NonNull
    private final TextView mSubtitle;
    @Nullable
    private final TextView mTime;
    @NonNull
    private final SeekBar mSeekBar;
    @Nullable
    private final ImageView mAlbumArt;

    @Nullable
    private PlaybackModel mModel;

    private boolean mUpdatesPaused;
    private boolean mNeedsMetadataUpdate;
    private int mAlbumArtSize;

    private final PlaybackModel.PlaybackObserver mPlaybackObserver =
            new PlaybackModel.PlaybackObserver() {
                @Override
                public void onPlaybackStateChanged() {
                    updateState();
                }

                @Override
                public void onSourceChanged() {
                    updateState();
                    updateMetadata();
                }

                @Override
                public void onMetadataChanged() {
                    updateMetadata();
                }
            };

    /**
     * Create a new MetadataController that operates on the provided Views
     * @param title Displays the track's title. Must not be {@code null}.
     * @param subtitle Displays the track's artist. Must not be {@code null}.
     * @param time Displays the track's progress as text. May be {@code null}.
     * @param seekBar Displays the track's progress visually. Must not be {@code null}.
     * @param albumArt Displays the track's album art. May be {@code null}.
     */
    public MetadataController(@NonNull TextView title, @NonNull TextView subtitle,
            @Nullable TextView time, @NonNull SeekBar seekBar, @Nullable ImageView albumArt) {
        mTitle = title;
        mSubtitle = subtitle;
        mTime = time;
        mSeekBar = seekBar;
        mSeekBar.setOnTouchListener((view, event) -> true);
        mAlbumArt = albumArt;
        mAlbumArtSize = title.getContext().getResources()
                .getDimensionPixelSize(R.dimen.playback_album_art_size_large);
    }

    /**
     * Registers the {@link PlaybackModel} this widget will use to follow playback state.
     * Consumers of this class must unregister the {@link PlaybackModel} by calling this method with
     * null.
     *
     * @param model {@link PlaybackModel} to subscribe, or null to unsubscribe.
     */
    public void setModel(@Nullable PlaybackModel model) {
        if (mModel != null) {
            mModel.unregisterObserver(mPlaybackObserver);
        }
        mModel = model;
        if (mModel != null) {
            mModel.registerObserver(mPlaybackObserver);
        }
    }

    private void updateState() {
        updateProgress();

        mSeekBar.removeCallbacks(mSeekBarRunnable);
        if (mModel != null && mModel.isPlaying()) {
            mSeekBar.post(mSeekBarRunnable);
        }
    }

    private void updateMetadata() {
        if(mUpdatesPaused) {
            mNeedsMetadataUpdate = true;
            return;
        }

        mNeedsMetadataUpdate = false;
        MediaItemMetadata metadata = mModel != null ? mModel.getMetadata() : null;
        mTitle.setText(metadata != null ? metadata.getTitle() : null);
        mSubtitle.setText(metadata != null ? metadata.getSubtitle() : null);
        if (mAlbumArt != null && metadata != null && (metadata.getAlbumArtUri() != null
                || metadata.getAlbumArtBitmap() != null)) {
            mAlbumArt.setVisibility(View.VISIBLE);
            metadata.getAlbumArt(mAlbumArt.getContext(), mAlbumArtSize, mAlbumArtSize, true)
                    .thenAccept(mAlbumArt::setImageBitmap);
        } else if (mAlbumArt != null) {
            mAlbumArt.setVisibility(View.GONE);
        }
    }

    private static final long SEEK_BAR_UPDATE_TIME_INTERVAL_MS = 1000;

    private final Runnable mSeekBarRunnable = new Runnable() {
        @Override
        public void run() {
            if (mModel == null || !mModel.isPlaying()) {
                return;
            }
            updateProgress();
            mSeekBar.postDelayed(this, SEEK_BAR_UPDATE_TIME_INTERVAL_MS);

        }
    };

    private void updateProgress() {
        if (mModel == null) {
            mTime.setVisibility(View.INVISIBLE);
            mSeekBar.setVisibility(View.INVISIBLE);
            return;
        }
        long maxProgress = mModel.getMaxProgress();
        long progress = mModel.getProgress();
        int visibility = maxProgress > 0 && progress != PlaybackState.PLAYBACK_POSITION_UNKNOWN
                ? View.VISIBLE : View.INVISIBLE;
        if (mTime != null) {
            String time = String.format("%s / %s",
                    TIME_FORMAT.format(new Date(progress)),
                    TIME_FORMAT.format(new Date(maxProgress)));
            mTime.setVisibility(visibility);
            mTime.setText(time);
        }
        mSeekBar.setVisibility(visibility);
        mSeekBar.setMax((int) maxProgress);
        mSeekBar.setProgress((int) progress);
    }


    public void pauseUpdates() {
        mUpdatesPaused = true;
    }

    public void resumeUpdates() {
        mUpdatesPaused = false;
        if (mNeedsMetadataUpdate) {
            updateMetadata();
        }
    }
}
