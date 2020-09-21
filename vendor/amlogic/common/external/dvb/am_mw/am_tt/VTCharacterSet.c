/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#include "includes.h"
#include "string.h"

#include "VTCharacterSet.h"
#include "VTCommon.h"

/// G0 Character Sets
typedef enum
{
    VTG0CHARACTERSET_LATIN,
    VTG0CHARACTERSET_CYRILLIC_1,
    VTG0CHARACTERSET_CYRILLIC_2,
    VTG0CHARACTERSET_CYRILLIC_3,
    VTG0CHARACTERSET_GREEK,
    VTG0CHARACTERSET_ARABIC,
    VTG0CHARACTERSET_HEBREW,
    VTG0CHARACTERSET_LASTONE
} eG0CharacterSet;

// G0 Latin National Option Sub-sets
typedef enum
{
    VTG0LATINSUBSET_NA          = -1,
    VTG0LATINSUBSET_CZECH       = 0,
    VTG0LATINSUBSET_ENGLISH,
    VTG0LATINSUBSET_ESTONIAN,
    VTG0LATINSUBSET_FRENCH,
    VTG0LATINSUBSET_GERMAN,
    VTG0LATINSUBSET_ITALIAN,
    VTG0LATINSUBSET_LETTISH,
    VTG0LATINSUBSET_POLISH,
    VTG0LATINSUBSET_PORTUGUESE,
    VTG0LATINSUBSET_RUMANIAN,
    VTG0LATINSUBSET_SLOVENIAN,
    VTG0LATINSUBSET_SWEDISN,
    VTG0LATINSUBSET_TURKISH,
    VTG0LATINSUBSET_DANISH,
    VTG0LATINSUBSET_LASTONE
} eG0LatinSubset;


// Character set designation
typedef struct
{
    eG0CharacterSet     G0CharacterSet;
    eG0LatinSubset      G0LatinSubset;
} TCharacterSetDesignation;


// The list of G0 characterset and subset designation for all codepages
static TCharacterSetDesignation m_CharacterSetDesignation[VTCODEPAGE_LASTONE] =
{
    /* VTCODEPAGE_ENGLISH    */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_ENGLISH     },
    /* VTCODEPAGE_FRENCH     */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_FRENCH      },
    /* VTCODEPAGE_SWEDISH    */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_SWEDISN     },
    /* VTCODEPAGE_CZECH      */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_CZECH       },
    /* VTCODEPAGE_GERMAN     */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_GERMAN      },
    /* VTCODEPAGE_PORTUGUESE */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_PORTUGUESE  },
    /* VTCODEPAGE_ITALIAN    */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_ITALIAN     },
    /* VTCODEPAGE_POLISH     */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_POLISH      },
    /* VTCODEPAGE_TURKISH    */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_TURKISH     },
    /* VTCODEPAGE_SLOVENIAN  */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_SLOVENIAN   },
    /* VTCODEPAGE_RUMANIAN   */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_RUMANIAN    },
    /* VTCODEPAGE_SERBIAN    */ { VTG0CHARACTERSET_CYRILLIC_1,  VTG0LATINSUBSET_NA          },
    /* VTCODEPAGE_RUSSIAN    */ { VTG0CHARACTERSET_CYRILLIC_2,  VTG0LATINSUBSET_NA          },
    /* VTCODEPAGE_ESTONIAN   */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_ESTONIAN    },
    /* VTCODEPAGE_UKRAINIAN  */ { VTG0CHARACTERSET_CYRILLIC_3,  VTG0LATINSUBSET_NA          },
    /* VTCODEPAGE_LETTISH    */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_LETTISH     },
    /* VTCODEPAGE_GREEK      */ { VTG0CHARACTERSET_GREEK,       VTG0LATINSUBSET_NA          },
    /* VTCODEPAGE_ENGLISHA   */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_ENGLISH     },
    /* VTCODEPAGE_FRENCHA    */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_FRENCH      },
    /* VTCODEPAGE_ARABIC     */ { VTG0CHARACTERSET_ARABIC,      VTG0LATINSUBSET_NA          },
    /* VTCODEPAGE_HEBREW     */ { VTG0CHARACTERSET_HEBREW,      VTG0LATINSUBSET_NA          },
    /* VTCODEPAGE_DANISH     */ { VTG0CHARACTERSET_LATIN,       VTG0LATINSUBSET_DANISH      },
};


static INT16U m_G0CharacterSet[VTG0CHARACTERSET_LASTONE][96] =
{
    // VTCHARACTERSET_LATIN
    0x0020/* */, 0x0021/*!*/, 0x0022/*"*/, 0x0023/*#*/, 0x00A4/* */, 0x0025/*%*/, 0x0026/*&*/, 0x0027/*'*/,
    0x0028/*(*/, 0x0029/*)*/, 0x002A/***/, 0x002B/*+*/, 0x002C/*,*/, 0x002D/*-*/, 0x002E/*.*/, 0x002F/*/*/,
    0x0030/*0*/, 0x0031/*1*/, 0x0032/*2*/, 0x0033/*3*/, 0x0034/*4*/, 0x0035/*5*/, 0x0036/*6*/, 0x0037/*7*/,
    0x0038/*8*/, 0x0039/*9*/, 0x003A/*:*/, 0x003B/*;*/, 0x003C/*<*/, 0x003D/*=*/, 0x003E/*>*/, 0x003F/*?*/,
    0x0040/*@*/, 0x0041/*A*/, 0x0042/*B*/, 0x0043/*C*/, 0x0044/*D*/, 0x0045/*E*/, 0x0046/*F*/, 0x0047/*G*/,
    0x0048/*H*/, 0x0049/*I*/, 0x004A/*J*/, 0x004B/*K*/, 0x004C/*L*/, 0x004D/*M*/, 0x004E/*N*/, 0x004F/*O*/,
    0x0050/*P*/, 0x0051/*Q*/, 0x0052/*R*/, 0x0053/*S*/, 0x0054/*T*/, 0x0055/*U*/, 0x0056/*V*/, 0x0057/*W*/,
    0x0058/*X*/, 0x0059/*Y*/, 0x005A/*Z*/, 0x005B/*[*/, 0x005C/* */, 0x005D/*]*/, 0x005E/*^*/, 0x005F/*_*/,
    0x0060/*`*/, 0x0061/*a*/, 0x0062/*b*/, 0x0063/*c*/, 0x0064/*d*/, 0x0065/*e*/, 0x0066/*f*/, 0x0067/*g*/,
    0x0068/*h*/, 0x0069/*i*/, 0x006A/*j*/, 0x006B/*k*/, 0x006C/*l*/, 0x006D/*m*/, 0x006E/*n*/, 0x006F/*o*/,
    0x0070/*p*/, 0x0071/*q*/, 0x0072/*r*/, 0x0073/*s*/, 0x0074/*t*/, 0x0075/*u*/, 0x0076/*v*/, 0x0077/*w*/,
    0x0078/*x*/, 0x0079/*y*/, 0x007A/*z*/, 0x007B/*{*/, 0x00A6/*|*/, 0x007D/*}*/, 0x007E/*~*/, 0x25A0/* */,

    // VTCHARACTERSET_CYRILLIC_1
    0x0020/* */, 0x0021/*!*/, 0x0022/*"*/, 0x0023/*#*/, 0x0024/*$*/, 0x0025/*%*/, 0x0026/*&*/, 0x0027/*'*/,
    0x0028/*(*/, 0x0029/*)*/, 0x002A/***/, 0x002B/*+*/, 0x002C/*,*/, 0x002D/*-*/, 0x002E/*.*/, 0x002F/*/*/,
    0x0030/*0*/, 0x0031/*1*/, 0x0032/*2*/, 0x0033/*3*/, 0x0034/*4*/, 0x0035/*5*/, 0x0036/*6*/, 0x0037/*7*/,
    0x0038/*8*/, 0x0039/*9*/, 0x003A/*:*/, 0x003B/*;*/, 0x003C/*<*/, 0x003D/*=*/, 0x003E/*>*/, 0x003F/*?*/,
    0x0427/*4*/, 0x0410/*A*/, 0x0411/*6*/, 0x0426/*U*/, 0x0414/*A*/, 0x0415/*E*/, 0x0424/* */, 0x0413/* */,
    0x0425/*X*/, 0x0418/*N*/, 0x0408/*J*/, 0x041A/*K*/, 0x041B/* */, 0x041C/*M*/, 0x041D/*H*/, 0x041E/*O*/,
    0x041F/*n*/, 0x040C/*K*/, 0x0420/*P*/, 0x0421/*C*/, 0x0422/*T*/, 0x0423/*Y*/, 0x0412/*B*/, 0x0490/*r*/,
    0x0409/* */, 0x040A/*H*/, 0x0417/*3*/, 0x040B/*h*/, 0x0416/*X*/, 0x0402/*h*/, 0x0428/*W*/, 0x040F/*Y*/,
    0x0447/*y*/, 0x0430/*a*/, 0x0431/*6*/, 0x0446/*u*/, 0x0434/*A*/, 0x0435/*e*/, 0x0444/* */, 0x0433/* */,
    0x0445/*x*/, 0x0438/*n*/, 0x0458/*j*/, 0x043A/*K*/, 0x043B/* */, 0x043C/*m*/, 0x043D/*H*/, 0x043E/*o*/,
    0x043F/*n*/, 0x045C/*k*/, 0x0440/*p*/, 0x0441/*c*/, 0x0442/*t*/, 0x0443/*y*/, 0x0432/*B*/, 0x0491/*r*/,
    0x0459/* */, 0x045A/*H*/, 0x0437/*3*/, 0x045B/*h*/, 0x0436/*x*/, 0x0452/*h*/, 0x0448/*w*/, 0x25A0/* */,

    // VTCHARACTERSET_CYRILLIC_2
    0x0020/* */, 0x0021/*!*/, 0x0022/*"*/, 0x0023/*#*/, 0x0024/*$*/, 0x0025/*%*/, 0x044B/* */, 0x0027/*'*/,
    0x0028/*(*/, 0x0029/*)*/, 0x002A/***/, 0x002B/*+*/, 0x002C/*,*/, 0x002D/*-*/, 0x002E/*.*/, 0x002F/*/*/,
    0x0030/*0*/, 0x0031/*1*/, 0x0032/*2*/, 0x0033/*3*/, 0x0034/*4*/, 0x0035/*5*/, 0x0036/*6*/, 0x0037/*7*/,
    0x0038/*8*/, 0x0039/*9*/, 0x003A/*:*/, 0x003B/*;*/, 0x003C/*<*/, 0x003D/*=*/, 0x003E/*>*/, 0x003F/*?*/,
    0x042E/* */, 0x0410/*A*/, 0x0411/*6*/, 0x0426/*U*/, 0x0414/*A*/, 0x0415/*E*/, 0x0424/* */, 0x0413/* */,
    0x0425/*X*/, 0x0418/*N*/, 0x0419/* */, 0x041A/*K*/, 0x041B/* */, 0x041C/*M*/, 0x041D/*H*/, 0x041E/*O*/,
    0x041F/*n*/, 0x042F/*R*/, 0x0420/*P*/, 0x0421/*C*/, 0x0422/*T*/, 0x0423/*Y*/, 0x0416/*X*/, 0x0412/*B*/,
    0x042C/*b*/, 0x042A/* */, 0x0417/*3*/, 0x0428/*W*/, 0x042D/*3*/, 0x0429/*W*/, 0x0427/*y*/, 0x042B/* */,
    0x044E/* */, 0x0430/*a*/, 0x0431/*6*/, 0x0446/*u*/, 0x0434/*A*/, 0x0435/*e*/, 0x0444/* */, 0x0433/* */,
    0x0445/*x*/, 0x0438/*n*/, 0x0439/* */, 0x043A/*K*/, 0x043B/* */, 0x043C/*m*/, 0x043D/*H*/, 0x043E/*o*/,
    0x043F/*n*/, 0x044F/*R*/, 0x0440/*p*/, 0x0441/*c*/, 0x0442/*t*/, 0x0443/*y*/, 0x0436/*x*/, 0x0432/*B*/,
    0x044C/*b*/, 0x044A/* */, 0x0437/*3*/, 0x0448/*w*/, 0x044D/*3*/, 0x0449/*w*/, 0x0447/*y*/, 0x25A0/* */,

    // VTCHARACTERSET_CYRILLIC_3
    0x0020/* */, 0x0021/*!*/, 0x0022/*"*/, 0x0023/*#*/, 0x0024/*$*/, 0x0025/*%*/, 0x0026/*&*/, 0x0027/*'*/,
    0x0028/*(*/, 0x0029/*)*/, 0x002A/***/, 0x002B/*+*/, 0x002C/*,*/, 0x002D/*-*/, 0x002E/*.*/, 0x002F/*/*/,
    0x0030/*0*/, 0x0031/*1*/, 0x0032/*2*/, 0x0033/*3*/, 0x0034/*4*/, 0x0035/*5*/, 0x0036/*6*/, 0x0037/*7*/,
    0x0038/*8*/, 0x0039/*9*/, 0x003A/*:*/, 0x003B/*;*/, 0x003C/*<*/, 0x003D/*=*/, 0x003E/*>*/, 0x003F/*?*/,
    0x042E/* */, 0x0410/*A*/, 0x0411/*6*/, 0x0426/*U*/, 0x0414/*A*/, 0x0415/*E*/, 0x0424/* */, 0x0413/* */,
    0x0425/*X*/, 0x0418/*N*/, 0x0419/* */, 0x041A/*K*/, 0x041B/* */, 0x041C/*M*/, 0x041D/*H*/, 0x041E/*O*/,
    0x041F/*n*/, 0x042F/*R*/, 0x0420/*P*/, 0x0421/*C*/, 0x0422/*T*/, 0x0423/*Y*/, 0x0416/*X*/, 0x0412/*B*/,
    0x042C/*b*/, 0x0406/*I*/, 0x0417/*3*/, 0x0428/*W*/, 0x0404/* */, 0x0429/*W*/, 0x0427/*y*/, 0x0407/*I*/,
    0x044E/* */, 0x0430/*a*/, 0x0431/*6*/, 0x0446/*u*/, 0x0434/*A*/, 0x0435/*e*/, 0x0444/* */, 0x0491/* */,
    0x0445/*x*/, 0x0438/*n*/, 0x0439/* */, 0x043A/*K*/, 0x043B/* */, 0x043C/*m*/, 0x043D/*H*/, 0x043E/*o*/,
    0x043F/*n*/, 0x044F/*R*/, 0x0440/*p*/, 0x0441/*c*/, 0x0442/*t*/, 0x0443/*y*/, 0x0436/*x*/, 0x0432/*B*/,
    0x044C/*b*/, 0x0456/*i*/, 0x0437/*3*/, 0x0448/*w*/, 0x0454/* */, 0x0449/*w*/, 0x0447/*y*/, 0x25A0/* */,

    // VTCHARACTERSET_GREEK
    0x0020/* */, 0x0021/*!*/, 0x0022/*"*/, 0x0023/*#*/, 0x0024/*$*/, 0x0025/*%*/, 0x0026/*&*/, 0x0027/*'*/,
    0x0028/*(*/, 0x0029/*)*/, 0x002A/***/, 0x002B/*+*/, 0x002C/*,*/, 0x002D/*-*/, 0x002E/*.*/, 0x002F/*/*/,
    0x0030/*0*/, 0x0031/*1*/, 0x0032/*2*/, 0x0033/*3*/, 0x0034/*4*/, 0x0035/*5*/, 0x0036/*6*/, 0x0037/*7*/,
    0x0038/*8*/, 0x0039/*9*/, 0x003A/*:*/, 0x003B/*;*/, 0x00AB/*<*/, 0x003D/*=*/, 0x00BB/*>*/, 0x003F/*?*/,
    0x0390/*i*/, 0x0391/*A*/, 0x0392/*B*/, 0x0393/* */, 0x0394/* */, 0x0395/*E*/, 0x0396/*Z*/, 0x0397/*H*/,
    0x0398/*8*/, 0x0399/*I*/, 0x039A/*K*/, 0x039B/* */, 0x039C/*M*/, 0x039D/*N*/, 0x039E/* */, 0x039F/*O*/,
    0x03A0/* */, 0x03A1/*P*/, 0x0384/*`*/, 0x03A3/* */, 0x03A4/*T*/, 0x03A5/*Y*/, 0x03A6/* */, 0x03A7/*X*/,
    0x03A8/* */, 0x03A9/* */, 0x03AA/*I*/, 0x03AB/*Y*/, 0x03AC/*a*/, 0x03AD/*e*/, 0x03AE/* */, 0x03AF/*i*/,
    0x03B0/*u*/, 0x03B1/*a*/, 0x03B2/*b*/, 0x03B3/*y*/, 0x03B4/* */, 0x03B5/*e*/, 0x03B6/* */, 0x03B7/*n*/,
    0x03B8/*8*/, 0x03B9/*i*/, 0x03BA/*k*/, 0x03BB/* */, 0x03BC/*u*/, 0x03BD/*v*/, 0x03BE/* */, 0x03BF/*o*/,
    0x03C0/*n*/, 0x03C1/*p*/, 0x03C2/*s*/, 0x03C3/*o*/, 0x03C4/*t*/, 0x03C5/*u*/, 0x03C6/* */, 0x03C7/*x*/,
    0x03C8/*y*/, 0x03C9/*w*/, 0x03CA/*i*/, 0x03CB/*u*/, 0x03CC/*o*/, 0x03CD/*u*/, 0x03CE/*w*/, 0x25A0/* */,

    // VTCHARACTERSET_ARABIC (Noon Ghunna not in Arial, used Arial Unicode MS code 0xFB9F)
    0x0020/* */, 0x0021/*!*/, 0x0022/*"*/, 0x00A3/* */, 0x0024/*$*/, 0x0025/*%*/, 0xFB9F/* */, 0xFEF2/* */,
    0x0028/*(*/, 0x0029/*)*/, 0x002A/***/, 0x002B/*+*/, 0x060C/*,*/, 0x002D/*-*/, 0x002E/*.*/, 0x002F/*/*/,
    0x0030/*0*/, 0x0031/*1*/, 0x0032/*2*/, 0x0033/*3*/, 0x0034/*4*/, 0x0035/*5*/, 0x0036/*6*/, 0x0037/*7*/,
    0x0038/*8*/, 0x0039/*9*/, 0x003A/*:*/, 0x061B/* */, 0x00AB/*<*/, 0x003D/*=*/, 0x00BB/*>*/, 0x061F/*?*/,
    0xFE94/* */, 0xFE80/* */, 0xFE92/* */, 0xFE90/* */, 0xFE98/* */, 0xFE96/* */, 0xFE8E/* */, 0xFE8D/* */,
    0xFE91/* */, 0xFE93/* */, 0xFE97/* */, 0xFE9B/* */, 0xFE9F/* */, 0xFEA3/* */, 0xFEA7/* */, 0xFEAA/* */,
    0xFEAC/* */, 0xFEAE/* */, 0xFEB0/* */, 0xFEB4/* */, 0xFEB8/* */, 0xFEBC/* */, 0xFEC0/* */, 0xFEC4/* */,
    0xFEC8/* */, 0xFECB/* */, 0xFECF/* */, 0xFE9C/* */, 0xFEA0/* */, 0xFEA4/* */, 0xFEA8/* */, 0x0023/*#*/,
    0x0640/* */, 0xFED3/* */, 0xFED7/* */, 0xFEDC/* */, 0xFEDF/* */, 0xFEE3/* */, 0xFEE7/* */, 0xFEEB/* */,
    0xFEEE/* */, 0xFEF0/* */, 0xFBFE/* */, 0xFE9A/* */, 0xFE9E/* */, 0xFEA2/* */, 0xFEA6/* */, 0xFBFF/* */,
    0xFBFD/* */, 0xFECC/* */, 0xFED0/* */, 0xFED4/* */, 0xFED2/* */, 0xFED8/* */, 0xFED6/* */, 0xFEDA/* */,
    0xFEE0/* */, 0xFEDE/* */, 0xFEE4/* */, 0xFEE2/* */, 0xFEE8/* */, 0xFEE6/* */, 0xFEFB/* */, 0x25A0/* */,

    // VTCHARACTERSET_HEBREW
    0x0020/* */, 0x0021/*!*/, 0x0022/*"*/, 0x00A3/* */, 0x0024/*$*/, 0x0025/*%*/, 0x0026/*&*/, 0x0027/*'*/,
    0x0028/*(*/, 0x0029/*)*/, 0x002A/***/, 0x002B/*+*/, 0x002C/*,*/, 0x002D/*-*/, 0x002E/*.*/, 0x002F/*/*/,
    0x0030/*0*/, 0x0031/*1*/, 0x0032/*2*/, 0x0033/*3*/, 0x0034/*4*/, 0x0035/*5*/, 0x0036/*6*/, 0x0037/*7*/,
    0x0038/*8*/, 0x0039/*9*/, 0x003A/*:*/, 0x003B/*;*/, 0x003C/*<*/, 0x003D/*=*/, 0x003E/*>*/, 0x003F/*?*/,
    0x0040/*@*/, 0x0041/*A*/, 0x0042/*B*/, 0x0043/*C*/, 0x0044/*D*/, 0x0045/*E*/, 0x0046/*F*/, 0x0047/*G*/,
    0x0048/*H*/, 0x0049/*I*/, 0x004A/*J*/, 0x004B/*K*/, 0x004C/*L*/, 0x004D/*M*/, 0x004E/*N*/, 0x004F/*O*/,
    0x0050/*P*/, 0x0051/*Q*/, 0x0052/*R*/, 0x0053/*S*/, 0x0054/*T*/, 0x0055/*U*/, 0x0056/*V*/, 0x0057/*W*/,
    0x0058/*X*/, 0x0059/*Y*/, 0x005A/*Z*/, 0x2190/*<*/, 0x00BD/* */, 0x2192/*>*/, 0x2191/*^*/, 0x0023/*#*/,
    0x05D0/* */, 0x05D1/* */, 0x05D2/* */, 0x05D3/* */, 0x05D4/* */, 0x05D5/* */, 0x05D6/* */, 0x05D7/* */,
    0x05D8/* */, 0x05D9/* */, 0x05DA/* */, 0x05DB/* */, 0x05DC/* */, 0x05DD/* */, 0x05DE/* */, 0x05DF/* */,
    0x05E0/* */, 0x05E1/* */, 0x05E2/* */, 0x05E3/* */, 0x05E4/* */, 0x05E5/* */, 0x05E6/* */, 0x05E7/* */,
    0x05E8/* */, 0x05E9/* */, 0x05EA/* */, 0x20AA/*S*/, 0x05F0/*|*/, 0x00BE/* */, 0x00F7/* */, 0x25A0/* */,
};


static INT16U m_G0LatinSubset[VTG0LATINSUBSET_LASTONE][13] =
{
    // VTG0LATINSUBSET_CZECH
    0x0023/*#*/, 0x016F/*u*/, 0x010D/*c*/, 0x0165/*t*/, 0x017E/*z*/, 0x00FD/*y*/, 0x00ED/*i*/, 0x0159/*r*/,
    0x00E9/*e*/, 0x00E1/*a*/, 0x0115/*e*/, 0x00FA/*u*/, 0x0161/*s*/,

    // VTG0LATINSUBSET_ENGLISH
    0x00A3/* */, 0x0024/*$*/, 0x0040/*@*/, 0x2190/*<*/, 0x00BD/* */, 0x2192/*>*/, 0x2191/*^*/, 0x0023/*#*/,
    0x2014/*-*/, 0x00BC/* */, 0x05F0/*|*/, 0x00BE/* */, 0x00F7/* */,

    // VTG0LATINSUBSET_ESTONIAN
    0x0023/*#*/, 0x00F5/*o*/, 0x0160/*S*/, 0x00C4/*A*/, 0x00D6/*O*/, 0x017D/*Z*/, 0x00DC/*U*/, 0x00D5/*O*/,
    0x0161/*s*/, 0x00E4/*a*/, 0x00F6/*o*/, 0x017E/*z*/, 0x00FC/*u*/,

    // VTG0LATINSUBSET_FRENCH
    0x00E9/*e*/, 0x00EF/*i*/, 0x00E0/*a*/, 0x00EB/*e*/, 0x00EA/*e*/, 0x00F9/*u*/, 0x00EE/*i*/, 0x0023/*#*/,
    0x00E8/*e*/, 0x00E2/*a*/, 0x00F4/*o*/, 0x00FB/*u*/, 0x00E7/*c*/,

    // VTG0LATINSUBSET_GERMAN
    0x0023/*#*/, 0x0024/*$*/, 0x00A7/*S*/, 0x00C4/*A*/, 0x00D6/*O*/, 0x00DC/*U*/, 0x005E/*^*/, 0x005F/*_*/,
    0x00B0/* */, 0x00E4/*a*/, 0x00F6/*o*/, 0x00FC/*u*/, 0x00DF/*B*/,

    // VTG0LATINSUBSET_ITALIAN
    0x00A3/* */, 0x0024/*$*/, 0x00E9/*e*/, 0x00B0/* */, 0x00E7/*c*/, 0x2192/*>*/, 0x2191/*^*/, 0x0023/*#*/,
    0x00F9/*u*/, 0x00E0/*a*/, 0x00F2/*o*/, 0x00E8/*e*/, 0x00EC/*i*/,

    // VTG0LATINSUBSET_LETTISH (there is no small E or I with cedilla, used dot below instead)
    0x0023/*#*/, 0x0024/*$*/, 0x0160/*S*/, 0x0117/*e*/, 0x1EB9/*e*/, 0x017D/*Z*/, 0x010D/*c*/, 0x016B/*u*/,
    0x0161/*s*/, 0x0105/*a*/, 0x0173/*u*/, 0x017E/*z*/, 0x1ECB/*i*/,

    // VTG0LATINSUBSET_POLISH (there is no Z with stroke in Arial, used Arial Unicode MS code)
    0x0023/*#*/, 0x0144/*n*/, 0x0105/*a*/, 0x01B5/*Z*/, 0x015A/*S*/, 0x0141/*L*/, 0x0107/*c*/, 0x00F3/*o*/,
    0x0119/*e*/, 0x017C/*z*/, 0x015B/*s*/, 0x0142/*l*/, 0x017A/*z*/,

    // VTG0LATINSUBSET_PORTUGUESE
    0x00E7/*c*/, 0x0024/*$*/, 0x0069/*i*/, 0x00E1/*a*/, 0x00E9/*e*/, 0x00ED/*i*/, 0x00F3/*o*/, 0x00FA/*u*/,
    0x00BF/*?*/, 0x00FC/*u*/, 0x00F1/*n*/, 0x00E8/*e*/, 0x00E0/*a*/,

    // VTG0LATINSUBSET_RUMANIAN
    0x0023/*#*/, 0x00A4/* */, 0x0162/*T*/, 0x00C2/*A*/, 0x015E/*S*/, 0x0103/*A*/, 0x00CE/*I*/, 0x0131/*i*/,
    0x0163/*t*/, 0x00E2/*a*/, 0x015F/*s*/, 0x0103/*a*/, 0x00EE/*i*/,

    // VTG0LATINSUBSET_SLOVENIAN
    0x0023/*#*/, 0x00CB/*E*/, 0x010C/*C*/, 0x0106/*C*/, 0x017D/*Z*/, 0x0110/*D*/, 0x0160/*S*/, 0x00EB/*e*/,
    0x010D/*c*/, 0x0107/*c*/, 0x017E/*z*/, 0x0111/*d*/, 0x0161/*s*/,

    // VTG0LATINSUBSET_SWEDISN
    0x0023/*#*/, 0x00A4/* */, 0x00C9/*E*/, 0x00C4/*A*/, 0x00D6/*O*/, 0x00C5/*A*/, 0x00DC/*U*/, 0x005F/*_*/,
    0x00E9/*e*/, 0x00E4/*a*/, 0x00F6/*o*/, 0x00E5/*a*/, 0x00FC/*u*/,

    // VTG0LATINSUBSET_TURKISH
    0x20A4/* */, 0x011F/*g*/, 0x0130/*I*/, 0x015E/*S*/, 0x00D6/*O*/, 0x00C7/*C*/, 0x00DC/*U*/, 0x011E/*G*/,
    0x0131/*i*/, 0x015F/*s*/, 0x00F6/*o*/, 0x00E7/*c*/, 0x00FC/*u*/,

    // VTG0LATINSUBSET_DANISH
    0x0023/*#*/, 0x0024/*$*/, 0x00A7/* */, 0x00C6/*A*/, 0x00D8/*O*/, 0x00C5/*A*/, 0x005E/*^*/, 0x005F/*_*/,
    0x00B0/* */, 0x00E6/*a*/, 0x00F8/*o*/, 0x00E5/*a*/, 0x00DF/*B*/,
};


void GetCharacterSet(INT8S VTCodepage, INT16U pBuffer[96])
{
    eG0CharacterSet G0CharacterSet = m_CharacterSetDesignation[VTCodepage].G0CharacterSet;//得到语言的种类

    memcpy(pBuffer, m_G0CharacterSet[G0CharacterSet], sizeof(INT16U) * 96);

    if (G0CharacterSet == VTG0CHARACTERSET_LATIN)
    {
        eG0LatinSubset G0LatinSubset = m_CharacterSetDesignation[VTCodepage].G0LatinSubset;

        pBuffer[0x03] = m_G0LatinSubset[G0LatinSubset][0];
        pBuffer[0x04] = m_G0LatinSubset[G0LatinSubset][1];
        pBuffer[0x20] = m_G0LatinSubset[G0LatinSubset][2];
        pBuffer[0x3B] = m_G0LatinSubset[G0LatinSubset][3];
        pBuffer[0x3C] = m_G0LatinSubset[G0LatinSubset][4];
        pBuffer[0x3D] = m_G0LatinSubset[G0LatinSubset][5];
        pBuffer[0x3E] = m_G0LatinSubset[G0LatinSubset][6];
        pBuffer[0x3F] = m_G0LatinSubset[G0LatinSubset][7];
        pBuffer[0x40] = m_G0LatinSubset[G0LatinSubset][8];
        pBuffer[0x5B] = m_G0LatinSubset[G0LatinSubset][9];
        pBuffer[0x5C] = m_G0LatinSubset[G0LatinSubset][10];
        pBuffer[0x5D] = m_G0LatinSubset[G0LatinSubset][11];
        pBuffer[0x5E] = m_G0LatinSubset[G0LatinSubset][12];
    }
}



