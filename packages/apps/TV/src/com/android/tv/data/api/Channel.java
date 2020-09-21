/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.tv.data.api;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.support.annotation.Nullable;
import com.android.tv.util.images.ImageLoader.ImageLoaderCallback;
import com.android.tv.data.InternalDataUtils;

import java.util.Map;

/**
 * Interface for {@link com.android.tv.data.ChannelImpl}.
 *
 * <p><b>NOTE</b> Normally you should not use an interface for a data object like {@code
 * ChannelImpl}, however there are many circular dependencies. An interface is the easiest way to
 * break the cycles.
 */
public interface Channel {

    long INVALID_ID = -1;
    int LOAD_IMAGE_TYPE_CHANNEL_LOGO = 1;
    int LOAD_IMAGE_TYPE_APP_LINK_ICON = 2;
    int LOAD_IMAGE_TYPE_APP_LINK_POSTER_ART = 3;
    /**
     * When a TIS doesn't provide any information about app link, and it doesn't have a leanback
     * launch intent, there will be no app link card for the TIS.
     */
    int APP_LINK_TYPE_NONE = -1;
    /**
     * When a TIS provide a specific app link information, the app link card will be {@code
     * APP_LINK_TYPE_CHANNEL} which contains all the provided information.
     */
    int APP_LINK_TYPE_CHANNEL = 1;
    /**
     * When a TIS doesn't provide a specific app link information, but the app has a leanback launch
     * intent, the app link card will be {@code APP_LINK_TYPE_APP} which launches the application.
     */
    int APP_LINK_TYPE_APP = 2;
    /** Channel number delimiter between major and minor parts. */
    char CHANNEL_NUMBER_DELIMITER = '-';

    long getId();

    Uri getUri();

    String getPackageName();

    String getInputId();

    String getType();

    String getDisplayNumber();

    @Nullable
    String getDisplayName();

    String getDescription();

    String getVideoFormat();

    boolean isPassthrough();

    String getDisplayText();

    String getAppLinkText();

    int getAppLinkColor();

    String getAppLinkIconUri();

    String getAppLinkPosterArtUri();

    String getAppLinkIntentUri();

    String getLogoUri();

    boolean isRecordingProhibited();

    boolean isPhysicalTunerChannel();

    boolean isBrowsable();

    boolean isSearchable();

    boolean isLocked();

    boolean hasSameReadOnlyInfo(Channel mCurrentChannel);

    void setChannelLogoExist(boolean result);

    void setBrowsable(boolean browsable);

    void setLocked(boolean locked);

    void copyFrom(Channel channel);

    void setLogoUri(String logoUri);

    boolean channelLogoExists();

    void loadBitmap(
            Context context,
            int loadImageTypeChannelLogo,
            int mChannelLogoImageViewWidth,
            int mChannelLogoImageViewHeight,
            ImageLoaderCallback<?> channelLogoCallback);

    int getAppLinkType(Context context);

    Intent getAppLinkIntent(Context context);

    void prefetchImage(
            Context mContext,
            int loadImageTypeChannelLogo,
            int mPosterArtWidth,
            int mPosterArtHeight);

    boolean isAnalogChannel();

    boolean isDigitalChannel();

    boolean isOtherChannel();

    boolean isRadioChannel();

    boolean isVideoChannel();

    boolean isFavourite();

    String getSignalType();

    String getContentRatings();

    int getFrequency();

    String getServiceType();

    Map<String, String> getChannelInternalProviderDataMap();

    boolean hasSameReadWriteInfo(Channel other);

    boolean IsHidden();

    boolean hasSameFavouriteInfo(String favInfo);
    String getFavouriteInfo();
    String getSatelliteInfo();
    String getTransponderInfo();
}
