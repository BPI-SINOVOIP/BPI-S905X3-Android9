/*------------------------------------------------------------------------------
 * Copyright � 2002-2005, Silicon Image, Inc.  All rights reserved.
 *
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of: Silicon Image, Inc.,
 * 1060 East Arques Avenue, Sunnyvale, California 94085
 *----------------------------------------------------------------------------
 */

#include "SiITypeDefs.h"
/*#include "UDebugHLIIC.h"*/


BYTE siiFindEEPROM(void);
BYTE siiBlockReadEEPROM(WORD Addr, BYTE NBytes, BYTE *Data);
void siiBlockWriteEEPROM(WORD Addr, BYTE NBytes, BYTE *Data);

#define siiWaitForAckEEPROM(TO) hlWaitForAck(EE_SLVADDR, TO)

