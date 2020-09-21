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
 * limitations under the License
 */

package com.android.server.telecom.callfiltering;

import android.content.Context;
import android.os.Bundle;

import com.android.internal.telephony.BlockChecker;

public class BlockCheckerAdapter {
    public BlockCheckerAdapter() { }

    /**
     * Check whether the number is blocked.
     *
     * @param context the context of the caller.
     * @param number the number to check.
     * @param extras the extra attribute of the number.
     * @return {@code true} if the number is blocked. {@code false} otherwise.
     */
    public boolean isBlocked(Context context, String number, Bundle extras) {
        return BlockChecker.isBlocked(context, number, extras);
    }
}
