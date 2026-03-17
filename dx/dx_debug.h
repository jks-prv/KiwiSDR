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

// Copyright (c) 2016-2026 John Seamons, ZL4VO/KF6VO

#pragma once

#include "ansi.h"

//#define DX_PRINT
#ifdef DX_PRINT

    // -dx 0xhhh
    
	#define DX_PRINT_MKRS 0x01
	#define dx_print_mkrs(cond, fmt, ...) \
		if ((dx_print & DX_PRINT_MKRS) && (cond)) cprintf(conn, fmt, ## __VA_ARGS__)

	#define DX_PRINT_MKRS_ALL 0x02
	#define dx_print_mkrs_all(cond, fmt, ...) \
		if ((dx_print & DX_PRINT_MKRS_ALL) && (cond)) cprintf(conn, fmt, ## __VA_ARGS__)

	#define DX_PRINT_ADM_MKRS 0x04
	#define dx_print_adm_mkrs(fmt, ...) \
		if (dx_print & DX_PRINT_ADM_MKRS) cprintf(conn, fmt, ## __VA_ARGS__)

	#define DX_PRINT_UPD 0x08
	#define dx_print_upd(fmt, ...) \
		if (dx_print & DX_PRINT_UPD) cprintf(conn, fmt, ## __VA_ARGS__)

	#define DX_PRINT_SEARCH 0x10
	#define dx_print_search(cond, fmt, ...) \
		if ((dx_print & DX_PRINT_SEARCH) && (cond)) cprintf(conn, fmt, ## __VA_ARGS__)

	#define DX_PRINT_FILTER 0x20
	#define dx_print_filter(cond, fmt, ...) \
		if ((dx_print & DX_PRINT_FILTER) && (cond)) cprintf(conn, fmt, ## __VA_ARGS__)

	#define DX_PRINT_DOW_TIME 0x40
	#define dx_print_dow_time(cond, fmt, ...) \
		if ((dx_print & DX_PRINT_DOW_TIME) && (cond)) printf(fmt, ## __VA_ARGS__)

	#define DX_PRINT_DEBUG 0x80
	#define dx_print_debug(cond, fmt, ...) \
		if ((dx_print & DX_PRINT_DEBUG) && (cond)) printf(fmt, ## __VA_ARGS__)

	#define DX_PRINT_MASKED 0x100
	#define dx_print_masked(fmt, ...) \
		if (dx_print & DX_PRINT_MASKED) printf(fmt, ## __VA_ARGS__)

    #define DX_DONE() \
        if (dx_print) _dx_done(conn, mark, max_quanta, loop, nt_loop, send, nt_send, msg_sl)

#else
	#define DX_PRINT_MKRS 0
	#define DX_PRINT_UPD 0
	#define DX_PRINT_FILTER 0
	#define dx_print_mkrs(cond, fmt, ...)
	#define dx_print_mkrs_all(cond, fmt, ...)
	#define dx_print_adm_mkrs(fmt, ...)
	#define dx_print_upd(fmt, ...)
	#define dx_print_search(cond, fmt, ...)
	#define dx_print_filter(cond, fmt, ...)
	#define dx_print_dow_time(cond, fmt, ...)
	#define dx_print_debug(cond, fmt, ...)
	#define dx_print_masked(fmt, ...)
    #define DX_DONE()
#endif
