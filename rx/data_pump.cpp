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

// Copyright (c) 2015-2026 John Seamons, ZL4VO/KF6VO

#include "types.h"
#include "config.h"
#include "kiwi.h"
#include "rx.h"
#include "misc.h"
#include "timer.h"
#include "web.h"
#include "spi.h"
#include "spi_dev.h"
#include "gps.h"
#include "coroutines.h"
#include "debug.h"
#include "data_pump.h"
#include "fpga.h"
#include "fastfir.h"
#include "ansi.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <math.h>

//#define DP_DEBUG
#ifdef DP_DEBUG
    #define dpd(x) x
    
    //#define DPP
    #ifdef DPP
        #define dpp(fmt, ...) \
            printf(fmt, ## __VA_ARGS__)
    #else
        #define dpp(fmt, ...)
    #endif
    
    #define DP_BUFPTR
    #ifdef DP_BUFPTR
        #define dpp2(fmt, ...) \
            real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
    #else
        #define dpp2(fmt, ...)
    #endif
    
    #define DPP_RESET
    #ifdef DPP_RESET
        #define dpp3(fmt, ...) \
            real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
    #else
        #define dpp3(fmt, ...)
    #endif
    
    #define DPP_STATE
    #ifdef DPP_STATE
        #define dpp4(fmt, ...) \
            printf(fmt, ## __VA_ARGS__);
        #define dpp4r(fmt, ...) \
            real_printf(fmt, ## __VA_ARGS__); fflush(stdout)
    #else
        #define dpp4(fmt, ...)
        #define dpp4r(fmt, ...)
    #endif
#else
    #define dpd(x)
    #define dpp(fmt, ...)
    #define dpp2(fmt, ...)
    #define dpp3(fmt, ...)
    #define dpp4(fmt, ...)
    #define dpp4r(fmt, ...)
#endif

rx_dpump_t rx_dpump[MAX_RX_CHANS];
dpump_t dpump;

#ifdef RX_SHMEM_DISABLE
    static rx_shmem_t rx_shmem;
    rx_shmem_t *rx_shmem_p = &rx_shmem;
#endif

bool have_snd_users;

#ifdef USE_SDR

struct rx_data_t {
    #ifdef SND_SEQ_CHECK
        struct rx_header_t {
            u2_t magic;
            u2_t snd_seq;
        } hdr;
    #endif
	iq3_t iq_t[MAX_NRX_SAMPS];
} __attribute__((packed));
static rx_data_t *rxd;

// Must copy to intermediate buffer because real_printf() delay would otherwise cause
// buffer being inspected to be overwritten.
//#define DP_DUMP_WB
#ifdef DP_DUMP_WB
    static rx_data_t rxd_debug;
#endif

struct rx_trailer_t {
	u2_t ticks[3];
	u2_t write_ctr_stored, write_ctr_current;
} __attribute__((packed));
static rx_trailer_t *rxt;

// Rescale factor from hardware samples to what CuteSDR code is expecting.
// All empirically measurement using sig gen.
const TYPEREAL rx_cicf_gain_dB[N_FW_SEL] = {
    4.5,        // FW_SEL_SDR_RX4_WF4
    4.5,        // FW_SEL_SDR_RX8_WF2
    4.5 + 8.6,  // FW_SEL_SDR_RX3_WF3   different because rx3 RX_CICF coeffs are different than others
    4.5,        // FW_SEL_SDR_RX14_WF0
    4.5,        // FW_SEL_SDR_WB
    4.5         // FW_SEL_SDR_RX8_WF3_SHARE
};
#define RX_GAIN_THS_4509    2.0
static TYPEREAL rescale;

static int nrx_loop, rx_xfer_size;
static u4_t last_run_us;

#ifdef SND_SEQ_CHECK
	static bool initial_seq;
	static u2_t snd_seq;
#endif

static void snd_service()
{
	int j;
	SPI_MISO *miso = &SPI_SHMEM->dpump_snd_miso;
	u4_t diff, moved=0;
	
    evLatency(EC_EVENT, EV_DPUMP, 0, "DATAPUMP", "snd_service() BEGIN");
    do {

        #ifdef SND_SEQ_CHECK
            rxd->hdr.magic = 0;
        #endif
        
        // use noduplex here because we don't want to yield
        evDPC(EC_TRIG3, EV_DPUMP, -1, "snd_svc", "CmdGetRX..");
    
        // CTRL_SND_INTR cleared as a side-effect of the kiwi.sdr.asm:CmdGetRX code
        spi_get_noduplex(CmdGetRX, miso, rx_xfer_size, nrx_loop);
        moved++;
        dpump.rx_adc_ovfl = miso->status & SPI_ADC_OVFL;
        if (dpump.rx_adc_ovfl) dpump.rx_adc_ovfl_cnt++;
        
        evDPC(EC_EVENT, EV_DPUMP, -1, "snd_svc", "..CmdGetRX");
        
        #ifdef SND_SEQ_CHECK
            evDP(EC_TRIG2, EV_DPUMP, -1, "snd_service", evprintf("SERVICED SEQ %d %%%%%%%%%%%%%%%%%%%%",
                rxd->snd_seq));
        #endif
        //evDP(EC_TRIG2, EV_DPUMP, 15000, "SND", "SERVICED ----------------------------------------");
        
        #ifdef SND_SEQ_CHECK
            if (rxd->hdr.magic != 0x0ff0) {
                printf("BAD MAGIC 0x%04x", rxd->hdr.magic)
                evDPC(EC_EVENT, EV_DPUMP, -1, "DATAPUMP", evprintf("BAD MAGIC 0x%04x", rxd->magic));
                if (ev_dump) evDPC(EC_DUMP, EV_DPUMP, ev_dump, "DATAPUMP", evprintf("DUMP in %.3f sec", ev_dump/1000.0));
            }
    
            if (!initial_seq) {
                snd_seq = rxd->hdr.snd_seq;
                initial_seq = true;
            }
            u2_t new_seq = rxd->hdr.snd_seq;
            if (snd_seq != new_seq) {
                real_printf("#%d %d:%d(%d)\n", dpump.audio_dropped, snd_seq, new_seq, new_seq-snd_seq);
                evDPC(EC_EVENT, EV_DPUMP, -1, "SEQ DROP", evprintf("$dp #%d %d:%d(%d)", dpump.audio_dropped, snd_seq, new_seq, new_seq-snd_seq));
                dpump.audio_dropped++;
                //TaskLastRun();
                bool dump = false;
                //bool dump = true;
                //bool dump = (new_seq-snd_seq < 0);
                //bool dump = (dpump.audio_dropped == 2);
                //bool dump = (dpump.audio_dropped == 6);
                if (dump && ev_dump) evNT(EC_DUMP, EV_NEXTTASK, ev_dump, "NextTask", evprintf("DUMP IN %.3f SEC",
                    ev_dump/1000.0));
                snd_seq = new_seq;
            }
            snd_seq++;
            bool dump = false;
            //bool dump = (snd_seq == 1000);
            if (dump && ev_dump) evNT(EC_DUMP, EV_NEXTTASK, ev_dump, "NextTask", evprintf("DUMP IN %.3f SEC",
                ev_dump/1000.0));
        #endif
        
        //#define DP_SHOW_TICKS
        #ifdef DP_SHOW_TICKS
            // ticks in correct place in buffer?
            static int ctrctr;
            ctrctr++;
            if ((ctrctr & 0x3f) == 0) {
                real_printf(YELLOW "Ticks:%04x-%04x-%04x" NORM " ",
                    rxt->ticks[2], rxt->ticks[1], rxt->ticks[0]);
                fflush(stdout);
            }
        #endif
    
        TYPECPX *i_samps[MAX_RX_CHANS];
        TYPECPX24 *i_wb_samps;
        
        for (int ch = 0; ch < rx_chans; ch++) {
            rx_dpump_t *rx = &rx_dpump[ch];
            i_samps[ch] = rx->in_samps[rx->wr_pos];
        }
        if (kiwi.isWB) {
            rx_dpump_t *rx = &rx_dpump[RX_CHAN0];
            i_wb_samps = rx->wb_samps[rx->wr_pos];
            dpp2("[%d=%p] ", rx->wr_pos, i_wb_samps);
        }

        iq3_t *iqp = (iq3_t *) &rxd->iq_t;
    
        #if 0
            // check 48-bit ticks counter timestamp
            static int debug_ticks;
            if (debug_ticks >= 1024 && debug_ticks < 1024+8) {
                for (int j=-1; j>-2; j--) {
                    real_printf("debug_iq3 %d %d(%d*%d) %02x%04x %02x%04x\n", j, nrx_samps*rx_wb_buf_chans+j, nrx_samps, rx_wb_buf_chans,
                        rxd->iq_t[nrx_samps*rx_wb_buf_chans+j].i3, rxd->iq_t[nrx_samps*rx_wb_buf_chans+j].i,
                        rxd->iq_t[nrx_samps*rx_wb_buf_chans+j].q3, rxd->iq_t[nrx_samps*rx_wb_buf_chans+j].q);
                }
                real_printf("debug_ticks %04x[2] %04x[1] %04x[0]\n", rxt->ticks[2], rxt->ticks[1], rxt->ticks[0]);
                real_printf("debug_bufcnt %04x\n\n", rxt->write_ctr_stored);
            }
            debug_ticks++;
        #endif
        
        if (kiwi.isWB) {
            #ifdef DP_DUMP_WB
                iq3_t *iqd_p = rxd_debug.iq_t;
                #define DP_DUMP_GO 100
                static u4_t go;
            #endif
    
            //if (rx_channels[RX_CHAN0].data_enabled) { real_printf("."); fflush(stdout); }

            for (j=0; j < nrx_samps; j++) {

                // rx1: wb0..wb[rx_wb_buf_chans-1]
                for (int wb = 0; wb < rx_wb_buf_chans; wb++) {
                    if (rx_channels[RX_CHAN0].data_enabled) {
                        s4_t i, q;
                        //#define WB_PATTERN
                        #ifdef WB_PATTERN

                            // sdr++: zoom in fully to see the picket
                            #define PICKET
                            #ifdef PICKET
                                static u4_t ctr;
                                if ((ctr % 1024) == 0) i = 0x3fff; else i = 0;
                                ctr++;
                                q = 0;
                            #else
                                i = S24_8_16(iqp->i3, iqp->i);
                                q = S24_8_16(iqp->q3, iqp->q);
                            #endif

                            i_wb_samps->re = q;     // preserve bit pattern (i.e. no scaling)
                            i_wb_samps->im = i;
                        #else
                            i = S24_8_16(iqp->i3, iqp->i);
                            q = S24_8_16(iqp->q3, iqp->q);
        
                            // sign extend
                            // for RX2_BITS = 18
                            // 2222 1111 111111
                            // 3210 9876 54321098 76543210
                            // eeee eeSd dddddddd dddddddd
                            //        12 345678
                            //                 12 34567890
                            
                            i_wb_samps->re = q;
                            i_wb_samps->im = i;
                        #endif
                        i_wb_samps++;
                    }

                    #ifdef DP_DUMP_WB
                        if (go == DP_DUMP_GO) {
                            iqd_p->i = iqp->i;
                            iqd_p->i3 = iqp->i3;
                            iqd_p->q = iqp->q;
                            iqd_p->q3 = iqp->q3;
                            iqd_p++;
                        }
                    #endif

                    iqp++;
                }
            }
        
            #ifdef DP_DUMP_WB
                if (go == DP_DUMP_GO) {
                    iqd_p = rxd_debug.iq_t;
                    for (j=0; j < nrx_samps; j++) {
                        real_printf("%04d: ", j);
                        for (int ch=0; ch < rx_wb_buf_chans; ch++) {
                            #define DP_DUMP_HEX
                            #ifdef DP_DUMP_HEX
                                real_printf("[ch%d %02x|%02x|%04x|%04x] ", ch, iqd_p->i3, iqd_p->q3, iqd_p->i, iqd_p->q);
                            #else
                                s4_t i, q;
                                i = S24_8_16(iqd_p->i3, iqd_p->i);
                                q = S24_8_16(iqd_p->q3, iqd_p->q);
                                real_printf("[ch%d %6d|%6d] ", ch, i, q);
                            #endif
                            iqd_p++;
                        }
                        real_printf("\n"); fflush(stdout);
                    }
                }
                
                go++;
            #endif
        } else {
            // not wideband
            
            // NB: I/Q reversed below to get correct sideband polarity
            // i.e. normal case: re=q im=i; spectral_inversion case: re=i im=q
            // Probably because mixer NCO polarity is wrong, i.e. cos/sin should really be cos/-sin
            // but we never took the time to verify this.
            if (kiwi.spectral_inversion) {
                for (j=0; j < nrx_samps; j++) {
    
                    for (int ch=0; ch < rx_chans; ch++) {
                        if (rx_channels[ch].data_enabled) {
                            s4_t i, q;
                            i = S24_8_16(iqp->i3, iqp->i);
                            q = S24_8_16(iqp->q3, iqp->q);
    
                            i_samps[ch]->re = i * rescale + DC_offset_I;
                            i_samps[ch]->im = q * rescale + DC_offset_Q;
                            i_samps[ch]++;
                        }
                        
                        iqp++;
                    }
                }
            } else {
                for (j=0; j < nrx_samps; j++) {
    
                    for (int ch=0; ch < rx_chans; ch++) {
                        if (rx_channels[ch].data_enabled) {
                            s4_t i, q;
                            i = S24_8_16(iqp->i3, iqp->i);
                            q = S24_8_16(iqp->q3, iqp->q);
    
                            // NB: I/Q reversed to get correct sideband polarity; fixme: why?
                            // [probably because mixer NCO polarity is wrong, i.e. cos/sin should really be cos/-sin]
                            i_samps[ch]->re = q * rescale + DC_offset_I;
                            i_samps[ch]->im = i * rescale + DC_offset_Q;
                            i_samps[ch]++;
                        }
                        iqp++;
                    }
                }
            }
        }

        // detect if the rx->in_samps[N_DPBUF] buffer has been overrun. (typ N_DPBUF = 32)
        int n_dpbuf = kiwi.isWB? N_WB_DPBUF : N_DPBUF;
        
        for (int ch=0; ch < rx_chans; ch++) {
            //if (ch == 1) real_printf("%d:en%d ", ch, rx_channels[ch].data_enabled); fflush(stdout);
            if (rx_channels[ch].data_enabled) {
                rx_dpump_t *rx = &rx_dpump[ch];
                //real_printf("%d:%d ", ch, rx->wr_pos); fflush(stdout);

                rx->ticks[rx->wr_pos] = S16x4_S64(0, rxt->ticks[2], rxt->ticks[1], rxt->ticks[0]);
    
                #ifdef SND_SEQ_CHECK
                    rx->in_seq[rx->wr_pos] = snd_seq;
                #endif
                
                rx->wr_pos = (rx->wr_pos+1) & (n_dpbuf-1);
                rx_channels[ch].wr++;
                
                diff = (rx->wr_pos >= rx->rd_pos)? (rx->wr_pos - rx->rd_pos) : (n_dpbuf - rx->rd_pos + rx->wr_pos);
                if (diff >= n_dpbuf) {
                    dpump.in_hist_resets++;
                    dpp3(RED "#");
                } else {
                    dpump.in_hist[diff]++;
                }
            }
        }
        
        // When each CmdGetRX at the top of the loop occurs the rx_audio_mem.v:raddr increments
        // (via GET_RX_SAMP/get_rx_samp_C). Meanwhile the DDC is writing ahead incrementing rx_audio_mem.v:waddr
        // and also storing the buf_ctr_A into the rxt->write_ctr_current of each buffer.
        // buf_ctr_A is a counter incremented after each SPI sized entry is created.
        // In this way each buffer can identify itself in the larger rx->in_samps[N_DPBUF:rx->wr_pos]
        // buffer on the Beagle side. This is important for detecting underrun of the larger buffer
        // if the Beagle can't keep up.
        //
        // When the actual CmdGetRX occurs the current buf_ctr_A (buf_ctr_C) is also transmitted.
        // This is the position of buffer number of the DDC writer. So the difference between the
        // SPI transfer stored and DDC current positions indicate how far the DDC writer is ahead.
        // This difference may indicate more than one CmdGetRX is required in the current run of the data pump.
        //
        // Note that this is different than the above code. Because the code below detects input overrun of rx->in_samps[]
        // by the snd data pump producer. Whereas the above code detects underrun on the output side of rx->in_samps[]
        // by its consumer (rx->rd_pos).
        
        u2_t current = rxt->write_ctr_current;
        u2_t stored = rxt->write_ctr_stored;
        if (current >= stored) {
            diff = current - stored;
        } else {
            diff = (0xffff - stored) + current;
        }
        
        dpp2("%d|%d ", stored, current);
        dpp2("%d|%d|%x-%04x ", stored, current, rxt->ticks[1], rxt->ticks[0]);
        
        evLatency(EC_EVENT, EV_DPUMP, 0, "DATAPUMP", evprintf("MOVED %d diff %d sto %d cur %d %.3f msec",
            moved, diff, stored, current, (timer_us() - last_run_us)/1e3));

        if (diff > (nrx_bufs-1)) {
		    dpump.resets++;
		    
		    // dump on excessive latency between runs
		    #ifdef EV_MEAS_DPUMP_LATENCY
                //if (ev_dump /*&& dpump.resets > 1*/) {
                u4_t last = timer_us() - last_run_us;
                if ((ev_dump || bg) && last_run_us != 0 && last >= 40000) {
                    evLatency(EC_EVENT, EV_DPUMP, 0, "DATAPUMP", evprintf("latency %.3f msec", last/1e3));
                    evLatency(EC_DUMP, EV_DPUMP, ev_dump, "DATAPUMP", evprintf("DUMP in %.3f sec", ev_dump/1000.0));
                }
            #endif
            
            //real_printf("."); fflush(stdout);
            dpp3(RED "X" NORM);

            #ifdef DP_BUFPTR
                u4_t sec = timer_ms() % 60000;
                dpp2(RED "%s%.3f:%d|%d>%u" NORM " ", (sec < 10000)? "0":"", (float) sec/1e3,
                    stored, current, diff);
            #endif
            
            dpp3(YELLOW "RST=%d " NORM, nrx_samps_wb);
            spi_set(CmdSetRXNsamps, nrx_samps_wb);
            diff = 0;
        } else {
            dpump.hist[diff]++;
            #if 0
                if (ev_dump && p1 && p2 && dpump.hist[p1] > p2) {
                    printf("DATAPUMP DUMP %d %d %d\n", diff, stored, current);
                    evLatency(EC_DUMP, EV_DPUMP, ev_dump, ">diff",
                        evprintf("MOVED %d, diff %d sto %d cur %d, DUMP", moved, diff, stored, current));
                }
            #endif
        }
        
        last_run_us = timer_us();
        
        if (!snd_itask_run) {
            dpp4r(YELLOW "OFF=0 " NORM);
            spi_set(CmdSetRXNsamps, 0);
            ctrl_clr_set(CTRL_SND_INTR, 0);
        }
    } while (diff > 1);
    evLatency(EC_EVENT, EV_DPUMP, 0, "DATAPUMP", evprintf("FINAL MOVED=%d", moved));

}

static void snd_pump(void *param)
{
	evDP(EC_EVENT, EV_DPUMP, -1, "dpump_init", evprintf("INIT: SPI CTRL_SND_INTR %d",
		GPIO_READ_BIT(SND_INTR)));

	while (1) {

		evDP(EC_EVENT, EV_DPUMP, -1, "data_pump", evprintf("SLEEPING: SPI CTRL_SND_INTR %d",
			GPIO_READ_BIT(SND_INTR)));

		//#define MEAS_DATA_PUMP
		#ifdef MEAS_DATA_PUMP
		    u4_t quanta = FROM_VOID_PARAM(TaskSleepReason("wait for interrupt"));
            static u4_t last, cps, max_quanta, sum_quanta;
            u4_t now = timer_sec();
            if (last != now) {
                for (; last < now; last++) {
                    if (last < (now-1))
                        real_printf("d- ");
                    else
                        real_printf("d%d|%d/%d ", cps, sum_quanta/(cps? cps:1), max_quanta);
                    fflush(stdout);
                }
                max_quanta = sum_quanta = 0;
                cps = 0;
            } else {
                if (quanta > max_quanta) max_quanta = quanta;
                sum_quanta += quanta;
                cps++;
            }
        #else
		    TaskSleepReason("snd: wait for intr");
        #endif

		evDP(EC_EVENT, EV_DPUMP, -1, "data_pump", evprintf("WAKEUP: SPI CTRL_SND_INTR %d",
			GPIO_READ_BIT(SND_INTR)));
		TaskStat(TSTAT_INCR|TSTAT_ZERO, 0, "dp");

		snd_service();
		
		for (int ch=0; ch < rx_chans; ch++) {
			rx_chan_t *rxc = &rx_channels[ch];
			if (!rxc->chan_enabled) continue;
			
			if (rxc->wb_task) {
                TaskWakeup(rxc->wb_task);
			}
            conn_t *c = rxc->conn;
            if (c != NULL) {
                assert(c->type == STREAM_SOUND);
                if (c->task) {
                    TaskWakeup(c->task);
                }
            }
		}
	}
}

void snd_pump_start_stop()
{
#ifdef USE_SDR
	bool no_users = true;
	
    for (int ch = 0; ch < rx_chans; ch++) {
        rx_chan_t *rx = &rx_channels[ch];
		if (rx->chan_enabled) {
			no_users = false;
			break;
		}
	}
	
    int nsamps = no_users? 0 : nrx_samps_wb;
	dpp4(CYAN "snd_pump_start_stop rx_chans=%d nsamps=%d loop=%d no_users=%d" NONL,
	    rx_chans, nsamps, nrx_loop, no_users);


	// stop the data pump when the last user leaves
	if (snd_itask_run && no_users) {
		snd_itask_run = false;
        dpp4r(YELLOW "STOP=0 " NORM);
        spi_set(CmdSetRXNsamps, 0);
		ctrl_clr_set(CTRL_SND_INTR, 0);
		dpp4(YELLOW "#### STOP nsamps=%d " NONL, nsamps);
		last_run_us = 0;
	}

	// start the data pump when the first user arrives
	if (!snd_itask_run && !no_users) {
		snd_itask_run = true;
		
	    for (int i = 0; i < rx_chans; i++) {
            rx_dpump_t *rx = &rx_dpump[i];
            rx->wr_pos = rx->rd_pos = 0;
	    }
		
        dpp4r(GREEN "START=%d " NORM, nrx_samps_wb);
        spi_set(CmdSetRXNsamps, nrx_samps_wb);
		ctrl_clr_set(CTRL_SND_INTR, 0);
		dpp4(YELLOW "START nsamps=%d " NONL, nsamps);
		last_run_us = 0;
	}
	
	have_snd_users = !no_users;
#endif
}

static void wf_pump(void *param)
{
    int ch;
    stat_reg_t stat;
	SPI_MISO *miso = &SPI_SHMEM->dpump_wf_miso;
    
	while (1) {
        TaskSleepReason("wf: wait for intr");
        wfp(BLUE " >" NORM);
        evShare(EC_SNAPSHOT, EV_WF, -1, "WF-share", evprintf("wf_pump WAKEUP -----------------------------------------------------------------"));
        spi_get_noduplex(CmdWFClrIntr, miso, sizeof(u2_t));
        stat.word = miso->word[0];
        u2_t ddc_full = stat.stat & MASK(wf_chans);
        evShare(EC_SNAPSHOT, EV_WF, -1, "WF-share", evprintf("wf_pump ddc_full=%d|%d|%d|%d", ddc_full, b2(ddc_full), b1(ddc_full), b0(ddc_full)));
        wfp(BLUE "%x" NORM, ddc_full);
        
        #ifdef WFP
            u4_t sec = timer_ms() % 60000;
            static u4_t last_sec;
            real_printf("%s%.3f:%d ", (sec < 10000)? "0":"", (float) sec/1e3, sec - last_sec);
            last_sec = sec;
        #endif
        /*
            u1_t full_pulse = (stat.dbg6 & 1)? 1:0;
            u1_t srq_noted = (stat.dbg6 & 2)? 1:0;
            u1_t srq_out = (stat.dbg6 & 4)? 1:0;
            //wfp("%s%x%d%d%d%s ", BLUE, ddc_full, full_pulse, srq_noted, srq_out, NORM);
            //wfp("%s%04x-%x%s ", BLUE, stat.word, ddc_full, NORM);
        */
        
        if (ddc_full == 0) {
            wfp(RED "WF_INTR NO ddc_full %04x " NONL, stat.word);
            continue;
        }
        //wfp("%04xI%x ", stat.word, ddc_full);
        //wfp("I%x", ddc_full);
        
        for (ch = 0; ch < rx_chans; ch++) {
	        wf_inst_t *wf = &WF_SHMEM->wf_inst[ch];
	        const char *color = ch? GREEN : YELLOW;

            // NB: not .chan_enabled because internal connections don't set chan_enabled
	        wfp2(" %sch%d %s" NORM, color, ch, rx_channels[ch].busy? "bsy1 " : (GREY "bsy0"));
	        if (!rx_channels[ch].busy) {
                //evShare(EC_SNAPSHOT, EV_WF, -1, "WF-share", evprintf(GREY "wf_pump CHANNEL NOT BUSY ch%d" NORM, ch));
	            continue;
	        }

	        wfp2("%sseq%d|%d " NORM, color, wf->wf_sleep_seq, wf->wf_intr_seq);

	        wfp2("%sst%d %s" NORM, color, wf->wf_state, wf->wake_buf_full? (GREY "ABF") : "0bf ");
	        if (wf->wake_buf_full) {
                evShare(EC_SNAPSHOT, EV_WF, -1, "WF-share", evprintf(GREY "wf_pump ALREADY wake_buf_full (why re-interrupting?) ch%d" NORM, ch));
	            continue;
	        }

			int ddc = wf->ddc_chan;
	        wfp2("%s%sddc%d " NORM, color, (ddc == -1)? RED : "", ddc);
	        if (ddc == -1) {
                evShare(EC_SNAPSHOT, EV_WF, -1, "WF-share", evprintf(BLUE "wf_pump NO DDC ALLOC! ch%d ddc%d" NORM, ch, ddc));
	            continue;
	        }
	        
            //if (ch == 0 && ddc == -1) { wfp("%srx%dddc%d%s", BLUE, ch, ddc, NORM); } //jks0
            //if ((ch == 0 && ddc != 1) || (ch == 1 && ddc != 0)) { wfp("%srx%dddc%d%s", BLUE, ch, ddc, NORM); } //jks01

            u1_t bit = 1 << ddc;
            wfp2("%sbit%x ful%x $%d " NORM, color, bit, ddc_full, (ddc_full & bit)? 1:0);
            evShare(EC_SNAPSHOT, EV_WF, -1, "WF-share", evprintf("wf_pump CHECK ch%d ddc%d bit%x ddc_full=%d|%d|%d|%d ddc_full&bit=%d",
                ch, ddc, bit, ddc_full, b2(ddc_full), b1(ddc_full), b0(ddc_full), (ddc_full & bit)? 1:0));
            if (ddc_full & bit) {
                if (!wf->tid) {
                    wfp(RED "NO-TID" NORM);
                    evShare(EC_SNAPSHOT, EV_WF, -1, "WF-share", evprintf(BLUE "wf_pump NO TID? ch%d ddc%d" NORM, ch, ddc));
                } else
                if (wf->wake_buf_full == 0) {
                    wf->wake_buf_full = 1;
	                wfd(wf->wf_intr_seq++;)
                    evShare(EC_SNAPSHOT, EV_WF, -1, "WF-share", evprintf(CYAN "wf_pump WAKEUP ch%d ddc%d" NORM, ch, ddc));
                    TaskWakeup(wf->tid);
                    wfp(BLUE "W%d%d" NORM, ch, ddc);
                } else {
                    // shouldn't happen
                    wfp(RED "AF%d%d" NORM, ch, ddc);
                    evShare(EC_SNAPSHOT, EV_WF, -1, "WF-share", evprintf(BLUE "wf_pump SHOULDN'T HAPPEN! ch%d ddc%d" NORM, ch, ddc));
                }
            } else {
                // !wake_buf_full and !(ddc_full & bit) i.e. waiting for wake_buf_full
            }
	    }
	}
}

static void data_pump_init()
{
    #if 1
        printf("CONV_FFT_SIZE=%d\n", CONV_FFT_SIZE);
        printf("CONV_FIR_SIZE=%d\n", CONV_FIR_SIZE);
        printf("CONV_FFT_TO_OUTBUF_RATIO=%d\n", CONV_FFT_TO_OUTBUF_RATIO);
        printf("FASTFIR_OUTBUF_SIZE=%d\n", FASTFIR_OUTBUF_SIZE);
        printf("MAX_NRX_SAMPS=%d\n", MAX_NRX_SAMPS);
        printf("MAX_WB_SAMPS=%d\n", MAX_WB_SAMPS);
        printf("NRX_SAMPS_BYTES_MAX=%d\n", NRX_SAMPS_BYTES_MAX);
    #endif

    rx_wb_buf_chans = kiwi.isWB? v_wb_buf_chans : rx_chans;
    nrx_samps = rx_wb_buf_chans? NRX_SAMPS_CHANS(rx_wb_buf_chans) : 0;
    nrx_loop = (nrx_samps * rx_wb_buf_chans) - 1;       // -1 important because e_cpu loop count
    nrx_samps_wb = kiwi.isWB? (nrx_samps * v_wb_buf_chans) : nrx_samps;

    rx_xfer_size = sizeof(iq3_t) * nrx_samps * rx_wb_buf_chans;
    #ifdef SND_SEQ_CHECK
        rx_xfer_size += sizeof(rx_data_t::rx_header_t);
    #endif
	rxd = (rx_data_t *) &SPI_SHMEM->dpump_snd_miso.word[0];
	rxt = (rx_trailer_t *) ((char *) rxd + rx_xfer_size);
	rx_xfer_size += sizeof(rx_trailer_t);
	dpp("rx_trailer_t=%d iq3_t=%d nsamps=%d|%d rx_xfer_size=%d/%d\n", sizeof(rx_trailer_t), sizeof(iq3_t),
	    nrx_samps, nrx_samps * rx_wb_buf_chans, rx_xfer_size, NRX_SAMPS_BYTES_MAX);

	// does a single nrx_samps transfer fit in the SPI buf?
	check(rx_xfer_size <= NRX_SAMPS_BYTES_MAX);       // in bytes
	
    check(nrx_samps < FASTFIR_OUTBUF_SIZE);     // see rx_dpump_t.in_samps[][]
    check(nrx_samps_wb < MAX_WB_SAMPS);         // see rx_dpump_t.wb_samps[][]

    // NB: assumes USE_RX_CICF is always used when pcb_ths_4509 true (safe assumption)
    TYPEREAL cicf_gain_dB = rx_cicf_gain_dB[kiwi.firmware_sel] + (kiwi.pcb_ths_4509? RX_GAIN_THS_4509 : 0);
	rescale = MPOW(2, -RXOUT_SCALE + CUTESDR_SCALE) * (VAL_USE_RX_CICF? MPOW(10, cicf_gain_dB/20.0) : 1);
	//printf("data pump: firmware_sel=%d RXOUT_SCALE=%d CUTESDR_SCALE=%d cicf_gain_dB=%.1f rescale=%.6g VAL_USE_RX_CICF=%d DC_offset_I=%f DC_offset_Q=%f\n", kiwi.firmware_sel, RXOUT_SCALE, CUTESDR_SCALE, cicf_gain_dB, rescale, VAL_USE_RX_CICF, DC_offset_I, DC_offset_Q);

	CreateTaskF(snd_pump, 0, DATAPUMP_PRIORITY, CTF_POLL_SND_INTR);
	
	if (kiwi.wf_share) CreateTaskF(wf_pump, 0, WF_PRIORITY, CTF_POLL_WF_INTR);
}

void data_pump_startup()
{
	// don't start data pump until first connection so GPS search can run at full speed on startup
	static bool data_pump_started;
	if (!data_pump_started) {
		data_pump_init();
		data_pump_started = true;
	}
}

void data_pump_dump()
{
    // SND
    rx_dpump_t *rx = &rx_dpump[RX_CHAN0];
    int n_dpbuf = kiwi.isWB? N_WB_DPBUF : N_DPBUF;
    
    u4_t diff = (rx->wr_pos >= rx->rd_pos)? (rx->wr_pos - rx->rd_pos) : (n_dpbuf - rx->rd_pos + rx->wr_pos);
    lprintf("data_pump SND RX_CHAN0 isWB=%d n_dpbuf=%d rd|wr_pos=%d|%d|%d \n",
        kiwi.isWB, n_dpbuf, rx->rd_pos, rx->wr_pos, diff);
    
    // WF
    lprintf("data_pump WF lock_seq_global=%u wf_seq_global=%u\n",
        WF_SHMEM->lock_seq_global, WF_SHMEM->wf_seq_global);
    for (int ddc = 0; ddc < wf_chans; ddc++) {
        lprintf("data_pump WF DDC%d locked=%d rx=%d use=%u\n",
            ddc, WF_SHMEM->ddc[ddc].lock, WF_SHMEM->ddc[ddc].lock_rx, WF_SHMEM->ddc[ddc].use_ct);
    }
    for (int ch = 0; ch < rx_chans; ch++) {
        wf_inst_t *wf = &WF_SHMEM->wf_inst[ch];
        if (rx_channels[ch].busy)
            lprintf("data_pump WF rx%d en=1 ddc=%d last_ddc=%d num_wakeups=%u lock_wait=%d lock_seq=%u wf_seq=%u wake_buf_full=%d\n",
                ch, wf->ddc_chan, wf->last_ddc, wf->num_wakeups, wf->lock_wait, wf->lock_seq, wf->wf_seq, wf->wake_buf_full);
        else
            lprintf("data_pump WF rx%d en=0\n", ch);
    }
}

#endif
