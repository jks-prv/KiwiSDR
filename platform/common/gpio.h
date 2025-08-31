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
#ifndef _PASM_

#ifdef GPIO_GEN1
 #define _GPIO_REVISION     0x000
 #define _GPIO_SYSCONFIG    0x010
 #define _GPIO_CLR_IRQ0		0x03c
 #define _GPIO_CLR_IRQ1		0x040
 #define _GPIO_OE			0x134
 #define _GPIO_IN			0x138
 #define _GPIO_OUT			0x13c
 #define _GPIO_CLR			0x190
 #define _GPIO_SET			0x194

 #define _GPIO_REG(g, off)   gpio_m[(g).bank][(off) >> 2]
 #define _GPIO_ADDR(g, off)  &_GPIO_REG(g, off)
 #define _GPIO_BIT(g)        (1 << (g).bit)
 
 #define GPIO_REVISION(g)	_GPIO_REG(g, _GPIO_REVISION)
 #define GPIO_SYSCONFIG(g)	_GPIO_REG(g, _GPIO_SYSCONFIG)
 #define GPIO_CLR_IRQ0(g)	_GPIO_REG(g, _GPIO_CLR_IRQ0) = _GPIO_BIT(g)
 #define GPIO_CLR_IRQ1(g)	_GPIO_REG(g, _GPIO_CLR_IRQ1) = _GPIO_BIT(g)
 #define GPIO_OE(g)			_GPIO_REG(g, _GPIO_OE)      // 0 = output
 #define GPIO_IN(g)			_GPIO_REG(g, _GPIO_IN)
 #define GPIO_OUT(g)        _GPIO_REG(g, _GPIO_OUT)
 #define GPIO_CLR(g)        _GPIO_REG(g, _GPIO_CLR)
 #define GPIO_SET(g)        _GPIO_REG(g, _GPIO_SET)
 
 #define GPIO_OUTPUT(g)		GPIO_OE(g) = GPIO_OE(g) & ~_GPIO_BIT(g);
 #define GPIO_INPUT(g)		GPIO_OE(g) = GPIO_OE(g) | _GPIO_BIT(g);
 #define GPIO_isOE(g)		((GPIO_OE(g) & _GPIO_BIT(g))? 0:1)

 struct gpio_t {
    u1_t bank, bit, pin, eeprom_off;
    bool isOutput;
    void init() {}
 };
#endif

#ifdef GPIO_GEN2
 // CPU_TDA4VM J721E_registers4.pdf table 2-2 pdfpg 56
 // CPU_AM67   J722S_Registers_Public_20250115.xlsx sheet 70_GPIO0
 #define _GPIO_ID			0x000
 #define _GPIO_IEN		    0x008   // enable = 1
 #define _GPIO_DIR		    0x010   // input = 1
 #define _GPIO_OUT			0x014
 #define _GPIO_SET			0x018
 #define _GPIO_CLR			0x01c
 #define _GPIO_IN			0x020
 
 #define GPIO_CLR_IRQ0(g)    // assume already cleared
 #define GPIO_CLR_IRQ1(g)
 
 // 9*16 = 144 i.e. 9 banks of 16 GPIOs each, regs store 32 GPIOs e.g. GPIO01 GPIO23 ...
 // e.g. GPIO_DIR() = n*0x28 + 0x10, n = bit/32, 0x10 = _GPIO_DIR
 //  GPIO_DIR01:0x10 GPIO_DIR23:0x38 GPIO_DIR45:0x60 GPIO_DIR67:0x88 GPIO_DIR8:0xb0
 #define _GPIO_OFF(g, off)   ((((g).bit_div32 * 0x28) + (off)) >> 2)
 #define _GPIO_REG(g, off)   gpio_m[GPIO_BANK(g)][_GPIO_OFF(g, off)]
 #define _GPIO_ADDR(g, off)  &_GPIO_REG(g, off)
 #define _GPIO_BIT(g)        (1 << ((g).bit_mod32))
 
 #define GPIO_ID(g)			gpio_m[GPIO_BANK(g)][0]
 #define GPIO_DIR(g)        _GPIO_REG(g, _GPIO_DIR)
 #define GPIO_IN(g)			_GPIO_REG(g, _GPIO_IN)
 #define GPIO_OUT(g)        _GPIO_REG(g, _GPIO_OUT)
 #define GPIO_CLR(g)        _GPIO_REG(g, _GPIO_CLR)
 #define GPIO_SET(g)        _GPIO_REG(g, _GPIO_SET)
 
 #define GPIO_OUTPUT(g)		GPIO_DIR(g) = GPIO_DIR(g) & ~_GPIO_BIT(g);
 #define GPIO_INPUT(g)		GPIO_DIR(g) = GPIO_DIR(g) | _GPIO_BIT(g);
 #define GPIO_isOUT(g)		((GPIO_DIR(g) & _GPIO_BIT(g))? 0:1)

 struct gpio_t {
    u1_t bank, bit, pin, eeprom_off;
    u1_t bit_div32, bit_mod32;
    bool isOutput;

    void init() {
        bit_div32 = bit / 32;
        bit_mod32 = bit % 32;
    }
 };
#endif

#define	GPIO_CLR_BIT(g)		GPIO_CLR(g) = _GPIO_BIT(g);
#define	GPIO_SET_BIT(g)		GPIO_SET(g) = _GPIO_BIT(g);
#define	GPIO_READ_BIT(g)	((GPIO_IN(g) & _GPIO_BIT(g))? 1:0)
#define	GPIO_WRITE_BIT(g,b)	{ if (b) { GPIO_SET_BIT(g) } else { GPIO_CLR_BIT(g) } }

typedef struct {
	gpio_t gpio;
	
	#define PIN_USED		0x8000
	#define PIN_DIR_IN		0x2000
	#define PIN_DIR_OUT		0x4000
	#define PIN_DIR_BIDIR	0x6000
	#define PIN_PMUX_BITS	0x007f
	u2_t attrs;
} __attribute__((packed)) pin_t;

#define	EE_NPINS 				74
extern pin_t eeprom_pins[EE_NPINS];
#define	EE_PINS_OFFSET_BASE		88

extern gpio_t GPIO_NONE;
#define GPIO_EQ(g1, g2) ((g1).pin == (g2).pin)
#define isGPIO(g)	    ((g).bit != 0xff)

#define GPIO_HIZ	-1

typedef enum { GPIO_DIR_IN, GPIO_DIR_OUT, GPIO_DIR_BIDIR } gpio_dir_e;

#endif
