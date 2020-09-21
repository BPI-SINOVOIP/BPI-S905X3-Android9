// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
/*
 *******************************************************************************
 * Copyright (C) 2015, Google, Inc., International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.dev.test.util;

import com.ibm.icu.util.LocaleMatcher.LanguageMatcherData;

/**
 * @author markdavis
 *
 */
public class LocaleMatcherShim {
    public static LanguageMatcherData load() {
        // In CLDR, has different value
        return null;
    }
}
