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



package com.google.android.tv.partner.support;

import android.support.annotation.Nullable;
import java.util.List;

/**
 * Hand copy of generated Autovalue class.
 *
 * TODO get autovalue working
 */

final class AutoValue_Lineup extends Lineup {

    private final String id;
    private final int type;
    private final String name;
    private final List<String> channels;

    AutoValue_Lineup(
            String id,
            int type,
            @Nullable String name,
            List<String> channels) {
        if (id == null) {
            throw new NullPointerException("Null id");
        }
        this.id = id;
        this.type = type;
        this.name = name;
        if (channels == null) {
            throw new NullPointerException("Null channels");
        }
        this.channels = channels;
    }

    @Override
    public String getId() {
        return id;
    }

    @Override
    public int getType() {
        return type;
    }

    @Nullable
    @Override
    public String getName() {
        return name;
    }

    @Override
    public List<String> getChannels() {
        return channels;
    }

    @Override
    public String toString() {
        return "Lineup{"
                + "id=" + id + ", "
                + "type=" + type + ", "
                + "name=" + name + ", "
                + "channels=" + channels
                + "}";
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }
        if (o instanceof Lineup) {
            Lineup that = (Lineup) o;
            return (this.id.equals(that.getId()))
                    && (this.type == that.getType())
                    && ((this.name == null) ? (that.getName() == null) : this.name.equals(that.getName()))
                    && (this.channels.equals(that.getChannels()));
        }
        return false;
    }

    @Override
    public int hashCode() {
        int h$ = 1;
        h$ *= 1000003;
        h$ ^= id.hashCode();
        h$ *= 1000003;
        h$ ^= type;
        h$ *= 1000003;
        h$ ^= (name == null) ? 0 : name.hashCode();
        h$ *= 1000003;
        h$ ^= channels.hashCode();
        return h$;
    }

}
