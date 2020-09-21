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
/*
 * Copyright (c) 2014-2017, The Linux Foundation.
 */

/*
 * Copyright 2012 Giesecke & Devrient GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package com.android.se.security.arf;

import com.android.se.Terminal;
import com.android.se.security.AccessRuleCache;
import com.android.se.security.arf.pkcs15.PKCS15Handler;

import java.io.IOException;
import java.util.MissingResourceException;
import java.util.NoSuchElementException;

/** Initializes and maintains the ARF access rules of a Secure Element */
public class ArfController {

    private PKCS15Handler mPkcs15Handler = null;
    private SecureElement mSecureElement = null;
    private AccessRuleCache mAccessRuleCache = null;
    private Terminal mTerminal = null;

    public ArfController(AccessRuleCache cache, Terminal terminal) {
        mAccessRuleCache = cache;
        mTerminal = terminal;
    }

    /** Initializes the ARF Rules for the Secure Element */
    public synchronized boolean initialize() throws IOException, MissingResourceException,
            NoSuchElementException {
        if (mSecureElement == null) {
            mSecureElement = new SecureElement(this, mTerminal);
        }
        if (mPkcs15Handler == null) {
            mPkcs15Handler = new PKCS15Handler(mSecureElement);
        }
        return mPkcs15Handler.loadAccessControlRules(mTerminal.getName());
    }

    public AccessRuleCache getAccessRuleCache() {
        return mAccessRuleCache;
    }
}
