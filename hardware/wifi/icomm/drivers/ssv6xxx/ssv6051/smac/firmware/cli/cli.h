#ifndef _CLI_H_
#define _CLI_H_


typedef void (*CliCmdFunc) ( s32, s8 ** );


typedef struct CLICmds_st
{
        const s8           *Cmd;
        CliCmdFunc          CmdHandler;
        const s8           *CmdUsage;
        
} CLICmds, *PCLICmds;

extern const CLICmds gCliCmdTable[];

void Cli_Init( void );
void Cli_Start( void );
void Cli_Task( void *args );



#endif /* _CLI_H_ */

