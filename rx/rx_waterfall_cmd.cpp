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
#include "options.h"
#include "config.h"
#include "kiwi.h"
#include "mode.h"
#include "printf.h"
#include "rx.h"
#include "rx_util.h"
#include "clk.h"
#include "mem.h"
#include "misc.h"
#include "str.h"
#include "timer.h"
#include "nbuf.h"
#include "web.h"
#include "spi.h"
#include "gps.h"
#include "coroutines.h"
#include "cuteSDR.h"
#include "rx_noise.h"
#include "teensy.h"
#include "debug.h"
#include "data_pump.h"
#include "cfg.h"
#include "mongoose.h"
#include "ima_adpcm.h"
#include "ext_int.h"
#include "rx.h"
#include "lms.h"
#include "dx.h"
#include "noise_blank.h"
#include "rx_sound.h"
#include "rx_sound_cmd.h"
#include "rx_waterfall.h"
#include "rx_waterfall_cmd.h"
#include "rx_filter.h"
#include "wdsp.h"
#include "fpga.h"
#include "rf_attn.h"

#ifdef DRM
 #include "DRM.h"
#endif

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/errno.h>
#include <time.h>
#include <sched.h>
#include <math.h>
#include <limits.h>

#include <algorithm>

void rx_waterfall_cmd(conn_t *conn, int n, char *cmd)
{
    int i, j, k;
	int rx_chan = conn->rx_channel;

	wf_inst_t *wf = &WF_SHMEM->wf_inst[rx_chan];
    int n_chunks = WF_SHMEM->n_chunks;

    cmd[n] = 0;		// okay to do this -- see nbuf.c:nbuf_allocq()

    TaskStat(TSTAT_INCR|TSTAT_ZERO, 0, "cmd");

    #if 0
        if (strcmp(conn->remote_ip, "") == 0 /* && strcmp(cmd, "SET keepalive") != 0 */)
            cprintf(conn, "WF <%s> cmd_recv 0x%x/0x%x\n", cmd, wf->cmd_recv, CMD_WF_ALL);
    #endif

    // SECURITY: this must be first for auth check
    bool keep_alive;
    if (rx_common_cmd(STREAM_WATERFALL, conn, cmd, &keep_alive)) {
        if ((conn->ip_trace || (TR_WF_CMDS && wf->tr_cmds < 32)) && !keep_alive) {
            clprintf(conn, "WF #%02d [rx_common_cmd] <%s> cmd_recv 0x%x/0x%x\n", wf->tr_cmds, cmd, wf->cmd_recv, CMD_WF_ALL);
            wf->tr_cmds++;
        }
        return;
    }

    if (conn->ip_trace || (TR_WF_CMDS && wf->tr_cmds < 32)) {
        clprintf(conn, "WF #%02d <%s> cmd_recv 0x%x/0x%x\n", wf->tr_cmds, cmd, wf->cmd_recv, CMD_WF_ALL);
        wf->tr_cmds++;
    }

    u2_t key = str_hash_lookup(&wf_cmd_hash, cmd);
    bool did_cmd = false;
    
    if (conn->ext_cmd != NULL)
        conn->ext_cmd(key, cmd, rx_chan);
    
    switch (key) {

    case CMD_SET_ZOOM: {
        int _zoom;
        float _start;
        bool zoom_start_chg = false, zoom_chg = false, start_chg = false;
        
        if (kiwi_str_begins_with(cmd, "SET zoom=")) {
            did_cmd = true;
            if (sscanf(cmd, "SET zoom=%d start=%f", &_zoom, &_start) == 2) {
                //cprintf(conn, "WF: zoom=%d/%d start=%.3f(%.1f)\n", _zoom, wf->zoom, _start, _start * wf->HZperStart / kHz);
                _zoom = CLAMP(_zoom, 0, MAX_ZOOM);
                float halfSpan_Hz = (ui_srate_Hz / (1 << _zoom)) / 2;
                wf->cf = (_start * wf->HZperStart) + halfSpan_Hz;
                #ifdef OPTION_HONEY_POT
                    cprintf(conn, "HONEY_POT W/F cf=%.3f\n", wf->cf / kHz);
                #endif
                zoom_start_chg = true;
            } else
            if (sscanf(cmd, "SET zoom=%d cf=%f", &_zoom, &wf->cf) == 2) {
                _zoom = CLAMP(_zoom, 0, MAX_ZOOM);
                float halfSpan_Hz = (ui_srate_Hz / (1 << _zoom)) / 2;
                wf->cf *= kHz;
                _start = (wf->cf - halfSpan_Hz) / wf->HZperStart;
                //cprintf(conn, "WF: zoom=%d cf=%.3f start=%.3f halfSpan=%.3f\n", _zoom, wf->cf/kHz, _start * wf->HZperStart / kHz, halfSpan_Hz/kHz);
                zoom_start_chg = true;
            }
        }
    
        if (!zoom_start_chg) break;
        conn->freqHz = wf->cf;      // for logging purposes
        
        // changing waterfall resets inactivity timer
        conn_t *csnd = conn_other(conn, STREAM_SOUND);
        if (csnd && conn->freqChangeLatch) {
            csnd->last_tune_time = timer_sec();
        }
        
        if (wf->zoom != _zoom) {
            wf->zoom = _zoom;
            zoom_chg = true;
            
            #define CIC1_DECIM 0x0001
            #define CIC2_DECIM 0x0100
            u2_t decim, r1, r2;
            
            #ifdef USE_WF_NEW
                // currently 11-levels of zoom: z0-z10, MAX_ZOOM == 10
                // z0-10: R = 2,4,8,16,32,64,128,256,512,1024,2048 for MAX_ZOOM == 10
                r1 = wf->zoom + 1;
                r2 = 1;		// R2 = 1
                decim = ?;
            #else
                // NB: because we only use half of the FFT with CIC can zoom one level less
                int zm1 = (WF_USING_HALF_CIC == 2)? (wf->zoom? (wf->zoom-1) : 0) : wf->zoom;

                #ifdef USE_WF_1CIC
        
                    // currently 15-levels of zoom: z0-z14, MAX_ZOOM == 14
                    if (zm1 == 0) {
                        // z0-1: R = 1,1
                        r1 = 0;
                    } else {
                        // z2-14: R = 2,4,8,16,32,64,128,256,512,1k,2k,4k,8k for MAX_ZOOM = 14
                        r1 = zm1;
                    }
            
                    // hardware limitation
                    assert(r1 >= 0 && r1 <= 15);
                    assert(WF_1CIC_MAXD <= 32768);
                    decim = CIC1_DECIM << r1;
                #else
                    // currently 15-levels of zoom: z0-z14, MAX_ZOOM == 14
                    if (zm1 == 0) {
                        // z0-1: R = 1 (R1 = R2 = 1)
                        r1 = r2 = 0;
                    } else
                    if (zm1 <= WF_2CIC_POW2) {
                        // z2-8: R = 2,4,8,16,32,64,128 (R1 = 1; R2 = 2,4,8,16,32,64,128)
                        r1 = 0;
                        r2 = zm1;
                    } else {
                        // z9-14: R = 128,256,512,1k,2k,4k (R1 = 2,4,8,16,32,64; R2 = 128)
                        r1 = zm1 - WF_2CIC_POW2;
                        r2 = WF_2CIC_POW2;
                    }
            
                    // hardware limitation
                    assert(r1 >= 0 && r1 <= 7);
                    assert(r2 >= 0 && r2 <= 7);
                    assert(WF_2CIC_MAXD <= 127);
                    decim = (CIC2_DECIM << r2) | (CIC1_DECIM << r1);
                #endif
            #endif
            
            float samp_wait_us =  WF_C_NSAMPS * (1 << zm1) / conn->adc_clock_corrected * 1000000.0;
            wf->chunk_wait_us = (int) ceilf(samp_wait_us / n_chunks);
            wf->samp_wait_ms = (int) ceilf(samp_wait_us / 1000);
            #ifdef WF_INFO
            if (!bg) cprintf(conn, "---- WF%d Z%d zm1 %d/%d R%04x n_chunks %d samp_wait_us %.1f samp_wait_ms %d chunk_wait_us %d\n",
                rx_chan, wf->zoom, zm1, 1<<zm1, decim, n_chunks, samp_wait_us, wf->samp_wait_ms, wf->chunk_wait_us);
            #endif
        
            wf->new_map = wf->new_map2 = wf->new_map3 = TRUE;
        
            if (wf->nb_enable[NB_BLANKER] && wf->nb_enable[NB_WF]) wf->nb_param_change[NB_BLANKER] = true;
        
            // when zoom changes reevaluate if overlapped sampling might be needed
            wf->check_overlapped_sampling = true;
        
            if (wf->isWF)
                spi_set(CmdSetWFDecim, rx_chan, decim);
        
            // We've seen cases where the wf connects, but the sound never does.
            // So have to check for conn->other being valid.
            conn_t *csnd = conn_other(conn, STREAM_SOUND);
            if (csnd) {
                csnd->zoom = wf->zoom;		// set in the AUDIO conn
            }
            conn->zoom = wf->zoom;      // for logging purposes
        
            //jksd
            //printf("ZOOM z=%d ->z=%d\n", wf->zoom, wf->zoom);
            //wf->prev_zoom = (wf->zoom == -1)? wf->zoom : wf->zoom;
            wf->cmd_recv |= CMD_ZOOM;
        }
    
        if (wf->start_f != _start) start_chg = true;
        wf->start_f = _start;
        //cprintf(conn, "WF: START %.0f ", start);
        int maxstart = MAX_START(wf->zoom);
        wf->start_f = CLAMP(wf->start_f, 0, maxstart);
        
        //printf(" CLAMPED %.0f %.3f\n", wf->start_f, wf->start_f * wf->HZperStart / kHz);

        wf->off_freq = wf->start_f * wf->HZperStart;
        wf->off_freq_inv = ((float) maxstart - wf->start_f) * wf->HZperStart;
    
        #ifdef USE_WF_NEW
            #error spectral_inversion
            wf->off_freq += conn->adc_clock_corrected / (4 << wf->zoom);
        #endif
    
        wf->i_offset = (u64_t) (s64_t) ((wf->spectral_inversion? wf->off_freq_inv : wf->off_freq) / conn->adc_clock_corrected * pow(2,48));
        wf->i_offset = -wf->i_offset;

        #ifdef WF_INFO
        if (!bg) cprintf(conn, "WF z%d OFFSET %.3f kHz i_offset 0x%012llx\n",
            wf->zoom, wf->off_freq/kHz, wf->i_offset);
        #endif
    
        if (wf->isWF)
            spi_set3(CmdSetWFFreq, rx_chan, (wf->i_offset >> 16) & 0xffffffff, wf->i_offset & 0xffff);
        //jksd
        //printf("START s=%d ->s=%d\n", wf->start_f, wf->start);
        //wf->prev_start = (wf->start == -1)? wf->start_f : wf->start;
        wf->start = wf->start_f;
        wf->new_scale_mask = true;
        wf->cmd_recv |= CMD_START;
    
        send_msg(conn, SM_NO_DEBUG, "MSG zoom=%d start=%d", wf->zoom, (u4_t) wf->start_f);
        //printf("waterfall: send zoom %d start %d\n", wf->zoom, wf->start_f);
        //jksd
        //wf->flush_wf_pipe = 6;
        //printf("flush_wf_pipe %d\n", debug_v);
        //wf->flush_wf_pipe = debug_v;
        
        // this also catches "start=" changes from panning
        if (wf->aper == AUTO && start_chg && !zoom_chg) {
            wf->aper_pan_timer = wf->mark;
            #ifdef WF_APER_INFO
                printf("waterfall: start_chg aper_pan_timer=%d\n", wf->aper_pan_timer);
            #endif
        }
        if (wf->aper == AUTO && zoom_chg) {
            wf->avg_clear = 1;
            wf->need_autoscale++;
            #ifdef WF_APER_INFO
                printf("waterfall: zoom_chg need_autoscale++=%d algo=%d\n", wf->need_autoscale, wf->aper_algo);
            #endif
        }
        break;
    }
    
    case CMD_SET_MAX_MIN_DB:
        i = sscanf(cmd, "SET maxdb=%d mindb=%d", &wf->maxdb, &wf->mindb);
        if (i == 2) {
            did_cmd = true;
            #ifdef WF_APER_INFO
                printf("waterfall: maxdb=%d mindb=%d\n", wf->maxdb, wf->mindb);
            #endif
            wf->cmd_recv |= CMD_DB;
        }
        break;

    case CMD_SET_CMAP:
        int cmap;
        if (sscanf(cmd, "SET cmap=%d", &cmap) == 1) {
            //waterfall_cmap(conn, wf, cmap);
            did_cmd = true;
        }
        break;

    case CMD_SET_APER:
        int aper, algo;
	    float aper_param;
        if (sscanf(cmd, "SET aper=%d algo=%d param=%f", &aper, &algo, &aper_param) == 3) {
            #ifdef WF_APER_INFO
                printf("### WF n/d/s=%d/%d/%d aper=%d algo=%d param=%f\n",
                    wf->need_autoscale, wf->done_autoscale, wf->sent_autoscale, aper, algo, aper_param);
            #endif
            wf->aper = aper;
            wf->aper_algo = algo;
            if (aper == AUTO) {
                wf->aper_param = aper_param;
                wf->avg_clear = 1;
                wf->need_autoscale++;
            }
            did_cmd = true;
        }
        break;
    
    case CMD_INTERPOLATE:
        if (sscanf(cmd, "SET interp=%d", &wf->interp) == 1) {
            if ((int) wf->interp >= WF_CIC_COMP) {
                wf->cic_comp = true;
                wf->interp = (wf_interp_t) ((int) wf->interp - WF_CIC_COMP);
            } else {
                wf->cic_comp = false;
            }
            if (wf->interp < 0 || wf->interp > WF_CMA) wf->interp = WF_MAX;
            cprintf(conn, "WF interp=%d(%s) cic_comp=%d\n", wf->interp, interp_s[wf->interp], wf->cic_comp);
            did_cmd = true;                
        }
        break;

    case CMD_WF_WINDOW_FUNC:
        if (sscanf(cmd, "SET window_func=%d", &i) == 1) {
            if (i < 0 || i >= N_WF_WINF) i = 0;
            wf->window_func = i;
            cprintf(conn, "WF window_func=%d\n", wf->window_func);
            did_cmd = true;                
        }
        break;

#if 0
    // not currently used
    case CMD_SET_BAND:
        int _wband;
        i = sscanf(cmd, "SET band=%d", &_wband);
        if (i == 1) {
            did_cmd = true;
            //printf("waterfall: band=%d\n", _wband);
            if (wf->wband != _wband) {
                wf->wband = _wband;
                //printf("waterfall: BAND %d\n", wf->wband);
            }
        }
        break;

    // not currently used
    case CMD_SET_SCALE:
        int _scale;
        i = sscanf(cmd, "SET scale=%d", &_scale);
        if (i == 1) {
            did_cmd = true;
            //printf("waterfall: scale=%d\n", _scale);
            if (wf->scale != _scale) {
                wf->scale = _scale;
                //printf("waterfall: SCALE %d\n", wf->scale);
            }
        }
        break;
#endif

    case CMD_SET_WF_SPEED:
        int _speed;
        i = sscanf(cmd, "SET wf_speed=%d", &_speed);
        if (i == 1) {
            did_cmd = true;
            //printf("WF wf_speed=%d\n", _speed);
            if (_speed == -1) _speed = WF_NSPEEDS-1;
            if (_speed >= 0 && _speed < WF_NSPEEDS)
                wf->speed = _speed;
            send_msg(conn, SM_NO_DEBUG, "MSG wf_fps=%d", wf_fps[wf->speed]);
            wf->cmd_recv |= CMD_SPEED;
        }
        break;

    case CMD_SEND_DB:
        i = sscanf(cmd, "SET send_dB=%d", &wf->send_dB);
        if (i == 1) {
            did_cmd = true;
            //cprintf(conn, "WF send_dB=%d\n", wf->send_dB);
        }
        break;

    // FIXME: keep these from happening in the first place?
    case CMD_EXT_BLUR:
        int ch;
        i = sscanf(cmd, "SET ext_blur=%d", &ch);
        if (i == 1) {
            did_cmd = true;
        }
        break;
    
    default:
        if (conn->mc != NULL)
            cprintf(conn, "#### WF key=%d DEFAULT CASE <%s>\n", key, cmd);
        did_cmd = true;     // force skip
        break;
 
    }   // switch
    
    if (did_cmd) return;

    if (conn->mc != NULL) {
        cprintf(conn, "#### WF hash=0x%04x key=%d \"%s\"\n", wf_cmd_hash.cur_hash, key, cmd);
        cprintf(conn, "WF BAD PARAMS: sl=%d %d|%d|%d [%s] ip=%s ####################################\n",
            strlen(cmd), cmd[0], cmd[1], cmd[2], cmd, conn->remote_ip);
        conn->unknown_cmd_recvd++;
    }
}
