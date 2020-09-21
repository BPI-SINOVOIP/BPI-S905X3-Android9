// IDTVKit.aidl
package org.dtvkit;

import org.dtvkit.ISignalHandler;
import org.dtvkit.IOverlayTarget;

interface IDTVKit {
    String request(String method, String json);
    void registerSignalHandler(ISignalHandler handler);
    void registerOverlayTarget(IOverlayTarget target);
}
