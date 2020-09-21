package com.android.verifiedboot.globalstate.callback;

import javacard.framework.Shareable;

public interface CallbackInterface extends Shareable {
    /**
     * Called when a triggerDataClear() occurs on the GlobalState applet.
     * When the applet is complete, it should call reportDataCleared()
     * on the GlobalStateInterface.
     */
    void clearData();
};
