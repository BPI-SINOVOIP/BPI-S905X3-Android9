/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_CTVGROUP_H)
#define _CTVGROUP_H

#include <utils/Vector.h>
using namespace android;
class CTvGroup {
public:
    CTvGroup();
    ~CTvGroup();
    static Vector<CTvGroup> selectByGroup();
    static void addGroup();
    static void deleteGroup();
    static void editGroup();
};

#endif  //_CTVGROUP_H
