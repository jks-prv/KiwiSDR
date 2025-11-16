/*------------------------------------------------------------------------------
* sdrnav_sbs.c : SBAS message
*
* Copyright (C) 2014 Taro Suzuki <gnsssdrlib@gmail.com>
*-----------------------------------------------------------------------------*/

#include "kiwi.h"
#include "gps.h"
#include "ephemeris.h"
#undef B
#undef K
#undef M
#undef I
#undef Q
#include "gnss_sdrlib.h"

#define GETBITP14U(b,o,l)   getbitu(b,o+14,l)   // 14 = skip preamble(8) and msg type(6)


extern void SBAS_init()
{
}

/* decode SBAS message ---------------------------------------------------------
*
*-----------------------------------------------------------------------------*/

// UDRE: user defined range error
//
// IOD: issue-of-data
//  IODP: IOD PRN mask
//  IODN: IOD Navigation
//  IODG: IOD GEO (GPS/GLONASS)
//  IODF: IOD fast corr
//  IODC: IOD GPS clock
//  IODE: IOD GPS eph
//  IODI: IOD iono grid point mask
//  IODS: IOD Service

void decode_msg_sbas(sdrnav_t *nav, int *status)
{
    int i, j;
    sdrsbas_t *sbas = &nav->sbas;
    uint8_t *b = sbas->msg;

    // sbas message type, 8+6+(212)+24 = 250
    int mt = sbas->id = getbitu(b,8,6);

    switch (mt) {
    
    case 1: {       // PRN mask, 210+2 = 212
        int iodp = GETBITP14U(b,210,2);
        break;
    }

    case 2: {       // fast corr, mask 1-13, 2+2+(13*12)+(13*4) = 212
        int iodf = GETBITP14U(b,0,2);
        int iodp = GETBITP14U(b,2,2);
        for (i = 0; i < 13; i++) {
            //prc[i] = GETBITP14U(b,4+i*12,2);
            //udrei[i] = GETBITP14U(b,4+13*12+i*12,2);
        }
        break;
    }

    case 12: {      // SBAS network time
        int tow  = GETBITP14U(b,107,20);
        int week = GETBITP14U(b,127,10);
        if (sbas->tow && sbas->week) {
            sbas->tow = tow + 1.0;
            sbas->week = week + 1024*2;     // FIXME: rollover count
        }
        if (gps.sbas_log) {
            printf("%s MT12 time: ", PRN(nav->sat));
            if (tow && week)
                printf("tow=%.1f week=%d\n", sbas->tow, sbas->week);
            else
                printf("not available\n");
        }
        break;
    }

    case 17: {      // GEO almanacs
        for (i = 0; i < 3; i++) {
            j = i*67;
            int id = GETBITP14U(b,j+0,2);
            int prn = GETBITP14U(b,j+2,8);
            int stat = GETBITP14U(b,j+10,8);
            if (prn == 0) continue;
            const char *service_provider_s[] = {
                "WAAS", "EGNOS", "MSAS", "3", "4", "5", "6", "7",
                "SPAN", "9", "10", "11", "12", "13", "14", "15"
            };
            if (gps.sbas_log)
                printf("%s MT17 status: #%d S%d ranging=%d corrections=%d integrity=%d service=%s\n",
                    PRN(nav->sat), i, prn,
                    (stat & 1)? 0:1, (stat & 2)? 0:1, (stat & 4)? 0:1,
                    service_provider_s[stat >> 4]);
        }
        #if 0
            for (i = 0; i < 26; i++)
                printf("%d:%02x ", i, GETBITP14U(b,i*8,8));
            printf("\n");
        #endif
        break;
    }

    default:
        sbas->tow += 1.0;
        break;
    }

    if (*status == 0 && mt >= 2 && mt <= 5)
        *status = GPS_STAT_GRN;
    if (mt == 0)
        *status = GPS_STAT_BLU;
}

extern int SBAS_subframe(sdrnav_t *nav, uint64_t buffloc, uint64_t cnt, int *error, int *status)
{
    int mt = -1;
    if (error) *error = GPS_ERR_PREAMBLE;
    nav->swsync = ON;
        
    /* check navigation frame synchronization */
    if (nav->swsync) {
        if (!nav->flagtow) {
            predecodefec(nav);      // FEC (foward error correction) decoding
            NextTask("SBAS_subframe");
            nav->flagsyncf = findpreamble(nav);     // frame synchronization (preamble search)
            NextTask("SBAS_subframe");
        }

        /* preamble is found */
        if (nav->flagsyncf && !nav->flagtow) {
            /* set reference sample data */
            nav->firstsf = buffloc;
            nav->firstsfcnt = cnt;
            //SDRPRINTF("*** find preamble! %s %d %d ***\n",
            //    PRN(nav->sat), (int)cnt, nav->polarity);
            nav->flagtow=ON;
        }
    }
    
    /* decoding navigation data */
    if (nav->flagtow && nav->swsync) {
        /* if frame bits are stored */
        if ((int)(cnt - nav->firstsfcnt) % nav->update == 0) {
            predecodefec(nav); /* FEC decoding */
            NextTask("SBAS_subframe");
            mt = decode_l1sbas(nav, error, status); /* navigation message decoding */
            
            #define GPS_SBAS_LIST_IDS
            #ifdef GPS_SBAS_LIST_IDS
                int prn = Sats[nav->sat].prn;
                if (gps.sbas_log && mt != -1) {
                    static bool seen[NUM_SBAS_SATS][64];
                    int z = prn - 120;
                    if ( z < NUM_SBAS_SATS && !seen[z][mt]) {
                        seen[z][mt] = true;
                        printf("%s MTs seen: ", PRN(nav->sat));
                        for (int x=0; x < 64; x++)
                            if (seen[z][x]) printf("%d ", x);
                        printf("\n");
                    }
                    //printf("%s: MT%d\n", PRN(nav->sat), mt);
                }
            #endif

            /* set reference tow data */
            if (nav->sdreph.tow_gpst==0) {
                /* reset if tow does not decoded */
                nav->flagsyncf=OFF;
                nav->flagtow=OFF;
            } else if (cnt - nav->firstsfcnt == 0) {
                nav->flagdec=ON;
                nav->sdreph.eph.sat = nav->sat; /* satellite number */
                nav->firstsftow = nav->sdreph.tow_gpst; /* tow */
            }
        }
    }
    
    return mt;
}

/* decode SBAS navigation data -------------------------------------------------
* decode SBAS navigation data and extract message
* args   : sdrnav_t *nav    I/O sdr navigation struct
* return : int                  word type
*-----------------------------------------------------------------------------*/
extern int decode_l1sbas(sdrnav_t *nav, int *error, int *status)
{
    int i, crc, crcmsg, bits[250];
    uint8_t bin[29] = {0}, pbin[3];

    /* copy navigation bits (250 bits/sec) */
    for (i = 0; i < 250; i++) bits[i] = nav->polarity * nav->fbitsdec[i];
    
    bits2byte(&bits[0], 226, 29, 1, bin);  /* body bits (right alignment for crc) */
    bits2byte(&bits[226], 24, 3, 0, pbin); /* crc24 */

    /* compute and check crc24 */
    crc = crc24q(bin, 29);
    crcmsg = getbitu(pbin, 0, 24);
    if (crc != crcmsg) {
        //SDRPRINTF("%s parity crc=%d msg=%d\n", PRN(nav->sat), crc, crcmsg);
        if (error) *error = GPS_ERR_CRC;
        return -1;
    }

    /* decode sbas message */
    bits2byte(bits, 250, 32, 0, nav->sbas.msg);
    decode_msg_sbas(nav, status);

    #ifdef KIWI
    #else
        /* tentative: get tow from other channel */
        if (sdrini.nch>1&&sdrch[sdrini.nch-2].nav.sdreph.week_gpst!=0) {
            nav->sbas.tow = sdrch[sdrini.nch-2].trk.tow[0];
            nav->sbas.week = sdrch[sdrini.nch-2].nav.sdreph.week_gpst;
        }
    #endif

    if (nav->sbas.week!=0) {
        nav->sdreph.tow_gpst = nav->sbas.tow;
        nav->sdreph.week_gpst = nav->sbas.week;
    }

    if (error) *error = GPS_ERR_NONE;
    return nav->sbas.id;
}
