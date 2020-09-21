/******************************************************************
*
*Copyright (C) 2012  Amlogic, Inc.
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
package com.droidlogic.appinstall;

import android.content.Context;
import android.util.AttributeSet;
import android.view.SoundEffectConstants;
import android.view.View;
import android.widget.*;
import android.view.KeyEvent;
import android.util.Log;

public class CheckAbleList extends ListView {
        private static final String TAG = "CheckAbleList";
        protected OnItemClickListener m_localClickListener = null;
        public CheckAbleList (Context context) {
            super (context);
            // TODO Auto-generated constructor stub
        }
        public CheckAbleList (Context context, AttributeSet attrs) {
            super (context, attrs);
        }

        public CheckAbleList (Context context, AttributeSet attrs, int defStyle) {
            super (context, attrs, defStyle);
        }

        public void setOnItemClickListener (OnItemClickListener listener) {
            super.setOnItemClickListener (listener);
            m_localClickListener = listener;
        }

        public boolean performItemClick (View view, int position, long id) {
            //i don't want to set the item checked when i click a item
            if (m_localClickListener != null) {
                playSoundEffect (SoundEffectConstants.CLICK);
                m_localClickListener.onItemClick (this, view, position, id);
                return true;
            }
            return false;
        }

        public void setAllItemChecked (boolean value) {
            int i = 0;
            for (; i < this.getCount(); i++) {
                setItemChecked (i, value);
                getAdapter().getView (i, getChildAt (i), this); //for redraw
            }
        }

        public boolean onKeyDown (int keyCode, KeyEvent event) {
            if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
                int PosInList = 0;
                PosInList = this.getSelectedItemPosition();
                if (!isItemChecked (PosInList)) {
                    setItemChecked (PosInList, true);
                }
                else {
                    setItemChecked (PosInList, false);
                }
                Log.e (TAG, "--------------------------------------------RC is checked");
            }
            return super.onKeyDown (keyCode, event);
        }
}
