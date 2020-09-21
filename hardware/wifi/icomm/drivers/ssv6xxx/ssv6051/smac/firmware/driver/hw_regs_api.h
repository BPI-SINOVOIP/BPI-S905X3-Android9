#ifndef _HW_REGS_API_H_
#define _HW_REGS_API_H_



/**
 * Define Constant vlaues for WSID:
 *
 */
#define WSID_0                          0
#define WSID_1                          1
#define WSID_GROUP_ADDR               0xe  
#define WSID_NOT_FOUND                0xf


/**
* struct mac_wsid_tabl_st - define wsid (wiress session ID) entry.
*
* @ 
*/
typedef struct wsid_entry_st {
    u32             valid:1;
	u32		  		peer_qos_en:1;		  /* 0: Non-QoS STA, 1: QoS STA */
	u32		 		peer_op_mode:2;		  /* 0:STA, 1: AP, 2: AD-HOC, 3: WDS */ 
  	u32		  		peer_ht_mode:2;		  /* 0: Non-HT, 1: HT_MF, 2: HT_GF, 3: RSVD */
	ETHER_ADDR  	peer_mac; 			  /* RA for Tx Frames, TA for Rx Frames */
	u8          	tx_ack_policy[8];     /* 2 bits per UP */	
	u16		  		tx_seq_ctrl[8];		  /* Tx Frame sequence number: 12bits */
	u16		  		rx_seq_ctrl[8];		  /* Duplicate Detection: support WMM only: 16bits */
} wsid_entry;




u32 security_key_buffer_init(void);
u32 __HWREG_Init(void);




#endif /* _HW_REGS_API_H_ */



