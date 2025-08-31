/*
--------------------------------------------------------------------------------
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
Boston, MA  02110-1301, USA.
--------------------------------------------------------------------------------
*/

// Copyright (c) 2022-2025 John Seamons, ZL4VO/KF6VO

#pragma once

// TDA4VM memory map (BBAI-64) TRM 2.1 pdfpg 126
#ifdef CPU_TDA4VM
 #define PRCM_BASE	NULL        // already powered up
 #define NPMUX      1
 #define PMUX_BASE	0x0011C000  // CTRL_MMR0_CFG0/CTRLMMR_PADCONFIG0
 #define GPIO0_BASE	0x00600000
 #define SPI_BASE	NULL        // SPI_PIO never used
 #define MMAP_SIZE	0x2000
#endif

// AM67 memory map (BYAI) TRM 2.1 pdfpg 40
#ifdef CPU_AM67
 #define PRCM_BASE	 NULL       // already powered up
 #define NPMUX       3
 #define PMUX01_BASE 0x000f4000 // PADCFG_CTRL0_CFG0
 #define PMUX2_BASE	 0x04084000 // MCU_PADCFG_CTRL0_CFG0
 #define GPIO0_BASE	 0x00600000
 #define GPIO1_BASE	 0x00601000
 #define GPIO2_BASE	 0x04201000 // really MCU_GPIO0_BASE
 #define SPI_BASE	 NULL       // SPI_PIO never used
 #define MMAP_SIZE	 0x2000
#endif


// PMUX: pin mux (remember: Linux doesn't allow write of pmux via mmap -- use device tree (dts) mechanism instead)

#ifdef CPU_TDA4VM
 #define    PMUX(g,off) pmux_m[0][(off) >> 2]

 // TRM 5.1.3.3.1.1 pad configuration regs
 // J721E_registers1.pdf 6.404 pdfpg 1048 padconfig1 reg
 // CAUTION: padconfig0 reg is different from all others (fields missing)
 // padconfig1 hw reset value 0x8214007
 
 #define	PMUX_ISOBP	0x00800000  // 23 isolation bypass
 #define	PMUX_ISOOV	0x00400000  // 22 isolation override

 #define	PMUX_TXDIS	0x00200000  // 21 TX disable

 #define	PMUX_DRIVE	0x00180000  // 20:19 LVCMOS drive strength
 #define	PMUX_SLOW	0x00100000
 #define	PMUX_FAST	0x00080000
 #define	PMUX_NOM	0x00000000

 #define	PMUX_RXEN	0x00040000  // 18 RX enable
 
 #define	PMUX_PU		0x00020000  // 17 1 = pull-up
 #define	PMUX_PD		0x00000000  // 17 0 = pull-down
 #define	PMUX_PDIS	0x00010000  // 16 1 = pull disable
 
 #define	PMUX_ST		0x00004000  // 14 schmitt trigger
 #define    PMUX_ATTR_S 20
 #define    PMUX_ATTR_E 14

 #define	PMUX_MODE   0x0000000f  // mode bits
 #define	PMUX_SPI    0x00000004  // SPI6 = mode 4
 #define	PMUX_GPIO   0x00000007  // GPIO = mode 7
 #define	PMUX_OFF    0x0000000f  // mode 15
#endif

#ifdef CPU_AM67
 #define    PMUX(g,off) pmux_m[(g).bank][(off) >> 2]

 // TRM 6.1.2.1.2 pad configuration regs
 // J722S_Registers_Public_20250115.xlsx sheet 90_PADCFG_CTRL0
 // CAUTION: padconfig0 reg is different from all others (fields missing)
 // padconfig0 0xf4000, padconfig1 0xf4004
 // padconfig1 hw reset value 0x8214007
 
 #define	PMUX_ISOBP	0x00800000  // 23 isolation bypass
 #define	PMUX_ISOOV	0x00400000  // 22 isolation override

 #define	PMUX_TXDIS	0x00200000  // 21 TX disable

 #define	PMUX_DRIVE	0x00180000  // 20:19 LVCMOS drive strength
 #define	PMUX_SLOW	0x00100000
 #define	PMUX_FAST	0x00080000
 #define	PMUX_NOM	0x00000000

 #define	PMUX_RXEN	0x00040000  // 18 RX enable
 
 #define	PMUX_PU		0x00020000  // 17 1 = pull-up
 #define	PMUX_PD		0x00000000  // 17 0 = pull-down
 #define	PMUX_PDIS	0x00010000  // 16 1 = pull disable
 
 #define	PMUX_ST		0x00004000  // 14 schmitt trigger
 #define    PMUX_ATTR_S 20
 #define    PMUX_ATTR_E 14

 #define	PMUX_MODE   0x0000000f  // mode bits, see: beagley_ai-pins.txt
 #define	PMUX_I2C    0x00000000  // I2C  = mode 0
 #define	PMUX_SPI    0x00000001  // SPI  = mode 1
 #define	PMUX_GPIO   0x00000007  // GPIO = mode 7
 #define	PMUX_OFF    0x0000000f  // mode 15
#endif

#define	PMUX_OUT	(PMUX_NOM | PMUX_PDIS)
#define	PMUX_OUT_PU	(PMUX_NOM | PMUX_PU)
#define	PMUX_OUT_PD	(PMUX_NOM | PMUX_PD)

#define	PMUX_IN		(PMUX_NOM | PMUX_TXDIS | PMUX_RXEN | PMUX_PDIS)
#define	PMUX_IN_PU	(PMUX_NOM | PMUX_TXDIS | PMUX_RXEN | PMUX_PU)
#define	PMUX_IN_PD	(PMUX_NOM | PMUX_TXDIS | PMUX_RXEN | PMUX_PD)

#define	PMUX_IO		(PMUX_NOM | PMUX_RXEN | PMUX_PDIS)
#define	PMUX_IO_PU	(PMUX_NOM | PMUX_RXEN | PMUX_PU)
#define	PMUX_IO_PD	(PMUX_NOM | PMUX_RXEN | PMUX_PD)

#define PMUX_NONE   -1

// GPIO

#ifdef CPU_TDA4VM
 #define    GPIO_GEN2
 #define	NBALL	        2   // max number of cpu package balls wired to a single header pin
 #define	GPIO0	        0
 #define	NGPIO	        1
 #define    GPIO_NPINS      128
 #define    GPIO_BANK(gpio) ((gpio).bank)
#endif

#ifdef CPU_AM67
 #define    GPIO_GEN2
 #define	NBALL	        2   // max number of cpu package balls wired to a single header pin
 #define	GPIO0	        0
 #define	GPIO1	        1
 #define	GPIO2           2   // really MCU_GPIO0_BASE
 #define	NGPIO	        3
 #define    GPIO_NPINS      128 // pow2, not actual
 #define    GPIO_BANK(gpio) ((gpio).bank)
#endif

#include "gpio.h"
