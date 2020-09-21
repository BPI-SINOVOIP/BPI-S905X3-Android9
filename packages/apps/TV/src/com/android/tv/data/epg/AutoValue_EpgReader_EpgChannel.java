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

package com.android.tv.data.epg;

import com.android.tv.data.api.Channel;

/**
 * Hand copy of generated Autovalue class.
 *
 * TODO get autovalue working
 */
final class AutoValue_EpgReader_EpgChannel extends EpgReader.EpgChannel {

    private final Channel channel;
    private final String epgChannelId;

    AutoValue_EpgReader_EpgChannel(
            Channel channel,
            String epgChannelId) {
        if (channel == null) {
            throw new NullPointerException("Null channel");
        }
        this.channel = channel;
        if (epgChannelId == null) {
            throw new NullPointerException("Null epgChannelId");
        }
        this.epgChannelId = epgChannelId;
    }

    @Override
    public Channel getChannel() {
        return channel;
    }

    @Override
    public String getEpgChannelId() {
        return epgChannelId;
    }

    @Override
    public String toString() {
        return "EpgChannel{"
                + "channel=" + channel + ", "
                + "epgChannelId=" + epgChannelId
                + "}";
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }
        if (o instanceof EpgReader.EpgChannel) {
            EpgReader.EpgChannel that = (EpgReader.EpgChannel) o;
            return (this.channel.equals(that.getChannel()))
                    && (this.epgChannelId.equals(that.getEpgChannelId()));
        }
        return false;
    }

    @Override
    public int hashCode() {
        int h = 1;
        h *= 1000003;
        h ^= this.channel.hashCode();
        h *= 1000003;
        h ^= this.epgChannelId.hashCode();
        return h;
    }

}

