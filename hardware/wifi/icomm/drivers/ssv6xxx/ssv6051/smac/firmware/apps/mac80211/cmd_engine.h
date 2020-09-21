#ifndef _CMD_ENGINE_H_
#define _CMD_ENGINE_H_

#include <cmd_def.h>

// s32 CmdEngine_SendHostEvent(void *MSG);

void CmdEngine_HCmdRequest( void *MSG );
void CmdEngine_Task( void *args );

s32 cmd_engine_init(void);



#endif /* _CMD_ENGINE_H_ */

