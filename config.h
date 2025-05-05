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

// Copyright (c) 2016-2025 John Seamons, ZL4VO/KF6VO

#pragma once

// these defines are set by the makefile:
// DEBUG
// VERSION_MAJ, VERSION_MIN
// ARCH_*, PLATFORM_*
// LOGGING_HOST, KIWI_UI_LIST, REPO
// {EDATA_DEVEL, EDATA_EMBED}

typedef enum { ESPEED_AUTO = 0, ESPEED_10M = 1, ESPEED_100M = 2 } espeed_e;

#define FW_CONFIGURED       -2
#define FW_OTHER            -1
#define FW_SEL_SDR_RX4_WF4  0
#define FW_SEL_SDR_RX8_WF2  1
#define FW_SEL_SDR_RX3_WF3  2
#define FW_SEL_SDR_RX14_WF0 3
#define FW_SEL_SDR_WB       4
#define FW_SEL_SDR_RX8_WF8  5
#define N_FW_SEL            6

#define MAX_RX_CHANS    16      // must be pow2, see coroutines.h:CTF_CHANNEL
#define MAX_WF_CHANS    4       // max WF DDCs
#define MAX_WB_CHANS    1
#define MAX_NRX_BUFS    16      // for RXBUF_SIZE_14CH
#define MAX_NRX_SAMPS   226     // for nch = 3

#ifdef PLATFORM_beagleY_ai
    #define NRX_BUG 2           // 2w/4b for SPI_32 cfg
#else
    #define NRX_BUG 0
#endif

#define NRX_SPI         SPIBUF_W
#define NRX_SPI_ST      2           // 2w/4b for SPI_32 cfg
#define NRX_OVHD        (3 + 1 + 1) // ticks 3w, ctr_stored 1w, ctr_current 1w
#define NRX_IQW         S2B(3)      // 1.5 words (24-bits) per I,Q
#define NRX_SAMPS_CHANS(nch) (S2B(NRX_SPI - NRX_BUG - NRX_SPI_ST - NRX_OVHD) / NRX_IQW / nch);
#define NRX_SAMPS_MAX         S2B(NRX_SPI - NRX_BUG - NRX_SPI_ST)
#define MAX_WB_SAMPS    1024
#define SPIBUF_BMAX     (SPIBUF_B - S2B(NRX_BUG))

#define N_CONN_ADMIN        1   // simultaneous admin connections

#define N_CAMP              4
#define N_QUEUERS           8

#define PROXY_SERVER_HOST   "proxy.kiwisdr.com"
#define PROXY_SERVER_PORT   8073

extern int fw_sel, fpga_id, rx_chans, rx_wb_buf_chans, wf_chans, wb_chans,
    nrx_bufs, nrx_samps_wb, nrx_samps, nrx_samps_total, nrx_samps_loop, nrx_samps_rem,
    snd_rate, snd_rate_i, wb_rate, rx_decim;

// INET6_ADDRSTRLEN (46) plus some extra in case ipv6 scope/zone is an issue
// can't be in net.h due to #include recursion problems
#define NET_ADDRSTRLEN      64
#define NET_ADDRSTRLEN_S    "64"

#define	STATS_INTERVAL_SECS			10

#define PHOTO_UPLOAD_MAX_SIZE   (2*M)
#define DX_UPLOAD_MAX_SIZE      (2*M)

typedef struct {
	const char *param, *value;
} index_html_params_t;
