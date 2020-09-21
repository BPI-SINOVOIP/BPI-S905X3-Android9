/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.databinding;

/**
 * Utility class for managing ObservableMap callbacks.
 */
public class MapChangeRegistry
        extends CallbackRegistry<ObservableMap.OnMapChangedCallback, ObservableMap, Object> {

    private static NotifierCallback<ObservableMap.OnMapChangedCallback, ObservableMap, Object> NOTIFIER_CALLBACK =
            new NotifierCallback<ObservableMap.OnMapChangedCallback, ObservableMap, Object>() {
                @Override
                public void onNotifyCallback(ObservableMap.OnMapChangedCallback callback, ObservableMap sender,
                        int arg, Object arg2) {
                    callback.onMapChanged(sender, arg2);
                }
            };

    public MapChangeRegistry() {
        super(NOTIFIER_CALLBACK);
    }

    /**
     * Notifies registered callbacks that an element has been added, removed, or changed.
     *
     * @param sender The map that has changed.
     * @param key The key of the element that changed.
     */
    public void notifyChange(ObservableMap sender, Object key) {
        notifyCallbacks(sender, 0, key);
    }
}
