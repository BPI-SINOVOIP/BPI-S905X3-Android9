package com.android.keychain.internal;

/** Interface for classes that provide information about keys in KeyChain.  */

public interface KeyInfoProvider {
    /**
     * Indicates whether a key associated with the given alias is allowed
     * to be selected by users.
     * @param alias Alias of the key to check.
     *
     * @return true if the provided alias is selectable by users.
     */
    public boolean isUserSelectable(String alias);
}
