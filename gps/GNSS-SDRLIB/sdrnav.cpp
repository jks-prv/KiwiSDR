/*------------------------------------------------------------------------------
* sdrnav.c : navigation data decode functions
*
* Copyright (C) 2014 Taro Suzuki <gnsssdrlib@gmail.com>
*-----------------------------------------------------------------------------*/
#include "gnss_sdrlib.h"

/* extract unsigned/signed bits ------------------------------------------------
* extract unsigned/signed bits from byte data (two components case)
* args   : uint8_t *buff    I   byte data
*          int    p1        I   first bit start position (bits)
*          int    l1        I   first bit length (bits)
*          int    p2        I   second bit start position (bits)
*          int    l2        I   seconf bit length (bits)
* return : extracted unsigned/signed bits
*-----------------------------------------------------------------------------*/
extern uint32_t getbitu2(const uint8_t *buff, int p1, int l1, int p2, int l2)
{
    return (getbitu(buff,p1,l1)<<l2)+getbitu(buff,p2,l2);
}
extern int32_t getbits2(const uint8_t *buff, int p1, int l1, int p2, int l2)
{
    if (getbitu(buff,p1,1))
        return (int32_t)((getbits(buff,p1,l1)<<l2)+getbitu(buff,p2,l2));
    else
        return (int32_t)getbitu2(buff,p1,l1,p2,l2);
}
/* extract unsigned/signed bits ------------------------------------------------
* extract unsigned/signed bits from byte data (three components case)
* args   : uint8_t *buff    I   byte data
*          int    p1        I   first bit start position (bits)
*          int    l1        I   first bit length (bits)
*          int    p2        I   second bit start position (bits)
*          int    l2        I   seconf bit length (bits)
*          int    p3        I   third bit start position (bits)
*          int    l3        I   third bit length (bits)
* return : extracted unsigned/signed bits
*-----------------------------------------------------------------------------*/
extern uint32_t getbitu3(const uint8_t *buff, int p1, int l1, int p2, int l2, 
                         int p3, int l3)
{
    return (getbitu(buff,p1,l1)<<(l2+l3))+(getbitu(buff,p2,l2)<<l3)+
               getbitu(buff,p3,l3);
}
extern int32_t getbits3(const uint8_t *buff, int p1, int l1, int p2, int l2,
                        int p3, int l3)
{
    if (getbitu(buff,p1,1))
        return (int32_t)((getbits(buff,p1,l1)<<(l2+l3))+
                   (getbitu(buff,p2,l2)<<l3)+getbitu(buff,p3,l3));
    else
        return (int32_t)getbitu3(buff,p1,l1,p2,l2,p3,l3);
}
/* merge two variables ---------------------------------------------------------
* merge two unsigned/signed variables
* args   : u/int32_t a      I   first variable (MSB data)
*          uint32_t  b      I   second variable (LSB data)
* return : merged unsigned/signed data
*-----------------------------------------------------------------------------*/
extern uint32_t merge_two_u(const uint32_t a, const uint32_t b, int n)
{
    return (a<<n)+b;
}
extern int32_t merge_two_s(const int32_t a, const uint32_t b, int n)
{
    return (int32_t)((a<<n)+b);
}
/* convert binary bits to byte data --------------------------------------------
* pack binary -1/1 bits to uint8_t array
* args   : int    *bits     I   binary bits (1 or -1)
*          int    nbits     I   number of bits (nbits<MAXBITS)
*          int    nbin      I   number of byte data
*          int    right     I   flag of right-align bits (if nbits<8*nbin)
*          uint8_t *bin     O   converted byte data
* return : none
*-----------------------------------------------------------------------------*/
extern void bits2byte(int *bits, int nbits, int nbin, int right, uint8_t *bin)
{
    int i,j,rem,bitscpy[MAXBITS]={0};
    unsigned char b;
    rem=8*nbin-nbits;

    memcpy(&bitscpy[right?rem:0],bits,sizeof(int)*nbits);

    for (i=0;i<nbin;i++) {
        b=0;
        for (j=0;j<8;j++) {
            b<<=1;
            if (bitscpy[i*8+j]<0) b|=0x01; /* -1=>1, 1=>0 */
        }
        bin[i]=b;
    }
}
/* block interleave ------------------------------------------------------------
* block interleave of input vector
* args   : int    *in       I   input bits
*          int    row       I   rows (where data is read)
*          int    col       I   columns (where data is written)
*          int    *out      O   interleaved bits
* return : none
* note : row*col<MAXBITS
*-----------------------------------------------------------------------------*/
extern void interleave(const int *in, int row, int col, int *out)
{
    int r,c;
    int tmp[MAXBITS];
    memcpy(tmp,in,sizeof(int)*row*col);
    for (r=0;r<row;r++) {
        for(c=0;c<col;c++) {
            out[r*col+c]=tmp[c*row+r];
        }
    }
}
/* decode forward error correction ----------------------------------------------
* pre-decode forward error correction (before preamble detection)
* args   : sdrnav_t *nav    I/O navigation struct
* return : none
*-----------------------------------------------------------------------------*/
extern void predecodefec(sdrnav_t *nav)
{
    int i,j;
    unsigned char enc[NAVFLEN_SBAS+NAVADDFLEN_SBAS];
    unsigned char dec[94];
    int dec2[NAVFLEN_SBAS/2];

    /* SBAS L1 / QZS L1SAIF */
    if (nav->ctype==CTYPE_L1SAIF||
        nav->ctype==CTYPE_L1SBAS) {
        /* 1/2 convolutional code */
        init_viterbi27_port(nav->fec,0);
        for (i=0; i < NAVFLEN_SBAS + NAVADDFLEN_SBAS; i++)
            enc[i] = (nav->fbits[i] == 1)? 0:255;
        update_viterbi27_blk_port(nav->fec,enc,(nav->flen+nav->addflen)/2);
        chainback_viterbi27_port(nav->fec,dec,nav->flen/2,0);
        for (i=0;i<94;i++) {
            for (j=0;j<8;j++) {
                dec2[8*i+j]=((dec[i]<<j)&0x80)>>7;
                nav->fbitsdec[8*i+j]=(dec2[8*i+j]==0)?1:-1;
                if (8*i+j==NAVFLEN_SBAS/2-1) {
                    break;
                }
            }
        }
    }
}
/* parity check ----------------------------------------------------------------
* parity check of navigation frame data
* args   : sdrnav_t *nav    I/O navigation struct
* return : int                  1:okay 0: wrong parity
*-----------------------------------------------------------------------------*/
extern int paritycheck(sdrnav_t *nav)
{
    int i,j,stat=0,crc,bits[MAXBITS];
    unsigned char bin[29]={0},pbin[3];

    /* copy */
    for (i=0;i<nav->flen+nav->addflen;i++) 
        bits[i]=nav->polarity*nav->fbitsdec[i];

    /* SBAS L1 / QZS SAIF parity check */
    if (nav->ctype==CTYPE_L1SAIF||nav->ctype==CTYPE_L1SBAS) {
        bits2byte(&bits[0],226,29,1,bin);
        bits2byte(&bits[226],24,3,0,pbin);

        /* compute CRC24 */
        crc=crc24q(bin,29);
        if (crc==getbitu(pbin,0,24)) {
            return 1;
        }
    }

    return 0;
}
/* find preamble bits ----------------------------------------------------------
* search preamble bits from navigation data bits
* args   : sdrnav_t *nav    I/O navigation struct
* return : int                  1:found 0: not found
*-----------------------------------------------------------------------------*/
extern int findpreamble(sdrnav_t *nav)
{
    int i,corr=0;

    /* L1-SBAS/SAIF */
    /* check 2 preambles */
    if (nav->ctype==CTYPE_L1SAIF||nav->ctype==CTYPE_L1SBAS) {
        for (i=0;i<nav->prelen/2;i++) {
            corr+=(nav->fbitsdec[i    ]*nav->prebits[ 0+i]);
            corr+=(nav->fbitsdec[i+250]*nav->prebits[ 8+i]);
        }
    }

    /* check preamble match */
    if (abs(corr)==nav->prelen) { /* preamble matched */
        nav->polarity=corr>0?1:-1; /* set bit polarity */
        /* parity check */
        if (paritycheck(nav)) {
            return 1;
        } else {
            // ????
            if (nav->ctype==CTYPE_L1SAIF||nav->ctype==CTYPE_L1SBAS) {
                if (nav->polarity==1) nav->flagpol=ON;
            }
        }
    }

    return 0;
}

extern void sdrnav_init()
{
    SBAS_init();
}
