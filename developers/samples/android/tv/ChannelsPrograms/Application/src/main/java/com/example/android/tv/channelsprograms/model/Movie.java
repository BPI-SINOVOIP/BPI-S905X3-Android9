/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.example.android.tv.channelsprograms.model;

import java.io.Serializable;

/** Represents a video entity with title, description, image thumbs and video url. */
public class Movie implements Serializable {

    private static final String TAG = "Movie";

    static final long serialVersionUID = 727566175075960653L;
    private long id;
    private String title;
    private String description;
    private String bgImageUrl;
    private String cardImageUrl;
    private String videoUrl;
    private String studio;
    private String category;
    // Program id / Watch Next id returned from the TV Provider.
    private long programId;
    private long watchNextId;

    public Movie() {}

    public long getProgramId() {
        return programId;
    }

    public void setProgramId(long programId) {
        this.programId = programId;
    }

    public long getWatchNextId() {
        return watchNextId;
    }

    public void setWatchNextId(long watchNextId) {
        this.watchNextId = watchNextId;
    }

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public String getTitle() {
        return title;
    }

    public void setTitle(String title) {
        this.title = title;
    }

    public String getDescription() {
        return description;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    public String getStudio() {
        return studio;
    }

    public void setStudio(String studio) {
        this.studio = studio;
    }

    public String getVideoUrl() {
        return videoUrl;
    }

    public void setVideoUrl(String videoUrl) {
        this.videoUrl = videoUrl;
    }

    public String getBackgroundImageUrl() {
        return bgImageUrl;
    }

    public void setBackgroundImageUrl(String bgImageUrl) {
        this.bgImageUrl = bgImageUrl;
    }

    public String getCardImageUrl() {
        return cardImageUrl;
    }

    public void setCardImageUrl(String cardImageUrl) {
        this.cardImageUrl = cardImageUrl;
    }

    public String getCategory() {
        return category;
    }

    public void setCategory(String category) {
        this.category = category;
    }

    @Override
    public String toString() {
        return "Movie{"
                + "id="
                + id
                + ", programId='"
                + programId
                + '\''
                + ", watchNextId='"
                + watchNextId
                + '\''
                + ", title='"
                + title
                + '\''
                + ", videoUrl='"
                + videoUrl
                + '\''
                + ", backgroundImageUrl='"
                + bgImageUrl
                + '\''
                + ", cardImageUrl='"
                + cardImageUrl
                + '\''
                + '}';
    }
}
