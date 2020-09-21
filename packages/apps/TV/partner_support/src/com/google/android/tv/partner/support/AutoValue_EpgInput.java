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



/**
 * Hand copy of generated Autovalue class.
 *
 * TODO get autovalue working
 */
final class AutoValue_EpgInput extends EpgInput {

    private final long id;
    private final String inputId;
    private final String lineupId;

    AutoValue_EpgInput(
            long id,
            String inputId,
            String lineupId) {
        this.id = id;
        if (inputId == null) {
            throw new NullPointerException("Null inputId");
        }
        this.inputId = inputId;
        if (lineupId == null) {
            throw new NullPointerException("Null lineupId");
        }
        this.lineupId = lineupId;
    }

    @Override
    public long getId() {
        return id;
    }

    @Override
    public String getInputId() {
        return inputId;
    }

    @Override
    public String getLineupId() {
        return lineupId;
    }

    @Override
    public String toString() {
        return "EpgInput{"
                + "id=" + id + ", "
                + "inputId=" + inputId + ", "
                + "lineupId=" + lineupId
                + "}";
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }
        if (o instanceof EpgInput) {
            EpgInput that = (EpgInput) o;
            return (this.id == that.getId())
                    && (this.inputId.equals(that.getInputId()))
                    && (this.lineupId.equals(that.getLineupId()));
        }
        return false;
    }

    @Override
    public int hashCode() {
        int h$ = 1;
        h$ *= 1000003;
        h$ ^= (int) ((id >>> 32) ^ id);
        h$ *= 1000003;
        h$ ^= inputId.hashCode();
        h$ *= 1000003;
        h$ ^= lineupId.hashCode();
        return h$;
    }

}
