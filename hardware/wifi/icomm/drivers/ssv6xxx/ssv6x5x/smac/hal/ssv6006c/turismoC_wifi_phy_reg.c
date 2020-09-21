/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define SSV6006_TURISMOC_PHY_TABLE_VER "20.00"
ssv_cabrio_reg ssv6006_turismoC_phy_setting[]={
    {0xccb0e010,0x00000FFF},
    {0xccb0e014,0x00807f03},
    {0xccb0e018,0x0055003C},
    {0xccb0e01C,0x00000064},
    {0xccb0e020,0x00000000},
    {0xccb0e02C,0x7004606C},
    {0xccb0e030,0x7004606C},
    {0xccb0e034,0x1A040400},
    {0xccb0e038,0x630F36D0},
    {0xccb0e03C,0x100c0003},
    {0xccb0e040,0x11600800},
    {0xccb0e044,0x00080868},
    {0xccb0e048,0xFF001160},
    {0xccb0e04C,0x00100040},
    {0xccb0e060,0x11501150},
    {0xccb0e12C,0x00001160},
    {0xccb0e130,0x00100040},
    {0xccb0e134,0x00080010},
    {0xccb0e180,0x00010060},
    {0xccb0e184,0xB5A19080},
    {0xccb0e188,0xB5A19080},
    {0xccb0e18c,0xB5A19080},
    {0xccb0e190,0x00010006},
    {0xccb0e194,0x06060606},
    {0xccb0e198,0x06060606},
    {0xccb0e19c,0x06060606},
    {0xccb0e080,0x0110000F},
    {0xccb0e098,0x00102000},
    {0xccb0e09C,0x00100018},
    {0xccb0e4b4,0x00002001},
    {0xccb0ecA4,0x00009001},
    {0xccb0ecB8,0x000C50CC},
    {0xccb0fc44,0x00028080},
    {0xccb0f008,0x00004775},
    {0xccb0f00c,0x10000075},
    {0xccb0f010,0x3F304905},
    {0xccb0f014,0x40182000},
    {0xccb0f018,0x20600000},
    {0xccb0f01C,0x0c010080},
    {0xccb0f03C,0x0000005a},
    {0xccb0f020,0x20202020},
    {0xccb0f024,0x20000000},
    {0xccb0f028,0x50505050},
    {0xccb0f02c,0x20202020},
    {0xccb0f030,0x20000000},
    {0xccb0f034,0x00002424},
    {0xccb0f09c,0x000030A0},
    {0xccb0f0C0,0x0f0003c0},
    {0xccb0f0C4,0x30023003},
    {0xccb0f0CC,0x00000120},
    {0xccb0f0D0,0x00000020},
    {0xccb0f130,0x40000000},
    {0xccb0f164,0x000e0090},
    {0xccb0f188,0x82000000},
    {0xccb0f190,0x00000020},
    {0xccb0f194,0x09360001},
    {0xccb0f3F8,0x00100001},
    {0xccb0f3FC,0x00010425},
    {0xccb0e804,0x00020000},
    {0xccb0e808,0x20280060},
    {0xccb0e80c,0x00003467},
    {0xccb0e810,0x00430000},
    {0xccb0e814,0x30000015},
    {0xccb0e818,0x00390005},
    {0xccb0e81C,0x05050005},
    {0xccb0e820,0x00570057},
    {0xccb0e824,0x00570057},
    {0xccb0e828,0x00236700},
    {0xccb0e82c,0x000d1746},
    {0xccb0e830,0x05051787},
    {0xccb0e834,0x07800000},
    {0xccb0e89c,0x009000B0},
    {0xccb0e8A0,0x00000000},
    {0xccb0ebF8,0x00100000},
    {0xccb0ebFC,0x00000001},
};
#define PAPDP_GAIN_SETTING 0x0
#define PAPDP_GAIN_SETTING_F2 0x0
#define PAPDP_GAIN_SETTING_2G 0x6
#define DEFAULT_DPD_BBSCALE_2500 0x60
#define DEFAULT_DPD_BBSCALE_5100 0x80
#define DEFAULT_DPD_BBSCALE_5500 0x6C
#define DEFAULT_DPD_BBSCALE_5700 0x6C
#define DEFAULT_DPD_BBSCALE_5900 0x66
