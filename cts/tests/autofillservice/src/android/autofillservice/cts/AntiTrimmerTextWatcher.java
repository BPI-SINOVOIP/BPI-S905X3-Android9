/*
 * Copyright (C) 2017 The Android Open Source Project
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
package android.autofillservice.cts;

import android.text.Editable;
import android.text.TextWatcher;
import android.widget.EditText;

import java.util.regex.Pattern;

/**
 * A {@link TextWatcher} that appends pound signs ({@code #} at the beginning and end of the text.
 */
public final class AntiTrimmerTextWatcher implements TextWatcher {

    /**
     * Regex used to revert a String that was "anti-trimmed".
     */
    public static final Pattern TRIMMER_PATTERN = Pattern.compile("#(.*)#");

    private final EditText mView;

    public AntiTrimmerTextWatcher(EditText view) {
        mView = view;
        mView.addTextChangedListener(this);
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        mView.removeTextChangedListener(this);
        mView.setText("#" + s + "#");
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
    }

    @Override
    public void afterTextChanged(Editable s) {
    }
}
