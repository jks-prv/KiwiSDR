/*
 * slowrx - an SSTV decoder
 * * * * * * * * * * * * * *
 * 
 * Copyright (c) 2007-2013, Oona Räisänen (OH2EIQ [at] sral.fi)
 */

#include "sstv.h"

/*
 * Mode specifications
 *
 * Name          Long, human-readable name for the mode
 * ShortName     Abbreviation for the mode, used in filenames
 * SyncTime      Duration of synchronization pulse in seconds
 * PorchTime     Duration of sync porch pulse in seconds
 * SeptrTime     Duration of channel separator pulse in seconds
 * PixelTime     Duration of one pixel in seconds
 * LineTime      Time in seconds from the beginning of a sync pulse to the beginning of the next one
 * ImgWidth      Pixels per scanline
 * NumLines      Number of scanlines
 * LineHeight    Height of one scanline in pixels (1 or 2)
 * ColorEnc      Color format (GBR, RGB, YUV, BW)
 * Format        Format of scanline (sync, porch, pixel data etc.)
 *
 *
 * Note that these timings do not fully describe the workings of the different modes.
 *
 * References: 
 *             
 *             Martin Bruchanov OK2MNM (2012, 2019)
 *             www.sstv-handbook.com/download/sstv_04.pdf (contains some errors)
 *
 *             JL Barber N7CXI, "Proposal for SSTV Mode Specifications". Presented at the
 *             Dayton SSTV forum, 20 May 2000.
 *             www.barberdsp.com/downloads/Dayton%20Paper.pdf
 *
 *             Dave Jones KB4YZ (1999): "SSTV Modes - Line Timing"
 *             www.tima.com/~djones/line.txt (link broken)
 *             see instead: kiwisdr.com/docs/SSTV_djones_line.txt
 *
 *             www.digigrup.org/ccdd/sstv.htm
 *
 *             github.com/ON4QZ/QSSTV
 *
 *             bruxy.regnet.cz/web/hamradio/EN/a-look-into-sstv-mode
 */


// NB: ordering must match values sstv.h enums

ModeSpec_t ModeSpec[] = {
    {},     // UNKNOWN
    {},     // VISX


////////////////////////////////
// AVT
//
// QSSTV, 
////////////////////////////////

  {
    "Amiga Video Transceiver 24",
    "AVT24",
    0,
    0,
    0e-3,
    0,
    0,
    128, 120, 1,
    RGB, FMT_111,
    UNSUPPORTED
  },

  {
    "Amiga Video Transceiver 90",
    "AVT90",
    0,
    0,
    0e-3,
    0,
    0,
    320, 256, 1,
    RGB, FMT_111,
    UNSUPPORTED
  },

  {
    "Amiga Video Transceiver 94",
    "AVT94",
    0,
    0,
    0e-3,
    0,
    0,
    320, 200, 1,
    RGB, FMT_111,
    UNSUPPORTED
  },


////////////////////////////////
// Martin
//
// NB:
//  Has a trailing gap, i.e. last "g" in SpGgBgRg
//  Not FMT_111_REV even though GBR
//
// Timings: N7CXI, QSSTV, OK2MNM
////////////////////////////////

  {
    "Martin M1",
    "M1",
    4.862e-3,       // sync
    0.572e-3,       // porch
    0.572e-3,       // gap
    0.4576e-3,      // pix 146.432/320
    446.446e-3,     // line SpGgBgRg 4.862+(3*(0.572+146.432))+0.572 = 446.446
    320, 256, 1,
    GBR, FMT_111
  },

  {
    "Martin M2",
    "M2",
    4.862e-3,       // sync
    0.572e-3,       // porch
    0.572e-3,       // gap
    0.2288e-3,      // pix 73.216/320
    226.798e-3,     // line SpGgBgRg 4.862+(3*(0.572+73.216))+0.572 = 226.798
    320, 256, 1,
    GBR, FMT_111
  },

  {
    "Martin M3",
    "M3",
    4.862e-3,       // sync
    0.572e-3,       // porch
    0.572e-3,       // gap
    0.4576e-3,      // pix 146.432/320
    446.446e-3,     // line SpGgBgRg 4.862+(3*(0.572+146.432))+0.572 = 446.446
    320, 128, 2,
    GBR, FMT_111,
    LINE_DOUBLE     // line doubling to fill-out 128 => 256 height
  },

  {
    "Martin M4",
    "M4",
    4.862e-3,       // sync
    0.572e-3,       // porch
    0.572e-3,       // gap
    0.2288e-3,      // pix 73.216/320
    226.798e-3,     // line SpGgBgRg 4.862+(3*(0.572+73.216))+0.572 = 226.798
    320, 128, 2,
    GBR, FMT_111,
    LINE_DOUBLE     // line doubling to fill-out 128 => 256 height
  },


////////////////////////////////
// Scottie
////////////////////////////////

  {  // N7CXI
    "Scottie S1",
    "S1",
    9e-3,           // sync
    1.5e-3,         // porch
    1.5e-3,         // gap
    0.4320125e-3,   // pix 138.244/320
    428.232e-3,     // line gGgBSpR = (2*1.5)+(3*138.244)+9+1.5 = 428.232
    320, 256, 1,
    GBR, FMT_111_REV
  },

  {  // N7CXI
    "Scottie S2",
    "S2",
    9e-3,           // sync
    1.5e-3,         // porch
    1.5e-3,         // gap
    0.2752e-3,      // pix 88.064/320
    277.692e-3,     // line gGgBSpR = (2*1.5)+(3*88.064)+9+1.5 = 277.692
    320, 256, 1,
    GBR, FMT_111_REV
  },

  {  // N7CXI
    "Scottie DX",
    "SDX",
    9e-3,           // sync
    1.5e-3,         // porch
    1.5e-3,         // gap
    1.08e-3,        // pix 345.6/320
    1050.3e-3,      // line gGgBSpR = (2*1.5)+(3*345.6)+9+1.5 = 1050.3
    320, 256, 1,
    GBR, FMT_111_REV
  },


////////////////////////////////
// Robot
//
// Timings: N7CXI, MMSSTV, BruXy (SSTV handbook is wrong)
////////////////////////////////

  {  // OK2MNM
    "Robot 12",
    "R12",
    9e-3,           // sync
    3e-3,           // porch
    6e-3,           // gap = 4.5 even/odd separator + 1.5 porch
    0.085415625e-3, // pix 27.333/320
    100e-3,         // line = SpY[UV]g 9+3+(54.666+27.333)+6 = 100
                    // 600 LPM = 100
    320, 120, 2,
    YUV, FMT_420,
    LINE_DOUBLE     // line doubling to fill-out 120 => 240 height
  },


  {  // OK2MNM
    "Robot 24",
    "R24",
    9e-3,           // sync
    3e-3,           // porch
    6e-3,           // gap = 4.5 even/odd separator + 1.5 porch
    0.1375e-3,      // pix 44/320
    200e-3,         // line = SpY[UV]g 9+3+88+(2*(6+44)) = 200
                    // 300 LPM = 200
    320, 120, 2,
    YUV, FMT_422,
    LINE_DOUBLE     // line doubling to fill-out 120 => 240 height
  },

  {  // N7CXI
    "Robot 36",
    "R36",
    9e-3,           // sync
    3e-3,           // porch
    6e-3,           // gap = 4.5 even/odd separator + 1.5 porch
    0.1375e-3,      // pix 44/320
    150e-3,         // line = SpY[UV]g 9+3+(88+44)+6 = 150
                    // 400 LPM = 150
    320, 240, 1,
    YUV, FMT_420
  },

  {  // N7CXI
    "Robot 72",
    "R72",
    9e-3,           // sync
    3e-3,           // porch
    6e-3,           // gap = 4.5 sep + 1.5 porch
    0.215625e-3,    // pix 69/320
    300e-3,         // line = SpYYgUgV = 9+3+138+(2*(6+69)) = 300
                    // 200 LPM = 300
    320, 240, 1,
    YUV, FMT_422
  },

  {  // N7CXI
    "Robot 8 B/W",
    "R8-BW",
    7e-3,
    0e-3,
    0e-3,
    0.188e-3,
    67e-3,
    320, 240, 2,
    BW, FMT_BW
  },

  {  // N7CXI
    "Robot 12 B/W",
    "R12-BW",
    7e-3,
    0e-3,
    0e-3,
    0.291e-3,
    100e-3,
    320, 240, 2,
    BW, FMT_BW
  },

  {  // N7CXI
    "Robot 24 B/W",
    "R24-BW",
    7e-3,
    0e-3,
    0e-3,
    0.291e-3,
    100e-3,
    320, 240, 1,
    BW, FMT_BW
  },


////////////////////////////////
// Wraase
////////////////////////////////

  { // MMSSTV
    "Wraase SC-2 60",
    "SC60",
    5.5006e-3,          // sync
    0.5e-3,             // porch
    0e-3,               // gap
    0.24415e-3,         // pix 78.128/320
    240.3846e-3,        // line SpRGB = 5.5006+0.5+(3*78.128) = 240.3846
                        // 249.6 LPM = 240.3846
    320, 256, 1,
    RGB, FMT_111
    //1, RGB, FMT_242     // OK2MNM
  },

  { // MMSSTV
    "Wraase SC-2 120",
    "SC120",
    5.52248e-3,         // sync
    0.5e-3,             // porch
    0e-3,               // gap
    0.4890625e-3,       // pix 156.5/320
    475.52248e-3,       // line SpRGB = 5.52248+0.5+(3*156.5) = 475.52248
                        // 126.175 LPM = 475.53
    320, 256, 1,
    RGB, FMT_111
    //1, RGB, FMT_242     // OK2MNM
  },

  {  // MMSSTV
    "Wraase SC-2 180",
    "SC180",
    5.5437e-3,          // sync (MMSSTV, LPM)
    0.5e-3,             // porch
    0e-3,               // gap
    0.734375e-3,        // pix 235/320
    711.0437e-3,        // line SpRGB = 5.5437+0.5+(3*235) = 711.0437
                        // 84.383 LPM = 711.0437
    320, 256, 1,
    RGB, FMT_111
  },


////////////////////////////////
// PD
//
// NB: Yodd U V Yeven, hence LineHeight = 2
// Timings: N7CXI, QSSTV, MMSSTV
////////////////////////////////

  {
    "PD-50",
    "PD50",
    20e-3,              // sync
    2.08e-3,            // porch
    0e-3,               // gap
    0.286e-3,           // pix 91.52/320
    388.16e-3,          // line SpYUVY = 20+2.08+91.52+91.52+91.52+91.52 = 388.16
    320, 256, 2,
    YUV, FMT_111
  },

  {
    "PD-90",
    "PD90",
    20e-3,
    2.08e-3,
    0e-3,
    0.532e-3,               // pix 170.24/320
    703.04e-3,              // SpYUVY = 20+2.08+170.24+170.24+170.24+170.24 = 703.04
    320, 256, 2,
    YUV, FMT_111
  },

  {
    "PD-120",
    "PD120",
    20e-3,                  // sync
    2.08e-3,                // porch
    0e-3,                   // gap
    0.19e-3,                // pix 121.6/640
    508.48e-3,              // SpYUVY = 20+2.08+121.6+121.6+121.6+121.6 = 508.48
    640, 496, 2,
    YUV, FMT_111
  },

  {
    "PD-160",
    "PD160",
    20e-3,
    2.08e-3,
    0e-3,
    0.382e-3,               // pix 195.584/512
    804.416e-3,             // SpYUVY = 20+2.08+195.584+195.584+195.584+195.584 = 804.416
    512, 400, 2,
    YUV, FMT_111
  },

  {
    "PD-180",
    "PD180",
    20e-3,
    2.08e-3,
    0e-3,
    0.286e-3,               // pix 183.040/640
    754.24e-3,              // SpYUVY = 20+2.08+183.040+183.040+183.040+183.040 = 754.240
    640, 496, 2,
    YUV, FMT_111
  },

  {
    "PD-240",
    "PD240",
    20e-3,
    2.08e-3,
    0e-3,
    0.382e-3,               // pix 244.480/640
    1000e-3,                // SpYUVY = 20+2.08+244.480+244.480+244.480+244.480 = 1000.0
    640, 496, 2,
    YUV, FMT_111
  },

  {
    "PD-290",
    "PD290",
    20e-3,
    2.08e-3,
    0e-3,
    0.286e-3,               // pix 228.800/800
    937.28e-3,              // SpYUVY = 20+2.08+228.800+228.800+228.800+228.800 = 937.280
    800, 616, 2,
    YUV, FMT_111
  },


////////////////////////////////
// Pasokon
//
// NB: Has a trailing gap, i.e. last "g" in SpGgBgRg
////////////////////////////////

  {  // N7CXI
    "Pasokon P3",
    "P3",
    25.0/4800.0,    // sync 25x pix
    0e-3,           // porch 0x pix
    5.0/4800.0,     // gap 5x pix
    1.0/4800.0,     // pix 4800 Hz = 0.208333e-3
    409.375e-3,     // line SpRgGgBg = (25+3*(5+640)+5)/4800 = 409.375
    640, 496, 1,
    RGB, FMT_111
  },

  {  // N7CXI
    "Pasokon P5",
    "P5",
    25.0/3200.0,    // sync 25x pix
    0e-3,           // porch 0x pix
    5.0/3200.0,     // gap 5x pix
    1.0/3200.0,     // pix 3200 Hz = 0.3125e-3
    614.0625e-3,    // line SpRgGgBg = (25+3*(5+640)+5)/3200 = 614.0625
    640, 496, 1,
    RGB, FMT_111
  },

  {  // N7CXI
    "Pasokon P7",
    "P7",
    25.0/2400.0,    // sync 25x pix
    0e-3,           // porch 0x pix
    5.0/2400.0,     // gap 5x pix
    1.0/2400.0,     // pix 2400 Hz = 0.416666e-3
    818.75e-3,      // line SpRgGgBg = (25+3*(5+640)+5)/2400 = 818.75
    640, 496, 1,
    RGB, FMT_111
  },


////////////////////////////////
// MMSSTV
//
// MP is like PD: Yodd U V Yeven
////////////////////////////////

  {
    "MMSSTV MP73",
    "MP73",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0e-3,           // gap
    0.4375e-3,      // pix 140/320
    570.0e-3,       // SpYUVY = 9+1+(4*140) = 570
    320, 128, 2,
    YUV, FMT_111,
    LINE_DOUBLE     // line doubling to fill-out 128 => 256 height
  },

  {
    "MMSSTV MP115",
    "MP115",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0e-3,           // gap
    0.696875e-3,    // pix 223/320
    902.0e-3,       // SpYUVY = 9+1+(4*223) = 902
    320, 128, 2,
    YUV, FMT_111,
    LINE_DOUBLE     // line doubling to fill-out 128 => 256 height
  },

  {
    "MMSSTV MP140",
    "MP140",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0e-3,           // gap
    0.84375e-3,     // pix 270/320
    1090.0e-3,      // SpYUVY = 9+1+(4*270) = 1090
    320, 128, 2,
    YUV, FMT_111,
    LINE_DOUBLE     // line doubling to fill-out 128 => 256 height
  },

  {
    "MMSSTV MP175",
    "MP175",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0e-3,           // gap
    1.0625e-3,      // pix 340/320
    1370.0e-3,      // SpYUVY = 9+1+(4*340) = 1370
    320, 128, 2,
    YUV, FMT_111,
    LINE_DOUBLE     // line doubling to fill-out 128 => 256 height
  },

// NB:
//  Has a trailing gap, i.e. last "g" in SpYYgUgVg (MMSSTV says so)
//  Our color bar test file has bar overlap?
  {
    "MMSSTV MR73",
    "MR73",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0.1e-3,         // gap
    0.215625e-3,    // pix 138/2/320
    286.3e-3,       // line SpYYgUgVg = 9+1+(138+0.1)+(2*(138/2+0.1)) = 286.3
    320, 256, 1,
    YUV, FMT_422
  },

  {
    "MMSSTV MR90",
    "MR90",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0.1e-3,         // gap
    0.2671875e-3,   // pix 171/2/320
    352.3e-3,       // line SpYYgUgVg = 9+1+(171+0.1)+(2*(171/2+.1)) = 352.3
    320, 256, 1,
    YUV, FMT_422
  },

  {
    "MMSSTV MR115",
    "MR115",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0.1e-3,         // gap
    0.34375e-3,     // pix 220/2/320
    450.3e-3,       // line SpYYgUgVg = 9+1+(220+0.1)+(2*(220/2+0.1)) = 450.3
    320, 256, 1,
    YUV, FMT_422
  },

  {
    "MMSSTV MR140",
    "MR140",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0.1e-3,         // gap
    0.4203125e-3,   // pix 269/2/320
    548.3e-3,       // line SpYYgUgVg = 9+1+(269+0.1)+(2*(269/2+0.1)) = 548.3
    320, 256, 1,
    YUV, FMT_422
  },

  {
    "MMSSTV MR175",
    "MR175",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0.1e-3,         // gap
    0.5265625e-3,   // pix 337/2/320
    684.3e-3,       // line SpYYgUgVg = 9+1+(337+0.1)+(2*(337/2+0.1)) = 684.3
    320, 256, 1,
    YUV, FMT_422
  },

// NB:
//  Has a trailing gap, i.e. last "g" in SpYYgUgVg (MMSSTV says so)
//  Our color bar test file has bar overlap?
  {
    "MMSSTV ML180",
    "ML180",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0.1e-3,         // gap
    0.137890625e-3, // pix 176.5/2/640
    363.3e-3,       // line SpYYgUgVg = 9+1+(176.5+0.1)+(2*(176.5/2+0.1)) = 363.3
    640, 496, 1,
    YUV, FMT_422
  },

  {
    "MMSSTV ML240",
    "ML240",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0.1e-3,         // gap
    0.184765625e-3, // pix 236.5/2/640
    483.3e-3,       // line SpYYgUgVg = 9+1+(236.5+0.1)+(2*(236.5/2+0.1)) = 483.3
    640, 496, 1,
    YUV, FMT_422
  },

  {
    "MMSSTV ML280",
    "ML280",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0.1e-3,         // gap
    0.216796875e-3, // pix 277.5/2/640
    565.3e-3,       // line SpYYgUgVg = 9+1+(277.5+0.1)+(2*(277.5/2+0.1)) = 565.3
    640, 496, 1,
    YUV, FMT_422
  },

  {
    "MMSSTV ML320",
    "ML320",
    9.0e-3,         // sync
    1.0e-3,         // porch
    0.1e-3,         // gap
    0.248046875e-3, // pix 317.5/2/640
    645.3e-3,       // line SpYYgUgVg = 9+1+(317.5+0.1)+(2*(317.5/2+0.1)) = 645.3
    640, 496, 1,
    YUV, FMT_422
  },


////////////////////////////////
// FAX480
//
// OK2MNM chap 4.3.1
////////////////////////////////
  {
    "FAX480",
    "FAX480",
    5.12e-3,        // sync
    0e-3,           // porch
    0e-3,           // gap
    0.512e-3,       // pix 262.144/512
    267.264e-3,     // line S0 = 5.12+262.144 = 267.264
                    // 224.497 LPM = 267.264
    512, 480, 1,
    BW, FMT_BW
  }
};


/*
 * Mapping of 7-bit VIS codes to modes
 *
 * Reference: Dave Jones KB4YZ (1998): "List of SSTV Modes with VIS Codes".
 *            www.tima.com/~djones/vis.txt (link broken)
 *
 */

u1_t   VISmap[] = {
//  x0     x1     x2     x3    x4     x5     x6     x7     x8     x9     xA     xB    xC    xD    xE     xF
    R12,   0,     R8BW,  0,    R24,   FX480, R12BW, 0,     R36,   0,     R24BW, 0,    R72,  0,    0,     0,     // 0x
    0,     0,     0,     0,    0,     0,     0,     0,     0,     0,     0,     0,    0,    0,    0,     0,     // 1x
    M4,    0,     0,     VISX, M3,    0,     0,     0,     M2,    0,     0,     0,    M1,   0,    0,     0,     // 2x
    0,     0,     0,     0,    0,     0,     0,     SC180, S2,    0,     0,     SC60, S1,   0,    0,     SC120, // 3x
    AVT24, 0,     0,     0,    AVT90, 0,     0,     0,     AVT94, 0,     0,     0,    SDX,  0,    0,     0,     // 4x
    0,     0,     0,     0,    0,     0,     0,     0,     0,     0,     0,     0,    0,    PD50, PD290, PD120, // 5x
    PD180, PD240, PD160, PD90, 0,     0,     0,     0,     0,     0,     0,     0,    0,    0,    0,     0,     // 6x
    0,     P3,    P5,    P7,   0,     0,     0,     0,     0,     0,     0,     0,    0,    0,    0,     0      // 7x
};


/*
 * MMSSTV extended VIS codes
 */
 
u1_t   VISXmap[] = {
//  x0    x1    x2    x3   x4    x5     x6     x7    x8     x9     xA     xB   xC     xD   xE    xF
    0,    0,    0,    0,   0,    ML180, ML240, 0,    0,     ML280, ML320, 0,   0,     0,   0,    0,     // 0x
    0,    0,    0,    0,   0,    0,     0,     0,    0,     0,     0,     0,   0,     0,   0,    0,     // 1x
    0,    0,    0,    0,   0,    MP73,  0,     0,    0,     MP115, MP140, 0,   MP175, 0,   0,    0,     // 2x
    0,    0,    0,    0,   0,    0,     0,     0,    0,     0,     0,     0,   0,     0,   0,    0,     // 3x
    0,    0,    0,    0,   0,    MR73,  MR90,  0,    0,     MR115, MR140, 0,   MR175, 0,   0,    0,     // 4x
    0,    0,    0,    0,   0,    0,     0,     0,    0,     0,     0,     0,   0,     0,   0,    0,     // 5x
    0,    0,    0,    0,   0,    0,     0,     0,    0,     0,     0,     0,   0,     0,   0,    0,     // 6x
    0,    0,    0,    0,   0,    0,     0,     0,    0,     0,     0,     0,   0,     0,   0,    0,     // 7x
};
