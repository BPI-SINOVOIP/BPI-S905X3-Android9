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
 * limitations under the License
 */



package com.android.tv.search;

import android.support.annotation.Nullable;

/**
 * Hand copy of generated Autovalue class.
 *
 * TODO get autovalue working
 */

final class AutoValue_LocalSearchProvider_SearchResult extends LocalSearchProvider.SearchResult {

    private final long channelId;
    private final String channelNumber;
    private final String title;
    private final String description;
    private final String imageUri;
    private final String intentAction;
    private final String intentData;
    private final String intentExtraData;
    private final String contentType;
    private final boolean isLive;
    private final int videoWidth;
    private final int videoHeight;
    private final long duration;
    private final int progressPercentage;

    private AutoValue_LocalSearchProvider_SearchResult(
            long channelId,
            @Nullable String channelNumber,
            @Nullable String title,
            @Nullable String description,
            @Nullable String imageUri,
            @Nullable String intentAction,
            @Nullable String intentData,
            @Nullable String intentExtraData,
            @Nullable String contentType,
            boolean isLive,
            int videoWidth,
            int videoHeight,
            long duration,
            int progressPercentage) {
        this.channelId = channelId;
        this.channelNumber = channelNumber;
        this.title = title;
        this.description = description;
        this.imageUri = imageUri;
        this.intentAction = intentAction;
        this.intentData = intentData;
        this.intentExtraData = intentExtraData;
        this.contentType = contentType;
        this.isLive = isLive;
        this.videoWidth = videoWidth;
        this.videoHeight = videoHeight;
        this.duration = duration;
        this.progressPercentage = progressPercentage;
    }

    @Override
    long getChannelId() {
        return channelId;
    }

    @Nullable
    @Override
    String getChannelNumber() {
        return channelNumber;
    }

    @Nullable
    @Override
    String getTitle() {
        return title;
    }

    @Nullable
    @Override
    String getDescription() {
        return description;
    }

    @Nullable
    @Override
    String getImageUri() {
        return imageUri;
    }

    @Nullable
    @Override
    String getIntentAction() {
        return intentAction;
    }

    @Nullable
    @Override
    String getIntentData() {
        return intentData;
    }

    @Nullable
    @Override
    String getIntentExtraData() {
        return intentExtraData;
    }

    @Nullable
    @Override
    String getContentType() {
        return contentType;
    }

    @Override
    boolean getIsLive() {
        return isLive;
    }

    @Override
    int getVideoWidth() {
        return videoWidth;
    }

    @Override
    int getVideoHeight() {
        return videoHeight;
    }

    @Override
    long getDuration() {
        return duration;
    }

    @Override
    int getProgressPercentage() {
        return progressPercentage;
    }

    @Override
    public String toString() {
        return "SearchResult{"
                + "channelId=" + channelId + ", "
                + "channelNumber=" + channelNumber + ", "
                + "title=" + title + ", "
                + "description=" + description + ", "
                + "imageUri=" + imageUri + ", "
                + "intentAction=" + intentAction + ", "
                + "intentData=" + intentData + ", "
                + "intentExtraData=" + intentExtraData + ", "
                + "contentType=" + contentType + ", "
                + "isLive=" + isLive + ", "
                + "videoWidth=" + videoWidth + ", "
                + "videoHeight=" + videoHeight + ", "
                + "duration=" + duration + ", "
                + "progressPercentage=" + progressPercentage
                + "}";
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }
        if (o instanceof LocalSearchProvider.SearchResult) {
            LocalSearchProvider.SearchResult that = (LocalSearchProvider.SearchResult) o;
            return (this.channelId == that.getChannelId())
                    && ((this.channelNumber == null) ? (that.getChannelNumber() == null) : this.channelNumber.equals(that.getChannelNumber()))
                    && ((this.title == null) ? (that.getTitle() == null) : this.title.equals(that.getTitle()))
                    && ((this.description == null) ? (that.getDescription() == null) : this.description.equals(that.getDescription()))
                    && ((this.imageUri == null) ? (that.getImageUri() == null) : this.imageUri.equals(that.getImageUri()))
                    && ((this.intentAction == null) ? (that.getIntentAction() == null) : this.intentAction.equals(that.getIntentAction()))
                    && ((this.intentData == null) ? (that.getIntentData() == null) : this.intentData.equals(that.getIntentData()))
                    && ((this.intentExtraData == null) ? (that.getIntentExtraData() == null) : this.intentExtraData.equals(that.getIntentExtraData()))
                    && ((this.contentType == null) ? (that.getContentType() == null) : this.contentType.equals(that.getContentType()))
                    && (this.isLive == that.getIsLive())
                    && (this.videoWidth == that.getVideoWidth())
                    && (this.videoHeight == that.getVideoHeight())
                    && (this.duration == that.getDuration())
                    && (this.progressPercentage == that.getProgressPercentage());
        }
        return false;
    }

    @Override
    public int hashCode() {
        int h$ = 1;
        h$ *= 1000003;
        h$ ^= (int) ((channelId >>> 32) ^ channelId);
        h$ *= 1000003;
        h$ ^= (channelNumber == null) ? 0 : channelNumber.hashCode();
        h$ *= 1000003;
        h$ ^= (title == null) ? 0 : title.hashCode();
        h$ *= 1000003;
        h$ ^= (description == null) ? 0 : description.hashCode();
        h$ *= 1000003;
        h$ ^= (imageUri == null) ? 0 : imageUri.hashCode();
        h$ *= 1000003;
        h$ ^= (intentAction == null) ? 0 : intentAction.hashCode();
        h$ *= 1000003;
        h$ ^= (intentData == null) ? 0 : intentData.hashCode();
        h$ *= 1000003;
        h$ ^= (intentExtraData == null) ? 0 : intentExtraData.hashCode();
        h$ *= 1000003;
        h$ ^= (contentType == null) ? 0 : contentType.hashCode();
        h$ *= 1000003;
        h$ ^= isLive ? 1231 : 1237;
        h$ *= 1000003;
        h$ ^= videoWidth;
        h$ *= 1000003;
        h$ ^= videoHeight;
        h$ *= 1000003;
        h$ ^= (int) ((duration >>> 32) ^ duration);
        h$ *= 1000003;
        h$ ^= progressPercentage;
        return h$;
    }

    static final class Builder extends LocalSearchProvider.SearchResult.Builder {
        private Long channelId;
        private String channelNumber;
        private String title;
        private String description;
        private String imageUri;
        private String intentAction;
        private String intentData;
        private String intentExtraData;
        private String contentType;
        private Boolean isLive;
        private Integer videoWidth;
        private Integer videoHeight;
        private Long duration;
        private Integer progressPercentage;
        Builder() {
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setChannelId(long channelId) {
            this.channelId = channelId;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setChannelNumber(@Nullable String channelNumber) {
            this.channelNumber = channelNumber;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setTitle(@Nullable String title) {
            this.title = title;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setDescription(@Nullable String description) {
            this.description = description;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setImageUri(@Nullable String imageUri) {
            this.imageUri = imageUri;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setIntentAction(@Nullable String intentAction) {
            this.intentAction = intentAction;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setIntentData(@Nullable String intentData) {
            this.intentData = intentData;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setIntentExtraData(@Nullable String intentExtraData) {
            this.intentExtraData = intentExtraData;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setContentType(@Nullable String contentType) {
            this.contentType = contentType;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setIsLive(boolean isLive) {
            this.isLive = isLive;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setVideoWidth(int videoWidth) {
            this.videoWidth = videoWidth;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setVideoHeight(int videoHeight) {
            this.videoHeight = videoHeight;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setDuration(long duration) {
            this.duration = duration;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult.Builder setProgressPercentage(int progressPercentage) {
            this.progressPercentage = progressPercentage;
            return this;
        }
        @Override
        LocalSearchProvider.SearchResult build() {
            String missing = "";
            if (this.channelId == null) {
                missing += " channelId";
            }
            if (this.isLive == null) {
                missing += " isLive";
            }
            if (this.videoWidth == null) {
                missing += " videoWidth";
            }
            if (this.videoHeight == null) {
                missing += " videoHeight";
            }
            if (this.duration == null) {
                missing += " duration";
            }
            if (this.progressPercentage == null) {
                missing += " progressPercentage";
            }
            if (!missing.isEmpty()) {
                throw new IllegalStateException("Missing required properties:" + missing);
            }
            return new AutoValue_LocalSearchProvider_SearchResult(
                    this.channelId,
                    this.channelNumber,
                    this.title,
                    this.description,
                    this.imageUri,
                    this.intentAction,
                    this.intentData,
                    this.intentExtraData,
                    this.contentType,
                    this.isLive,
                    this.videoWidth,
                    this.videoHeight,
                    this.duration,
                    this.progressPercentage);
        }
    }

}
