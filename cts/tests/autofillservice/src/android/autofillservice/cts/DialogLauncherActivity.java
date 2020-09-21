/*
 * Copyright 2017 The Android Open Source Project
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

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;

/**
 * Activity that has buttons to launch dialogs that should then be autofillable.
 */
public class DialogLauncherActivity extends AbstractAutoFillActivity {

    private FillExpectation mExpectation;
    private LoginDialog mDialog;
    Button mLaunchButton;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.dialog_launcher_activity);
        mLaunchButton = findViewById(R.id.launch_button);
        mDialog = new LoginDialog(this);
        mLaunchButton.setOnClickListener((v) -> mDialog.show());
    }

    void onUsername(Visitor<EditText> v) {
        syncRunOnUiThread(() -> v.visit(mDialog.mUsernameEditText));
    }

    void launchDialog(UiBot uiBot) throws Exception {
        syncRunOnUiThread(() -> mLaunchButton.performClick());
        // TODO: should assert by id, but it's not working
        uiBot.assertShownByText("Username");
    }

    void assertInDialogBounds(Rect rect) {
        final int[] location = new int[2];
        final View view = mDialog.getWindow().getDecorView();
        view.getLocationOnScreen(location);
        assertThat(location[0]).isAtMost(rect.left);
        assertThat(rect.right).isAtMost(location[0] + view.getWidth());
        assertThat(location[1]).isAtMost(rect.top);
        assertThat(rect.bottom).isAtMost(location[1] + view.getHeight());
    }

    void maximizeDialog() {
        final WindowManager wm = getWindowManager();
        final Display display = wm.getDefaultDisplay();
        final DisplayMetrics metrics = new DisplayMetrics();
        display.getMetrics(metrics);
        syncRunOnUiThread(
                () -> mDialog.getWindow().setLayout(metrics.widthPixels, metrics.heightPixels));
    }

    void expectAutofill(String username, String password) {
        assertWithMessage("must call launchDialog first").that(mDialog.mUsernameEditText)
                .isNotNull();
        mExpectation = new FillExpectation(username, password);
        mDialog.mUsernameEditText.addTextChangedListener(mExpectation.mCcUsernameWatcher);
        mDialog.mPasswordEditText.addTextChangedListener(mExpectation.mCcPasswordWatcher);
    }

    void assertAutofilled() throws Exception {
        assertWithMessage("expectAutoFill() not called").that(mExpectation).isNotNull();
        if (mExpectation.mCcUsernameWatcher != null) {
            mExpectation.mCcUsernameWatcher.assertAutoFilled();
        }
        if (mExpectation.mCcPasswordWatcher != null) {
            mExpectation.mCcPasswordWatcher.assertAutoFilled();
        }
    }

    private final class FillExpectation {
        private final OneTimeTextWatcher mCcUsernameWatcher;
        private final OneTimeTextWatcher mCcPasswordWatcher;

        private FillExpectation(String username, String password) {
            mCcUsernameWatcher = username == null ? null
                    : new OneTimeTextWatcher("username", mDialog.mUsernameEditText, username);
            mCcPasswordWatcher = password == null ? null
                    : new OneTimeTextWatcher("password", mDialog.mPasswordEditText, password);
        }

        private FillExpectation(String username) {
            this(username, null);
        }
    }

    public final class LoginDialog extends AlertDialog {

        private EditText mUsernameEditText;
        private EditText mPasswordEditText;

        public LoginDialog(Context context) {
            super(context);
        }

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            setContentView(R.layout.login_activity);
            mUsernameEditText = findViewById(R.id.username);
            mPasswordEditText = findViewById(R.id.password);
        }
    }
}
