/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef MEDIA_CONFIG_API___
#define MEDIA_CONFIG_API___
int media_config_list(char *val, int len);
int media_set_cmd_str(const char *cmd, const char *val);
int media_get_cmd_str(const char *cmd, char *val, int len);
int media_decoder_set_cmd_str(const char *cmd, const char *val);
int media_decoder_get_cmd_str(const char *cmd, char *val, int len);
int media_parser_set_cmd_str(const char *cmd, const char *val);
int media_parser_get_cmd_str(const char *cmd, char *val, int len);
int media_video_set_cmd_str(const char *cmd, const char *val);
int media_video_get_cmd_str(const char *cmd, char *val, int len);
int media_sync_set_cmd_str(const char *cmd, const char *val);
int media_sync_get_cmd_str(const char *cmd, char *val, int len);
int media_codecmm_set_cmd_str(const char *cmd, const char *val);
int media_codecmm_get_cmd_str(const char *cmd, char *val, int len);
int media_audio_set_cmd_str(const char *cmd, const char *val);
int media_audio_get_cmd_str(const char *cmd, char *val, int len);
#endif

