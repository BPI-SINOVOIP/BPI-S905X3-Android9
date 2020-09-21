#include <regs.h>

static OsTimer monitor_timer;
static u32 repeatTimes = 0;
static u8 monitorMode = 0;
static u8 monitorState = 0;

void mointerHandler()
{
    /*
        #define M_ENG_CPU                   0x00
        #define M_ENG_HWHCI               0x01
        #define M_ENG_EMPTY	                0x02
        #define M_ENG_ENCRYPT            0x03
        #define M_ENG_MACRX               0x04  
        #define M_ENG_MIC	                0x05
        #define M_ENG_TX_EDCA0           0x06
        #define M_ENG_TX_EDCA1           0x07
        #define M_ENG_TX_EDCA2           0x08
        #define M_ENG_TX_EDCA3           0x09
        #define M_ENG_TX_MNG              0x0A
        #define M_ENG_ENCRYPT_SEC      0x0B
        #define M_ENG_MIC_SEC             0x0C
        #define M_ENG_RESERVED_1       0x0D
        #define M_ENG_RESERVED_2       0x0E
        #define M_ENG_TRASH_CAN         0x0F
        #define M_ENG_MAX                   (M_ENG_TRASH_CAN+1)
        */
	if(repeatTimes > 0)
	{
		switch(monitorMode)
		{
			case 1:
				{
					printf("%2d %2d %2d %2d\n",
						GET_FFO6_CNT,GET_FFO7_CNT, GET_FFO8_CNT, GET_FFO9_CNT);
				}
				break;
			case 2:
				{
					printf("TX %3d RX %3d AVA %3d %08x\n",GET_TX_ID_ALC_LEN,GET_RX_ID_ALC_LEN,GET_AVA_TAG);
				}
				break;
			case 0:
			default:
				{
					printf("\n");
					printf("MCU-HCI-SEC-RX -MIC-TX0-TX1-TX2-TX3-TX4-SEC-MIC-TSH\n");		
					printf("%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
						GET_FFO0_CNT, GET_FFO1_CNT, GET_FFO3_CNT, GET_FFO4_CNT, GET_FFO5_CNT, GET_FFO6_CNT,
						GET_FFO7_CNT, GET_FFO8_CNT, GET_FFO9_CNT, GET_FFO10_CNT, GET_FFO11_CNT, GET_FFO12_CNT, GET_FFO15_CNT);
					printf("TX[%d]RX[%d]AVA[%d] \n",GET_TX_ID_ALC_LEN,GET_RX_ID_ALC_LEN,GET_AVA_TAG);

					//if(GET_MRX_MISS)
						//printf("MAC RX allocate fail[%d]\n",GET_MRX_ALC_FAIL);
					//if(GET_RLS_ERR)
						//printf("MMU release fail ID[0x%x] STATE[0x%x]\n",GET_MRX_ALC_FAIL,GET_RL_STATE);
					//if(GET_ALC_ERR)
						//printf("MMU allocate fail ID[0x%x] STATE[0x%x]\n",GET_MRX_ALC_FAIL,GET_AL_STATE);

					//TX_ID_THOLD
				}		
				break;
		}

		repeatTimes--;
		return;
	}
}

static u32 timeInterval = 10;
s32 mac_monitor_init()
{
	//Init 10ms timer 
	if( OS_TimerCreate(&monitor_timer, timeInterval, (u8)TRUE, NULL, (OsTimerHandler)mointerHandler) == OS_FAILED)
		return OS_FAILED;

    OS_TimerStop(monitor_timer);
	return OS_SUCCESS;
}

void mac_monitor_start(u32 monitorTime,u8 mode)
{
	repeatTimes = monitorTime*1000/timeInterval;
	monitorMode = mode;
	monitorState = 1;

	printf("[%ds]\n",monitorTime);

	OS_TimerStart(monitor_timer);
}

void mac_monitor_stop()
{
	if(monitorState)
	{
		if(repeatTimes)
			OS_MsDelay(100);
		else
		{
			OS_TimerStop(monitor_timer);
			monitorState = 0;
		}
	}
}


