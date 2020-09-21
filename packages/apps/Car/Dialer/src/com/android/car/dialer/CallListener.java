package com.android.car.dialer;

import com.android.car.dialer.telecom.UiCall;

/** Interface for listening to call state changes. */
public interface CallListener {
    void onAudioStateChanged(boolean isMuted, int route, int supportedRouteMask);

    void onCallStateChanged(UiCall call, int state);

    void onCallUpdated(UiCall call);

    void onCallAdded(UiCall call);

    void onCallRemoved(UiCall call);
}
