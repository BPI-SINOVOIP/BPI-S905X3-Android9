/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.droidlogic.app;

import android.content.Context;
import android.graphics.Typeface;
import android.util.Log;
import java.io.File;
/**
 * Created by daniel on 06/11/2017.
 */

public class CustomFonts {
    final String TAG = "CustomFonts";
    Typeface mMonoSerifTf;
    Typeface mMonoSerifItTf;
    Typeface mCasualTf;
    Typeface mCasualItTf;
    Typeface mPropSansTf;
    Typeface mPropSansItTf;
    Typeface mSmallCapitalTf;
    Typeface mSmallCapitalItTf;
    Typeface mCursiveTf;
    Typeface mCursiveItTf;

    CustomFonts(Context context) {
        try {
            mMonoSerifTf = Typeface.createFromFile(new File(context.getDataDir(), "font/cinecavD_mono.ttf"));
            mMonoSerifItTf = Typeface.createFromFile(new File(context.getDataDir(), "font/cinecavD_mono_it.ttf"));
            mCasualTf = Typeface.createFromFile(new File(context.getDataDir(), "font/cinecavD_casual.ttf"));
            mCasualItTf = Typeface.createFromFile(new File(context.getDataDir(), "font/cinecavD_casual_it.ttf"));
            mPropSansTf = Typeface.createFromFile(new File(context.getDataDir(), "font/cinecavD_serif.ttf"));
            mPropSansItTf = Typeface.createFromFile(new File(context.getDataDir(), "font/cinecavD_serif_it.ttf"));
            mSmallCapitalTf = Typeface.createFromFile(new File(context.getDataDir(), "font/cinecavD_sc.ttf"));
            mSmallCapitalItTf = Typeface.createFromFile(new File(context.getDataDir(), "font/cinecavD_sc_it.ttf"));
            mCursiveTf = Typeface.createFromFile(new File(context.getDataDir(), "font/cinecavD_script.ttf"));
            mCursiveItTf = Typeface.createFromFile(new File(context.getDataDir(), "font/cinecavD_script_it.ttf"));
        } catch (Exception e) {
            Log.e(TAG, "error " + e.toString());
        }
    }
}
