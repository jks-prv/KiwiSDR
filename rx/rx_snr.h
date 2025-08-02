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

// Copyright (c) 2023-2025 John Seamons, ZL4VO/KF6VO

#pragma once

#include "types.h"

#define SNR_MEAS_MAX    (24 * 7)

#define SNR_BAND_ALL        0
#define SNR_BAND_HF         1
#define SNR_BAND_0_2        2
#define SNR_BAND_2_10       3
#define SNR_BAND_10_20      4
#define SNR_BAND_20_MAX     5
#define SNR_BAND_NSTD       6
#define SNR_BAND_CUSTOM     6

#define SNR_BAND_HAM_LF     7
#define SNR_BAND_HAM_MF     8
#define SNR_BAND_HAM_160    9
#define SNR_BAND_HAM_80     10
#define SNR_BAND_HAM_60     11
#define SNR_BAND_HAM_40     12
#define SNR_BAND_HAM_30     13
#define SNR_BAND_HAM_20     14
#define SNR_BAND_HAM_17     15
#define SNR_BAND_HAM_15     16
#define SNR_BAND_HAM_12     17
#define SNR_BAND_HAM_10     18
#define SNR_BAND_AM_BCB     19
#define SNR_NBANDS          20

typedef struct {
    float fkHz_lo, fkHz_hi;
    u1_t min, max, pct_50, pct_95, snr;
} SNR_data_t;

typedef struct {
    #define SNR_F_ANT       0x0f
    #define SNR_F_VALID     0x10
    #define SNR_F_TLOCAL    0x20
	u1_t flags;
    u2_t seq;
    time_t tstamp;
    SNR_data_t data[SNR_NBANDS];
} SNR_meas_t;

extern SNR_meas_t SNR_meas_data[SNR_MEAS_MAX];
extern int SNR_meas_tid;

void SNR_meas(void *param);
