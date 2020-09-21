package com.android.verifiedboot.globalstate.owner;

import javacard.framework.Shareable;

public interface OwnerInterface extends Shareable {
    /**
     * Stores the calling applet's AID which must implement the NotifyInterface.
     * When a global data clear is requested, dataChanged() will be called.
     *
     * @param unregister whether to register or unregister for data change.
     *
     * Returns true if registered and false if not.
     */
    boolean notifyOnDataClear(boolean unregister);

    /**
     * Stores that the calling applet completed onDataClear() call.
     * If this is called, the applet will received a onDataClear() will
     * be renotified on each isDataCleared() call from any other applet.
     */
    void reportDataCleared();

    /**
     * Returns true if the caller has pending data to clear.
     */
    boolean dataClearNeeded();

    /**
     * Returns true if there is no pending clients needing to clear.
     *
     * This is often called by the boot support client to determine
     * if a new notify call is needed.
     */
    boolean globalDataClearComplete();

    /**
     * Notifies all applets that a dataClear is underway.
     * (The calling applet will also be notified, if registered.)
     *
     * @param resume indicates renotifying only pending applets.
     */
    void triggerDataClear(boolean resume);


    /**
     * Returns true if the external signal indicates the bootloader
     * is still in control of the application proceesor.
     */
    boolean inBootloader();

    /**
     * Sets the {@link #production} value.
     *
     * @param val 
     * @return true if the value could be assigned.
     */
    boolean setProduction(boolean val);

    /**
     * Returns if the Interface has been transitioned to production mode.
     *
     * @return true if in production mode and false if in provisioning.
     */
    boolean production();
};
