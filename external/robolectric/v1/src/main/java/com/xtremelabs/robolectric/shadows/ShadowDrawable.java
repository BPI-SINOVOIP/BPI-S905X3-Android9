package com.xtremelabs.robolectric.shadows;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.TypedValue;

import com.xtremelabs.robolectric.Robolectric;
import com.xtremelabs.robolectric.internal.Implementation;
import com.xtremelabs.robolectric.internal.Implements;
import com.xtremelabs.robolectric.internal.RealObject;

import java.io.InputStream;
import java.util.ArrayList;

import static com.xtremelabs.robolectric.Robolectric.shadowOf;

@SuppressWarnings({"UnusedDeclaration"})
@Implements(Drawable.class)
public class ShadowDrawable {
    private static int defaultIntrinsicWidth = -1;
    private static int defaultIntrinsicHeight = -1;
    static ArrayList<String> corruptStreamSources = new ArrayList<String>();

    @RealObject Drawable realObject;

    private Rect bounds = new Rect(0, 0, 0, 0);
    private int intrinsicWidth = defaultIntrinsicWidth;
    private int intrinsicHeight = defaultIntrinsicHeight;
    private int alpha;
    private InputStream inputStream;
    private int level;
    private int loadedFromResourceId = -1;
    private boolean wasInvalidated;

    @Implementation
    public static Drawable createFromStream(InputStream is, String srcName) {
        if (corruptStreamSources.contains(srcName)) {
            return null;
        }
        BitmapDrawable drawable = new BitmapDrawable(Robolectric.newInstanceOf(Bitmap.class));
        shadowOf(drawable).setSource(srcName);
        shadowOf(drawable).setInputStream(is);
        return drawable;
    }

    @Implementation
    public static Drawable createFromResourceStream(Resources res, TypedValue value, InputStream is, String srcName) {
        return createFromStream(is, srcName);
    }

    @Implementation
    public static Drawable createFromResourceStream(Resources res, TypedValue value, InputStream is, String srcName, BitmapFactory.Options opts) {
        return createFromStream(is, srcName);
    }

    @Implementation
    public static Drawable createFromPath(String pathName) {
        BitmapDrawable drawable = new BitmapDrawable(Robolectric.newInstanceOf(Bitmap.class));
        shadowOf(drawable).setPath(pathName);
        return drawable;
    }

    public static Drawable createFromResourceId(int resourceId) {
        Bitmap bitmap = Robolectric.newInstanceOf(Bitmap.class);
        shadowOf(bitmap).setLoadedFromResourceId(resourceId);
        BitmapDrawable drawable = new BitmapDrawable(bitmap);
        return drawable;
    }

    @Implementation
    public final Rect getBounds() {
        return bounds;
    }

    @Implementation
    public void setBounds(Rect rect) {
        this.bounds = rect;
    }

    @Implementation
    public void setBounds(int left, int top, int right, int bottom) {
        bounds = new Rect(left, top, right, bottom);
    }

    @Implementation
    public Rect copyBounds() {
        Rect bounds = new Rect();
        copyBounds(bounds);
        return bounds;
    }

    @Implementation
    public void copyBounds(Rect bounds) {
        bounds.set(getBounds());
    }

    @Implementation
    public int getIntrinsicWidth() {
        return intrinsicWidth;
    }

    @Implementation
    public int getIntrinsicHeight() {
        return intrinsicHeight;
    }

    public static void addCorruptStreamSource(String src) {
        corruptStreamSources.add(src);
    }

    public static void setDefaultIntrinsicWidth(int defaultIntrinsicWidth) {
        ShadowDrawable.defaultIntrinsicWidth = defaultIntrinsicWidth;
    }

    public static void setDefaultIntrinsicHeight(int defaultIntrinsicHeight) {
        ShadowDrawable.defaultIntrinsicHeight = defaultIntrinsicHeight;
    }

    public void setIntrinsicWidth(int intrinsicWidth) {
        this.intrinsicWidth = intrinsicWidth;
    }

    public void setIntrinsicHeight(int intrinsicHeight) {
        this.intrinsicHeight = intrinsicHeight;
    }

    public InputStream getInputStream() {
        return inputStream;
    }

    public void setInputStream(InputStream inputStream) {
        this.inputStream = inputStream;
    }

    @Implementation
    public int getLevel() {
        return level;
    }

    @Implementation
    public boolean setLevel(int level) {
        this.level = level;
        // This should return true if the new level causes a layout change.
        // Doing this in robolectric would require parsing level sets which
        // is not currently supported.
        return false;
    }

    @Override @Implementation
    public boolean equals(Object o) {
        if (realObject == o) return true;
        if (o == null || realObject.getClass() != o.getClass()) return false;

        ShadowDrawable that = shadowOf((Drawable) o);

        if (intrinsicHeight != that.intrinsicHeight) return false;
        if (intrinsicWidth != that.intrinsicWidth) return false;
        if (bounds != null ? !bounds.equals(that.bounds) : that.bounds != null) return false;

        return true;
    }

    @Override @Implementation
    public int hashCode() {
        int result = bounds != null ? bounds.hashCode() : 0;
        result = 31 * result + intrinsicWidth;
        result = 31 * result + intrinsicHeight;
        return result;
    }

    @Implementation
    public void setAlpha(int alpha) {
        this.alpha = alpha;
    }

    @Implementation
    public void invalidateSelf() {
        wasInvalidated = true;
    }

    public int getAlpha() {
        return alpha;
    }

    public static void reset() {
        corruptStreamSources.clear();
    }

    public int getLoadedFromResourceId() {
        return loadedFromResourceId;
    }

    public void setLoadedFromResourceId(int resourceId) {
        loadedFromResourceId = resourceId;
    }

    public boolean wasInvalidated() {
        return wasInvalidated;
    }
}
