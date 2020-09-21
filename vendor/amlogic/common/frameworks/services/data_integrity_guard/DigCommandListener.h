/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _DIGCOMMANDLISTENER_H__
#define _DIGCOMMANDLISTENER_H__

#include <sysutils/FrameworkListener.h>
#include <utils/Errors.h>
#include <sysutils/FrameworkCommand.h>

class DigCommandListener : public FrameworkListener {
public:
    DigCommandListener();
    virtual ~DigCommandListener() {}

private:
    class DigCmd : public FrameworkCommand {
    public:
        DigCmd();
        virtual ~DigCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };
};

#endif /* _DIGCOMMANDLISTENER_H__ */
