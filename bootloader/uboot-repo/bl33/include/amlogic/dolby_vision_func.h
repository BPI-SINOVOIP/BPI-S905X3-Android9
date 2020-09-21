#ifndef _DV_H_
#define _DV_H_

#include <linux/types.h>

#define DEF_G2L_LUT_SIZE_2P        8
#define DEF_G2L_LUT_SIZE           (1 << DEF_G2L_LUT_SIZE_2P)

enum signal_format_e {
	FORMAT_INVALID = -1,
	FORMAT_DOVI = 0,
	FORMAT_HDR10 = 1,
	FORMAT_SDR = 2,
	FORMAT_DOVI_LL = 3
};

enum priority_mode_e {
	VIDEO_PRIORITY = 0,
	GRAPHIC_PRIORITY = 1
};

enum cp_signal_range_e {
	SIG_RANGE_SMPTE = 0,  /* head range */
	SIG_RANGE_FULL  = 1,  /* full range */
	SIG_RANGE_SDI   = 2           /* PQ */
};

enum graphics_format_e
{
	GF_SDR_YUV = 0,  /* BT.709 YUV BT1886 */
	GF_SDR_RGB = 1,  /* BT.709 RGB BT1886 */
	GF_HDR_YUV = 2,  /* BT.2020 YUV PQ */
	GF_HDR_RGB = 3   /* BT.2020 RGB PQ */
};

struct composer_register_ipcore_s {
	uint32_t Composer_Mode;
	uint32_t VDR_Resolution;
	uint32_t Bit_Depth;
	uint32_t Coefficient_Log2_Denominator;
	uint32_t BL_Num_Pivots_Y;
	uint32_t BL_Pivot[5];
	uint32_t BL_Order;
	uint32_t BL_Coefficient_Y[8][3];
	uint32_t EL_NLQ_Offset_Y;
	uint32_t EL_Coefficient_Y[3];
	uint32_t Mapping_IDC_U;
	uint32_t BL_Num_Pivots_U;
	uint32_t BL_Pivot_U[3];
	uint32_t BL_Order_U;
	uint32_t BL_Coefficient_U[4][3];
	uint32_t MMR_Coefficient_U[22][2];
	uint32_t MMR_Order_U;
	uint32_t EL_NLQ_Offset_U;
	uint32_t EL_Coefficient_U[3];
	uint32_t Mapping_IDC_V;
	uint32_t BL_Num_Pivots_V;
	uint32_t BL_Pivot_V[3];
	uint32_t BL_Order_V;
	uint32_t BL_Coefficient_V[4][3];
	uint32_t MMR_Coefficient_V[22][2];
	uint32_t MMR_Order_V;
	uint32_t EL_NLQ_Offset_V;
	uint32_t EL_Coefficient_V[3];
};

/** @brief DM registers for IPCORE 1 */
struct dm_register_ipcore_1_s {
	uint32_t SRange;
	uint32_t Srange_Inverse;
	uint32_t Frame_Format_1;
	uint32_t Frame_Format_2;
	uint32_t Frame_Pixel_Def;
	uint32_t Y2RGB_Coefficient_1;
	uint32_t Y2RGB_Coefficient_2;
	uint32_t Y2RGB_Coefficient_3;
	uint32_t Y2RGB_Coefficient_4;
	uint32_t Y2RGB_Coefficient_5;
	uint32_t Y2RGB_Offset_1;
	uint32_t Y2RGB_Offset_2;
	uint32_t Y2RGB_Offset_3;
	uint32_t EOTF;
	uint32_t A2B_Coefficient_1;
	uint32_t A2B_Coefficient_2;
	uint32_t A2B_Coefficient_3;
	uint32_t A2B_Coefficient_4;
	uint32_t A2B_Coefficient_5;
	uint32_t C2D_Coefficient_1;
	uint32_t C2D_Coefficient_2;
	uint32_t C2D_Coefficient_3;
	uint32_t C2D_Coefficient_4;
	uint32_t C2D_Coefficient_5;
	uint32_t C2D_Offset;
	uint32_t Active_area_left_top;
	uint32_t Active_area_bottom_right;
};

/** @brief DM registers for IPCORE 2 */
struct dm_register_ipcore_2_s {
	uint32_t SRange;
	uint32_t Srange_Inverse;
	uint32_t Y2RGB_Coefficient_1;
	uint32_t Y2RGB_Coefficient_2;
	uint32_t Y2RGB_Coefficient_3;
	uint32_t Y2RGB_Coefficient_4;
	uint32_t Y2RGB_Coefficient_5;
	uint32_t Y2RGB_Offset_1;
	uint32_t Y2RGB_Offset_2;
	uint32_t Y2RGB_Offset_3;
	uint32_t Frame_Format;
	uint32_t EOTF;
	uint32_t A2B_Coefficient_1;
	uint32_t A2B_Coefficient_2;
	uint32_t A2B_Coefficient_3;
	uint32_t A2B_Coefficient_4;
	uint32_t A2B_Coefficient_5;
	uint32_t C2D_Coefficient_1;
	uint32_t C2D_Coefficient_2;
	uint32_t C2D_Coefficient_3;
	uint32_t C2D_Coefficient_4;
	uint32_t C2D_Coefficient_5;
	uint32_t C2D_Offset;
	uint32_t VDR_Resolution;
};

/** @brief DM registers for IPCORE 3 */
struct dm_register_ipcore_3_s {
	uint32_t D2C_coefficient_1;
	uint32_t D2C_coefficient_2;
	uint32_t D2C_coefficient_3;
	uint32_t D2C_coefficient_4;
	uint32_t D2C_coefficient_5;
	uint32_t B2A_Coefficient_1;
	uint32_t B2A_Coefficient_2;
	uint32_t B2A_Coefficient_3;
	uint32_t B2A_Coefficient_4;
	uint32_t B2A_Coefficient_5;
	uint32_t Eotf_param_1;
	uint32_t Eotf_param_2;
	uint32_t IPT_Scale;
	uint32_t IPT_Offset_1;
	uint32_t IPT_Offset_2;
	uint32_t IPT_Offset_3;
	uint32_t Output_range_1;
	uint32_t Output_range_2;
	uint32_t RGB2YUV_coefficient_register1;
	uint32_t RGB2YUV_coefficient_register2;
	uint32_t RGB2YUV_coefficient_register3;
	uint32_t RGB2YUV_coefficient_register4;
	uint32_t RGB2YUV_coefficient_register5;
	uint32_t RGB2YUV_offset_0;
	uint32_t RGB2YUV_offset_1;
	uint32_t RGB2YUV_offset_2;
};

/** @brief DM luts for IPCORE 1 and 2 */
struct dm_lut_ipcore_s {
	uint32_t TmLutI[64*4];
	uint32_t TmLutS[64*4];
	uint32_t SmLutI[64*4];
	uint32_t SmLutS[64*4];
	uint32_t G2L[DEF_G2L_LUT_SIZE];
};

/** @brief hdmi metadata for IPCORE 3 */
struct md_reister_ipcore_3_s {
	uint32_t raw_metadata[512];
	uint32_t size;
};

struct hdr_10_infoframe_s {
	uint8_t infoframe_type_code;
	uint8_t infoframe_version_number;
	uint8_t length_of_info_frame;
	uint8_t data_byte_1;
	uint8_t data_byte_2;
	uint8_t display_primaries_x_0_LSB;
	uint8_t display_primaries_x_0_MSB;
	uint8_t display_primaries_y_0_LSB;
	uint8_t display_primaries_y_0_MSB;
	uint8_t display_primaries_x_1_LSB;
	uint8_t display_primaries_x_1_MSB;
	uint8_t display_primaries_y_1_LSB;
	uint8_t display_primaries_y_1_MSB;
	uint8_t display_primaries_x_2_LSB;
	uint8_t display_primaries_x_2_MSB;
	uint8_t display_primaries_y_2_LSB;
	uint8_t display_primaries_y_2_MSB;
	uint8_t white_point_x_LSB;
	uint8_t white_point_x_MSB;
	uint8_t white_point_y_LSB;
	uint8_t white_point_y_MSB;
	uint8_t max_display_mastering_luminance_LSB;
	uint8_t max_display_mastering_luminance_MSB;
	uint8_t min_display_mastering_luminance_LSB;
	uint8_t min_display_mastering_luminance_MSB;
	uint8_t max_content_light_level_LSB;
	uint8_t max_content_light_level_MSB;
	uint8_t max_frame_average_light_level_LSB;
	uint8_t max_frame_average_light_level_MSB;
};

struct hdr10_param_s {
	uint32_t min_display_mastering_luminance;
	uint32_t max_display_mastering_luminance;
	uint16_t Rx;
	uint16_t Ry;
	uint16_t Gx;
	uint16_t Gy;
	uint16_t Bx;
	uint16_t By;
	uint16_t Wx;
	uint16_t Wy;
	uint16_t max_content_light_level;
	uint16_t max_pic_average_light_level;
};

struct ext_level_1_s {
	uint8_t min_PQ_hi;
	uint8_t min_PQ_lo;
	uint8_t max_PQ_hi;
	uint8_t max_PQ_lo;
	uint8_t avg_PQ_hi;
	uint8_t avg_PQ_lo;
};

struct ext_level_2_s {
	uint8_t target_max_PQ_hi;
	uint8_t target_max_PQ_lo;
	uint8_t trim_slope_hi;
	uint8_t trim_slope_lo;
	uint8_t trim_offset_hi;
	uint8_t trim_offset_lo;
	uint8_t trim_power_hi;
	uint8_t trim_power_lo;
	uint8_t trim_chroma_weight_hi;
	uint8_t trim_chroma_weight_lo;
	uint8_t trim_saturation_gain_hi;
	uint8_t trim_saturation_gain_lo;
	uint8_t ms_weight_hi;
	uint8_t ms_weight_lo;
};

struct ext_level_4_s {
	uint8_t anchor_PQ_hi;
	uint8_t anchor_PQ_lo;
	uint8_t anchor_power_hi;
	uint8_t anchor_power_lo;
};

struct ext_level_5_s {
	uint8_t active_area_left_offset_hi;
	uint8_t active_area_left_offset_lo;
	uint8_t active_area_right_offset_hi;
	uint8_t active_area_right_offset_lo;
	uint8_t active_area_top_offset_hi;
	uint8_t active_area_top_offset_lo;
	uint8_t active_area_bottom_offset_hi;
	uint8_t active_area_bottom_offset_lo;
};

struct ext_level_6_s {
	uint8_t max_display_mastering_luminance_hi;
	uint8_t max_display_mastering_luminance_lo;
	uint8_t min_display_mastering_luminance_hi;
	uint8_t min_display_mastering_luminance_lo;
	uint8_t max_content_light_level_hi;
	uint8_t max_content_light_level_lo;
	uint8_t max_frame_average_light_level_hi;
	uint8_t max_frame_average_light_level_lo;
};

struct ext_level_255_s {
	uint8_t dm_run_mode;
	uint8_t dm_run_version;
	uint8_t dm_debug0;
	uint8_t dm_debug1;
	uint8_t dm_debug2;
	uint8_t dm_debug3;
};

struct ext_md_s {
	uint32_t available_level_mask;
	struct ext_level_1_s level_1;
	struct ext_level_2_s level_2;
	struct ext_level_4_s level_4;
	struct ext_level_5_s level_5;
	struct ext_level_6_s level_6;
	struct ext_level_255_s level_255;
};

struct dovi_setting_s {
	struct composer_register_ipcore_s comp_reg;
	struct dm_register_ipcore_1_s dm_reg1;
	struct dm_register_ipcore_2_s dm_reg2;
	struct dm_register_ipcore_3_s dm_reg3;
	struct dm_lut_ipcore_s dm_lut1;
	struct dm_lut_ipcore_s dm_lut2;
	/* for dovi output */
	struct md_reister_ipcore_3_s md_reg3;
	/* for hdr10 output */
	struct hdr_10_infoframe_s hdr_info;
	/* current process */
	enum signal_format_e src_format;
	enum signal_format_e dst_format;
	/* enhanced layer */
	bool el_flag;
	bool el_halfsize_flag;
	/* frame width & height */
	uint32_t video_width;
	uint32_t video_height;

	/* use for stb 2.4 */
	enum graphics_format_e g_format;
	uint32_t g_bitdepth;
	uint32_t dovi2hdr10_nomapping;
	uint32_t use_ll_flag;
	uint32_t ll_rgb_desired;
	uint32_t diagnostic_enable;
	uint32_t diagnostic_mux_select;
	uint32_t dovi_ll_enable;
	uint32_t vout_width;
	uint32_t vout_height;
	u8 vsvdb_tbl[32];
	u8 hdrdb_tbl[7];
	struct ext_md_s ext_md;
	uint32_t vsvdb_len;
	uint32_t vsvdb_changed;
	uint32_t mode_changed;
};


extern int control_path(
	enum signal_format_e in_format,
	enum signal_format_e out_format,
	char *in_comp, int in_comp_size,
	char *in_md, int in_md_size,
	enum priority_mode_e set_priority,
	int set_bit_depth, int set_chroma_format, int set_yuv_range,
	int set_graphic_min_lum, int set_graphic_max_lum,
	int set_target_min_lum, int set_target_max_lum,
	int set_no_el,
	struct hdr10_param_s *hdr10_param,
	struct dovi_setting_s *output);
extern void *metadata_parser_init(int flag);
extern int metadata_parser_reset(int flag);
extern int metadata_parser_process(
	char  *src_rpu, int rpu_len,
	char  *dst_comp, int *comp_len,
	char  *dst_md, int *md_len, bool src_eos);
extern void metadata_parser_release(void);

struct dolby_vision_func_s {
	const char *version_info;
	void * (*metadata_parser_init)(int flag);
	int (*metadata_parser_reset)(int flag);
	int (*metadata_parser_process)(
		char  *src_rpu, int rpu_len,
		char  *dst_comp, int *comp_len,
		char  *dst_md, int *md_len, bool src_eos);
	void (*metadata_parser_release)(void);
	int (*control_path)(
		enum signal_format_e in_format,
		enum signal_format_e out_format,
		char *in_comp, int in_comp_size,
		char *in_md, int in_md_size,
		enum priority_mode_e set_priority,
		int set_bit_depth, int set_chroma_format, int set_yuv_range,
		int set_graphic_min_lum, int set_graphic_max_lum,
		int set_target_min_lum, int set_target_max_lum,
		int set_no_el,
		struct hdr10_param_s *hdr10_param,
		struct dovi_setting_s *output);
	int (*reserved_func)(void);
};

extern const struct dolby_vision_func_s DV_func;
#endif
