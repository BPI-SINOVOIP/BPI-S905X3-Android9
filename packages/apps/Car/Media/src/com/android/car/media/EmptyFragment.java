package com.android.car.media;

import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.android.car.media.common.MediaSource;
import com.android.car.media.widgets.ViewUtils;

/**
 * Empty fragment to show while we are loading content
 */
public class EmptyFragment extends Fragment {
    private ProgressBar mProgressBar;
    private ImageView mErrorIcon;
    private TextView mErrorMessage;

    private int mProgressBarDelay;
    private Handler mHandler = new Handler();
    private int mFadeDuration;
    private MediaActivity.BrowseState mState = MediaActivity.BrowseState.EMPTY;
    private MediaSource mMediaSource;
    private Runnable mProgressIndicatorRunnable = new Runnable() {
        @Override
        public void run() {
            ViewUtils.showViewAnimated(mProgressBar, mFadeDuration);
        }
    };
    @Override
    public View onCreateView(LayoutInflater inflater, final ViewGroup container,
            Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_empty, container, false);
        mProgressBar = view.findViewById(R.id.loading_spinner);
        mProgressBarDelay = getContext().getResources()
                .getInteger(R.integer.progress_indicator_delay);
        mFadeDuration = getContext().getResources().getInteger(
                R.integer.new_album_art_fade_in_duration);
        mErrorIcon = view.findViewById(R.id.error_icon);
        mErrorMessage = view.findViewById(R.id.error_message);
        update();
        return view;
    }

    @Override
    public void onPause() {
        super.onPause();
        mHandler.removeCallbacks(mProgressIndicatorRunnable);
    }

    /**
     * Updates the state of this fragment
     *
     * @param state browsing state to display
     * @param mediaSource media source currently being browsed
     */
    public void setState(MediaActivity.BrowseState state, MediaSource mediaSource) {
        mHandler.removeCallbacks(mProgressIndicatorRunnable);
        mMediaSource = mediaSource;
        mState = state;
        if (this.getView() != null) {
            update();
        }
    }

    private void update() {
        switch (mState) {
            case LOADING:
                // Display the indicator after a certain time, to avoid flashing the indicator
                // constantly, even when performance is acceptable.
                mHandler.postDelayed(mProgressIndicatorRunnable, mProgressBarDelay);
                mErrorIcon.setVisibility(View.GONE);
                mErrorMessage.setVisibility(View.GONE);
                break;
            case ERROR:
                mProgressBar.setVisibility(View.GONE);
                mErrorIcon.setVisibility(View.VISIBLE);
                mErrorMessage.setVisibility(View.VISIBLE);
                mErrorMessage.setText(getContext().getString(
                        R.string.cannot_connect_to_app,
                        mMediaSource != null
                                ? mMediaSource.getName()
                                : getContext().getString(R.string.unknown_media_provider_name)));
                break;
            case EMPTY:
                mProgressBar.setVisibility(View.GONE);
                mErrorIcon.setVisibility(View.GONE);
                mErrorMessage.setVisibility(View.VISIBLE);
                mErrorMessage.setText(getContext().getString(R.string.nothing_to_play));
                break;
            default:
                // Fail fast on any other state.
                throw new IllegalStateException("Invalid state for this fragment: " + mState);
        }
    }
}
