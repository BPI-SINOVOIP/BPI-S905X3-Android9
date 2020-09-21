#ifndef _DRV_AMPDU_H_
#define _DRV_AMPDU_H_

#include <regs.h>
#include <uart/drv_uart.h>


// constant
#define SSV6200_AMPDU_DELIMITER_LENGTH   (4)
#define SSV6200_RETRY_BIT_SHIFT          (11)

// parameter
#define AMPDU_FW_retry_counter_max       (7)
#define AMPDU_FW_retry_time_us           (200)

// macro
#define us_timer0_int_sts    (0x1 << TU0_TM_INT_STS_DONE_SFT)
#define us_timer0_int_mask   (0x1 << TU0_TM_INT_MASK_SFT)
#define us_timer0_stop(tmr)  (tmr)->TMR_CTRL = (us_timer0_int_sts)

struct ssv6200_rx_desc_from_soc
{
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             l3cs_err:1;
    u32             l4cs_err:1;
    u32             align2:1;
    u32             RSVD_0:2;
    u32             psm:1;
    u32             stype_b5b4:2;
    u32             extra_info:1;  

	/* The definition of WORD_2: */
    u32             fCmd;
};

typedef struct AMPDU_DELIMITER_st
{
    u16			reserved:4; 	//0-3
    u16			length:12; 		//4-15
    u8			crc;
    u8			signature;
} AMPDU_DELIMITER, *p_AMPDU_DELIMITER;

struct ssv6200_80211_hdr {
    __le16 frame_ctl;
    __le16 duration_id;
    u8 addr1[6];
    u8 addr2[6];
    u8 addr3[6];
	__le16 seq_ctrl;
	u8 addr4[6];
};

void ampdu_fw_retry_init(void);
void irq_us_timer0_handler(void *m_data);
bool ampdu_tx_report_handler (u32 rx_data);
bool ampdu_rx_ctrl_handler (u32 rx_data);
void ampdu_dump_retry_list(void);

#endif /* _DRV_AMPDU_H_ */
