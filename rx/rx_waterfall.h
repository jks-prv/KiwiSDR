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

// Copyright (c) 2019-2026 John Seamons, ZL4VO/KF6VO

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

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <math.h>
#include <fftw3.h>


//#define WF_INFO
#ifdef WF_INFO
	#define wf_printf(fmt, ...) \
		if (!bg) cprintf(wf->conn, fmt, ## __VA_ARGS__)
#else
	#define wf_printf(fmt, ...)
#endif

//#define WF_DEBUG
#ifdef WF_DEBUG
    #define wfd(x) x
    
    //#define WFP
    #ifdef WFP
        #define wfp(fmt, ...) \
            real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
    #else
        #define wfp(fmt, ...)
    #endif
    
    //#if 1       // WF_INTR detail
    #if 0
        #define wfp2(fmt, ...) \
            real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
    #else
        #define wfp2(fmt, ...)
    #endif
    
    //#if 1       // channel interleaving
    #if 0
        #define wfp3(fmt, ...) \
            real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
    #else
        #define wfp3(fmt, ...)
    #endif
    
    //#if 1       // sleep/intr seq
    #if 0
        #define wfp4(fmt, ...) \
            real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
    #else
        #define wfp4(fmt, ...)
    #endif
    
    //#if 1       // SPI
    #if 0
        #define wfp5(fmt, ...) \
            printf(fmt "\n", ## __VA_ARGS__);
            //real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
    #else
        #define wfp5(fmt, ...)
    #endif
    
    //#if 1       // size checks
    #if 0
        #define wfp6(fmt, ...) \
            printf(fmt "\n", ## __VA_ARGS__);
            //real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
    #else
        #define wfp6(fmt, ...)
    #endif
    
    //#if 1       // buffer selection debugging
    #if 0
        #define wfp7(fmt, ...) \
            real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
            //printf(fmt "\n", ## __VA_ARGS__);
    #else
        #define wfp7(fmt, ...)
    #endif
    
    //#if 1       // ddc assignment
    #if 0
        #define wfp8(fmt, ...) \
            real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
            //printf(fmt "\n", ## __VA_ARGS__);
    #else
        #define wfp8(fmt, ...)
    #endif
#else
    #define wfd(x)
    #define wfp(fmt, ...)
    #define wfp2(fmt, ...)
    #define wfp3(fmt, ...)
    #define wfp4(fmt, ...)
    #define wfp5(fmt, ...)
    #define wfp6(fmt, ...)
    #define wfp7(fmt, ...)
    #define wfp8(fmt, ...)
#endif


#define TR_WF_CMDS      0
#define SM_WF_DEBUG		false

//#define WF_SHMEM_DISABLE

#define	WF_FIRST_HALF_FFT	2	    // the result is contained in the first half of the complex FFT
#define	WF_BETTER_LOOKING	2	    // increase in FFT size for better looking display
#define	WF_USING_HALF_CIC	2       // only half of FFT to avoid CIC droop

#define WF_OUTPUT	        1024	// conceptually same as WF_WIDTH although not required
#define WF_NFFT             (WF_OUTPUT * WF_FIRST_HALF_FFT * WF_BETTER_LOOKING)
#define WF_NFFT_MAX         (WF_OUTPUT * WF_FIRST_HALF_FFT * WF_BETTER_LOOKING * WF_USING_HALF_CIC)
#define WF_NFFT_USED_MAX    (WF_NFFT_MAX / WF_FIRST_HALF_FFT)

#define WF_NSAMP            WF_NFFT // max hardware sample buffer length
#define WF_NSAMP_MAX        WF_NFFT_MAX // max hardware sample buffer length

#define	WF_WIDTH            1024	// width of waterfall display

#define MAX_ZOOM        14
#define ZOOM_CAP        (kiwi.wf_share ? 11 : 14)
#define	MAX_START(z)	((WF_WIDTH << MAX_ZOOM) - (WF_WIDTH << (MAX_ZOOM - z)))

struct fft_t {
	fftwf_complex hw_c_samps[WF_NSAMP];
	fftwf_complex hw_fft[WF_NFFT];

	fftwf_complex hw_c_sampsL[WF_NSAMP_MAX];
	fftwf_complex hw_fftL[WF_NFFT_MAX];
};

struct wf_pkt_t {
	char id4[4];
	u4_t x_bin_server;
	#define WF_ZOOM                 0x0000ffff
	#define WF_FLAGS                0xffff0000
	#define WF_FLAGS_COMPRESSION    0x00010000
	#define WF_FLAGS_NO_SYNC        0x00020000
	#define WF_FLAGS_DEBUG          0x00040000
	u4_t flags_x_zoom_server;
	u4_t seq;
	union {
		u1_t buf[WF_WIDTH];
		struct {
			#define ADPCM_PAD 10
			u1_t adpcm_pad[ADPCM_PAD];
			u1_t buf2[WF_WIDTH];
		};
	} un;
} __attribute__((packed));


// Use odd values so periodic signals like radars running at even-Hz rates don't
// beat against update rate and produce artifacts or blanking.

#define	WF_SPEED_MAX		23
#define	WEB_SERVER_POLL_US	(1000000 / WF_SPEED_MAX / 2)

#define WF_SPEED_OFF        0
#define	WF_SPEED_1FPS		1
#define	WF_SPEED_SLOW		5
#define	WF_SPEED_MED		13
#define	WF_SPEED_FAST		WF_SPEED_MAX

#define WF_NSPEEDS 5
static const int wf_fps[] = { WF_SPEED_OFF, WF_SPEED_1FPS, WF_SPEED_SLOW, WF_SPEED_MED, WF_SPEED_FAST };

enum { WF_SELECT_OFF = 0, WF_SELECT_1FPS = 1, WF_SELECT_SLOW = 2, WF_SELECT_MED = 3, WF_SELECT_FAST = 4 };

#define WF_COMP_OFF     0
#define WF_COMP_ON      1

enum aper_t { MAN=0, AUTO };
enum aper_algo_t { IIR=0, MMA, EMA, OFF };

#define WF_CIC_COMP 10
enum wf_interp_t { WF_MAX=0, WF_MIN, WF_LAST, WF_DROP, WF_CMA };
static const char *interp_s[] = { "max", "min", "last", "drop", "cma" };

enum { WF_WAIT_DDC, WF_RESET, WF_WAIT_BUF_FULL, WF_WAIT_FRAME };

struct wf_inst_t {
	conn_t *conn;
	int rx_chan;
	int ddc_chan, last_ddc;
	int nfft, fft_used, plot_width, plot_width_clamped;
    int nxfer, samps, tsamps;
	int maxdb, mindb, send_dB;
	float fft_scale[WF_WIDTH], fft_scale_div2[WF_WIDTH], fft_offset;
	u2_t fft2wf_map[WF_NFFT_MAX];		// map is 1:1 with fft
	u2_t wf2fft_map[WF_WIDTH];          // map is 1:1 with plot
	u2_t drop_sample[WF_WIDTH];
	int start, prev_start, zoom, prev_zoom;
	float start_f;
	u4_t mark;
	int speed, fft_used_limit;
	bool new_map, new_map2, new_map3, compression, isWF, isFFT;
	bool cic_comp;
	wf_interp_t interp;
	int window_func;
	u4_t zoom_all_seq;
	
	tid_t tid;
	
    // wf_share
	volatile u4_t wake_buf_full;
	u4_t lock_seq;
	bool lock_wait;
	volatile u4_t lock_wakeup;
	u4_t num_wakeups;
	u4_t dirty;
    u1_t ddc_dirty[MAX_WF_DDC];
    u4_t wf_seq;
    int cicf_wait_usec;
    wfd(u1_t wf_state;)
    wfd(u4_t wf_sleep_seq;)
    wfd(u4_t wf_intr_seq;)
	
	int tr_cmds;
	u4_t cmd_recv;
	float cf, HZperStart;
	float off_freq, off_freq_inv;
	u64_t i_offset;
    u4_t decim;
	bool new_scale_mask;
	bool spectral_inversion;
	u4_t aper_pan_timer;
	int scale;
	int wband;

	// NB: matches rx_noise.h which is not included here to prevent re-compile cascade
    #define NOISE_TYPES 4
    #define NOISE_PARAMS 8
	int nb_enable[NOISE_TYPES];
	float nb_param[NOISE_TYPES][NOISE_PARAMS];
	bool nb_param_change[NOISE_TYPES], nb_setup;
	u4_t last_noise_pulse;

	snd_t *snd;
	u4_t snd_seq;
	wf_pkt_t out;
	int out_bytes;
	bool check_overlapped_sampling, overlapped_sampling;
	int samp_wait_ms, chunk_wait_us;
	
	u4_t last_frames_ms, waterfall_frames;
	
	int aper, aper_algo;
	float aper_param;
	int need_autoscale, done_autoscale, sent_autoscale, avg_clear, signal, noise;
    #define APER_PWR_LEN WF_OUTPUT
    float avg_pwr[APER_PWR_LEN];
    u4_t report_sec;
    int last_noise, last_signal;
    
	bool want_rtn_snd, have_rtn_snd;
};

struct ddc_inst_t {
    bool lock;
    u1_t last_user;
    u2_t bufsize;
    int lock_rx;        // which rx has the lock 
    u4_t use_ct;
    u4_t wf_seq;
};

#define WINF_WF_HANNING         0
#define WINF_WF_HAMMING         1
#define WINF_WF_BLACKMAN_HARRIS 2
#define WINF_WF_NONE            3
#define WF_N_WINF               4

#define WF_WIN_4K               0
#define WF_WIN_8K               1
#define WF_N_WINSZ              2

struct wf_shmem_t {
    wf_inst_t wf_inst[MAX_RX_CHANS];    // NB: MAX_RX_CHANS even though there may be fewer MAX_WF_DDC
    fft_t fft_inst[MAX_RX_CHANS];
	fftwf_plan hw_dft_plan_8k;
	fftwf_plan hw_dft_plan_4k;
    float window_function[WF_N_WINF][WF_N_WINSZ][WF_NSAMP_MAX];
    float CIC_comp[WF_NFFT_MAX];
    
    // wf_share
    ddc_inst_t ddc[MAX_WF_DDC];
    u4_t wf_seq_global;
	u4_t lock_seq_global;
	#define CICF_BUSY WF_SHMEM->cicf_busy
	volatile u4_t cicf_busy;
};     

#include "shmem_config.h"

#ifdef MULTI_CORE
    //#define WF_SHMEM_DISABLE_TEST
    #ifdef WF_SHMEM_DISABLE_TEST
        #warning dont forget to remove WF_SHMEM_DISABLE_TEST
        #define WF_SHMEM_DISABLE
    #else
        // shared memory enabled
    #endif
#else
    #define WF_SHMEM_DISABLE
#endif

#include "shmem.h"

#ifdef WF_SHMEM_DISABLE
    extern wf_shmem_t *wf_shmem_p;
    #define WF_SHMEM wf_shmem_p
#else
    #define WF_SHMEM (&shmem->wf_shmem)
#endif


enum wf_cmd_key_e {
    CMD_SET_ZOOM=1, CMD_SET_MAX_MIN_DB, CMD_SET_CMAP, CMD_SET_APER, CMD_SET_BAND,
    CMD_SET_SCALE, CMD_SET_WF_SPEED, CMD_SEND_DB, CMD_EXT_BLUR, CMD_INTERPOLATE, CMD_WF_WINDOW_FUNC
};


#define WF_SETUP_FREQ   0x1
#define WF_SETUP_REM    0x2

void c2s_wf_ddc_setup(int chan_or_ddc, wf_inst_t *wf, u4_t flags);
