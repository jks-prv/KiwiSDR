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

// Copyright (c) 2019-2025 John Seamons, ZL4VO/KF6VO

#pragma once

#include "types.h"
#include "clk.h"
#include "misc.h"
#include "nbuf.h"
#include "web.h"
#include "spi.h"
#include "gps.h"
#include "coroutines.h"
#include "debug.h"
#include "data_pump.h"
#include "cfg.h"
#include "datatypes.h"
#include "ext_int.h"
#include "rx.h"
#include "rx_sound.h"
#include "dx.h"
#include "non_block.h"

//#define WF_INFO
#ifdef WF_INFO
	#define wf_printf(fmt, ...) \
		if (!bg) cprintf(wf->conn, fmt, ## __VA_ARGS__)
#else
	#define wf_printf(fmt, ...)
#endif

#define	CMD_ZOOM	0x01
#define	CMD_START	0x02
#define	CMD_DB		0x04
#define	CMD_SPEED	0x08
#define	CMD_WF_ALL  (CMD_ZOOM | CMD_START | CMD_DB | CMD_SPEED)

extern str_hash_t wf_cmd_hash;

#define WF_RD_OFFSET 512
extern int wf_rd_offset, wf_slowdown;

void rx_waterfall_cmd(conn_t *conn, int n, char *cmd);
void rx_waterfall_aperture_auto(wf_inst_t *wf, u1_t *bp);
