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

package com.android.car;

import android.annotation.Nullable;
import android.annotation.RawRes;
import android.content.Context;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;

/**
 * An implementation of {@link com.android.car.CarConfigurationService.JsonReader} that will
 * parse a JSON file on the system that is mapped to {@code R.raw.car_config}.
 */
public class JsonReaderImpl implements CarConfigurationService.JsonReader {
    private static final int BUF_SIZE = 0x1000; // 2K chars (4K bytes)
    private static final String JSON_FILE_ENCODING = "UTF-8";

    /**
     * Takes a resource file that is considered to be a JSON file and parses it into a String that
     * is returned.
     *
     * @param resId The resource id pointing to the json file.
     * @return A {@code String} representing the file contents, or {@code null} if
     */
    @Override
    @Nullable
    public String jsonFileToString(Context context, @RawRes int resId) {
        InputStream is = context.getResources().openRawResource(resId);

        // Note: this "try" will close the Reader, thus closing the associated InputStreamReader
        // and InputStream.
        try (Reader reader = new BufferedReader(new InputStreamReader(is, JSON_FILE_ENCODING))) {
            char[] buffer = new char[BUF_SIZE];
            StringBuilder stringBuilder = new StringBuilder();

            int bufferedContent;
            while ((bufferedContent = reader.read(buffer)) != -1) {
                stringBuilder.append(buffer, /* offset= */ 0, bufferedContent);
            }

            return stringBuilder.toString();
        } catch (IOException e) {
            return null;
        }
    }
}
