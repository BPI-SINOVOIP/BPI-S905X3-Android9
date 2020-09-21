/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   yongguang.hong
 *  @version  1.0
 *  @date     2018/03/19
 *  @par function description:
 *  - 1 control bluetooth RC
 */

#ifndef ANDROID_REMOTE_CONTROL_SERVER_H
#define ANDROID_REMOTE_CONTROL_SERVER_H


typedef void (*setMicStatusCallback)(int);
typedef void (*onDeviceStatusCallback)(int);

typedef struct {
    setMicStatusCallback setMicStatusCb;
    onDeviceStatusCallback onDeviceStatusCb;
} rc_callbacks_t;

class RemoteControlImpl;

class RemoteControlServer {
public:
    RemoteControlServer();
    virtual ~RemoteControlServer();

    void setCallback(rc_callbacks_t *rcCallback);
    bool isServiceStart() {return (mInstance != NULL);}

    static RemoteControlServer *getInstance(void);

private:
    RemoteControlImpl* mImpl;
    static RemoteControlServer* mInstance;
};


#endif // ANDROID_REMOTE_CONTROL_SERVER_H
