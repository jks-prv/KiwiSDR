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

#include "types.h"
#include "kiwi.h"
#include "printf.h"
#include "services.h"
#include "rx_util.h"
#include "rx_snr.h"
#include "rx_waterfall_cmd.h"
#include "rx_waterfall.h"
#include "ant_switch.h"

// SNR measurement

int snr_meas_interval_min, snr_all, snr_HF;
int ant_connected = 1;
SNR_meas_t SNR_meas_data[SNR_MEAS_MAX];
static s2_t dB_raw[WF_WIDTH];

int SNR_calc(SNR_meas_t *meas, int band, double f_lo, double f_hi, int zoom, bool filter = false)
{
    SNR_data_t *data = &meas->data[band];
    memset(data, 0, sizeof(*data));
    if ((f_lo == 0 && f_hi == 0) || f_lo < 0 || f_hi < 0 || f_hi < f_lo) return 0;

    static int dB[WF_WIDTH];
    int i, j, rv = 0, masked = 0;
    double lo_kHz = f_lo - freq.offset_kHz, hi_kHz = f_hi - freq.offset_kHz;

    zoom = CLAMP(zoom, 0, MAX_ZOOM);
    double span_kHz = (ui_srate_kHz / (1 << zoom));
    double cf_kHz = zoom? (lo_kHz + (hi_kHz - lo_kHz) / 2) : (ui_srate_kHz/2);
    double left_kHz = cf_kHz - span_kHz/2;
    if (left_kHz < 0) left_kHz = 0;
    int start = (lo_kHz - left_kHz) / span_kHz * WF_WIDTH;
    start = CLAMP(start, 0, WF_WIDTH);
    int stop  = (hi_kHz - left_kHz) / span_kHz * WF_WIDTH;
    stop = CLAMP(stop, 0, WF_WIDTH);

    for (i = (int) start; i < stop; i++) {
        //#define TEST_VDSL
        #ifdef TEST_VDSL
            if (band == SNR_BAND_ALL || band == SNR_BAND_HF) {
                double kpp = span_kHz / WF_WIDTH;
                double f = i*kpp;
                if (f >= 2000 && f <= 3000) dB_raw[i] = -70;
                if (f >= 4000 && f <= 6500) dB_raw[i] = -70;
            }
        #endif

        if (dB_raw[i] <= -190) masked++;
        dB[i] = dB_raw[i];
    }
    printf("SNR_calc-%d f=%.0f-%.0f z=%d bb=%.0f-%.0f fo=%.0f cf=%.0f span=%.3f left=%.3f start|stop=%d|%d masked_bins=%d\n",
        band, f_lo, f_hi, zoom, lo_kHz, hi_kHz, freq.offset_kHz, cf_kHz, span_kHz, left_kHz, start, stop, masked);

    // dB[bins = start:stop]
    
    // apply VDSL filter
    if (filter && (band == SNR_BAND_ALL || band == SNR_BAND_HF)) {
        int run = 0, prev_dBm = -190, have_run = 0, last_idx;
        double kpp = span_kHz / WF_WIDTH, last_start;
        int threshold = cfg_int_("snr_filter_thresh");
        int delta = cfg_int_("snr_filter_delta");
        int runlen_kHz = cfg_int_("snr_filter_runlen");
        int runlen = ceilf((float) runlen_kHz / span_kHz * WF_WIDTH);
        printf("SNR_calc-%d filtering: thresh=%d delta=%d runlen=%d(kHz)|%d(bins)\n",
            band, threshold, delta, runlen_kHz, runlen);
        
        for (i = (int) start; i < stop; i++) {
            double f = i*kpp;
            int dBm = dB[i];
            int diff = abs(dBm - prev_dBm);
            //printf("f:%.0f dBm:%d|%d(%d)\n", f, dBm, prev_dBm, diff);
            //real_printf("%.0f:%d:%d|%d(%d) ", f, i, dBm, prev_dBm, diff); fflush(stdout);
            if (dBm >= threshold && prev_dBm >= threshold) {
                if (diff <= delta) {
                    if (run == 0) {
                        last_start = f;
                        last_idx = i;
                    }
                    run++;
                    //printf("  #%d\n", run);
                    //real_printf(GREEN "#%d" NORM " ", run); fflush(stdout);
                } else {
                    if (run > runlen) have_run = run;
                    run = 0;
                    //printf("  RESET delta\n");
                    //real_printf(BLUE "DELTA" NORM " "); fflush(stdout);
                }
            } else {
                if (run) //printf("  RESET level\n");
                //if (run) { real_printf(RED "LEVEL" NORM " "); fflush(stdout); }
                if (run > runlen) have_run = run;
                run = 0;
            }
            if (have_run) {
                //real_printf("\n");
                printf("SNR_calc-%d FILTERED OUT: %.0f-%.0f(%.0f)\n",
                    band, last_start, f, have_run * kpp);
                for (j = last_idx; j < i+1; j++) {
                    dB[j] = -190;       // notch the run
                }
                have_run = 0;
            }
            prev_dBm = dBm;
        }
    }
    
    // disregard masked and notched areas
    int bins = 0;
    for (i = (int) start; i < stop; i++) {
        if (dB[i] <= -190) continue;
        dB[bins] = dB[i];
        bins++;
    }

    if (bins >= 32) {
        data->fkHz_lo = f_lo; data->fkHz_hi = f_hi;
        int pct_95 = 95;
        int pct_50 = median_i(dB, bins, &pct_95);    // does int qsort on dB array
        //#define TEST_USE_MAX
        #ifdef TEST_USE_MAX
            pct_95 = dB[bins-1];
        #endif
        #define FIT(x) ( ((x) > 0)? 0 : ( ((-(x)) > 255)? 255 : -(x) ) )
        data->pct_50 = FIT(pct_50);
        data->pct_95 = FIT(pct_95);
        data->min = FIT(dB[0]); data->max = FIT(dB[bins-1]);
        int snr = pct_95 - pct_50;
        if (snr < 0) snr = 0;
        rv = data->snr = snr;
        //printf("SNR_calc-%d 50|%d,%d 95|%d,%d\n", band, pct_50, data->pct_50, pct_95, data->pct_95);
        
        // Antenna disconnect detector
        // Rules based on observed cases. Avoiding false-positives is very important.
        // Disconnected if:
        // 1) freq.offset_kHz == 0 (via band == SNR_BAND_HF),
        //      i.e. no V/UHF transverter setups that might be inherently quiet when no signals.
        // 2) HF snr <= 3
        // 3) Not greater than 2 peaks stronger than -100 dBm
        if (band == SNR_BAND_HF) {
            int peaks = 0;
            for (i = 0; i < bins; i++) {
                if (dB[i] >= -100) peaks++;
            }
            int _ant_connected = (data->snr <= 3 && peaks <= 2)? 0:1;
            if (ant_connected != _ant_connected) {
            
                // wakeup the registration process (if any) so that the change in
                // antenna disconnection shows up on rx.kiwisdr.com as soon as possible
                bool woke = wakeup_reg_kiwisdr_com(WAKEUP_REG);
                ant_connected = _ant_connected;
                printf("SNR_calc-%d%s ant_connected=%d\n",
                    band, woke? " WAKEUP_REGISTRATION" : "", ant_connected);
            }
        }
        
        printf("SNR_calc-%d min,max=%d,%d noise(50%%)=%d signal(95%%)=%d snr=%d\n",
            band, -(data->min), -(data->max), -(data->pct_50), -(data->pct_95), data->snr);
    } else {
        printf("SNR_calc-%d WARNING: not enough bins for meaningful measurement\n", band);
    }
    
    return rv;
}

int SNR_meas_tid;

typedef struct {
    float fkHz_lo, fkHz_hi;
    u1_t band, zoom;
} SNR_bands_t;
SNR_bands_t SNR_bands[SNR_NBANDS];

#define SNR_INTERVAL_CUSTOM 8
static int snr_interval_min[] = { 0, 60, 4*60, 6*60, 24*60, 1, 5, 10, 0 };

void SNR_meas(void *param)
{
    int i, j, n;
    static internal_conn_t iconn;
    static u2_t snr_seq;
	bool on_demand = true;      // run once after restart
    TaskSleepSec(20);           // long enough for autoruns to start first
    
    //#define SNR_MEAS_SELECT WF_SELECT_1FPS
    //#define SNR_MEAS_SPEED WF_SPEED_1FPS
    //#define SNR_MEAS_NAVGS 64
    #define SNR_MEAS_SELECT WF_SELECT_FAST
    #define SNR_MEAS_SPEED WF_SPEED_FAST
    #define SNR_MEAS_NAVGS 32
    //printf("SNR_meas SNR_MEAS_SELECT/SPEED=%d/%d\n", SNR_MEAS_SELECT, SNR_MEAS_SPEED);
    //printf("SNR_meas sizeof(SNR_meas_data)=%d\n", sizeof(SNR_meas_data));
    
    do {
        static int meas_idx;
        kiwi.snr_meas_active = true;
        bool filter = cfg_true("snr_meas_filter");
	
        // "snr_meas_interval_hrs" is really the menu index
        int custom_min = cfg_int_("snr_meas_custom_min");
        custom_min = CLAMP(custom_min, 0, 10080);   // 7-days max
        int idx = cfg_int_("snr_meas_interval_hrs");
        idx = CLAMP(idx, 0, SNR_INTERVAL_CUSTOM);
        if (idx == SNR_INTERVAL_CUSTOM && custom_min != 0) {
            snr_meas_interval_min = custom_min;
        } else {
            snr_meas_interval_min = snr_interval_min[idx];
        }
        printf("SNR_meas: snr_meas_interval_min=%d\n", snr_meas_interval_min);

        for (int loop = 0; loop == 0 && (snr_meas_interval_min || on_demand); loop++) {     // so break can be used below

            int f_lo = freq.offset_kHz, f_hi = freq.offmax_kHz;
            int itu = cfg_int_("init.ITU_region") + 1;
        
            #define SNR_setup(b, lo, hi, z) \
                SNR_bands[b].band = b; \
                SNR_bands[b].fkHz_lo = lo; \
                SNR_bands[b].fkHz_hi = hi; \
                SNR_bands[b].zoom = z;
        
            SNR_setup(SNR_BAND_ALL,     f_lo,   f_hi,   0);
            SNR_setup(SNR_BAND_HF,      1800,   f_hi,   0);
            SNR_setup(SNR_BAND_0_2,     0,      1800,   0);
            SNR_setup(SNR_BAND_2_10,    1800,   10000,  0);
            SNR_setup(SNR_BAND_10_20,   10000,  20000,  0);
            SNR_setup(SNR_BAND_20_MAX,  20000,  f_hi,   0);
        
            int custom_lo = cfg_int_("snr_meas_custom_lo");
            custom_lo = CLAMP(custom_lo, f_lo, f_hi);
            int custom_hi = cfg_int_("snr_meas_custom_hi");
            custom_hi = CLAMP(custom_hi, f_lo, f_hi);
            int custom_zoom = cfg_int_("snr_meas_custom_zoom");
            custom_zoom = CLAMP(custom_zoom, 0, MAX_ZOOM);
            SNR_setup(SNR_BAND_CUSTOM, custom_lo, custom_hi, custom_zoom);
        
            SNR_setup(SNR_BAND_HAM_LF,  135.7,  137.8,  13);
            SNR_setup(SNR_BAND_HAM_MF,  472,    479,    11);
            SNR_setup(SNR_BAND_HAM_160, 1800,   2000,   7);
            SNR_setup(SNR_BAND_HAM_80,  3500,   (itu == 1)? 3800 : ((itu == 2)? 4000:3900), 5);
            SNR_setup(SNR_BAND_HAM_60,  5250,   5450,   7);
            SNR_setup(SNR_BAND_HAM_40,  7000,   (itu == 1)? 7200:7300, 6);
            SNR_setup(SNR_BAND_HAM_30,  10100,  (itu == 3)? 10157.3:10150, 8);
            SNR_setup(SNR_BAND_HAM_20,  14000,  14350,  6);
            SNR_setup(SNR_BAND_HAM_17,  18068,  18168,  7);
            SNR_setup(SNR_BAND_HAM_15,  21000,  21450,  6);
            SNR_setup(SNR_BAND_HAM_12,  24890,  24990,  7);
            SNR_setup(SNR_BAND_HAM_10,  28000,  29700,  4);
            
            SNR_setup(SNR_BAND_AM_BCB,  530,    (itu == 2)? 1700:1602, 4);

            // select antenna if required
            ant_switch_check_set_default();
            TaskSleepSec(3);

            if (internal_conn_setup(ICONN_WS_WF, &iconn, 0, PORT_BASE_INTERNAL_SNR, WS_FL_PREEMPT_AUTORUN | WS_FL_NO_LOG,
                NULL, 0, 0, 0,
                "SNR-measure", "internal%20task", "SNR",
                WF_ZOOM_MIN, 15000, -110, -10, SNR_MEAS_SELECT, WF_COMP_OFF) == false) {
                printf("SNR_meas: all channels busy\n");
                break;
            };
    
            SNR_meas_t *meas = &SNR_meas_data[meas_idx];
            memset(meas, 0, sizeof(*meas));
            meas_idx = ((meas_idx + 1) % SNR_MEAS_MAX);
            meas->flags = antsw.snr_ant & SNR_F_ANT;

	        // relative to local time if timezone has been determined, utc otherwise
	        if (cfg_true("snr_local_time")) {
                bool is_local_time;
                meas->tstamp = local_time(&is_local_time);
                if (is_local_time) meas->flags |= SNR_F_TLOCAL;
            } else {
                meas->tstamp = utc_time();
            }
            
            meas->seq = snr_seq++;
            
            for (int band = 0; band < SNR_NBANDS; band++) {
                SNR_bands_t *bp = &SNR_bands[band];
                float cf_kHz = bp->fkHz_lo + (bp->fkHz_hi - bp->fkHz_lo) / 2;
                //printf("SNR_meas-%d CONSIDER\n", band);
                
                if (band == SNR_BAND_CUSTOM) {
                    if ((custom_lo == 0 && custom_hi == 0) || custom_lo < f_lo || custom_hi > f_hi) {
                        printf("SNR_meas CUSTOM band: not enabled\n");
                        continue;
                    }
                } else
                if (band > SNR_BAND_CUSTOM) {
                    if (!cfg_true("snr_meas_ham")) {
                        printf("SNR_meas HAM & AM BCB bands: not enabled\n");
                        break;
                    }
                }
                
                //printf("SNR_meas-%d SET zoom=%d cf=%.3f\n", band, bp->zoom, cf_kHz);
                if (bp->zoom != 0) {
                    input_msg_internal(iconn.cwf, (char *) "SET zoom=%d cf=%.3f", bp->zoom, cf_kHz);
                    TaskSleepMsec(500);
                }
        
                memset(dB_raw, 0, sizeof(dB_raw));
                nbuf_t *nb = NULL;
                bool early_exit = false;
                int nsamps;
                for (nsamps = 0; nsamps < SNR_MEAS_NAVGS && !early_exit;) {
                    do {
                        if (nb) web_to_app_done(iconn.cwf, nb);
                        n = web_to_app(iconn.cwf, &nb, INTERNAL_CONNECTION);
                        if (n == 0) continue;
                        if (n == -1) {
                            early_exit = true;
                            break;
                        }
                        wf_pkt_t *wf = (wf_pkt_t *) nb->buf;
                        //printf("SNR_meas nsamps=%d rcv=%d <%.3s>\n", nsamps, n, wf->id4);
                        if (strncmp(wf->id4, "W/F", 3) != 0) continue;
                        for (j = 0; j < WF_WIDTH; j++) {
                            dB_raw[j] += dB_wire_to_dBm(wf->un.buf[j]);
                        }
                        nsamps++;
                    } while (n);
                    TaskSleepMsec(900 / SNR_MEAS_SPEED);
                }
                //printf("SNR_meas DONE nsamps=%d\n", nsamps);
                if (nsamps) {
                    for (i = 0; i < WF_WIDTH; i++) {
                            dB_raw[i] /= nsamps;
                    }
                }
                
                if (band == SNR_BAND_ALL) {
                    for (int b = 0; b < SNR_BAND_NSTD; b++) {
                        bp = &SNR_bands[b];
                        int snr = SNR_calc(meas, b, bp->fkHz_lo, bp->fkHz_hi, bp->zoom, filter);
                        if (b == SNR_BAND_ALL) snr_all = snr;
                        if (b == SNR_BAND_HF) {
                            if (f_lo != 0) {    // only measure HF if no transverter frequency offset has been set
                                snr_HF = 0;
                                break;
                            }
                            snr_HF = snr;
                        }
                    }
                    band = SNR_BAND_NSTD - 1;
                    continue;
                } else
                
                if (band == SNR_BAND_CUSTOM) {
                    printf("SNR_meas: CUSTOM %.0f-%.0f z%d\n", bp->fkHz_lo, bp->fkHz_hi, bp->zoom);
                    SNR_calc(meas, band, bp->fkHz_lo, bp->fkHz_hi, bp->zoom);
                } else {
                    SNR_calc(meas, band, bp->fkHz_lo, bp->fkHz_hi, bp->zoom);
                }
            }

            meas->flags |= SNR_F_VALID;
            kiwi.snr_initial_meas_done = true;  // CAUTION: this must always get set else ant switch hangs
            internal_conn_shutdown(&iconn);
            
            #ifdef OPTION_HONEY_POT
                snr_all = snr_HF = 55;
            #endif

            if (on_demand) {
                printf("SNR_meas: admin MEASURE NOW %d:%d\n", snr_all, freq.offset_kHz? -1 : snr_HF);
                snd_send_msg(SM_SND_ADM_ALL, SM_NO_DEBUG,
                    "MSG snr_stats=%d,%d", snr_all, freq.offset_kHz? -1 : snr_HF);
            }
        }
        
        // if disabled check again in an hour to see if re-enabled
        u64_t min = snr_meas_interval_min? snr_meas_interval_min : 60;
        
        // an on_demand occurs where there is an on-demand request, such as the admin control tab
        // "measure SNR now" button or the /snr?meas control URL
        printf("SNR_meas: SLEEP min=%lld snr_meas_interval_min=%d %lld\n", min, snr_meas_interval_min, SEC_TO_USEC(min*60));
        kiwi.snr_meas_active = false;
        // rv > 0: deadline expired
        // rv == 0: on-demand measurement or antenna switched
        int rv = (int) FROM_VOID_PARAM(TaskSleepSec(min * 60));
        on_demand = (rv == 0)? true:false;
        printf("SNR_meas: WAKEUP on_demand=%d snr_ant=%d seq=%d\n", on_demand, antsw.snr_ant & SNR_F_ANT, snr_seq);
    } while (1);
}
