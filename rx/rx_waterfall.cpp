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

// Copyright (c) 2014-2025 John Seamons, ZL4VO/KF6VO

#include "types.h"
#include "kiwi.h"
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
#include "rx_noise.h"
#include "noiseproc.h"
#include "dx.h"
#include "non_block.h"
#include "noise_blank.h"
#include "str.h"
#include "mem.h"
#include "rx_waterfall.h"
#include "rx_waterfall_cmd.h"
#include "rx_util.h"
#include "options.h"
#include "test.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <math.h>
#include <fftw3.h>

#ifdef USE_SDR

int wf_rd_offset = WF_RD_OFFSET;
int wf_slowdown;

#ifdef WF_SHMEM_DISABLE
    static wf_shmem_t wf_shmem;
    wf_shmem_t *wf_shmem_p = &wf_shmem;
#endif

// Doesn't work because currently no way to use SPI from LINUX_CHILD_PROCESS()
// So sample_wf() can't be run as an process, but compute_frame() can.
//#define WF_IPC_SAMPLE_WF

#if defined(WF_SHMEM_DISABLE) || !defined(WF_IPC_SAMPLE_WF)
    #define WFSleepReasonMsec(r, t) TaskSleepReasonMsec(r, t)
    #define WFSleepReasonUsec(r, t) TaskSleepReasonUsec(r, t)
    #define WFNextTask(r) NextTask(r)
#else
    // WF_IPC_SAMPLE_WF
    #define WFSleepReasonMsec(r, t) kiwi_msleep(t)
    #define WFSleepReasonUsec(r, t) kiwi_usleep(t)
    #define WFNextTask(r)
#endif

#define	WF_OUT_HDR	((int) (sizeof(wf_pkt_t) - sizeof(out->un)))
#define	WF_OUT_NOM	((int) (WF_OUT_HDR + sizeof(out->un.buf)))
		
void c2s_waterfall_once()
{
	// Do this here, rather than the c2s_waterfall_init or the beginning of c2s_waterfall(),
	// because it takes too long and causes the data pump to overrun.
	// And also uses a huge amount of stack space meaning it must run on main task.
	//
	// NB: Using fft_inst[0] here is just for fftwf_plan_dft_1d() setup purposes.
	// The actual channel-specific buffer values are specified by fftwf_execute_dft() later on.
    fft_t* fft = &WF_SHMEM->fft_inst[0];
    u4_t plan = bg? FFTW_MEASURE : FFTW_ESTIMATE;
	fftwf_set_timelimit(10);
    if (bg) printf("WF: FFTW_MEASURE...\n");
    WF_SHMEM->hw_dft_plan = fftwf_plan_dft_1d(WF_NFFT, fft->hw_c_samps, fft->hw_fft, FFTW_FORWARD, plan);
    if (bg) printf("WF: ...FFTW_MEASURE\n");
}

void c2s_waterfall_init()
{
	int i;
	
    wf_cmd_hash.max_hash_len = 9;
    str_hash_init("wf", &wf_cmd_hash, wf_cmd_hashes);

	const float adc_scale_decim = powf(2, -16);		// gives +/- 0.5 float samples
	//const float adc_scale_decim = powf(2, -15);		// gives +/- 1.0 float samples

    // window functions (adc_scale is folded in here since it's constant)
	
	//#define WINDOW_GAIN		2.0
	#define WINDOW_GAIN		1.0
	
	for (int winf = 0; winf < N_WF_WINF; winf++) {
        float *window = WF_SHMEM->window_function[winf];

        for (i=0; i < WF_NFFT; i++) {
            window[i] = adc_scale_decim * WINDOW_GAIN;
        
            switch (winf) {
        
            case WINF_WF_HANNING:
                window[i] *= (0.5 - 0.5 * cos( (K_2PI*i)/(float)(WF_NFFT-1) ));
                break;
            
            case WINF_WF_HAMMING:
                window[i] *= (0.54 - 0.46 * cos( (K_2PI*i)/(float)(WF_NFFT-1) ));
                break;

            case WINF_WF_BLACKMAN_HARRIS:
                window[i] *= (0.35875
                    - 0.48829 * cos( (K_2PI*i)/(float)(WF_NFFT-1) )
                    + 0.14128 * cos( (2.0*K_2PI*i)/(float)(WF_NFFT-1) )
                    - 0.01168 * cos( (3.0*K_2PI*i)/(float)(WF_NFFT-1) ));
                break;

            case WINF_WF_NONE:
            default:
                break;
            }
        }
    }

    // compensates for small droop at top end of waterfall/spectrum display
    //real_printf("WF CIC_comp:\n");
    for (i=0; i < WF_NFFT; i++) {

        // CIC compensating filter
        const TYPEREAL f = fabs(fmod(TYPEREAL(i)/WF_NFFT+0.5f, 1.0f) - 0.5f);
        const TYPEREAL p1 = -2.969f;
        const TYPEREAL p2 = 36.26f;
        const TYPEREAL sincf = f ? MSIN(f*K_PI)/(f*K_PI) : 1.0f;
        float cic_comp = pow(sincf, -5) + p1*exp(p2*(f-0.5f));
        WF_SHMEM->CIC_comp[i] = 0.5 + cic_comp / 2.0;   // scaling value is empirical
        //real_printf("[%.3f %.3f] ", cic_comp, WF_SHMEM->CIC_comp[i]);
    }
    //real_printf("\n\n");

	WF_SHMEM->n_chunks = nwf_nxfer;
	
	assert(WF_NFFT <= WF_NBUF);     // hardware sample buffer length limitation
	
#ifdef WF_SHMEM_DISABLE
#else
    #ifdef WF_IPC_SAMPLE_WF
        void sample_wf(int rx_chan);
        shmem_ipc_setup("kiwi.waterfall", SIG_IPC_WF, sample_wf);
    #else
        void compute_frame(int rx_chan);
        shmem_ipc_setup("kiwi.waterfall", SIG_IPC_WF, compute_frame);
    #endif
#endif
}

void c2s_waterfall_compression(int rx_chan, bool compression)
{
    //printf("WF%d: compression=%d\n", rx_chan, compression);
	WF_SHMEM->wf_inst[rx_chan].compression = compression;
}

void c2s_waterfall_no_sync(int rx_chan, bool no_sync)
{
    //printf("WF%d: no_sync=%d\n", rx_chan, no_sync);
	WF_SHMEM->wf_inst[rx_chan].no_sync = no_sync;
}


CNoiseProc m_NoiseProc_wf[MAX_RX_CHANS];


void c2s_waterfall_setup(void *param)
{
	conn_t *conn = (conn_t *) param;
	int rx_chan = conn->rx_channel;

	send_msg(conn, SM_WF_DEBUG, "MSG center_freq=%d bandwidth=%d adc_clk_nom=%.0f", (int) ui_srate_Hz/2, (int) ui_srate_Hz, ADC_CLOCK_NOM);
	send_msg(conn, SM_WF_DEBUG, "MSG kiwi_up=1 rx_chan=%d", rx_chan);       // rx_chan needed by extint_send_extlist() on js side
	extint_send_extlist(conn);

    // If not wanting a wf, because specifically requested none via !conn->isWF_conn or configured none via cfg_no_wf,
    // send wf_chans=0 to force audio FFT to be used.
    // But need to send actual value via wf_chans_real for use elsewhere.
    bool no_wf = (!conn->isWF_conn || cfg_no_wf);
	send_msg(conn, SM_WF_DEBUG, "MSG wf_fft_size=1024 wf_fps=%d wf_fps_max=%d zoom_max=%d rx_chans=%d wf_chans=%d wf_chans_real=%d wf_cal=%d wf_setup",
		WF_SPEED_FAST, WF_SPEED_MAX, MAX_ZOOM, rx_chans, no_wf? 0:wf_chans, wf_chans, waterfall_cal);
	if (do_gps && !do_sdr) send_msg(conn, SM_WF_DEBUG, "MSG gps");

    dx_last_community_download();
}

void c2s_waterfall(void *param)
{
	conn_t *conn = (conn_t *) param;
	conn->wf_cmd_recv_ok = false;
	rx_common_init(conn);
	int rx_chan = conn->rx_channel;
	rx_chan_t *rxc = &rx_channels[rx_chan];
	int i, j, k, n;
	//float adc_scale_samps = powf(2, -ADC_BITS);

	int _dvar, _pipe;
	double adc_clock_corrected = 0;
	u4_t dx_update_seq = 0;
	int wf_cal = waterfall_cal;
	
	wf_inst_t *wf = &WF_SHMEM->wf_inst[rx_chan];
	memset(wf, 0, sizeof(wf_inst_t));
	wf->tid = TaskID();
	wf->conn = conn;
	wf->rx_chan = rx_chan;
	wf->tr_cmds = 0;
	wf->cmd_recv = 0;
	wf->zoom = -1;
	wf->start_f = -1;
	wf->HZperStart = ui_srate_Hz / (WF_WIDTH << MAX_ZOOM);
	wf->new_map = false;
	wf->new_scale_mask = false;
	wf->spectral_inversion = kiwi.spectral_inversion;
	wf->aper_pan_timer = 0;
	wf->scale = 1;
	wf->wband = -1;
	wf->compression = true;
	wf->isWF = (rx_chan < wf_chans && conn->isWF_conn);
	wf->isFFT = !wf->isWF;
    wf->mark = timer_ms();
    wf->prev_start = wf->prev_zoom = -1;
    wf->snd = &snd_inst[rx_chan];

    wf->check_overlapped_sampling = true;
    strncpy(wf->out.id4, "W/F ", 4);

	#define WF_IQ_T 4
	assert(sizeof(iq_t) == WF_IQ_T);
	assert((nwf_samps * WF_IQ_T) <= SPIBUF_B);

	//clprintf(conn, "WF INIT conn: %p mc: %p %s:%d %s\n",
	//	conn, conn->mc, conn->remote_ip, conn->remote_port, conn->mc->uri);

    #if 0
        if (strcmp(conn->remote_ip, "") == 0)
            cprintf(conn, "WF INIT conn: %p mc: %p %s:%d %s\n",
                conn, conn->mc, conn->remote_ip, conn->remote_port, conn->mc->uri);
    #endif

	//evWFC(EC_DUMP, EV_WF, 10000, "WF", "DUMP 10 SEC");
	
	nbuf_t *nb = NULL;

	while (TRUE) {

        // admin spectral inversion setting changed
        if (wf->spectral_inversion != kiwi.spectral_inversion) {
            wf->new_map = wf->new_map2 = wf->new_map3 = TRUE;
            wf->spectral_inversion = kiwi.spectral_inversion;
        }
		
		// reload freq NCO if adc clock has been corrected
		// reload freq NCO if spectral inversion changed
		if (wf->start_f >= 0 && wf->zoom != -1 && (adc_clock_corrected != conn->adc_clock_corrected || wf->new_map)) {
			adc_clock_corrected = conn->adc_clock_corrected;
			wf->off_freq = wf->start_f * wf->HZperStart;
            wf->off_freq_inv = ((float) MAX_START(wf->zoom) - wf->start_f) * wf->HZperStart;
			wf->i_offset = (u64_t) (s64_t) ((wf->spectral_inversion? wf->off_freq_inv : wf->off_freq) / conn->adc_clock_corrected * pow(2,48));
			wf->i_offset = -wf->i_offset;
			if (wf->isWF)
			    spi_set3(CmdSetWFFreq, rx_chan, (wf->i_offset >> 16) & 0xffffffff, wf->i_offset & 0xffff);
			//printf("WF%d freq updated due to ADC clock correction\n", rx_chan);
		}

		if (nb) web_to_app_done(conn, nb);
		n = web_to_app(conn, &nb);
		if (n) {
		    rx_waterfall_cmd(conn, n, nb->buf);
		    continue;
		}
        check(nb == NULL);
		
		if (do_gps && !do_sdr) {
			wf_pkt_t *out = &wf->out;
			int *ns_bin = ClockBins();
			int max=0;
			
			for (n=0; n<1024; n++) if (ns_bin[n] > max) max = ns_bin[n];
			if (max == 0) max=1;
			u1_t *bp = out->un.buf;
			for (n=0; n<1024; n++) {
				*bp++ = (u1_t) (int) (-256 + (ns_bin[n] * 255 / max));	// simulate negative dBm
			}
			int delay = 10000 - (timer_ms() - wf->mark);
			if (delay > 0) TaskSleepReasonMsec("wait frame", delay);
			wf->mark = timer_ms();
			app_to_web(conn, (char*) &out, WF_OUT_NOM);
		}
		
		if (!do_sdr) {
			NextTask("WF skip");
			continue;
		}

		if (conn->stop_data) {
			//clprintf(conn, "WF stop_data rx_server_remove()\n");
			rx_enable(rx_chan, RX_CHAN_FREE);
			rx_server_remove(conn);
			panic("shouldn't return");
		}

		// no keep-alive seen for a while or the bug where the initial cmds are not received and the connection hangs open
		// and locks-up a receiver channel
		conn->keep_alive = timer_sec() - conn->keepalive_time;
		bool keepalive_expired = (conn->keep_alive > KEEPALIVE_SEC);
		bool connection_hang = (conn->keepalive_count > 4 && wf->cmd_recv != CMD_WF_ALL);
		if (keepalive_expired || connection_hang || conn->kick) {
			//if (keepalive_expired) clprintf(conn, "WF KEEP-ALIVE EXPIRED\n");
			//if (connection_hang) clprintf(conn, "WF CONNECTION HANG\n");
			//if (conn->kick) clprintf(conn, "WF KICK\n");
		
			// Ask sound task to stop (must not do while, for example, holding a lock).
			// We've seen cases where the sound connects, then times out. But the wf has never connected.
			// So have to check for conn->other being valid.
			conn_t *csnd = conn_other(conn, STREAM_SOUND);
			if (csnd) {
				csnd->stop_data = TRUE;
			} else {
				rx_enable(rx_chan, RX_CHAN_FREE);		// there is no SND, so free rx_chan[] now
			}
			
			//clprintf(conn, "WF rx_server_remove()\n");
			rx_server_remove(conn);
			panic("shouldn't return");
		}

        // Handle LOG_ARRIVED and missing ident for WF-only connections.
        bool too_much = ((wf->cmd_recv & CMD_SET_ZOOM) && (timer_sec() > (conn->arrival + 15)));
        if (conn->isMaster && !conn->arrived && (conn->ident || too_much)) {
            if (!conn->ident)
			    kiwi_str_redup(&conn->ident_user, "user", (char *) "(no identity)");
            rx_loguser(conn, LOG_ARRIVED);
            conn->arrived = TRUE;
        }

		// Don't process any waterfall data until we've received all necessary commands.
		// Also, stop waterfall if speed is zero.
		if (wf->cmd_recv != CMD_WF_ALL || wf->speed == WF_SPEED_OFF) {
		    conn->wf_cmd_recv = wf->cmd_recv;
			TaskSleepMsec(100);
			continue;
		}
		
		if (!conn->wf_cmd_recv_ok) {
			#ifdef TR_WF_CMDS
				clprintf(conn, "WF cmd_recv ALL 0x%x/0x%x\n", wf->cmd_recv, CMD_WF_ALL);
			#endif
			conn->wf_cmd_recv_ok = true;
		}
		
        if (wf->isFFT) {
            TaskSleepMsec(250);
            continue;
        }
        
        // coalesse the repeated start_chg resulting from a WF pan/scroll
        if (wf->aper_pan_timer && wf->mark > (wf->aper_pan_timer + 1000)) {
            wf->avg_clear = 1;
            wf->need_autoscale++;
            wf->aper_pan_timer = 0;
        }
        
		wf->fft_used = WF_NFFT / WF_USING_HALF_FFT;		// the result is contained in the first half of a complex FFT
		
		// if any CIC is used (z != 0) only look at half of it to avoid the aliased images
		if (wf->zoom != 0) wf->fft_used /= WF_USING_HALF_CIC;
		
		float span = conn->adc_clock_corrected / 2 / (1 << wf->zoom);
		float disp_fs = ui_srate_Hz / (1 << wf->zoom);
		
		// NB: plot_width can be greater than WF_WIDTH because it relative to the ratio of the
		// (adc_clock_corrected/2) / ui_srate_Hz, which can be > 1 (hence plot_width_clamped).
		// All this is necessary because we might be displaying less than what adc_clock_corrected/2 implies because
		// of using third-party obtained frequency scale images in our UI (this is not currently an issue).
		wf->plot_width = WF_WIDTH * span / disp_fs;
		wf->plot_width_clamped = (wf->plot_width > WF_WIDTH)? WF_WIDTH : wf->plot_width;
		
		if (wf->new_map) {
			assert(wf->fft_used <= MAX_FFT_USED);

			wf->fft_used_limit = 0;

			if (wf->fft_used >= wf->plot_width) {
				// FFT >= plot

                // no unwrap
                for (i=0; i < wf->fft_used; i++) {
                    j = wf->plot_width * i/wf->fft_used;
                    if (wf->spectral_inversion)
                        j = (j < WF_WIDTH)? (WF_WIDTH-1 - j) : -1;
                    wf->fft2wf_map[i] = j;
                }

                // Not like above where fft2wf_map[] can map multiple FFT values per pixel for use
                // in e.g. averaging. Only a single value is needed based on plot width due to
                // the fact this is drop sampling.
                int fft_used_inv = roundf((float) wf->fft_used * (wf->plot_width_clamped-1)/wf->plot_width);
                for (i=0; i < wf->plot_width_clamped; i++) {
                    j = roundf((float) wf->fft_used * i/wf->plot_width);
                    if (wf->spectral_inversion) j = fft_used_inv - j;
                    wf->drop_sample[i] = j;
                }
			} else {
				// FFT < plot

                // no unwrap
                for (i=0; i<wf->plot_width_clamped; i++) {
                    j = wf->spectral_inversion? (wf->plot_width-1 - i) : i;
                    wf->wf2fft_map[i] = wf->fft_used * j/wf->plot_width;
                }
			}
			
            wf_printf("WF NEW_MAP z%d i%d cic%d fft_used %d/%d span %.1f disp_fs %.1f plot_width %d/%d FFT %s plot\n",
                wf->zoom, wf->interp, wf->cic_comp, wf->fft_used, WF_NFFT, span/kHz, disp_fs/kHz, wf->plot_width_clamped, wf->plot_width,
                (wf->plot_width_clamped < wf->fft_used)? ">=":"<");
			
			wf->new_map = FALSE;
		}
		
        // admin requested that all clients get updated cfg (e.g. admin changed dx type menu)
        if (rxc->cfg_update_seq != cfg_cfg.update_seq) {
            rxc->cfg_update_seq = cfg_cfg.update_seq;
            rx_server_send_config(conn);
        }

        // get client to request updated dx list because admin edited masked list
        // or made any other change to dx label list
		if (dx_update_seq != dx.update_seq) {
            send_msg(conn, false, "MSG request_dx_update");
		    dx_update_seq = dx.update_seq;
		    wf->new_scale_mask = true;
		}
		
		if (shmem->zoom_all_seq != wf->zoom_all_seq) {
		    //cprintf(conn, "WF zoom_all=%d\n", shmem->zoom_all);
            send_msg(conn, false, "MSG zoom_all=%d", shmem->zoom_all);
		    wf->zoom_all_seq = shmem->zoom_all_seq;
		}
		
		// forward admin changes of waterfall cal to client side
		if (waterfall_cal != wf_cal) {
            send_msg(conn, false, "MSG wf_cal=%d", waterfall_cal);
		    wf_cal = waterfall_cal;
		}
		
		if (wf->new_scale_mask) {
			// FIXME: Is this right? Why is this so strange?
			float fft_scale;
			float maxmag = wf->zoom? wf->fft_used : wf->fft_used/2;
			//jks
			//fft_scale = 20.0 / (maxmag * maxmag);
			//wf->fft_offset = 0;
			
			// makes GEN attn 0 dB = 0 dBm
			fft_scale = 5.0 / (maxmag * maxmag);
			wf->fft_offset = wf->zoom? -0.08 : -0.8;
			
			// fixes GEN z0/z1+ levels, but breaks regular z0/z1+ noise floor continuity -- why?
			//float maxmag = wf->zoom? wf->fft_used : wf->fft_used/4;
			//fft_scale = (wf->zoom? 2.0 : 5.0) / (maxmag * maxmag);
			
			// apply masked frequencies
			if (dx.masked_len != 0 && !(conn->other != NULL && conn->other->tlimit_exempt_by_pwd)) {
                for (i=0; i < wf->plot_width_clamped; i++) {
                    float scale = fft_scale;
                    int f = roundf((wf->start + (i << (MAX_ZOOM - wf->zoom))) * wf->HZperStart);
                    for (j=0; j < dx.masked_len; j++) {
                        dx_mask_t *dmp = &dx.masked_list[j];
                        //cprintf(conn, "MASKED %.2f|%.2f|%.2f %s\n", dmp->masked_lo/1e3, f/1e3, dmp->masked_hi/1e3, (f >= dmp->masked_lo && f <= dmp->masked_hi)? "Y":"N");
                        if (f >= dmp->masked_lo && f <= dmp->masked_hi) {
                            scale = 0;
                            break;
                        }
                    }
                    wf->fft_scale[i] = scale;
                    wf->fft_scale_div2[i] = scale / 2;
                }
			} else {
                for (i=0; i < wf->plot_width_clamped; i++) {
                    wf->fft_scale[i] = fft_scale;
                    wf->fft_scale_div2[i] = fft_scale / 2;
                }
			}

		    wf->new_scale_mask = false;
		}

        void sample_wf(int rx_chan);
        #ifdef WF_SHMEM_DISABLE
            sample_wf(rx_chan);
        #else
            #ifdef WF_IPC_SAMPLE_WF
                shmem_ipc_invoke(SIG_IPC_WF, wf->rx_chan);      // invoke sample_wf()
            #else
                sample_wf(rx_chan);
            #endif
        #endif
	}
}

void sample_wf(int rx_chan)
{
	wf_inst_t *wf = &WF_SHMEM->wf_inst[rx_chan];
    int i, k;
    u4_t now, now2, diff;
    u64_t now64, deadline;
    
    // create waterfall
    
    #define DESIRED_SCALE 2         // makes >= z11 overlapped
    //#define DESIRED_SCALE 1.25    // makes >= z10 overlapped, but z10 has too much glitching
    assert(wf_fps[wf->speed] != 0);
    int desired = 1000 / wf_fps[wf->speed];
    int desired_scaled = desired * DESIRED_SCALE;

    // desired frame rate greater than what full sampling can deliver, so start overlapped sampling
    if (wf->check_overlapped_sampling) {
        wf->check_overlapped_sampling = false;

        if (wf->samp_wait_ms >= desired_scaled) {
            wf->overlapped_sampling = true;
            
            wf_printf("---- WF%d OLAP z%d samp_wait %d >= %d(%d) desired\n",
                rx_chan, wf->zoom, wf->samp_wait_ms, desired_scaled , desired);
            
            evWFC(EC_TRIG1, EV_WF, -1, "WF", "OVERLAPPED CmdWFReset");
            spi_set(CmdWFReset, rx_chan, WF_SAMP_RD_RST | WF_SAMP_WR_RST | WF_SAMP_CONTIN);
            WFSleepReasonMsec("fill pipe", wf->samp_wait_ms+1);		// fill pipeline
        } else {
            wf->overlapped_sampling = false;
            wf_printf("---- WF%d NON-OLAP z%d samp_wait %d < %d(%d) desired\n",
                rx_chan, wf->zoom, wf->samp_wait_ms, desired_scaled, desired);
        }
    }
    
    SPI_CMD first_cmd;
    
    if (wf->overlapped_sampling) {

        //
        // Start reading immediately at synchronized write address plus a small offset to get to
        // the old part of the buffer.
        // This presumes zoom factor isn't so large that buffer fills too slowly and we
        // overrun reading the last little bit (offset part). Also presumes that we can read the
        // first part quickly enough that the write doesn't catch up to us.
        //
        // CmdGetWFContSamps asserts WF_SAMP_SYNC | WF_SAMP_CONTIN in kiwi.sdr.asm code
        //
        first_cmd = CmdGetWFContSamps;
    } else {
        evWFC(EC_TRIG1, EV_WF, -1, "WF", "NON-OVERLAPPED CmdWFReset");
        spi_set(CmdWFReset, rx_chan, WF_SAMP_RD_RST | WF_SAMP_WR_RST);
        first_cmd = CmdGetWFSamples;
    }

    SPI_MISO *miso;
    s4_t ii, qq;
    iq_t *iqp;

    int chunk, sn;
    int n_chunks = WF_SHMEM->n_chunks;
    float *window = WF_SHMEM->window_function[wf->window_func];
    fft_t *fft = &WF_SHMEM->fft_inst[rx_chan];

    for (chunk=0, sn=0; sn < WF_NFFT; chunk++) {
        miso = &SPI_SHMEM->wf_miso[rx_chan];
        assert(chunk < n_chunks);

        if (wf->overlapped_sampling) {
            evWF(EC_TRIG1, EV_WF, -1, "WF", "CmdGetWFContSamps");
        } else {
            // wait until current chunk is available in WF sample buffer
            now64 = timer_us64();
            deadline = now64 + (wf->chunk_wait_us * (chunk? 1:2));
            
            while (now64 < deadline) {
                diff = deadline - now64;
                //real_printf("%d(%d) ", diff, wf->chunk_wait_us); fflush(stdout);
                if (diff) {
                    evWF(EC_EVENT, EV_WF, -1, "WF", "TaskSleep wait chunk buffer");
                    WFSleepReasonUsec("wait chunk", diff);
                    evWF(EC_EVENT, EV_WF, -1, "WF", "TaskSleep wait chunk buffer done");
                    now64 = timer_us64();
                }
            }
        }
    
        if (chunk == 0) {
            //#define WF_MEAS_OLAP
            #ifdef WF_MEAS_OLAP
                if (wf->overlapped_sampling && rx_chan == 0 && wf->zoom >= 10) now2 = timer_us();
            #endif

            spi_get_noduplex(first_cmd, miso, nwf_samps * sizeof(iq_t), rx_chan);
        } else
        if (chunk < n_chunks-1) {
            spi_get_noduplex(CmdGetWFSamples, miso, nwf_samps * sizeof(iq_t), rx_chan);
        }

        evWFC(EC_EVENT, EV_WF, -1, "WF", evprintf("%s SAMPLING chunk %d",
            wf->overlapped_sampling? "OVERLAPPED":"NON-OVERLAPPED", chunk));
        
        iqp = (iq_t*) &(miso->word[0]);
        
        for (k=0; k < nwf_samps; k++) {
            if (sn >= WF_NFFT) break;
            ii = (s4_t) (s2_t) iqp->i;
            qq = (s4_t) (s2_t) iqp->q;
            iqp++;

            float fi = ((float) ii) * window[sn];
            float fq = ((float) qq) * window[sn];
            
            fft->hw_c_samps[sn][I] = fi;
            fft->hw_c_samps[sn][Q] = fq;
            sn++;
        }

        #if 1
            if (wf->overlapped_sampling && chunk == n_chunks/2 && wf->zoom >= 10 && wf_slowdown) {
                WFSleepReasonMsec("slow down", wf_slowdown);
                //if (rx_chan == 0) { real_printf("%d", chunk); fflush(stdout); }
            }
        #endif
    }
    
    #ifdef WF_MEAS_OLAP
        if (wf->overlapped_sampling && rx_chan == 0 && wf->zoom >= 10) {
            real_printf("%.3f ", (float) (timer_us() - now2) / 1e3f); fflush(stdout);
        }
    #endif

    #ifndef EV_MEAS_WF
        static int wf_cnt;
        evWFC(EC_EVENT, EV_WF, -1, "WF", evprintf("WF %d: loop done", wf_cnt));
        wf_cnt++;
    #endif

    if (wf->nb_enable[NB_CLICK]) {
        now = timer_sec();
        if (now != wf->last_noise_pulse) {
            wf->last_noise_pulse = now;
            TYPEREAL pulse = wf->nb_param[NB_CLICK][NB_PULSE_GAIN] * 0.49;
            for (int i=0; i < wf->nb_param[NB_CLICK][NB_PULSE_SAMPLES]; i++) {
                fft->hw_c_samps[i][I] = pulse;
                fft->hw_c_samps[i][Q] = 0;
            }
        }
    }

    if (wf->nb_enable[NB_BLANKER] && wf->nb_enable[NB_WF]) {
        if (wf->nb_param_change[NB_BLANKER]) {
            //u4_t srate = round(conn->adc_clock_corrected) / (1 << (wf->zoom+1));
            u4_t srate = WF_NFFT;
            //printf("NB WF sr=%d usec=%.0f th=%.0f\n", srate, wf->nb_param[NB_BLANKER][0], wf->nb_param[NB_BLANKER][1]);
            m_NoiseProc_wf[rx_chan].SetupBlanker("WF", srate, wf->nb_param[NB_BLANKER]);
            wf->nb_param_change[NB_BLANKER] = false;
            wf->nb_setup = true;
        }

        if (wf->nb_setup)
            m_NoiseProc_wf[rx_chan].ProcessBlankerOneShot(WF_NFFT, (TYPECPX*) fft->hw_c_samps, (TYPECPX*) fft->hw_c_samps);
    }

    void compute_frame(int rx_chan);
    #ifdef WF_SHMEM_DISABLE
        compute_frame(rx_chan);
    #else
        #ifdef WF_IPC_SAMPLE_WF
            compute_frame(rx_chan);
        #else
            shmem_ipc_invoke(SIG_IPC_WF, rx_chan);      // invoke compute_frame()
        #endif
    #endif

    #ifndef WF_IPC_SAMPLE_WF
        if (wf->aper == AUTO && wf->done_autoscale > wf->sent_autoscale) {
            wf->sent_autoscale++;

            if (wf->last_noise != wf->noise || wf->last_signal != wf->signal) {
                send_msg(wf->conn, false, "MSG maxdb=%d", wf->signal);
                send_msg(wf->conn, false, "MSG mindb=%d", wf->noise);
                wf->last_noise = wf->noise;
                wf->last_signal = wf->signal;
            }

            if (wf->aper_algo != OFF) {
                wf->need_autoscale++;   // go again
            }
        }
    #endif
    
    wf_pkt_t *out = &wf->out;
    app_to_web(wf->conn, (char*) out, WF_OUT_HDR + wf->out_bytes);
    waterfall_bytes[rx_chan] += wf->out_bytes;
    waterfall_bytes[rx_chans] += wf->out_bytes; // [rx_chans] is the sum of all waterfalls
    waterfall_frames[rx_chan]++;
    waterfall_frames[rx_chans]++;       // [rx_chans] is the sum of all waterfalls
    wf->waterfall_frames++;
    evWF(EC_EVENT, EV_WF, -1, "WF", "compute_frame: done");

    now = timer_ms();
    int actual = now - wf->mark;
    
    // full sampling faster than needed by frame rate
    if (wf_full_rate) {
        WFNextTask("loop");
    } else {
        int delay = desired - actual;
        //printf("%d %d %d\n", delay, actual, desired);
        if (desired > actual) {
            evWF(EC_EVENT, EV_WF, -1, "WF", "TaskSleep wait FPS");
            WFSleepReasonMsec("wait frame", delay);
            evWF(EC_EVENT, EV_WF, -1, "WF", "TaskSleep wait FPS done");
        } else {
            WFNextTask("loop");
        }
    }
    
    now = timer_ms();
    wf->mark = now;
    diff = now - wf->last_frames_ms;
    if (diff > 10000) {
        float secs = (float) diff / 1e3;
        int fps = (int) roundf((float) wf->waterfall_frames / secs);
        TaskStat2(TSTAT_SET, fps, "fps");
        wf->waterfall_frames = 0;
        wf->last_frames_ms = now;
    }
}

void compute_frame(int rx_chan)
{
	wf_inst_t *wf = &WF_SHMEM->wf_inst[rx_chan];
	int i;
	wf_pkt_t *out = &wf->out;
	float pwr[MAX_FFT_USED];
    fft_t *fft = &WF_SHMEM->fft_inst[rx_chan];
    
    // don't use compression for zoom level zero because of bad interaction
    // of narrow strong carriers with compression algorithm
    bool use_compression = (wf->compression && wf->zoom != 0);
		
	evWF(EC_EVENT, EV_WF, -1, "WF", "compute_frame: FFT start");
    #ifdef OPTION_WF_FFT_MEAS
        static u4_t loopct;
        bool meas = ((loopct++ & 0xf) == 0);
        u4_t us;
        if (meas) us = timer_us();
        fftwf_execute_dft(WF_SHMEM->hw_dft_plan, fft->hw_c_samps, fft->hw_fft);
        if (meas) { real_printf("WF%.3f ", (float)(timer_us() - us)/1e3); fflush(stdout); }
    #else
        fftwf_execute_dft(WF_SHMEM->hw_dft_plan, fft->hw_c_samps, fft->hw_fft);
    #endif
	evWF(EC_EVENT, EV_WF, -1, "WF", "compute_frame: FFT done");

	u1_t *buf_p = use_compression? out->un.buf2 : out->un.buf;
	u1_t *bp = buf_p;
			
	if (!wf->fft_used_limit) wf->fft_used_limit = wf->fft_used;

	// zero-out the DC component in lowest bins (around -90 dBFS)
	// otherwise when scrolling wf it will move but then not continue at the new location
	// Blackman-Harris window DC component is a little wider for z=0
	int bin_dc_offset = (wf->zoom == 0 && wf->window_func == WINF_WF_BLACKMAN_HARRIS)? 4:2;    
	for (i = 0; i < bin_dc_offset; i++) pwr[i] = 0;
	
    if (wf->zoom <= 1) {    // don't apply compensation when CIC not in use
        for (i = bin_dc_offset; i < wf->fft_used_limit; i++) {
            float re = fft->hw_fft[i][I], im = fft->hw_fft[i][Q];
            pwr[i] = re*re + im*im;
        }
    } else {
        bool no_cic_comp = (wf->overlapped_sampling || !wf->cic_comp);
        for (i = bin_dc_offset; i < wf->fft_used_limit; i++) {
            float re, im;
            if (no_cic_comp) {
                re = fft->hw_fft[i][I]; im = fft->hw_fft[i][Q];
            } else {
                float comp = WF_SHMEM->CIC_comp[i];
                re = ((float) fft->hw_fft[i][I]) * comp; im = ((float) fft->hw_fft[i][Q]) * comp;
            }
            pwr[i] = re*re + im*im;
        }
    }
		
	// fixme proper power-law scaling..
	
	// from the tutorials at http://www.fourier-series.com/fourierseries2/flash_programs/DFT_windows/index.html
	// recall:
	// pwr = mag*mag			
	// pwr = i*i + q*q
	// mag = sqrt(i*i + q*q) = sqrt(pwr)
	// pwr = mag*mag = i*i + q*q
	// pwr(dB) = 10 * log10(pwr)		i.e. no sqrt() needed
	// mag[amp](dB) = 20 * log10(mag[ampl])
	// pwr gain = pow10(db/10)
	// mag[ampl] gain = pow10(db/20)
	// *** mag_db == pwr_db ***
	
	// with 'bc -l', l() = log base e
	// log10(n) = l(n)/l(10)
	// 10^x = e^(x*ln(10)) = e(x*l(10))
	
    #ifdef WF_INFO
	    float max_dB = wf->maxdb;
	    float min_dB = wf->mindb;
	    float range_dB = max_dB - min_dB;
	    float pix_per_dB = 255.0 / range_dB;
	#endif

	int bin=0, _bin=-1;
	float p, dB;

    float pwr_out[WF_WIDTH];
    memset(pwr_out, 0, sizeof(pwr_out));

    u1_t cma_avgs[WF_WIDTH];
    if (wf->interp == WF_CMA) memset(cma_avgs, 0, sizeof(cma_avgs));
	
	if (wf->fft_used >= wf->plot_width) {
		// FFT >= plot
		
		if (wf->interp == WF_DROP) {
            for (i=0; i < wf->plot_width_clamped; i++) {
                pwr_out[i] = pwr[wf->drop_sample[i]];
            }
		} else {
            
            for (i=0; i < wf->fft_used_limit; i++) {
                p = pwr[i];
                bin = wf->fft2wf_map[i];
                if (bin >= WF_WIDTH || bin < 0) {
                    if (wf->new_map2) {
                        wf_printf(">= FFT: Z%d WF_NFFT %d i %d fft_used %d plot_width %d pix_per_dB %.3f range %.0f:%.0f\n",
                            wf->zoom, WF_NFFT, i, wf->fft_used, wf->plot_width, pix_per_dB, max_dB, min_dB);
                        wf->new_map2 = FALSE;
                    }
                    
                    wf->fft_used_limit = i;		// we now know what the limit is
                    break;
                }
            
                if (bin == _bin) {

                    // Using the max or min value when multiple FFT values per bin gives low-level artifacts!
                    switch (wf->interp) {
                        case WF_CMA:    pwr_out[bin] += p; cma_avgs[bin]++; break;
                        case WF_MAX:    if (p > pwr_out[bin]) pwr_out[bin] = p; break; // gives dark lines
                        case WF_MIN:    if (p < pwr_out[bin]) pwr_out[bin] = p; break; // gives bright lines
                        case WF_LAST:   pwr_out[bin] = p; break;  // using last value (random min/max) seems okay
                        default:        break;
                    }
                } else {
                    if (wf->interp == WF_CMA) {
                        pwr_out[bin] = p;
                        cma_avgs[bin] = 1;
                    } else {
                        pwr_out[bin] = p;
                    }
                    _bin = bin;
                }
            }
        }

		for (i=0; i<WF_WIDTH; i++) {
            float scale;
            if (wf->interp == WF_CMA) {
			    // A fft2wf_map[] value causing two FFT values to average into one bin is common.
			    // So we fold avgs == 2 into wf->fft_scale_div2 computed earlier and save the divide.
			    int avgs = cma_avgs[i];
			    scale = (avgs == 1)? wf->fft_scale[i] : ((avgs == 2)? wf->fft_scale_div2[i] : (wf->fft_scale[i] / avgs));
                p = pwr_out[i];
			} else {
			    scale = wf->fft_scale[i];
                p = pwr_out[i];
			}

			dB = dB_fast(p * scale) + wf->fft_offset;

			// We map 0..-200 dBm to (u1_t) 255..55
			// If we map it the reverse way, (u1_t) 0..255 => 0..-255 dBm (which is more natural), then the
			// noise in the bottom bits due to the ADPCM compression will effect the high-order dBm bits
			// which is bad.
			if (dB > 0) dB = 0;
			if (dB < -200.0) dB = -200.0;
			dB--;
			*bp++ = (u1_t) (int) dB;
		}
	} else {
		// FFT < plot
		if (wf->new_map2) {
			//printf("< FFT: Z%d WF_NFFT %d fft_used %d plot_width_clamped %d pix_per_dB %.3f range %.0f:%.0f\n",
			//	wf->zoom, WF_NFFT, wf->fft_used, wf->plot_width_clamped, pix_per_dB, max_dB, min_dB);
			wf->new_map2 = FALSE;
		}

		for (i=0; i<wf->plot_width_clamped; i++) {
			p = wf->wf2fft_map[i];
			
			dB = dB_fast(p * wf->fft_scale[i]) + wf->fft_offset;
			if (dB > 0) dB = 0;
			if (dB < -200.0) dB = -200.0;
			dB--;
			*bp++ = (u1_t) (int) dB;
		}
	}

    out->x_bin_server = wf->start;
    out->flags_x_zoom_server = wf->zoom;
	evWF(EC_EVENT, EV_WF, -1, "WF", "compute_frame: fill out buf");

    if (wf->aper == AUTO)
        rx_waterfall_aperture_auto(wf, buf_p);

	ima_adpcm_state_t adpcm_wf;
	
	if (use_compression) {
		memset(out->un.adpcm_pad, out->un.buf2[0], sizeof(out->un.adpcm_pad));
		memset(&adpcm_wf, 0, sizeof(ima_adpcm_state_t));
		encode_ima_adpcm_u8_e8(out->un.buf, out->un.buf, ADPCM_PAD + WF_WIDTH, &adpcm_wf);
		wf->out_bytes = (ADPCM_PAD + WF_WIDTH) * sizeof(u1_t) / 2;
		out->flags_x_zoom_server |= WF_FLAGS_COMPRESSION;
	} else {
		wf->out_bytes = WF_WIDTH * sizeof(u1_t);
	}

	// sync this waterfall line to audio packet currently going out
	out->seq = wf->snd_seq;
}

void c2s_waterfall_shutdown(void *param)
{
    conn_t *c = (conn_t*)(param);
    if (c && c->mc)
        rx_server_websocket(WS_MODE_CLOSE, c->mc);
}

#endif
