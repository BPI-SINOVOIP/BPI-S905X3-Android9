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

import android.app.Dialog;

/**
 * Letting the class, assumed to be Fragment, create a Dialog on it. Should be useful
 * you want to utilize some capability in {@link SettingsPreferenceFragment} but don't want
 * the class inherit the class itself (See {@link ProxySelector} for example).
 */
public interface DialogCreatable {

    public Dialog onCreateDialog ( int dialogId );
}
