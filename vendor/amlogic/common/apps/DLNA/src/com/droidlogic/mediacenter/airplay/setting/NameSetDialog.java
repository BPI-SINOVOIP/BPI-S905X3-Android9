/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.mediacenter.airplay.setting;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.TextView;
import com.droidlogic.mediacenter.R;
public class NameSetDialog extends AlertDialog implements TextWatcher {

        private View mView;
        private TextView mName;
        private String mOldName;
        private final DialogInterface.OnClickListener mListener;

        public NameSetDialog ( Context context, DialogInterface.OnClickListener listener, String oldName ) {
            super ( context );
            mListener = listener;
            mOldName = oldName;
        }

        public String getName() {
            return mName != null ? mName.getText().toString() : null;
        }

        @Override
        protected void onCreate ( Bundle savedInstanceState ) {
            mView = getLayoutInflater().inflate ( R.layout.name_set_dialog, null );
            setView ( mView );
            setInverseBackgroundForced ( true );
            Context context = getContext();
            setTitle ( R.string.setting_device_title );
            mName = ( TextView ) mView.findViewById ( R.id.name );
            mName.setText ( mOldName );
            setButton ( DialogInterface.BUTTON_POSITIVE,
                        context.getString ( R.string.dialog_btn_ok ), mListener );
            setButton ( DialogInterface.BUTTON_NEGATIVE,
                        context.getString ( R.string.dialog_btn_cancel ), mListener );
            mName.addTextChangedListener ( this );
            super.onCreate ( savedInstanceState );
            validate();
        }

        private void validate() {
            if ( ( mName != null && mName.length() == 0 ) ) {
                getButton ( DialogInterface.BUTTON_POSITIVE ).setEnabled ( false );
            } else {
                getButton ( DialogInterface.BUTTON_POSITIVE ).setEnabled ( true );
            }
        }

        @Override
        public void afterTextChanged ( Editable arg0 ) {
            validate();
        }

        @Override
        public void beforeTextChanged ( CharSequence arg0, int arg1, int arg2,
                                        int arg3 ) {
            // TODO Auto-generated method stub
        }

        @Override
        public void onTextChanged ( CharSequence arg0, int arg1, int arg2, int arg3 ) {
            // TODO Auto-generated method stub
        }

}
