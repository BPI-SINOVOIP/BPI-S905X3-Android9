/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DroidLogicKeyEvent
 */

package com.droidlogic.app;

import android.view.KeyEvent;

public class DroidLogicKeyEvent extends KeyEvent{

    //tv key begin
    public static final int KEYCODE_TV_SHORTCUTKEY_GLOBALSETUP    = 2001;
    public static final int KEYCODE_TV_SHORTCUTKEY_SOURCE_LIST    = 2002;
    public static final int KEYCODE_TV_SHORTCUTKEY_3DMODE         = 2003;
    public static final int KEYCODE_TV_SHORTCUTKEY_DISPAYMODE     = 2004;
    public static final int KEYCODE_TV_SHORTCUTKEY_VIEWMODE       = 2005;
    public static final int KEYCODE_TV_SHORTCUTKEY_VOICEMODE      = 2006;
    public static final int KEYCODE_TV_SHORTCUTKEY_TVINFO         = 2007;
    public static final int KEYCODE_EARLY_POWER                   = 2008;
    public static final int KEYCODE_TV_SLEEP                      = 2009;
    public static final int KEYCODE_TV_SOUND_CHANNEL              = 2010;
    public static final int KEYCODE_TV_REPEAT                     = 2011;
    public static final int KEYCODE_TV_SUBTITLE                   = 2012;
    public static final int KEYCODE_TV_SWITCH                     = 2013;
    public static final int KEYCODE_TV_WASU                       = 2014;
    public static final int KEYCODE_TV_VTION                      = 2015;
    public static final int KEYCODE_TV_BROWSER                    = 2016;
    public static final int KEYCODE_TV_ALTERNATE                  = 2017;
    public static final int KEYCODE_FAV                           = 2018;
    public static final int KEYCODE_LIST                          = 2019;
    public static final int KEYCODE_MEDIA_AUDIO_CONTROL           = 2020;
    //tv key end

    public DroidLogicKeyEvent(KeyEvent origEvent) {
        super(origEvent);
        // TODO Auto-generated constructor stub
    }
}
