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
 *  @author   Tellen Yu
 *  @version  2.0
 *  @date     2014/11/04
 *  @par function description:
 *  - 1 set display mode for recovery
 */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "ubootenv/Ubootenv.h"
#include "DisplayMode.h"

int main(int argc, char** argv)
{
    const char* path = NULL;
    if(argc >= 2){
        path = argv[1];
    }

    Ubootenv *pUbootenv = new Ubootenv();

    DisplayMode displayMode(path, pUbootenv);
    displayMode.init();

    //don't end this progress, wait for hdmi plug detect thread.
    while (1) {
        usleep(10000000);
    }

    return 0;
}

