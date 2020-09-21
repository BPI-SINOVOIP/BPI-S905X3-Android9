/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _C_FBC_LINKER_H_
#define _C_FBC_LINKER_H_

#include "CFbcCommand.h"
#include "CFbcProtocol.h"
#include "CFbcUpgrade.h"

class CFbcLinker {
public:
    CFbcLinker();
    ~CFbcLinker();
    static CFbcLinker *getFbcLinkerInstance();
    int sendCommandToFBC(unsigned char *data, int count, int flag);
    int startFBCUpgrade(char *file_name, int mode, int blk_size);

    CFbcUpgrade *pUpgradeIns;
    CFbcProtocol *pProtocolIns;

private:
    static CFbcLinker *mFbcLinkerInstance;
};
#endif
