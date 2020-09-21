/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.bluetooth.avrcp;


/*************************************************************************************************
 * Interface for classes which handle callbacks from AvrcpMediaManager.
 * These callbacks should map to native responses and used to communicate with the native layer.
 ************************************************************************************************/

public interface AvrcpMediaRspInterface {
    void setAddrPlayerRsp(byte[] address, int rspStatus);

    void setBrowsedPlayerRsp(byte[] address, int rspStatus, byte depth, int numItems,
            String[] textArray);

    void mediaPlayerListRsp(byte[] address, int rspStatus, MediaPlayerListRsp rspObj);

    void folderItemsRsp(byte[] address, int rspStatus, FolderItemsRsp rspObj);

    void changePathRsp(byte[] address, int rspStatus, int numItems);

    void getItemAttrRsp(byte[] address, int rspStatus, ItemAttrRsp rspObj);

    void playItemRsp(byte[] address, int rspStatus);

    void getTotalNumOfItemsRsp(byte[] address, int rspStatus, int uidCounter, int numItems);

    void addrPlayerChangedRsp(int type, int playerId, int uidCounter);

    void avalPlayerChangedRsp(byte[] address, int type);

    void uidsChangedRsp(int type);

    void nowPlayingChangedRsp(int type);

    void trackChangedRsp(int type, byte[] uid);
}

