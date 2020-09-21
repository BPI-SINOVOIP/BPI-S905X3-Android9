/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.car.media.common;

import android.car.Car;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.request.target.Target;

import java.util.Objects;

/**
 * {@link Fragment} that can be used to display and control the currently playing media item.
 * Its requires the android.Manifest.permission.MEDIA_CONTENT_CONTROL permission be held by the
 * hosting application.
 */
public class PlaybackFragment extends Fragment {
    private ActiveMediaSourceManager mActiveMediaSourceManager;
    private PlaybackModel mModel;
    private CrossfadeImageView mAlbumBackground;
    private PlaybackControls mPlaybackControls;
    private ImageView mAppIcon;
    private TextView mAppName;
    private TextView mTitle;
    private TextView mSubtitle;
    private MediaItemMetadata mCurrentMetadata;

    private PlaybackModel.PlaybackObserver mPlaybackObserver =
            new PlaybackModel.PlaybackObserver() {
        @Override
        public void onSourceChanged() {
            updateMetadata();
        }

        @Override
        public void onMetadataChanged() {
            updateMetadata();
        }
    };
    private ActiveMediaSourceManager.Observer mActiveSourceObserver =
            new ActiveMediaSourceManager.Observer() {
        @Override
        public void onActiveSourceChanged() {
            mModel.setMediaController(mActiveMediaSourceManager.getMediaController());
        }
    };

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.car_playback_fragment, container, false);
        mActiveMediaSourceManager = new ActiveMediaSourceManager(getContext());
        mModel = new PlaybackModel(getContext());
        mAlbumBackground = view.findViewById(R.id.album_background);
        mPlaybackControls = view.findViewById(R.id.playback_controls);
        mPlaybackControls.setModel(mModel);
        mAppIcon = view.findViewById(R.id.app_icon);
        mAppName = view.findViewById(R.id.app_name);
        mTitle = view.findViewById(R.id.title);
        mSubtitle = view.findViewById(R.id.subtitle);

        mAlbumBackground.setOnClickListener(v -> {
            MediaSource mediaSource = mModel.getMediaSource();
            Intent intent;
            if (mediaSource != null && mediaSource.isCustom()) {
                // We are playing a custom app. Jump to it, not to the template
                intent = getContext().getPackageManager()
                        .getLaunchIntentForPackage(mediaSource.getPackageName());
            } else if (mediaSource != null) {
                // We are playing a standard app. Open the template to browse it.
                intent = new Intent(Car.CAR_INTENT_ACTION_MEDIA_TEMPLATE);
                intent.putExtra(Car.CAR_EXTRA_MEDIA_PACKAGE, mediaSource.getPackageName());
            } else {
                // We are not playing. Open the template to start playing something.
                intent = new Intent(Car.CAR_INTENT_ACTION_MEDIA_TEMPLATE);
            }
            startActivity(intent);
        });

        return view;
    }

    @Override
    public void onStart() {
        super.onStart();
        mActiveMediaSourceManager.registerObserver(mActiveSourceObserver);
        mModel.registerObserver(mPlaybackObserver);
    }

    @Override
    public void onStop() {
        super.onStop();
        mActiveMediaSourceManager.unregisterObserver(mActiveSourceObserver);
        mModel.unregisterObserver(mPlaybackObserver);
    }

    private void updateMetadata() {
        MediaSource mediaSource = mModel.getMediaSource();

        if (mediaSource == null) {
            mTitle.setText(null);
            mSubtitle.setText(null);
            mAppName.setText(null);
            mAlbumBackground.setImageBitmap(null, true);
            return;
        }

        MediaItemMetadata metadata = mModel.getMetadata();
        if (Objects.equals(mCurrentMetadata, metadata)) {
            return;
        }
        mCurrentMetadata = metadata;
        mTitle.setText(metadata != null ? metadata.getTitle() : null);
        mSubtitle.setText(metadata != null ? metadata.getSubtitle() : null);
        if (metadata != null) {
            metadata.getAlbumArt(getContext(),
                    Target.SIZE_ORIGINAL,
                    Target.SIZE_ORIGINAL,
                    false)
                    .thenAccept(bitmap -> {
                        //bitmap = ImageUtils.blur(getContext(), bitmap, 1f, 10f);
                        mAlbumBackground.setImageBitmap(bitmap, true);
                    });
        } else {
            mAlbumBackground.setImageBitmap(null, true);
        }
        mAppName.setText(mediaSource.getName());
        mAppIcon.setImageBitmap(mediaSource.getRoundPackageIcon());
    }
}
