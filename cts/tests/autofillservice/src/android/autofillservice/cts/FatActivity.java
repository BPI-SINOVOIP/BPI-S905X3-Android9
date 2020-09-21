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

import static android.autofillservice.cts.Helper.findViewByAutofillHint;
import static android.view.View.IMPORTANT_FOR_AUTOFILL_AUTO;
import static android.view.View.IMPORTANT_FOR_AUTOFILL_NO;
import static android.view.View.IMPORTANT_FOR_AUTOFILL_NO_EXCLUDE_DESCENDANTS;
import static android.view.View.IMPORTANT_FOR_AUTOFILL_YES;
import static android.view.View.IMPORTANT_FOR_AUTOFILL_YES_EXCLUDE_DESCENDANTS;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;

/**
 * An activity containing mostly widgets that should be removed from an auto-fill structure to
 * optimize it.
 */
public class FatActivity extends AbstractAutoFillActivity {

    static final String ID_CAPTCHA = "captcha";
    static final String ID_INPUT = "input";
    static final String ID_INPUT_CONTAINER = "input_container";
    static final String ID_IMAGE = "image";
    static final String ID_IMPORTANT_IMAGE = "important_image";
    static final String ID_ROOT = "root";

    static final String ID_NOT_IMPORTANT_CONTAINER_EXCLUDING_DESCENDANTS =
            "not_important_container_excluding_descendants";
    static final String ID_NOT_IMPORTANT_CONTAINER_EXCLUDING_DESCENDANTS_CHILD =
            "not_important_container_excluding_descendants_child";
    static final String ID_NOT_IMPORTANT_CONTAINER_EXCLUDING_DESCENDANTS_GRAND_CHILD =
            "not_important_container_excluding_descendants_grand_child";

    static final String ID_IMPORTANT_CONTAINER_EXCLUDING_DESCENDANTS =
            "important_container_excluding_descendants";
    static final String ID_IMPORTANT_CONTAINER_EXCLUDING_DESCENDANTS_CHILD =
            "important_container_excluding_descendants_child";
    static final String ID_IMPORTANT_CONTAINER_EXCLUDING_DESCENDANTS_GRAND_CHILD =
            "important_container_excluding_descendants_grand_child";

    static final String ID_NOT_IMPORTANT_CONTAINER_MIXED_DESCENDANTS =
            "not_important_container_mixed_descendants";
    static final String ID_NOT_IMPORTANT_CONTAINER_MIXED_DESCENDANTS_CHILD =
            "not_important_container_mixed_descendants_child";
    static final String ID_NOT_IMPORTANT_CONTAINER_MIXED_DESCENDANTS_GRAND_CHILD =
            "not_important_container_mixed_descendants_grand_child";

    private LinearLayout mRoot;
    private EditText mCaptcha;
    private EditText mInput;
    private ImageView mImage;
    private ImageView mImportantImage;

    private View mNotImportantContainerExcludingDescendants;
    private View mNotImportantContainerExcludingDescendantsChild;
    private View mNotImportantContainerExcludingDescendantsGrandChild;

    private View mImportantContainerExcludingDescendants;
    private View mImportantContainerExcludingDescendantsChild;
    private View mImportantContainerExcludingDescendantsGrandChild;

    private View mNotImportantContainerMixedDescendants;
    private View mNotImportantContainerMixedDescendantsChild;
    private View mNotImportantContainerMixedDescendantsGrandChild;

    private MyView mViewWithAutofillHints;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.fat_activity);

        mRoot = findViewById(R.id.root);
        mCaptcha = findViewById(R.id.captcha);
        mInput = findViewById(R.id.input);
        mImage = findViewById(R.id.image);
        mImportantImage = findViewById(R.id.important_image);

        mNotImportantContainerExcludingDescendants = findViewById(
                R.id.not_important_container_excluding_descendants);
        mNotImportantContainerExcludingDescendantsChild = findViewById(
                R.id.not_important_container_excluding_descendants_child);
        mNotImportantContainerExcludingDescendantsGrandChild = findViewById(
                R.id.not_important_container_excluding_descendants_grand_child);

        mImportantContainerExcludingDescendants = findViewById(
                R.id.important_container_excluding_descendants);
        mImportantContainerExcludingDescendantsChild = findViewById(
                R.id.important_container_excluding_descendants_child);
        mImportantContainerExcludingDescendantsGrandChild = findViewById(
                R.id.important_container_excluding_descendants_grand_child);

        mNotImportantContainerMixedDescendants = findViewById(
                R.id.not_important_container_mixed_descendants);
        mNotImportantContainerMixedDescendantsChild = findViewById(
                R.id.not_important_container_mixed_descendants_child);
        mNotImportantContainerMixedDescendantsGrandChild = findViewById(
                R.id.not_important_container_mixed_descendants_grand_child);

        mViewWithAutofillHints = (MyView) findViewByAutofillHint(this, "importantAmI");
        assertThat(mViewWithAutofillHints).isNotNull();

        // Sanity check for importantForAutofill modes
        assertThat(mRoot.getImportantForAutofill()).isEqualTo(IMPORTANT_FOR_AUTOFILL_AUTO);
        assertThat(mInput.getImportantForAutofill()).isEqualTo(IMPORTANT_FOR_AUTOFILL_YES);
        assertThat(mCaptcha.getImportantForAutofill()).isEqualTo(IMPORTANT_FOR_AUTOFILL_NO);
        assertThat(mImage.getImportantForAutofill()).isEqualTo(IMPORTANT_FOR_AUTOFILL_NO);
        assertThat(mImportantImage.getImportantForAutofill()).isEqualTo(IMPORTANT_FOR_AUTOFILL_YES);

        assertThat(mNotImportantContainerExcludingDescendants.getImportantForAutofill())
                .isEqualTo(IMPORTANT_FOR_AUTOFILL_NO_EXCLUDE_DESCENDANTS);
        assertThat(mNotImportantContainerExcludingDescendantsChild.getImportantForAutofill())
                .isEqualTo(IMPORTANT_FOR_AUTOFILL_YES);
        assertThat(mNotImportantContainerExcludingDescendantsGrandChild.getImportantForAutofill())
                .isEqualTo(IMPORTANT_FOR_AUTOFILL_AUTO);

        assertThat(mImportantContainerExcludingDescendants.getImportantForAutofill())
                .isEqualTo(IMPORTANT_FOR_AUTOFILL_YES_EXCLUDE_DESCENDANTS);
        assertThat(mImportantContainerExcludingDescendantsChild.getImportantForAutofill())
                .isEqualTo(IMPORTANT_FOR_AUTOFILL_YES);
        assertThat(mImportantContainerExcludingDescendantsGrandChild.getImportantForAutofill())
                .isEqualTo(IMPORTANT_FOR_AUTOFILL_AUTO);

        assertThat(mNotImportantContainerMixedDescendants.getImportantForAutofill())
                .isEqualTo(IMPORTANT_FOR_AUTOFILL_NO);
        assertThat(mNotImportantContainerMixedDescendantsChild.getImportantForAutofill())
                .isEqualTo(IMPORTANT_FOR_AUTOFILL_YES);
        assertThat(mNotImportantContainerMixedDescendantsGrandChild.getImportantForAutofill())
                .isEqualTo(IMPORTANT_FOR_AUTOFILL_NO);

        assertThat(mViewWithAutofillHints.getImportantForAutofill())
                .isEqualTo(IMPORTANT_FOR_AUTOFILL_AUTO);
        assertThat(mViewWithAutofillHints.isImportantForAutofill()).isTrue();
    }

    /**
     * Visits the {@code input} in the UiThread.
     */
    void onInput(Visitor<EditText> v) {
        syncRunOnUiThread(() -> {
            v.visit(mInput);
        });
    }

    /**
     * Custom view that defines an autofill type so autofill hints are set on {@code ViewNode}.
     */
    public static class MyView extends View {
        public MyView(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        @Override
        public int getAutofillType() {
            return AUTOFILL_TYPE_TEXT;
        }
    }
}
