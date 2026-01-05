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
// http://www.aholme.co.uk/GPS/Main.htm
//////////////////////////////////////////////////////////////////////////

#ifndef _GPS_H_
#define _GPS_H_

#include "types.h"
#include "correlator.h"
#include "ephemeris.h"

#include <stdint.h>

#define MAX_SATS	32
#define MAX_CHANS	12

#define	PI			3.14159265358979323846

#define GPS_OMEGA_E	7.2921151467e-5

#define GPS_MU		3.986005e14

#define GPS_C		2.99792458e8

#define GPS_F		-4.442807633e-10

#define GPS_D2R		(PI/180.0)

#define GPS_R2D		(180.0/PI)

#define GPS_A		6378137.0

#define GPS_FINV	298.257223563

#define GPS_B		(GPS_A*(1.0-1.0/GPS_FINV))

#define GPS_E2		(1.0-(GPS_B*GPS_B)/(GPS_A*GPS_A))

#define GPS_U		(GPS_MU/(GPS_A*GPS_A))

#define GPS_ST		86164.09053083288

#define GPS_OMEGA	(2.0*PI/GPS_ST)

#define	GPS_T		0.075

#define	MAX_TRACK	60

#define	L1			1575.42e6

#define	CODE_RATE	1.023e6

#define	CODE_LEN	1023

#define	CODE_CHIPS	1

#define	CA_RATE		(CODE_RATE*CODE_CHIPS)

#define	CA_LEN		(CODE_LEN*CODE_CHIPS)

#define	F_HZ		(2.0*PI)

#define	LEN_MSG		300

#define	MSG_RATE	50

#define	T_BITS		(1.0/MSG_RATE)

#define	T_SUB		6

#define	T_NAV		30

#define	BITS_SUB	(T_SUB*MSG_RATE)

#define	BITS_NAV	(T_NAV*MSG_RATE)

#define	WORD_BITS	30

#define	WORD_PAR	6

#define	WORD_DATA	24

#define	WORDS_SUB	10

#define	SUB_BITS	(WORDS_SUB*WORD_BITS)

#define	SUB_DATA	(WORDS_SUB*WORD_DATA)

#define	SUB_PAR		(WORDS_SUB*WORD_PAR)

#define	SUB_BYTES	(SUB_DATA/8)

#define	SUB_BUFF	(49+SUB_BITS+WORD_PAR)

#define	SUB_START	49

#define	SUB_FIRST	49

#define	SUB_LAST	(SUB_FIRST+SUB_BITS-1)

#define	SUB_CHECK	(SUB_LAST+1)

#define	SUB_END		(SUB_CHECK+WORD_PAR-1)

#define	SUB_TOW		(30-17)

#define	PREAMBLE	0x8b

#define	NAV_ERROR	500

#define	TRACK_T		1

#define	TRACK_TICKS	(TRACK_T/TICK)

#define	ONE_SEC		(1/TICK)

#define	ONE_MS		(ONE_SEC/1000)

#define	PREDICT		(ONE_SEC/5)

#define	PREDICT_T	(PREDICT*TICK)

#define	MAX_NAV		50

#define	MAX_NAV2	(MAX_NAV/2)

#define	MAX_ERROR	2500

#define	MAX_ERROR2	(MAX_ERROR/2)

#define	CLAMP(x,a,b) ((x)<(a)?(a):(x)>(b)?(b):(x))

#define	MAX_AGE		10

#define	MAX_AGE2	(MAX_AGE/2)

#define	HALF_CHIP	(1.0/CA_RATE/2.0)

#define	MAX_LAG		(2.0/CA_RATE)

#define	SEC_LAG		(1.0/CA_RATE)

#define	MAX_TIME	(1e-3)

#define	MAX_CHIP	(MAX_TIME*CA_RATE)

#define	MAX_LEN		(MAX_TIME*FS)

#define	MAX_LEN2	(MAX_LEN/2)

#define	MAX_CODE	(MAX_TIME*CA_RATE)

#define	MAX_CODE2	(MAX_CODE/2)

#define	MAX_F		5000

#define	MAX_DF		1000

#define	MAX_F2		(MAX_F/2)

#define	MAX_DF2		(MAX_DF/2)

#define	MAX_RATE	(MAX_F*F_HZ)

#define	MAX_DRATE	(MAX_DF*F_HZ)

#define	MAX_RATE2	(MAX_RATE/2)

#define	MAX_DRATE2	(MAX_DRATE/2)

#define	MAX_SNR		20

#define	MAX_PHASE	(2.0*PI)

#define	MAX_PHASE2	(MAX_PHASE/2)

#define	MAX_FREQ	1000

#define	MAX_FREQ2	(MAX_FREQ/2)

#define	MAX_FREQ3	(MAX_FREQ2/2)

#define	MIN_SIG		0.1

#define	MAX_SIG		1.0

#define	MAX_SIG2	(MAX_SIG/2)

#define	MIN_SNR		1

#define	MAX_SNR2	(MAX_SNR/2)

#define	ACQ_T		0.020

#define	ACQ_LEN		(ACQ_T*FS)

#define	ACQ_LEN2	(ACQ_LEN/2)

#define	ACQ_CODE	(ACQ_T*CA_RATE)

#define	ACQ_CODE2	(ACQ_CODE/2)

#define	ACQ_FREQ	5000

#define	ACQ_FREQ2	(ACQ_FREQ/2)

#define	ACQ_STEP	500

#define	ACQ_STEP2	(ACQ_STEP/2)

#define	ACQ_BINS	(ACQ_FREQ/ACQ_STEP)

#define	ACQ_BINS2	(ACQ_BINS/2)

#define	ACQ_THRESH	12

#define	ACQ_TRACK	3

#define	ACQ_SKIP	50

#define	ACQ_DELAY	10

#define	MIN_CORR	1e-5

#define	MAX_CORR	1e5

#define	MAX_CORR2	(MAX_CORR/2)

#define	MAX_LOOP	100

#define	MAX_OFFSET	2000

#define	MAX_OFFSET2	(MAX_OFFSET/2)

#define	MAX_DOFFSET	1000

#define	MAX_DOFFSET2 (MAX_DOFFSET/2)

#define	MAX_MSEC	20

#define	MAX_MSEC2	(MAX_MSEC/2)

#define	MAX_MSEC3	(MAX_MSEC2/2)

#define	MAX_MSEC4	(MAX_MSEC3/2)

#define	MAX_INIT	20

#define	MAX_INIT2	(MAX_INIT/2)

#define	MAX_FIX		20

#define	MAX_FIX2	(MAX_FIX/2)

#define	MAX_DOP		10

#define	MAX_DOP2	(MAX_DOP/2)

#define	MAX_DOP3	(MAX_DOP2/2)

#define	MAX_VAR		100

#define	MAX_VAR2	(MAX_VAR/2)

#define	MAX_VAR3	(MAX_VAR2/2)

#define	MAX_VAR4	(MAX_VAR3/2)

#define	MAX_VAR5	(MAX_VAR4/2)

#define	MAX_VEL		300

#define	MAX_VEL2	(MAX_VEL/2)

#define	MAX_VEL3	(MAX_VEL2/2)

#define	MAX_VEL4	(MAX_VEL3/2)

#define	MAX_VEL5	(MAX_VEL4/2)

#define	MAX_DVEL	50

#define	MAX_DVEL2	(MAX_DVEL/2)

#define	MAX_DVEL3	(MAX_DVEL2/2)

#define	MAX_DVEL4	(MAX_DVEL3/2)

#define	MAX_DVEL5	(MAX_DVEL4/2)

#define	MAX_ACC		50

#define	MAX_ACC2	(MAX_ACC/2)

#define	MAX_ACC3	(MAX_ACC2/2)

#define	MAX_ACC4	(MAX_ACC3/2)

#define	MAX_ACC5	(MAX_ACC4/2)

#define	MAX_DACC	20

#define	MAX_DACC2	(MAX_DACC/2)

#define	MAX_DACC3	(MAX_DACC2/2)

#define	MAX_DACC4	(MAX_DACC3/2)

#define	MAX_DACC5	(MAX_DACC4/2)

#define	MAX_W		200

#define	MAX_W2		(MAX_W/2)

#define	MAX_W3		(MAX_W2/2)

#define	MAX_W4		(MAX_W3/2)

#define	MAX_W5		(MAX_W4/2)

#define	MAX_DW		100

#define	MAX_DW2		(MAX_DW/2)

#define	MAX_DW3		(MAX_DW2/2)

#define	MAX_DW4		(MAX_DW3/2)

#define	MAX_DW5		(MAX_DW4/2)

#define	MAX_BIAS	1000

#define	MAX_BIAS2	(MAX_BIAS/2)

#define	MAX_BIAS3	(MAX_BIAS2/2)

#define	MAX_BIAS4	(MAX_BIAS3/2)

#define	MAX_BIAS5	(MAX_BIAS4/2)

#define	MAX_DB		100

#define	MAX_DB2		(MAX_DB/2)

#define	MAX_DB3		(MAX_DB2/2)

#define	MAX_DB4		(MAX_DB3/2)

#define	MAX_DB5		(MAX_DB4/2)

#define	MAX_DV		100

#define	MAX_DV2		(MAX_DV/2)

#define	MAX_DV3		(MAX_DV2/2)

#define	MAX_DV4		(MAX_DV3/2)

#define	MAX_DV5		(MAX_DV4/2)

#define	MAX_T		10

#define	MAX_T2		(MAX_T/2)

#define	MAX_T3		(MAX_T2/2)

#define	MAX_T4		(MAX_T3/2)

#define	MAX_T5		(MAX_T4/2)

#define	MAX_DT		1

#define	MAX_DT2		(MAX_DT/2)

#define	MAX_DT3		(MAX_DT2/2)

#define	MAX_DT4		(MAX_DT3/2)

#define	MAX_DT5		(MAX_DT4/2)

#define	MAX_L		20000

#define	MAX_L2		(MAX_L/2)

#define	MAX_L3		(MAX_L2/2)

#define	MAX_L4		(MAX_L3/2)

#define	MAX_L5		(MAX_L4/2)

#define	MAX_DL		1000

#define	MAX_DL2		(MAX_DL/2)

#define	MAX_DL3		(MAX_DL2/2)

#define	MAX_DL4		(MAX_DL3/2)

#define	MAX_DL5		(MAX_DL4/2)

#define	MAX_S		1

#define	MAX_S2		(MAX_S/2)

#define	MAX_S3		(MAX_S2/2)

#define	MAX_S4		(MAX_S3/2)

#define	MAX_S5		(MAX_S4/2)

#define	MAX_DS		0.1

#define	MAX_DS2		(MAX_DS/2)

#define	MAX_DS3		(MAX_DS2/2)

#define	MAX_DS4		(MAX_DS3/2)

#define	MAX_DS5		(MAX_DS4/2)

#define	MAX_G		1

#define	MAX_G2		(MAX_G/2)

#define	MAX_G3		(MAX_G2/2)

#define	MAX_G4		(MAX_G3/2)

#define	MAX_G5		(MAX_G4/2)

#define	MAX_DG		0.1

#define	MAX_DG2		(MAX_DG/2)

#define	MAX_DG3		(MAX_DG2/2)

#define	MAX_DG4		(MAX_DG3/2)

#define	MAX_DG5		(MAX_DG4/2)

#define	MAX_Z		1e7

#define	MAX_Z2		(MAX_Z/2)

#define	MAX_Z3		(MAX_Z2/2)

#define	MAX_Z4		(MAX_Z3/2)

#define	MAX_Z5		(MAX_Z4/2)

#define	MAX_DZ		1e6

#define	MAX_DZ2		(MAX_DZ/2)

#define	MAX_DZ3		(MAX_DZ2/2)

#define	MAX_DZ4		(MAX_DZ3/2)

#define	MAX_DZ5		(MAX_DZ4/2)

#define	MAX_ZW		1e7

#define	MAX_ZW2		(MAX_ZW/2)

#define	MAX_ZW3		(MAX_ZW2/2)

#define	MAX_ZW4		(MAX_ZW3/2)

#define	MAX_ZW5		(MAX_ZW4/2)

#define	MAX_DZW		1e6

#define	MAX_DZW2	(MAX_DZW/2)

#define	MAX_DZW3	(MAX_DZW2/2)

#define	MAX_DZW4	(MAX_DZW3/2)

#define	MAX_DZW5	(MAX_DZW4/2)

#define	MAX_P		1e7

#define	MAX_P2		(MAX_P/2)

#define	MAX_P3		(MAX_P2/2)

#define	MAX_P4		(MAX_P3/2)

#define	MAX_P5		(MAX_P4/2)

#define	MAX_DP		1e6

#define	MAX_DP2		(MAX_DP/2)

#define	MAX_DP3		(MAX_DP2/2)

#define	MAX_DP4		(MAX_DP3/2)

#define	MAX_DP5		(MAX_DP4/2)

#define	MAX_K		1e7

#define	MAX_K2		(MAX_K/2)

#define	MAX_K3		(MAX_K2/2)

#define	MAX_K4		(MAX_K3/2)

#define	MAX_K5		(MAX_K4/2)

#define	MAX_DK		1e6

#define	MAX_DK2		(MAX_DK/2)

#define	MAX_DK3		(MAX_DK2/2)

#define	MAX_DK4		(MAX_DK3/2)

#define	MAX_DK5		(MAX_DK4/2)

#define	MAX_I		1e7

#define	MAX_I2		(MAX_I/2)

#define	MAX_I3		(MAX_I2/2)

#define	MAX_I4		(MAX_I3/2)

#define	MAX_I5		(MAX_I4/2)

#define	MAX_DI		1e6

#define	MAX_DI2		(MAX_DI/2)

#define	MAX_DI3		(MAX_DI2/2)

#define	MAX_DI4		(MAX_DI3/2)

#define	MAX_DI5		(MAX_DI4/2)

#define	MAX_D		1e7

#define	MAX_D2		(MAX_D/2)

#define	MAX_D3		(MAX_D2/2)

#define	MAX_D4		(MAX_D3/2)

#define	MAX_D5		(MAX_D4/2)

#define	MAX_DD		1e6

#define	MAX_DD2		(MAX_DD/2)

#define	MAX_DD3		(MAX_DD2/2)

#define	MAX_DD4		(MAX_DD3/2)

#define	MAX_DD5		(MAX_DD4/2)

#define	MAX_E		1e7

#define	MAX_E2		(MAX_E/2)

#define	MAX_E3		(MAX_E2/2)

#define	MAX_E4		(MAX_E3/2)

#define	MAX_E5		(MAX_E4/2)

#define	MAX_DE		1e6

#define	MAX_DE2		(MAX_DE/2)

#define	MAX_DE3		(MAX_DE2/2)

#define	MAX_DE4		(MAX_DE3/2)

#define	MAX_DE5		(MAX_DE4/2)

#define	MAX_FX		1e7

#define	MAX_FX2		(MAX_FX/2)

#define	MAX_FX3		(MAX_FX2/2)

#define	MAX_FX4		(MAX_FX3/2)

#define	MAX_FX5		(MAX_FX4/2)

#define	MAX_DFX		1e6

#define	MAX_DFX2	(MAX_DFX/2)

#define	MAX_DFX3	(MAX_DFX2/2)

#define	MAX_DFX4	(MAX_DFX3/2)

#define	MAX_DFX5	(MAX_DFX4/2)

#define	MAX_Y		1e7

#define	MAX_Y2		(MAX_Y/2)

#define	MAX_Y3		(MAX_Y2/2)

#define	MAX_Y4		(MAX_Y3/2)

#define	MAX_Y5		(MAX_Y4/2)

#define	MAX_DY		1e6

#define	MAX_DY2		(MAX_DY/2)

#define	MAX_DY3		(MAX_DY2/2)

#define	MAX_DY4		(MAX_DY3/2)

#define	MAX_DY5		(MAX_DY4/2)

#define	MAX_X		1e7

#define	MAX_X2		(MAX_X/2)

#define	MAX_X3		(MAX_X2/2)

#define	MAX_X4		(MAX_X3/2)

#define	MAX_X5		(MAX_X4/2)

#define	MAX_DX		1e6

#define	MAX_DX2		(MAX_DX/2)

#define	MAX_DX3		(MAX_DX2/2)

#define	MAX_DX4		(MAX_DX3/2)

#define	MAX_DX5		(MAX_DX4/2)

#define	MAX_N		1e7

#define	MAX_N2		(MAX_N/2)

#define	MAX_N3		(MAX_N2/2)

#define	MAX_N4		(MAX_N3/2)

#define	MAX_N5		(MAX_N4/2)

#define	MAX_DN		1e6

#define	MAX_DN2		(MAX_DN/2)

#define	MAX_DN3		(MAX_DN2/2)

#define	MAX_DN4		(MAX_DN3/2)

#define	MAX_DN5		(MAX_DN4/2)

#define	MAX_M		1e7

#define	MAX_M2		(MAX_M/2)

#define	MAX_M3		(MAX_M2/2)

#define	MAX_M4		(MAX_M3/2)

#define	MAX_M5		(MAX_M4/2)

#define	MAX_DM		1e6

#define	MAX_DM2		(MAX_DM/2)

#define	MAX_DM3		(MAX_DM2/2)

#define	MAX_DM4		(MAX_DM3/2)

#define	MAX_DM5		(MAX_DM4/2)

#define	MAX_Q		1e7

#define	MAX_Q2		(MAX_Q/2)

#define	MAX_Q3		(MAX_Q2/2)

#define	MAX_Q4		(MAX_Q3/2)

#define	MAX_Q5		(MAX_Q4/2)

#define	MAX_DQ		1e6

#define	MAX_DQ2		(MAX_DQ/2)

#define	MAX_DQ3		(MAX_DQ2/2)

#define	MAX_DQ4		(MAX_DQ3/2)

#define	MAX_DQ5		(MAX_DQ4/2)

#define	MAX_R		1e7

#define	MAX_R2		(MAX_R/2)

#define	MAX_R3		(MAX_R2/2)

#define	MAX_R4		(MAX_R3/2)

#define	MAX_R5		(MAX_R4/2)

#define	MAX_DR		1e6

#define	MAX_DR2		(MAX_DR/2)

#define	MAX_DR3		(MAX_DR2/2)

#define	MAX_DR4		(MAX_DR3/2)

#define	MAX_DR5		(MAX_DR4/2)

#define	MAX_U1		1e7

#define	MAX_U12		(MAX_U1/2)

#define	MAX_U13		(MAX_U12/2)

#define	MAX_U14		(MAX_U13/2)

#define	MAX_U15		(MAX_U14/2)

#define	MAX_DU1		1e6

#define	MAX_DU12	(MAX_DU1/2)

#define	MAX_DU13	(MAX_DU12/2)

#define	MAX_DU14	(MAX_DU13/2)

#define	MAX_DU15	(MAX_DU14/2)

#define	MAX_U2		1e7

#define	MAX_U22		(MAX_U2/2)

#define	MAX_U23		(MAX_U22/2)

#define	MAX_U24		(MAX_U23/2)

#define	MAX_U25		(MAX_U24/2)

#define	MAX_DU2		1e6

#define	MAX_DU22	(MAX_DU2/2)

#define	MAX_DU23	(MAX_DU22/2)

#define	MAX_DU24	(MAX_DU23/2)

#define	MAX_DU25	(MAX_DU24/2)

#define	MAX_U3		1e7

#define	MAX_U32		(MAX_U3/2)

#define	MAX_U33		(MAX_U32/2)

#define	MAX_U34		(MAX_U33/2)

#define	MAX_U35		(MAX_U34/2)

#define	MAX_DU3		1e6

#define	MAX_DU32	(MAX_DU3/2)

#define	MAX_DU33	(MAX_DU32/2)

#define	MAX_DU34	(MAX_DU33/2)

#define	MAX_DU35	(MAX_DU34/2)

#define	MAX_U4		1e7

#define	MAX_U42		(MAX_U4/2)

#define	MAX_U43		(MAX_U42/2)

#define	MAX_U44		(MAX_U43/2)

#define	MAX_U45		(MAX_U44/2)

#define	MAX_DU4		1e6

#define	MAX_DU42	(MAX_DU4/2)

#define	MAX_DU43	(MAX_DU42/2)

#define	MAX_DU44	(MAX_DU43/2)

#define	MAX_DU45	(MAX_DU44/2)

#define	MAX_U5		1e7

#define	MAX_U52		(MAX_U5/2)

#define	MAX_U53		(MAX_U52/2)

#define	MAX_U54		(MAX_U53/2)

#define	MAX_U55		(MAX_U54/2)

#define	MAX_DU5		1e6

#define	MAX_DU52	(MAX_DU5/2)

#define	MAX_DU53	(MAX_DU52/2)

#define	MAX_DU54	(MAX_DU53/2)

#define	MAX_DU55	(MAX_DU54/2)

typedef struct {
	int sat;
	int chan;
	int state;
	int count;
	int invert;
	int bit;
	int nav;
	int sub;
	int sync;
	int lock;
	int miss;
	int data;
	double snr;
	double freq;
	double dfreq;
	double phase;
	double dphase;
	double code;
	double dcode;
	double snr2;
	double freq2;
	double dfreq2;
	double phase2;
	double dphase2;
	double code2;
	double dcode2;
	double lock2;
	double miss2;
	double data2;
	double ddata2;
	double snr3;
	double freq3;
	double dfreq3;
	double phase3;
	double dphase3;
	double code3;
	double dcode3;
	double lock3;
	double miss3;
	double data3;
	double ddata3;
	double lock4;
	double miss4;
	double data4;
	double ddata4;
	double lock5;
	double miss5;
	double data5;
	double ddata5;
} CHAN;

typedef struct {
	int sat;
	int chan;
	int state;
	int acq;
	int track;
	int count;
	int lock;
	int miss;
	double snr;
	double freq;
	double code;
} SAT;

typedef struct {
	double x;
	double y;
	double z;
	double b;
} POS;

typedef struct {
	double x;
	double y;
	double z;
	double b;
	double vx;
	double vy;
	double vz;
	double vb;
} VEL;

typedef struct {
	double x;
	double y;
	double z;
	double b;
	double vx;
	double vy;
	double vz;
	double vb;
	double ax;
	double ay;
	double az;
	double ab;
} ACC;

typedef struct {
	double lat;
	double lon;
	double alt;
	double vlat;
	double vlon;
	double valt;
	double alat;
	double alon;
	double aalt;
} LLA;

typedef struct {
	unsigned tLS_valid:1;
	int delta_tLS;
	int delta_tLSF;
} GPS_UTC;

extern GPS_UTC gps;

unsigned bin(const char *s, int n);
int parity(u4_t word);
int preamble(u4_t word);
void init_cacode();
void init_e1bcode();
void init_nav();
void nav_task(int i);

#endif
