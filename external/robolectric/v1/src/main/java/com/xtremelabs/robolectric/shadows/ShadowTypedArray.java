package com.xtremelabs.robolectric.shadows;

import android.content.res.Resources;
import android.content.res.TypedArray;
import com.xtremelabs.robolectric.Robolectric;
import com.xtremelabs.robolectric.internal.Implementation;
import com.xtremelabs.robolectric.internal.Implements;

@SuppressWarnings({"UnusedDeclaration"})
@Implements(TypedArray.class)
public class ShadowTypedArray {
    @Implementation
    public Resources getResources() {
        return Robolectric.application.getResources();
    }

    @Implementation
    public boolean getBoolean(int index, boolean defValue) {
        return defValue;
    }

    @Implementation
    public float getFloat(int index, float defValue) {
        return defValue;
    }

    @Implementation
    public int getInt(int index, int defValue) {
        return defValue;
    }

    @Implementation
    public int getInteger(int index, int defValue) {
        return defValue;
    }

    @Implementation
    public float getDimension(int index, float defValue) {
        return defValue;
    }

    @Implementation
    public int getDimensionPixelOffset(int index, int defValue) {
        return defValue;
    }

    @Implementation
    public int getDimensionPixelSize(int index, int defValue) {
        return defValue;
    }

    @Implementation
    public int getLayoutDimension(int index, int defValue) {
        return defValue;
    }

    @Implementation
    public int getResourceId(int index, int defValue) {
        return defValue;
    }
}
