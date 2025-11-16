//////////////////////////////////////////////////////////////////////////
// Homemade GPS Receiver
// Copyright (C) 2013 Andrew Holme
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// http://www.holmea.demon.co.uk/GPS/Main.htm
//////////////////////////////////////////////////////////////////////////

#include "gps.h"
#include "kiwi.h"
#include "spi.h"
#include "spi_dev.h"
#include "misc.h"
#include "ephemeris.h"
#include "fec.h"
#include "gnss_sdrlib.h"
#include "ansi.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <debug.h>

const int PWR_LEN = 8;

struct UPLOAD { // Embedded CPU CHANNEL structure
    uint16_t nav_ms;                // Milliseconds C/A: 0-19, E1B: 0, SBAS: 0-1
    uint16_t nav_bits;              // Bit count
    uint16_t nav_glitch;            // Glitch count
    uint16_t nav_prev;              // Last data bit
    uint16_t nav_buf[MAX_NAV_BITS/16];  // NAV data buffer
    uint16_t ca_freq[4];            // Loop filter integrator
    uint16_t lo_freq[4];            // Loop filter integrator
    uint16_t iq[3][4];              // Last PIQ, PEIQ, PLIQ samples
    uint16_t ca_gain[2];            // Code loop ki, kp
    uint16_t lo_gain[2];            // Carrier loop ki, kp
    uint16_t ca_unlocked;
    uint16_t sat_mode;
    uint16_t LO_polarity;
    uint16_t even_odd;
    //uint16_t pp[2], pe[2], pl[2];
};

struct CHANNEL { // Locally-held channel data
    UPLOAD ul;                      // Copy of embedded CPU channel state
    float pwr_tot, pwr[PWR_LEN];    // Running average of signal power
    int pwr_pos;                    // Circular counter
    int gain_adj_lo, gain_adj_cg;   // AGC
    int ch, sat;                    // Association
    int isE1B, isSBAS;
    int ACF_mode;
    int inverted;
    int alert;                      // Subframe alert flag
    int probation;                  // Temporarily disables use if channel noisy
    int holding, rd_pos;            // NAV data bit counters
    int tholding;
    u4_t id;
    int codegen_init;
    int subframe_bits, nsync, total_bits, bits_tow, expecting_preamble, drop_seq;
    bool abort;
    //jks2
    int LASTsub;
    sdrnav_t nav;
	SPI_MISO *miso;
	
    void  Reset(int sat, int codegen_init);
    void  Start(int sat, int t_sample, int lo_shift, int ca_shift, int snr);
    void  SetGainAdjLO(int);
    int   GetGainAdjLO();
    void  SetGainAdjCG(int);
    void  CheckPower();
    float GetPower();
    void  Service();
    void  Acquisition(bool delay);
    void  Tracking();
    void  SignalLost();
    void  UploadEmbeddedState();
    int   ParityCheck(char *buf, int *nbits);
    void  Subframe(char *buf);
    void  Status();
    int   RemoteBits(uint16_t wr_pos);
    bool  GetSnapshot(uint16_t wr_pos, int *p_sat, int *p_bits, int *p_bits_tow, float *p_pwr);
    bool  isSat(sat_e type, int prn);
};

static CHANNEL Chans[GPS_MAX_CHANS];

static unsigned BusyFlags;

///////////////////////////////////////////////////////////////////////////////////////////////

static double Get32(uint16_t *u) {
	return ( u[0] * pow(2, 0)
		   + u[1] * pow(2, 16));
}

static double Get64_frac(uint16_t *u) {
	return ( u[0] * pow(2, -64)
		   + u[1] * pow(2, -48)
		   + u[2] * pow(2, -32)
		   + u[3] * pow(2, -16));
}

static double GetFreq(uint16_t *u) { // Convert NCO command to Hertz
	return Get64_frac(u) * FS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

#define L1_PRELEN 8

const char L1preambleUpright[8] = {1,0,0,0,1,0,1,1};
const char L1preambleInverse[8] = {0,1,1,1,0,1,0,0};

static int L1_parity(char *p, char *word, char D29, char D30) {
    char *d = word-1;
    for (int i=1; i<25; i++) d[i] ^= D30;
    p[0] = D29 ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[6] ^ d[10] ^ d[11] ^ d[12] ^ d[13] ^ d[14] ^ d[17] ^ d[18] ^ d[20] ^ d[23];
    p[1] = D30 ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[7] ^ d[11] ^ d[12] ^ d[13] ^ d[14] ^ d[15] ^ d[18] ^ d[19] ^ d[21] ^ d[24];
    p[2] = D29 ^ d[1] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[8] ^ d[12] ^ d[13] ^ d[14] ^ d[15] ^ d[16] ^ d[19] ^ d[20] ^ d[22];
    p[3] = D30 ^ d[2] ^ d[4] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[13] ^ d[14] ^ d[15] ^ d[16] ^ d[17] ^ d[20] ^ d[21] ^ d[23];
    p[4] = D30 ^ d[1] ^ d[3] ^ d[5] ^ d[6] ^ d[7] ^ d[9] ^ d[10] ^ d[14] ^ d[15] ^ d[16] ^ d[17] ^ d[18] ^ d[21] ^ d[22] ^ d[24];
    p[5] = D29 ^ d[3] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[10] ^ d[11] ^ d[13] ^ d[15] ^ d[19] ^ d[22] ^ d[23] ^ d[24];
    return memcmp(d+25, p, 6);
}

#define E1B_PRELEN      10
#define E1B_NSYM        240
#define E1B_NBIT        120
#define E1B_TAIL        6
#define E1B_TSYM_PP     (E1B_PRELEN + E1B_NSYM)
#define E1B_TSYM_PW     (E1B_TSYM_PP * 2)

const char E1BpreambleUpright[10] = {0,1,0,1,1,0,0,0,0,0};
const char E1BpreambleInverse[10] = {1,0,1,0,0,1,1,1,1,1};

const char SBASpreambleUpright[3][8] = {{0,1,0,1,0,0,1,1}, {1,0,0,1,1,0,1,0}, {1,1,0,0,0,1,1,0}};
const char SBASpreambleInverse[3][8] = {{1,0,1,0,1,1,0,0}, {0,1,1,0,0,1,0,1}, {0,0,1,1,1,0,0,1}};

///////////////////////////////////////////////////////////////////////////////////////////////

bool CHANNEL::isSat(sat_e type, int prn) {
    SATELLITE *s = &Sats[sat];
    return (s->type == type && s->prn == prn);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CHANNEL::UploadEmbeddedState() {
    spi_get(CmdGetChan, miso, sizeof(ul), ch);
    memcpy(&ul, miso->byte, sizeof(ul));
	evGPS(EC_EVENT, EV_GPS, -1, "GPS", evprintf("UES ch %d ul %p ms %d bits %d glitches %d buf %d %d %d %d",
		ch+1, &ul, ul.nav_ms, ul.nav_bits, ul.nav_glitch,
		ul.nav_buf[0], ul.nav_buf[1], ul.nav_buf[2], ul.nav_buf[3]));
}

///////////////////////////////////////////////////////////////////////////////////////////////

int CHANNEL::GetGainAdjLO() {
    return gain_adj_lo;
}

void CHANNEL::SetGainAdjLO(int adj) {
    gain_adj_lo = adj;

    int lo_ki = 20;
    int lo_kp = 27;

    #define E1B_LO_GAIN_ADJ -3
    int e1b_gain_adj = gps_lo_gain? gps_lo_gain : E1B_LO_GAIN_ADJ;
    lo_ki += (isE1B? e1b_gain_adj : 0) + gain_adj_lo;
    lo_kp += (isE1B? e1b_gain_adj : 0) + gain_adj_lo;
    //printf("ch%02d %s CmdSetGainLO gain=%d ki=%d kp=%d\n", ch+1, PRN(sat), gain_adj_lo, lo_ki, lo_kp);
    spi_set(CmdSetGainLO, ch, lo_ki + ((lo_kp-lo_ki)<<16));
}

void CHANNEL::SetGainAdjCG(int adj) {
    gain_adj_cg = adj;

    int ca_ki = 20-9;	// 11
    int ca_kp = 27-4;	// 23, kp-ki = 12
    
    #define E1B_CG_GAIN_ADJ 0
    int e1b_gain_adj = gps_cg_gain? gps_cg_gain : E1B_CG_GAIN_ADJ;
    ca_ki += (isE1B? e1b_gain_adj : 0) + gain_adj_cg;
    ca_kp += (isE1B? e1b_gain_adj : 0) + gain_adj_cg;
    //printf("ch%02d %s CmdSetGainCG gain=%d ki=%d kp=%d\n", ch+1, PRN(sat), gain_adj_cg, ca_ki, ca_kp);
    spi_set(CmdSetGainCG, ch, ca_ki + ((ca_kp-ca_ki)<<16));
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CHANNEL::Reset(int sat, int codegen_init) {
    this->sat = sat;
    isE1B = is_E1B(sat);
    isSBAS = is_SBAS(sat);
    this->codegen_init = codegen_init;
    
    spi_set(CmdSetSat, ch, codegen_init);
    //printf("Reset ch%02d codegen_init=0x%03x %s\n", ch+1, codegen_init, PRN(sat));

    if (isE1B) {
        // download E1B code table
        // assumes BRAM write address is zero from when channel was last reset on signal loss
        int dbg = 0;
        SPI_MOSI *code_buf = &SPI_SHMEM->gps_e1b_code_mosi;

        for (int i=0; i < E1B_CODE_XFERS; i++) {    // number of SPIBUF_WA sized xfers needed (currently 2)
            if (dbg && i == 0) printf("E1B download ch%2d try %s\nprn: ", ch+1, PRN(sat));

            for (int j=0; j < E1B_CODE_LOOP; j++) {     // code amount needed that also fits in SPIBUF_W
                u2_t *code = &code_buf->words[j+1];     // NB: spi_mosi_data_t.cmd is in words[0]
                *code = 0;
                for (int chan = 0; chan < gps_chans; chan++) {
                    CHANNEL *c = &Chans[chan];
                    //int busy = BusyFlags & (1<<chan);
                    //int prn = (busy && c->isE1B)? Sats[c->sat].prn : 0;
                    int prn = c->isE1B? Sats[c->sat].prn : 0;
                    if (dbg && i == 0 && j == 0) printf("%d ", prn);
                    int bit = (prn > 0)? E1B_code1[prn-1][(i*E1B_CODE_LOOP)+j] : 0;
                    *code = (*code >> 1) | (bit? (1 << (gps_chans-1)): 0);  // ch0 in lsb
                    //if (0 && j == 0) printf("ch%2d busy=%d isE1B=%d prn%02d code 0x%03x\n",
                    //    chan+1, busy? 1:0, busy? c->isE1B:0, prn, *code);
                }
                if (dbg && i == 0 && j == 0) printf("\n");
                if (dbg && i == 0 && j < 16) printf("code(%4d) 0x%03x\n", j, *code);
                if (dbg && i == 1 && j >= (E1B_CODE_LOOP-16)) printf("code(%4d) 0x%03x\n", (i*E1B_CODE_LOOP)+j, *code);
            }
            spi_set_buf_noduplex(CmdSetE1Bcode, code_buf, S2B(E1B_CODE_LOOP));

            // pause so as not to potentially starve other eCPU tasks
            TaskSleepMsec(1);
        }
        //printf("**** downloaded E1B code table ch%02d %s\n", ch+1, PRN(sat));
    }
    
    // SBAS? ...
    
    uint32_t ca_rate = CPS/FS*powf(2,32);
    spi_set(CmdSetRateCG, ch, ca_rate);

    int ca_ki = 20-9;	// 11
    int ca_kp = 27-4;	// 23, kp-ki = 12
    
    #define E1B_CG_GAIN_ADJ 0
    int e1b_gain_adj = gps_cg_gain? gps_cg_gain : E1B_CG_GAIN_ADJ;
    ca_ki += isE1B? e1b_gain_adj : 0;
    ca_kp += isE1B? e1b_gain_adj : 0;
    spi_set(CmdSetGainCG, ch, ca_ki + ((ca_kp-ca_ki)<<16));

    SetGainAdjLO(0);
    SetGainAdjCG(0);

    memset(pwr, 0, sizeof pwr);
    pwr_tot=0;
    pwr_pos=0;
    probation=2;    // skip the first few valid subframes so TOW can become valid
    abort = alert = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CHANNEL::Start( // called from search thread to initiate acquisition
    int sat,
    int t_sample,
    int lo_shift,
    int ca_shift,
    int snr) {

    this->sat = sat;
    isE1B = is_E1B(sat);
    isSBAS = is_SBAS(sat);
    nav.sat = sat;
    Ephemeris[sat].Init(sat);
    subframe_bits = isE1B? E1B_TSYM_PW : (isSBAS? (NAVFLEN_SBAS + NAVADDFLEN_SBAS) : 300);
    ACF_mode = 0;

    // Estimate Doppler from FFT bin shift
    double lo_dop = lo_shift*BIN_SIZE;
    double ca_dop = (lo_dop/L1_f)*CPS;

    // NCO rates
    uint32_t lo_rate = (FC  + lo_dop)/FS*pow(2,32);
    uint32_t ca_rate = (CPS + ca_dop)/FS*pow(2,32);

    // Initialise code and carrier NCOs
    spi_set(CmdSetRateLO, ch, lo_rate);
    spi_set(CmdSetRateCG, ch, ca_rate);

    // Seconds elapsed since sample taken
    double secs = (timer_us()-t_sample) / 1e6;

    // Code creep due to code rate Doppler
    int code_creep = nearbyint((ca_dop*secs/CPS)*FS);

    // Align code generator by pausing NCO
    int code_period_ms = isE1B? E1B_CODE_PERIOD : L1_CODE_PERIOD;
    int code_period_samples = FS_I/1000 * code_period_ms;

    uint32_t ca_pause = code_period_samples - ((ca_shift+code_creep) % code_period_samples);
    uint32_t ca_pause_old = (code_period_samples*2 - (ca_shift+code_creep)) % code_period_samples;
    if (ca_pause != ca_pause_old) {
        lprintf("ca_pause %d ca_pause_old %d ca_shift=%d code_creep=%d code_period_ms=%d code_period_samples=%d\n",
            ca_pause, ca_pause_old, ca_shift, code_creep, code_period_ms, code_period_samples);
    }

    if (ca_pause > 0xffff) {
        lprintf("> 0xffff: ca_pause %d 0x%x ca_shift=%d code_creep=%d code_period_ms=%d code_period_samples=%d\n",
            ca_pause, ca_pause, ca_shift, code_creep, code_period_ms, code_period_samples);
        lprintf("> 0xffff: lo_shift=%d lo_dop=%f ca_dop=%f secs=%f\n", lo_shift, lo_dop, ca_dop, secs);
        //assert(ca_pause <= 0xffff);     // hardware limit
    }
	if (ca_pause) spi_set(CmdPause, ch, ca_pause-1);

    // Wait 3 epochs to be sure phase errors are valid before ...
    TaskSleepMsec(3);
    // ... enabling embedded PI controllers
    spi_set(CmdSetMask, BusyFlags |= 1<<ch);
    TaskWakeupF(id, TWF_CHECK_WAKING);

	GPSstat(STAT_DEBUG, secs, ch, ca_shift, code_creep, ca_pause);

#ifndef QUIET
    printf("Channel %d %s lo_dop %f lo_rate 0x%x ca_dop %f ca_rate 0x%x pause %d\n\n",
    	ch, PRN(sat), lo_dop, lo_rate, ca_dop, ca_rate, ca_pause-1);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CHANNEL::Service() {

    /* entered from channel thread after search thread has called Start() */

    Acquisition(true);
    Tracking();
    SignalLost();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CHANNEL::Acquisition(bool delay) {

    // Code loop always locks, but carrier loop sometimes needs help!

    // Carrier might fall outside Costas loop capture range because error in
    // initial Doppler estimate (FFT bin size) is larger than loop bandwidth.

    // Give them 5 seconds ...
    if (delay)
	    TaskSleepMsec(5000);

    // Get accurate Doppler measurement from locked code NCO
    UploadEmbeddedState();

	double ca_freq = GetFreq(ul.ca_freq);
	double ca_dop = ca_freq - CPS;

    // Put carrier NCO precisely on-frequency.  Now it will lock.
	double ca2lo_dop = ca_dop*L1_f/CPS;
	double f_lo_rate = (FC + ca2lo_dop)/FS;
	uint32_t lo_rate = f_lo_rate*pow(2,32);

#ifndef QUIET
	printf("Channel %d %s new lo_rate 0x%x\n", ch, PRN(sat), lo_rate);
#endif

    spi_set(CmdSetRateLO, ch, lo_rate);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CHANNEL::Tracking() {
    //#define MAXBUF (E1B_TSYM_PW + MAX_NAV_BITS - 1)
    #define MAXBUF (NAVFLEN_SBAS + NAVADDFLEN_SBAS + MAX_NAV_BITS - 1)
    char xbuf[MAXBUF];

    //jksx-sbas const int POLLING_PS = 4;  // Poll 4 times per second
    int polling_ps = isSBAS? 4 : 4;
    int POLLING_US = 1000000 / polling_ps;
    
    assert( E1B_BPS/polling_ps < MAX_NAV_BITS/2);   // make sure there is enough buffering for poll rate
    assert(SBAS_BPS/polling_ps < MAX_NAV_BITS/2);   // make sure there is enough buffering for poll rate

    int TIMEOUT = 60 * polling_ps;   // Bail after 60 seconds on LOS
    int TIMEOUT_E1B = 90 * polling_ps;

	//int LOW_BITS_TIMEOUT = 60 * polling_ps;
	int LOW_BITS_TIMEOUT = 90 * polling_ps;
	const int LOW_BITS = 96;

	int LOW_RSSI_TIMEOUT = 30 * polling_ps;
	int LOW_RSSI_E1B_TIMEOUT = 15 * polling_ps;
    const float LOW_RSSI = 300*300;

	int KICK_PLL_RSSI_INTERVAL = 16 * polling_ps;
    const float KICK_PLL_RSSI = 500*500;

	float sumpwr=0;
    holding=0;
    tholding=0;

	evGPS(EC_TRIG3, EV_GPS, ch, "GPS", "trig3 Tracking1");
	static int firsttime[16];
	firsttime[ch]=1;
	nsync = 0;
	total_bits = 0;
	expecting_preamble = 0;
	drop_seq = 0;
	int seen_data = 0;
	u4_t t_last_data = 0;
    gps.ch[ch].ACF_mode = ACF_mode = 0;

    if (isE1B) {
        //int polys[2] = { 0x4f, -0x6d };       // k=7; Galileo E1B; "-" means G2 inverted
        int polys[2] = { 0x4f, 0x6d };          // k=7; Galileo E1B
        set_viterbi27_polynomial_port(polys);
        nav.fec = create_viterbi27_port(E1B_NBIT);

        spi_set(CmdSetPolarity, ch, 0);
    }

    if (isSBAS) {
        nav.ctype = nav.sdreph.ctype = CTYPE_L1SBAS;
        nav.sbas_seq = 0;
        nav.sdreph.prn = Sats[sat].prn;
        nav.sdreph.eph.iodc = -1;

        nav.rate = NAVRATE_SBAS;
        nav.flen = NAVFLEN_SBAS;
        nav.addflen = NAVADDFLEN_SBAS;
        nav.prelen = NAVPRELEN_SBAS;
        nav.sdreph.cntth = NAVEPHCNT_SBAS;
        nav.update = (int)(nav.flen/3 * nav.rate);
        int pre_sbs[24]= { 1,-1, 1,-1, 1, 1,-1,-1,
                          -1, 1, 1,-1,-1, 1,-1, 1,
                          -1,-1, 1, 1, 1,-1,-1, 1}; /* SBAS L1/QZS L1SAIF preamble */
        memcpy(nav.prebits, pre_sbs, sizeof(int) * nav.prelen);

        int polys[2] = { 0x4f, 0x6d };          // k=7; Galileo E1B
        set_viterbi27_polynomial_port(polys);
        nav.fec = create_viterbi27_port(NAVFLEN_SBAS/2);

        /* overlay code (all 1) */
        nav.ocode = (short *)calloc(nav.rate, sizeof(short));
        for (int i = 0; i < nav.rate; i++) nav.ocode[i] = 1;

        nav.bitsync =  (int *)  calloc(nav.rate, sizeof(int));
        nav.fbits =    (char *) calloc(nav.flen + nav.addflen, sizeof(int));
        nav.fbitsdec = (int *)  calloc(nav.flen + nav.addflen, sizeof(int));
    }

    //#define GPS_SHOW_BPS
    #ifdef GPS_SHOW_BPS
        #define T_INTERVAL 10
        #define T_INTERVAL_MS (T_INTERVAL * 1000)
        u4_t t_trig = timer_ms() + T_INTERVAL_MS;
    #endif
    
    for (int watchdog=0; watchdog < (isE1B? TIMEOUT_E1B:TIMEOUT) && !abort; watchdog++) {
    
	    //evGPS(EC_EVENT, EV_GPS, ch, "GPS", evprintf("TaskSleepMsec(250) ch %d", ch+1));
        TaskSleepUsec(POLLING_US);
        UploadEmbeddedState();
        TaskStat(TSTAT_INCR|TSTAT_ZERO, 0, "trk");

        // Process NAV data
        //for(int avail = RemoteBits(ul.nav_bits) & ~0xF; avail; avail-=16) {
        int avail = RemoteBits(ul.nav_bits) & ~0xF;
        evGPS(EC_EVENT, EV_GPS, ch, "GPS", evprintf("Tracking ch %d avail %d holding %d W/D %d", ch+1, avail, holding, watchdog/polling_ps));
        evGPS(EC_TRIG3, EV_GPS, ch, "GPS", "trig3 Tracking2");

		//if (!firsttime[ch] && avail >= (MAX_NAV_BITS - 16)) {
		if (avail >= (MAX_NAV_BITS - 16)) {
			GPSstat(STAT_NOVFL, 0, ch);
			//evGPS(EC_DUMP, EV_GPS, -1, "GPS", evprintf("MAX_NAV_BITS ch %d", ch+1));
		}
		firsttime[ch]=0;
		
        for(; avail; avail-=16) {
            int word = ul.nav_buf[rd_pos/16];
            for (int i=0; i<16; i++) {
                word <<= 1;
                xbuf[holding++] = (word >> 16) & 1;
                tholding++;
            }
            rd_pos += 16;
            rd_pos &= MAX_NAV_BITS-1;
			bits_tow += 16;
        }
        
        #ifdef GPS_SHOW_BPS
            if (ch == 0) {
                if (timer_ms() >= t_trig) {
                    printf("%s: %d bps\n", PRN(sat), tholding/T_INTERVAL);
                    tholding = 0;
                    t_trig += T_INTERVAL_MS;
                }
            }
        #endif

        while (holding >= subframe_bits) {      // Enough for a subframe?
            int nbits, err;
            
if (isSBAS) {
#if 0
    int bad = 0;
    for (int z=0; z<500; z+=2) {
        if (xbuf[z] == xbuf[z+1]) {
            //real_printf(GREEN "%d", xbuf[z]);
        } else {
            //real_printf(RED "%d", xbuf[z]);
            bad++;
        }
    }
    //real_printf(NONL);
    real_printf("ch%d %s hold=%d bad=%02d %s\n", ch, PRN(sat), holding, bad, (bad < 3)? (CYAN "--OK--" NORM) : "");
#endif
#if 0
    for (int z=0; z<500; z++) {
        real_printf(GREEN "%d", xbuf[z]);
    }
    real_printf(NONL);
    real_printf("ch%d %s hold=%d\n", ch, PRN(sat), holding);
#endif
}
            if ((err = ParityCheck(xbuf, &nbits)) == 0) {
				//printf("%s %s(%d) nbits=%d\n", PRN(sat), gps_err_s[err], err, nbits);
				watchdog=0; sumpwr=0;
			} else {
				if (0 && err > GPS_ERR_PREAMBLE) {
				    printf("%s %s(%d) nbits=%d\n", PRN(sat), gps_err_s[err], err, nbits);
				}
				if (err == GPS_ERR_CRC) {
				    GPSstat(STAT_SUB, 0, ch, PARITY);
				}
			}
			
			// ParityCheck() return nbits = 1 to shift buf by 1 bit until a preamble match
            //printf("ch%02d %s hold %d nbits %d\n", ch+1, PRN(sat), holding, nbits);

            //jks2
            if (err == 0) {
                assert(nbits == subframe_bits);
                expecting_preamble = 1;
                int sub = Ephemeris[sat].sub;
                LASTsub = sub;
                if (0) {
                    int tow_pg = Ephemeris[sat].tow_pg;
                    int tow = Ephemeris[sat].tow;
                    printf("%s PRE  TOW %d(%d) %s%d|%d hold %d hold-subframe_bits %d bits_tow %d %.3f %.1f\n",
                        PRN(sat), tow/6, tow, isE1B? "pg":"sf", sub, tow_pg, holding, holding-subframe_bits, bits_tow, (float) bits_tow/50, (float) bits_tow/subframe_bits);
                }
                //printf("%s set  lsf%d\n", PRN(sat), LASTsub);
                if (LASTsub == 1) {
                    //printf("%s SF1  hold %d\n", PRN(sat), holding);
                }
                if (seen_data <= 20) seen_data++;
                t_last_data = timer_sec();
                
                //#define E1B_UNAMBIGUOUS_TRACKING
                #ifdef E1B_UNAMBIGUOUS_TRACKING
                    if (isE1B && ACF_mode == 0 && seen_data > 10) {
                        printf("ch%02d %s ACF SET inverted %d ##############################################\n", ch+1, PRN(sat), inverted);
                        //SetGainAdjCG(-1);
                        spi_set(CmdSetPolarity, ch, inverted+1);
                        gps.ch[ch].ACF_mode = ACF_mode = inverted+1;
                    }
                    
                    if (isE1B && ACF_mode && ACF_mode != inverted+1) {
                        printf("ch%02d %s ACF FLIP inverted %d =============================================\n", ch+1, PRN(sat), inverted);
                        spi_set(CmdSetPolarity, ch, inverted+1);
                        gps.ch[ch].ACF_mode = ACF_mode = inverted+1;
                    }
                #endif
            } else {
                //if (nbits != 1) printf("%s error %d\n", PRN(sat), nbits);
            }
            
            // Not immediately obvious, but crucial that the update of TOW in Ephemeris[] and
            // decrement of holding occur atomically. Otherwise GetSnapshot() / LoadAtomic()
            // won't have synchronized information for GetClock().
            
            memmove(xbuf, xbuf + nbits, holding -= nbits);
			GPSstat(STAT_WDOG, 0, ch, watchdog/polling_ps, holding, ul.ca_unlocked);
			
			if (ch+1 == gps.kick_lo_pll_ch) {
			    printf("%s manual-KICK ch%02d LO PLL =========================================================\n",
			        PRN(sat), ch+1);
			    Acquisition(false);
			    gps.kick_lo_pll_ch = 0;
			}
        }

		Status();
#ifndef QUIET
		printf(" wdog %d\n", watchdog/polling_ps);
#endif
    	GPSstat(STAT_WDOG, 0, ch, watchdog/polling_ps, holding, ul.ca_unlocked);
    	//GPSstat(STAT_WDOG, 0, ch, watchdog/polling_ps, holding, 0);
    	//GPSstat(STAT_EPL, 0, ch, (ul.pe[1]<<16) | ul.pe[0], (ul.pp[1]<<16) | ul.pp[0], (ul.pl[1]<<16) | ul.pl[0]);
        CheckPower();
        
        sumpwr += GetPower();
        float avgpwr = sumpwr/(watchdog+1);
        
        //if ((watchdog >= LOW_RSSI_TIMEOUT) && isE1B)
        //    printf("%s LRT %d avgpwr %f LOW_RSSI %f GetPower() %f\n", PRN(sat), watchdog, sqrt(avgpwr), sqrt(LOW_RSSI), sqrt(GetPower()));

        if ((watchdog >= (isE1B? LOW_RSSI_E1B_TIMEOUT : LOW_RSSI_TIMEOUT)) && (avgpwr <= LOW_RSSI))
        	break;

        if ((watchdog >= LOW_BITS_TIMEOUT) && (holding <= LOW_BITS) && !isE1B)
        	break;

        if (!seen_data && avgpwr > KICK_PLL_RSSI && (watchdog % KICK_PLL_RSSI_INTERVAL) == KICK_PLL_RSSI_INTERVAL-1) {
            //printf("%s auto-KICK ch%02d LO PLL avgpwr=%d wdog=%d ================================================\n",
            //    PRN(sat), ch+1, sqrt(avgpwr), watchdog/polling_ps);
            Acquisition(false);
        }
        
        #if 0
            if (timer_sec() - t_last_data > 15) {
                GPSstat(STAT_SUB, 0, ch, -1);       // blank if nothing received recently
                t_last_data = timer_sec();
                //printf("%s nothing recently\n", PRN(sat));
            }
        #endif
    }

    if (isE1B || isSBAS) delete_viterbi27_port(nav.fec);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CHANNEL::SignalLost() {
    // Re-enable search for this sat
    SearchEnable(sat);

    // Disable embedded PI controllers
    BusyFlags &= ~(1<<ch);
    spi_set(CmdSetMask, BusyFlags);

    GPSstat(STAT_POWER, -1, ch); // Flatten bar graph
    isE1B = isSBAS = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

int CHANNEL::RemoteBits(uint16_t wr_pos) { // NAV bits held in FPGA circular buffer
	evGPS(EC_EVENT, EV_GPS, ch, "GPS", evprintf("RemoteBits ch %d W %d R %d diff %d",
		ch+1, wr_pos, rd_pos, (MAX_NAV_BITS-1) & (wr_pos-rd_pos)));
	evGPS(EC_EVENT, EV_GPS, ch, "GPS", evprintf("RemoteBits ch %d ul %p bits %d glitches %d buf %d %d %d %d",
		ch+1, &ul, ul.nav_bits, ul.nav_glitch,
		ul.nav_buf[0], ul.nav_buf[1], ul.nav_buf[2], ul.nav_buf[3]));
    return (MAX_NAV_BITS-1) & (wr_pos-rd_pos);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CHANNEL::CheckPower() {
    // Running average of received signal power
    float pi = S32_16_16(ul.iq[0][1], ul.iq[0][0]);
    float pq = S32_16_16(ul.iq[0][3], ul.iq[0][2]);
    float pp = powf(pi, 2) + powf(pq, 2);

    if (isE1B && GPS_INTEG_BITS == 16) pp /= 2;   // FIXME: why?

    #if 0 && GPS_INTEG_BITS != 16
        if (ch == gps.IQ_data_ch-1) {
            int pp_i = sqrtf(pp);
            int pe_i = sqrt(Get64_frac(ul.iq[1]) * powf(2, 64));    // FIXME: divide I/Q by 4 instead?
            int pl_i = sqrt(Get64_frac(ul.iq[2]) * powf(2, 64));
            if (isE1B) pe_i /= 2, pl_i /= 2;
            //int pe_i = sqrt(powf(Get64_frac(ul.iq[1]), 2) / 4 * powf(2, 64));
            //int pl_i = sqrt(powf(Get64_frac(ul.iq[2]), 2) / 4 * powf(2, 64));
            int npe_i = pe_i*100/pp_i;
            int npl_i = pl_i*100/pp_i;
            double lo_f = GetFreq(ul.lo_freq) - FC;
            double ca_f = GetFreq(ul.ca_freq) - CPS;
            static double last_lo_f, last_ca_f;
            float lo_e = (lo_f-last_lo_f)/last_lo_f*100.0;
            float ca_e = (ca_f-last_ca_f)/last_ca_f*100.0;
            printf("ch%02d %s  LO %8.3f (%6.1f%%) CA %9.6f (%6.1f%%)  EPL %4d %4d %4d | %3d %3d %3d (%3d)",
                ch+1, PRN(sat),
                lo_f, lo_e, ca_f, ca_e,
                pe_i, pp_i, pl_i, npe_i, 100, npl_i, npe_i-npl_i);
            if (pe_i >= pp_i || pl_i >= pp_i) printf(" UNLOCKED");
            printf("\n");
            last_lo_f = lo_f;
            last_ca_f = ca_f;
        }
    #endif

    pwr_tot -= pwr[pwr_pos];
    pwr_tot += pwr[pwr_pos++] = pp;
    pwr_pos %= PWR_LEN;

    float mean = GetPower();

    // Carrier loop gain proportional to signal power (k^2).
    // Loop unstable if gain not reduced for strong signals

    // AGC hysteresis thresholds
    const float HYST1_LO = 1200*1200;
    const float HYST1_HI = 1400*1400;

    const float HYST2_LO = 1600*1600;
    const float HYST2_HI = 1800*1800;
    
    int gain = GetGainAdjLO();
    if (gain == 0) {
        if (mean > HYST2_HI) SetGainAdjLO(-2);      // quarter loop gain
        else
        if (mean > HYST1_HI) SetGainAdjLO(-1);      // half loop gain
    } else
    if (gain == -1) {
        if (mean < HYST1_LO) SetGainAdjLO(0);       // default
        else
        if (mean > HYST2_HI) SetGainAdjLO(-2);      // quarter loop gain
    }
    else {  // gain == -2
        if (mean < HYST1_LO) SetGainAdjLO(0);       // default
        else
        if (mean < HYST2_LO) SetGainAdjLO(-1);      // half loop gain
    }

    GPSstat(STAT_POWER, mean, ch, GetGainAdjLO());
}

float CHANNEL::GetPower() {
    return pwr_tot / PWR_LEN;
}

///////////////////////////////////////////////////////////////////////////////////////////////

static unsigned subframe_dump;

void CHANNEL::Subframe(char *buf) {
	unsigned sub = bin(buf+49,3);

    alert = bin(buf+47,1);
    
    //#define SUBFRAME_DEBUG
    #if defined(SUBFRAME_DEBUG) || !defined(QUIET)
        unsigned page = bin(buf+62,6);
    #endif

    #ifndef	QUIET
        printf("%s sub %d ", PRN(sat), sub);
        if (sub > 3) printf("page %02d", page);
        printf("\n");
    #endif

    #ifdef SUBFRAME_DEBUG
        if (gps_debug) {
            unsigned tlm = bin(buf+8,14);
            unsigned tow = bin(buf+30,17);
            static unsigned last_good_tlm, last_good_tow;
            static bool gps_debugging;
            static int sub_seen[SUBFRAMES+1];
            
            if (!gps_debugging) {
                lprintf("GPS: subframe debugging enabled\n");
                gps_debugging = true;
            }
            
            if (sub < 1 || sub > SUBFRAMES) {
                lprintf("GPS: unknown subframe %d %s preamble 0x%02x[0x8b] tlm %d[%d] tow %d[%d] alert/AS %d data-id %d page-id %d novfl %d tracking %d good %d frames %d par_errs %d\n",
                    sub, PRN(sat), bin(buf,8), tlm, last_good_tlm, tow, last_good_tow, bin(buf+47,2), bin(buf+60,2), page, gps.ch[ch].novfl, gps.tracking, gps.good, gps.ch[ch].frames, gps.ch[ch].par_errs);
                for (int i=0; i<10; i++) {
                    lprintf("GPS: w%d b%3d %06x %02x\n", i, i*30, bin(buf+i*30,24), bin(buf+i*30+24,6));
                }
                //subframe_dump = 5 * 25;   // full 12.5 min cycle
                subframe_dump = 5 * 2;      // two subframe cycles
                return;
            }
            
            if (subframe_dump) {
                if (!sub_seen[sub]) {
                    lprintf("GPS: dump #%2d subframe %d page %2d %s novfl %d tracking %d good %d frames %d par_errs %d\n",
                        subframe_dump, sub, (sub > 3)? page : -1, PRN(sat), gps.ch[ch].novfl, gps.tracking, gps.good, gps.ch[ch].frames, gps.ch[ch].par_errs);
                    sub_seen[sub] = 1;
                    int prev = (sub == 1)? 5 : (sub-1);
                    sub_seen[prev] = 0;
                    subframe_dump--;
                }
            }
            
            last_good_tlm = tlm;
            last_good_tow = tow;
        } else
    #else
        { if (sub < 1 || sub > SUBFRAMES) return; }
    #endif

	GPSstat(STAT_SUB, 0, ch, sub, alert? (gps.include_alert_gps? 2:1) : 0);
}

void CHANNEL::Status() {
    double lo_f = GetFreq(ul.lo_freq) - FC;
    double ca_f = GetFreq(ul.ca_freq) - CPS;

	GPSstat(STAT_LO, lo_f, ch);
	GPSstat(STAT_CA, ca_f, ch);
    #ifndef QUIET
        printf("chan %d %s rssi %4.0f adj %2d freq %5.6f %6.6f ", ch, PRN(sat), sqrt(GetPower()), gain_adj_lo, lo_f, ca_f);
    #endif
}

///////////////////////////////////////////////////////////////////////////////////////////////

int CHANNEL::ParityCheck(char *buf, int *nbits) {
    char p[6];
    int id, error = GPS_ERR_NONE;

    if (isSBAS) {
        //nav.polarity = ???;
        nav.fbits = buf;
        nav.tow_updated = 0;
        id = SBAS_subframe(&nav, 0, 0, &error, &gps.ch[ch].sbas_status);
        if (id == -1) {
            //if (error == GPS_ERR_CRC) printf("%s SBAS-GPS_ERR_CRC\n", PRN(sat));
            *nbits = (error == GPS_ERR_PREAMBLE)? 1 : NAVFLEN_SBAS;
        } else {
            GPSstat(STAT_SUB, 0, ch, nav.sbas_seq+1, 0);
            nav.sbas_seq++;
            nav.sbas_seq = nav.sbas_seq % 5;
            *nbits = NAVFLEN_SBAS;
        }
        return error;
    } else
    
    if (isE1B) {
        // Upright or inverted preamble?  Resolves phase ambiguity.
        #ifdef TEST_VECTOR
            inverted = 0;
        #else
            if      (memcmp(buf, E1BpreambleUpright, E1B_PRELEN) == 0 &&
                     memcmp(buf+E1B_TSYM_PP, E1BpreambleUpright, E1B_PRELEN) == 0) inverted = 0;
            else if (memcmp(buf, E1BpreambleInverse, E1B_PRELEN) == 0 &&
                     memcmp(buf+E1B_TSYM_PP, E1BpreambleInverse, E1B_PRELEN) == 0) inverted = 1;
            else {
                *nbits = 1;
                return GPS_ERR_PREAMBLE;
            }
        #endif
        
        if (nsync == 0) total_bits = 0;
        //printf("ch%02d %s SYNC-%c #%d %d (%d)\n", ch+1, PRN(sat), inverted? 'I':'N',
        //    nsync++, total_bits, total_bits+subframe_bits);
        //printf("ch%02d %s SYNC-%c #%d ", ch+1, PRN(sat), inverted? 'I':'N', nsync++);
        
        nav.polarity = inverted? -1:1;
        nav.flen = 500;
        nav.fbits = buf;
        nav.tow_updated = 0;
        id = E1B_subframe(&nav, &error);
        //if (error) printf("%s E1B_subframe %s(%d)\n", PRN(sat), gps_err_s[error], error);
        if (error) probation = 2;
        if (error == GPS_ERR_SLIP) {
            *nbits = E1B_TSYM_PP;    // need to slip by a page (half a word)
            return GPS_ERR_SLIP;
        }
        if (error && (error != GPS_ERR_ALERT && error != GPS_ERR_OOS)) {
            //if (isSat(E1B,36)) real_printf("e%d:%d ", error, id); fflush(stdout);
            *nbits = subframe_bits;     // any other errors (GPS_ERR_CRC etc)
            return error;
        }

        if (nav.tow_updated)    // page had TOW so reset bit counter
            bits_tow = holding - subframe_bits;

        //if (isSat(E1B,36)) real_printf("%d ", id); fflush(stdout);
        Status();
        if (id == 5) alert = (error == GPS_ERR_ALERT || error == GPS_ERR_OOS);
        if (id >= 1 && id <= 5) GPSstat(STAT_SUB, 0, ch, id, alert? (gps.include_alert_gps? 2:1) : 0);

        // show alert for 8 seconds (time between id 5 and 4 update) before dropping channel
        if (alert && id == 4) {
            GPSstat(STAT_SUB, 0, ch, id, 0);    // remove alert indicator
            abort = true;
        }

        if (probation) probation--;
        *nbits = subframe_bits;
        return error;
    } else {
        // L1 C/A
        
        // Upright or inverted preamble?  Setting of parity bits resolves phase ambiguity.
        if      (memcmp(buf, L1preambleUpright, L1_PRELEN) == 0) p[4]=p[5]=0;
        else if (memcmp(buf, L1preambleInverse, L1_PRELEN) == 0) p[4]=p[5]=1;
        else {
            if (expecting_preamble) {
                if (0 && expecting_preamble == 1)
                    printf("%s EXPECTED PREAMBLE\n", PRN(sat));
                expecting_preamble++;
            }
            *nbits = 1;
            return GPS_ERR_PREAMBLE;
        }
    
        #if 0
            if (gps_debug && Sats[sat].prn == abs(gps_debug) && LASTsub == 3) {
                printf("%s SF4 seq %d\n", PRN(sat), drop_seq);
                drop_seq++;
                LASTsub = 4;
                if (drop_seq & 1) {
                    printf("%s drop SF4 seq %d\n", PRN(sat), drop_seq);
                    *nbits = 1;
                    return GPS_ERR_DEBUG;
                }
            }
        #endif
        
        expecting_preamble = 0;
        
        // Parity check up to ten 30-bit words ...
        for (int i = 0; i < subframe_bits; i += 30) {
            if (L1_parity(p, buf+i, p[4], p[5])) {
                Status();
    #ifndef QUIET
                puts("parity");
    #endif
                // nullify probation effect from parity error during preamble search
                // forced by our dropped preamble bit simulation
                if (!(gps_debug && Sats[sat].prn == abs(gps_debug) && LASTsub == 4))
                probation=2;
				//printf("%s Nav-PARITY nbits=%d\n", PRN(sat), i+30);
                *nbits = i+30;
                return GPS_ERR_CRC;
            }
        }

        Status();
        Subframe(buf);
        Ephemeris[sat].Subframe(buf);
        if (probation) probation--;
        bits_tow = holding - subframe_bits;
        *nbits = subframe_bits;
        return GPS_ERR_NONE;
    }

    return error;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CHANNEL::GetSnapshot(
    uint16_t wr_pos,    // in: circular nav_buf pointer
    int *p_sat,         // out: Ephemeris[] index
    int *p_bits,        // out: total NAV bits held locally + remotely
    int *p_bits_tow,    // out: bits since last valid TOW
    float *p_pwr) {     // out: signal power for least-squares weighting
    
    if (alert && !gps.include_alert_gps) return false; // subframe alert flag
    if (probation) return false; // temporarily too noisy
    
    #ifdef E1B_UNAMBIGUOUS_TRACKING
        if (isE1B && ACF_mode == 0) {
            //printf("%s ACF_mode %d\n", PRN(sat), ACF_mode);
            return false;
        }
    #endif

    *p_sat  = sat;
    int remote = RemoteBits(wr_pos);
    *p_bits = holding + remote;
    *p_bits_tow = bits_tow + remote;
    *p_pwr  = GetPower();

    return true; // ok to use
}

bool ChanSnapshot( // called from solver thread
    int ch, uint16_t wr_pos, int *p_sat, int *p_bits, int *p_bits_tow, float *p_pwr) {

    if (BusyFlags & (1<<ch))
        return Chans[ch].GetSnapshot(wr_pos, p_sat, p_bits, p_bits_tow, p_pwr);
    else
        return false; // channel not enabled
}

///////////////////////////////////////////////////////////////////////////////////////////////

void ChanTask(void *param) { // one thread per channel
    static int inst;
    int ch = (int) FROM_VOID_PARAM(param); // which channel am I?
    Chans[ch].ch = ch;
    unsigned bit = 1<<ch;
    Chans[ch].id = TaskID();
    Chans[ch].miso = &SPI_SHMEM->gps_channel_miso[ch];

    for (;;) {
    	TaskSleep();
    	gps.tracking++;
        if (BusyFlags & bit) Chans[ch].Service(); // returns after loss of signal
    	gps.tracking--;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

int ChanReset(int sat, int codegen_init) {  // called from search thread before sampling
    int ch, nbusy, cur_QZSS;
    
    bool QZSS_JA = (gps.acq_QZSS && gps.QZSS_prio);

    if (QZSS_JA) for (ch = nbusy = cur_QZSS = 0; ch < gps_chans; ch++) {
        if (!(BusyFlags & (1<<ch))) continue;
        if (Sats[Chans[ch].sat].type == QZSS) cur_QZSS++;
        nbusy++;
    }

    #define QZSS_RESERVED 2
    int nfree = gps_chans - nbusy;
    int nresv = MIN(QZSS_RESERVED, gps.n_QZSS - cur_QZSS);
    bool QZSS_limit = (QZSS_JA && nfree <= nresv);

    for (ch = 0; ch < gps_chans; ch++) {
        if (BusyFlags & (1<<ch)) continue;

        // if QZSS_JA mode enabled don't let non-QZSS get last QZSS_RESERVED channels (reduced by number currently active)
        if (QZSS_limit && Sats[sat].type != QZSS) {
            //printf("QZSS_RESERVED ch%02d %s nfree=%d nresv=%d\n", ch, PRN(sat), nfree, nresv);
            return -1;
        }

        Chans[ch].Reset(sat, codegen_init);
        return ch;
    }

    return -1; // all channels busy
}

///////////////////////////////////////////////////////////////////////////////////////////////

void ChanStart( // called from search thread to initiate acquisition of detected sat
    int ch,
    int sat,
    int t_sample,
    int lo_shift,
    int ca_shift,
    int snr) {

    Chans[ch].Start(sat, t_sample, lo_shift, ca_shift, snr);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void ChanRemove(sat_e type, int prn) {

    for (int ch = 0; ch < gps_chans; ch++) {
        if (!(BusyFlags & (1<<ch))) continue;
        if (Sats[Chans[ch].sat].type != type) continue;
        if (prn != -1 && prn != Sats[Chans[ch].sat].prn) continue;
        Chans[ch].abort = true;
        printf("GPS: ch%d ABORT %s\n", ch, PRN(Chans[ch].sat));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

void ChanInit() {
    sdrnav_init();
}
