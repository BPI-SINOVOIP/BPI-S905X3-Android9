package com.droidlogic.readdbfile;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

import java.lang.ref.WeakReference;

public abstract class WeakHandler<T> extends Handler {
    private final WeakReference<T> mRef;

    /**
     * Constructs a new handler with a weak reference to the given referent using the provided
     * Looper instead of the default one.
     *
     * @param looper The looper, must not be null.
     * @param ref the referent to track
     */
    public WeakHandler(Looper looper, T ref) {
        super(looper);
        mRef = new WeakReference<>(ref);
    }

    /**
     * Constructs a new handler with a weak reference to the given referent.
     *
     * @param ref the referent to track
     */
    public WeakHandler(T ref) {
        mRef = new WeakReference<>(ref);
    }

    /** Calls {@link #handleMessage(Message, Object)} if the WeakReference is not cleared. */
    @Override
    public final void handleMessage(Message msg) {
        T referent = mRef.get();
        if (referent == null) {
            return;
        }
        handleMessage(msg, referent);
    }

    /**
     * Subclasses must implement this to receive messages.
     *
     * <p>If the WeakReference is cleared this method will no longer be called.
     *
     * @param msg the message to handle
     * @param referent the referent. Guaranteed to be non null.
     */
    protected abstract void handleMessage(Message msg, T referent);
}
