#ifndef _ERRNO_H_
#define _ERRNO_H_ 




/* Define constant value for the reason of trapping to CPU */


/**
 *  Reasons for trapping Tx/Rx frames in hardware to CPU.
 *
 *  @ID_TRAP_NONE: Shall not happen. In this case, just drop the frame.
 *  @ID_TRAP_HOSTCMD: Host commands to wifi controller.
 *  @ID_TRAP_INVAFRAME: Invalid frame.
 *
 *  @ID_TRAP_NULLDATA: IEEE 802.11 Data frame with NULL function.
 *  @ID_TRAP_WDS: IEEE 802.11 frames with 4-ADDR format.
 *  @
 *
 *
 */
//sgSoftMac_Func
#define ID_TRAP_NONE                     0
//If register AMPDU-SINFFER enable ... trap AMPDU-PACKET to CPU.
//[1]-	AMPDU-TRAP
#define ID_TRAP_WAPI					 2
#define ID_TRAP_UNKNOW_FTYPE             3 
#define ID_TRAP_BAD_CTRL_LEN             4
#define ID_TRAP_BAD_MGMT_LEN             5
#define ID_TRAP_BAD_DATA_LEN             6
#define ID_TRAP_NO_WSID_FOUND            7
#define ID_TRAP_WDS_FORMAT               8
#define ID_TRAP_TX_ETHER_TRAP            9
#define ID_TRAP_RX_ETHER_TRAP           10
//[11]
#define ID_TRAP_UNSUPPORT_TID           12
#define ID_TRAP_MODULE_REJECT           13
#define ID_TRAP_NULL_DATA               14
#define ID_TRAP_BAD_LLCSNAP             15
#define ID_TRAP_TX_XMIT_SUCCESS 		16
#define ID_TRAP_TX_XMIT_FAIL 			17
//[18]
//[19]
#define ID_TRAP_FRAG_OUT_OF_ENTRY       20 
//[21]
#define ID_TRAP_DUPLICATE               22
#define ID_TRAP_MC_FILTER               23
#define ID_TRAP_UNKNOW_CTYPE            24
//[25]
//[26]
//[27]
#define ID_TRAP_HCI_BAD_WSID            28
#define ID_TRAP_SW_TRAP                 29
#define ID_TRAP_CTRL_NOT_BA_BAR         30
#define ID_TRAP_DECISION_FLOW           31

//For security Error
//Security reserved [32-40]
#define ID_TRAP_SECURITY				32
#define ID_TRAP_KEY_IDX_MISMATCH       	ID_TRAP_SECURITY+1	//-33-WEP.TKIP.CCMP
#define ID_TRAP_DECODE_FAIL				ID_TRAP_SECURITY+2	//-34-WEP.TKIP.CCMP
#define ID_TRAP_REPLAY					ID_TRAP_SECURITY+4	//-36-TKIP.CCMP
#define ID_TRAP_SECURITY_35				35					//Index + Decode 
#define ID_TRAP_SECURITY_37				37					//Index + Replay
#define ID_TRAP_SECURITY_38				38					//Decode + Replay
#define ID_TRAP_SECURITY_39				39					//Index + Decode + Replay
#define ID_TRAP_SECURITY_BAD_LEN		ID_TRAP_SECURITY+8	//-40-[wep payload < 8][tkip payload < 12][ccmp payload < 16]
#define ID_TRAP_TKIP_MIC_ERROR			41					
//Next start number [42]

//Trap for software reason
#define ID_TRAP_SW_START                42
#define ID_TRAP_SW_TXL34CS              ID_TRAP_SW_START+0
#define ID_TRAP_SW_RXL34CS              ID_TRAP_SW_START+1
#define ID_TRAP_SW_DEFRAG               ID_TRAP_SW_START+2
#define ID_TRAP_SW_EDCATX               ID_TRAP_SW_START+3
#define ID_TRAP_SW_RXDATA               ID_TRAP_SW_START+4
#define ID_TRAP_SW_RXMGMT               ID_TRAP_SW_START+5
#define ID_TRAP_SW_RXCTRL               ID_TRAP_SW_START+6
#define ID_TRAP_SW_FRAGMENT             ID_TRAP_SW_START+7
#define ID_TRAP_SW_TXTPUT               ID_TRAP_SW_START+8
#define ID_TRAP_MAX                     51





















#endif /* _ERRNO_H_ */
