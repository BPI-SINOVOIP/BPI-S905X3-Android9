package com.android.systemui.statusbar.policy;

public class PoweroffUtils {
    static {
        System.loadLibrary("poweroff_jni");
    }

    public static void Poweroff() {
        native_Poweroff();
    }

    private native static void native_Poweroff();
}
