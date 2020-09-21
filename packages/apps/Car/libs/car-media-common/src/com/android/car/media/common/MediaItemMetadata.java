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

import android.annotation.DrawableRes;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.media.MediaDescription;
import android.media.MediaMetadata;
import android.media.browse.MediaBrowser;
import android.media.session.MediaSession;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;
import android.widget.ImageView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.RequestBuilder;
import com.bumptech.glide.request.RequestOptions;
import com.bumptech.glide.request.target.SimpleTarget;
import com.bumptech.glide.request.target.Target;
import com.bumptech.glide.request.transition.Transition;

import java.util.Objects;
import java.util.concurrent.CompletableFuture;

/**
 * Abstract representation of a media item metadata.
 */
public class MediaItemMetadata implements Parcelable {
    private static final String TAG = "MediaItemMetadata";
    @NonNull
    private final MediaDescription mMediaDescription;
    @Nullable
    private final Long mQueueId;
    private final boolean mIsBrowsable;
    private final boolean mIsPlayable;

    /** Creates an instance based on a {@link MediaMetadata} */
    public MediaItemMetadata(@NonNull MediaMetadata metadata) {
        this(metadata.getDescription(), null, false, false);
    }

    /** Creates an instance based on a {@link MediaSession.QueueItem} */
    public MediaItemMetadata(@NonNull MediaSession.QueueItem queueItem) {
        this(queueItem.getDescription(), queueItem.getQueueId(), false, true);
    }

    /** Creates an instance based on a {@link MediaBrowser.MediaItem} */
    public MediaItemMetadata(@NonNull MediaBrowser.MediaItem item) {
        this(item.getDescription(), null, item.isBrowsable(), item.isPlayable());
    }

    /** Creates an instance based on a {@link Parcel} */
    public MediaItemMetadata(@NonNull Parcel in) {
        mMediaDescription = (MediaDescription) in.readValue(
                MediaDescription.class.getClassLoader());
        mQueueId = in.readByte() == 0x00 ? null : in.readLong();
        mIsBrowsable = in.readByte() != 0x00;
        mIsPlayable = in.readByte() != 0x00;
    }

    /**
     * Creates a clone of this item
     *
     * @deprecated this method will be removed as part of b/79089344
     */
    @Deprecated
    public MediaItemMetadata(@NonNull MediaItemMetadata item) {
        mMediaDescription = item.mMediaDescription;
        mQueueId = item.mQueueId;
        mIsBrowsable = item.mIsBrowsable;
        mIsPlayable = item.mIsPlayable;
    }

    private MediaItemMetadata(MediaDescription description, Long queueId, boolean isBrowsable,
            boolean isPlayable) {
        mMediaDescription = description;
        mQueueId = queueId;
        mIsPlayable = isPlayable;
        mIsBrowsable = isBrowsable;
    }

    /** @return media item id */
    @Nullable
    public String getId() {
        return mMediaDescription.getMediaId();
    }

    /** @return media item title */
    @Nullable
    public CharSequence getTitle() {
        return mMediaDescription.getTitle();
    }

    /** @return media item subtitle */
    @Nullable
    public CharSequence getSubtitle() {
        return mMediaDescription.getSubtitle();
    }

    /** @return media item description */
    @Nullable
    public CharSequence getDescription() {
        return mMediaDescription.getSubtitle();
    }

    /**
     * An id that can be used on {@link PlaybackModel#onSkipToQueueItem(long)}
     *
     * @return the id of this item in the session queue, or NULL if this is not a session queue
     * item.
     */
    @Nullable
    public Long getQueueId() {
        return mQueueId;
    }

    /**
     * @return album art bitmap, or NULL if this item doesn't have a local album art. In this,
     * the {@link #getAlbumArtUri()} should be used to obtain a reference to a remote bitmap.
     */
    public Bitmap getAlbumArtBitmap() {
        return mMediaDescription.getIconBitmap();
    }

    /**
     * @return an {@link Uri} referencing the album art bitmap.
     */
    public Uri getAlbumArtUri() {
        return mMediaDescription.getIconUri();
    }

    /**
     * Updates the given {@link ImageView} with the album art of the given media item. This is an
     * asynchronous operation.
     * Note: If a view is set using this method, it should also be cleared using this same method.
     * Given that the loading is asynchronous, missing to use this method for clearing could cause
     * a delayed request to set an undesired image, or caching entries to be used for images not
     * longer necessary.
     *
     * @param context          {@link Context} used to load resources from
     * @param metadata         metadata to use, or NULL if the {@link ImageView} should be cleared.
     * @param imageView        loading target
     * @param loadingIndicator a drawable resource that would be set into the {@link ImageView}
     *                         while the image is being downloaded, or 0 if no loading indicator
     *                         is required.
     */
    public static void updateImageView(Context context, @Nullable MediaItemMetadata metadata,
            ImageView imageView, @DrawableRes int loadingIndicator) {
        Glide.with(context).clear(imageView);
        if (metadata == null) {
            imageView.setImageBitmap(null);
            return;
        }
        Bitmap image = metadata.getAlbumArtBitmap();
        if (image != null) {
            imageView.setImageBitmap(image);
            return;
        }
        Uri imageUri = metadata.getAlbumArtUri();
        if (imageUri != null) {
            Glide.with(context)
                    .load(imageUri)
                    .apply(RequestOptions.placeholderOf(loadingIndicator))
                    .into(imageView);
            return;
        }
        imageView.setImageBitmap(null);
    }

    /**
     * Loads the album art of this media item asynchronously. The loaded image will be scaled to
     * fit into the given view size.
     * Using {@link #updateImageView(Context, MediaItemMetadata, ImageView, int)} method is
     * preferred. Only use this method if you are going to apply transformations to the loaded
     * image.
     *
     * @param width  desired width (should be > 0)
     * @param height desired height (should be > 0)
     * @param fit    whether the image should be scaled to fit (fitCenter), or it should be cropped
     *               (centerCrop).
     * @return a {@link CompletableFuture} that will be completed once the image is loaded, or the
     * loading fails.
     */
    public CompletableFuture<Bitmap> getAlbumArt(Context context, int width, int height,
            boolean fit) {
        Bitmap image = getAlbumArtBitmap();
        if (image != null) {
            return CompletableFuture.completedFuture(image);
        }
        Uri imageUri = getAlbumArtUri();
        if (imageUri != null) {
            CompletableFuture<Bitmap> bitmapCompletableFuture = new CompletableFuture<>();
            RequestBuilder<Bitmap> builder = Glide.with(context)
                    .asBitmap()
                    .load(getAlbumArtUri());
            if (fit) {
                builder = builder.apply(RequestOptions.fitCenterTransform());
            } else {
                builder = builder.apply(RequestOptions.centerCropTransform());
            }
            Target<Bitmap> target = new SimpleTarget<Bitmap>(width, height) {
                @Override
                public void onResourceReady(@NonNull Bitmap bitmap,
                        @Nullable Transition<? super Bitmap> transition) {
                    bitmapCompletableFuture.complete(bitmap);
                }

                @Override
                public void onLoadFailed(@Nullable Drawable errorDrawable) {
                    bitmapCompletableFuture.completeExceptionally(
                            new IllegalStateException("Unknown error"));
                }
            };
            builder.into(target);
            return bitmapCompletableFuture;
        }
        return CompletableFuture.completedFuture(null);
    }

    public boolean isBrowsable() {
        return mIsBrowsable;
    }

    /**
     * @return Content style hint for browsable items, if provided as an extra, or
     * 0 as default value if not provided.
     */
    public int getBrowsableContentStyleHint() {
        Bundle extras = mMediaDescription.getExtras();
        if (extras != null && extras.getBoolean(
                ContentStyleMediaConstants.CONTENT_STYLE_SUPPORTED, false)) {
            return extras.getInt(ContentStyleMediaConstants.CONTENT_STYLE_BROWSABLE_HINT, 0);
        }
        return 0;
    }

    public boolean isPlayable() {
        return mIsPlayable;
    }

    /**
     * @return Content style hint for playable items, if provided as an extra, or
     * 0 as default value if not provided.
     */
    public int getPlayableContentStyleHint() {
        Bundle extras = mMediaDescription.getExtras();
        if (extras != null && extras.getBoolean(
                ContentStyleMediaConstants.CONTENT_STYLE_SUPPORTED, false)) {
            return extras.getInt(ContentStyleMediaConstants.CONTENT_STYLE_PLAYABLE_HINT, 0);
        }
        return 0;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        MediaItemMetadata that = (MediaItemMetadata) o;
        return mIsBrowsable == that.mIsBrowsable
                && mIsPlayable == that.mIsPlayable
                && Objects.equals(getId(), that.getId())
                && Objects.equals(getTitle(), that.getTitle())
                && Objects.equals(getSubtitle(), that.getSubtitle())
                && Objects.equals(getAlbumArtUri(), that.getAlbumArtUri())
                && Objects.equals(mQueueId, that.mQueueId);
    }

    @Override
    public int hashCode() {
        return Objects.hash(mMediaDescription.getMediaId(), mQueueId, mIsBrowsable, mIsPlayable);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeValue(mMediaDescription);
        if (mQueueId == null) {
            dest.writeByte((byte) (0x00));
        } else {
            dest.writeByte((byte) (0x01));
            dest.writeLong(mQueueId);
        }
        dest.writeByte((byte) (mIsBrowsable ? 0x01 : 0x00));
        dest.writeByte((byte) (mIsPlayable ? 0x01 : 0x00));
    }

    @SuppressWarnings("unused")
    public static final Parcelable.Creator<MediaItemMetadata> CREATOR =
            new Parcelable.Creator<MediaItemMetadata>() {
                @Override
                public MediaItemMetadata createFromParcel(Parcel in) {
                    return new MediaItemMetadata(in);
                }

                @Override
                public MediaItemMetadata[] newArray(int size) {
                    return new MediaItemMetadata[size];
                }
            };

    @Override
    public String toString() {
        return "[Id: "
                + (mMediaDescription != null ? mMediaDescription.getMediaId() : "-")
                + ", Queue Id: "
                + (mQueueId != null ? mQueueId : "-")
                + ", title: "
                + (mMediaDescription != null ? mMediaDescription.getTitle() : "-")
                + ", subtitle: "
                + (mMediaDescription != null ? mMediaDescription.getSubtitle() : "-")
                + ", album art URI: "
                + (mMediaDescription != null ? mMediaDescription.getIconUri() : "-")
                + "]";
    }
}