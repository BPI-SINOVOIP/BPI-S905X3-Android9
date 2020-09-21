/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.documentsui.dirlist;

import static com.android.documentsui.base.SharedMinimal.TAG;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.FragmentManager;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.support.annotation.Nullable;
import android.support.design.widget.Snackbar;
import android.support.design.widget.TextInputLayout;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

import com.android.documentsui.BaseActivity;
import com.android.documentsui.DocumentsApplication;
import com.android.documentsui.Metrics;
import com.android.documentsui.R;
import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.base.Shared;
import com.android.documentsui.ui.Snackbars;

/**
 * Dialog to rename file or directory.
 */
public class RenameDocumentFragment extends DialogFragment {
    private static final String TAG_RENAME_DOCUMENT = "rename_document";
    private DocumentInfo mDocument;
    private EditText mEditText;
    private TextInputLayout mRenameInputWrapper;
    private @Nullable DialogInterface mDialog;

    public static void show(FragmentManager fm, DocumentInfo document) {
        final RenameDocumentFragment dialog = new RenameDocumentFragment();
        dialog.mDocument = document;
        dialog.show(fm, TAG_RENAME_DOCUMENT);
    }

    /**
     * Creates the dialog UI.
     * @param savedInstanceState
     * @return
     */
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Context context = getActivity();
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        LayoutInflater dialogInflater = LayoutInflater.from(builder.getContext());
        View view = dialogInflater.inflate(R.layout.dialog_file_name, null, false);

        mEditText = (EditText) view.findViewById(android.R.id.text1);
        mRenameInputWrapper = (TextInputLayout) view.findViewById(R.id.rename_input_wrapper);
        builder.setTitle(R.string.menu_rename);
        builder.setView(view);
        builder.setPositiveButton(android.R.string.ok, null);
        builder.setNegativeButton(android.R.string.cancel, null);

        final AlertDialog dialog = builder.create();

        dialog.setOnShowListener(this::onShowDialog);

        // Workaround for the problem - virtual keyboard doesn't show on the phone.
        Shared.ensureKeyboardPresent(context, dialog);

        mEditText.setOnEditorActionListener(
                new OnEditorActionListener() {
                    @Override
                    public boolean onEditorAction(
                            TextView view, int actionId, @Nullable KeyEvent event) {
                        if ((actionId == EditorInfo.IME_ACTION_DONE) || (event != null
                                && event.getKeyCode() == KeyEvent.KEYCODE_ENTER
                                && event.hasNoModifiers())) {
                            renameDocuments(mEditText.getText().toString());
                        }
                        return false;
                    }
                });
        mEditText.requestFocus();
        return dialog;
    }

    private void onShowDialog(DialogInterface dialog){
        mDialog = dialog;
        Button button = ((AlertDialog) dialog).getButton(AlertDialog.BUTTON_POSITIVE);
        button.setOnClickListener(this::onClickDialog);
    }

    private void onClickDialog(View view) {
        renameDocuments(mEditText.getText().toString());
    }

    /**
     * Sets/Restores the data.
     * @param savedInstanceState
     * @return
     */
    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if(savedInstanceState == null) {
            // Fragment created for the first time, we set the text.
            // mDocument value was set in show
            mEditText.setText(mDocument.displayName);
        }
        else {
            // Fragment restored, text was restored automatically.
            // mDocument value needs to be restored.
            mDocument = savedInstanceState.getParcelable(Shared.EXTRA_DOC);
        }
        // Do selection in both cases, because we cleared it.
        selectFileName(mEditText);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        // Clear selection before storing state and restore it manually,
        // because otherwise after rotation selection is displayed with cut/copy menu visible :/
        clearFileNameSelection(mEditText);

        super.onSaveInstanceState(outState);

        outState.putParcelable(Shared.EXTRA_DOC, mDocument);
    }

    /**
     * Validates if string is a proper document name.
     * Checks if string is not empty. More rules might be added later.
     * @param docName string representing document name
     * @returns true if string is a valid name.
     **/
    private boolean isValidDocumentName(String docName) {
        return !docName.isEmpty();
    }

    /**
     * Fills text field with the file name and selects the name without extension.
     *
     * @param editText text field to be filled
     */
    private void selectFileName(EditText editText) {
        String text = editText.getText().toString();
        int separatorIndex = text.indexOf(".");
        editText.setSelection(0,
                (separatorIndex == -1 || mDocument.isDirectory()) ? text.length() : separatorIndex);
    }

    /**
     * Clears selection in text field.
     *
     * @param editText text field to be cleared.
     */
    private void clearFileNameSelection(EditText editText) {
        editText.setSelection(0, 0);
    }

    private void renameDocuments(String newDisplayName) {
        BaseActivity activity = (BaseActivity) getActivity();

        if (newDisplayName.equals(mDocument.displayName)) {
            mDialog.dismiss();
        } else if (!isValidDocumentName(newDisplayName)) {
            Log.w(TAG, "Failed to rename file - invalid name:" + newDisplayName);
            Snackbars.makeSnackbar(getActivity(), R.string.rename_error,
                    Snackbar.LENGTH_SHORT).show();
        } else if (activity.getInjector().getModel().hasFileWithName(newDisplayName)){
            mRenameInputWrapper.setError(getContext().getString(R.string.name_conflict));
            selectFileName(mEditText);
            Metrics.logRenameFileError(getContext());
        } else {
            new RenameDocumentsTask(activity, newDisplayName).execute(mDocument);
        }

    }

    private class RenameDocumentsTask extends AsyncTask<DocumentInfo, Void, DocumentInfo> {
        private final BaseActivity mActivity;
        private final String mNewDisplayName;

        public RenameDocumentsTask(BaseActivity activity, String newDisplayName) {
            mActivity = activity;
            mNewDisplayName = newDisplayName;
        }

        @Override
        protected void onPreExecute() {
            mActivity.setPending(true);
        }

        @Override
        protected DocumentInfo doInBackground(DocumentInfo... document) {
            assert(document.length == 1);

            return mActivity.getInjector().actions.renameDocument(mNewDisplayName, document[0]);
        }

        @Override
        protected void onPostExecute(DocumentInfo result) {
            if (result != null) {
                Metrics.logRenameFileOperation(getContext());
            } else {
                Snackbars.showRenameFailed(mActivity);
                Metrics.logRenameFileError(getContext());
            }
            if (mDialog != null) {
                mDialog.dismiss();
            }
            mActivity.setPending(false);
        }
    }
}
