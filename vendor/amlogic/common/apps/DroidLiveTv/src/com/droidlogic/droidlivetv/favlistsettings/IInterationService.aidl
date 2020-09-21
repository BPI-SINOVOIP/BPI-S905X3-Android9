// IInterationService.aidl
package com.droidlogic.droidlivetv.favlistsettings;

import com.droidlogic.droidlivetv.favlistsettings.IInterationCallback;

// Declare any non-default types here with import statements
interface IInterationService {
    void registerInterationCallback(/*String value*/IInterationCallback callback);
    void unRegisterInterationCallback(/*String value*/IInterationCallback callback);
    //void sendInterationCallback(in Map<String, String> map);
    void sendInterationCallback(String value);
}
