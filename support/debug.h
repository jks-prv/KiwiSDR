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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "types.h"
#include "kiwi.gen.h"

#define	EC_EVENT		    0
#define	EC_DUMP			    1
#define EC_DUMP_CONT        2
#define EC_TASK_SCHED       3
#define	EC_TASK_IDLE        4
#define	EC_TASK_SWITCH      5
#define	EC_TASK_INTR        6
#define	EC_TRIG1		    7
#define	EC_TRIG2		    8
#define	EC_TRIG3		    9
#define	EC_TRIG_REALTIME    10
#define	EC_TRIG_ACCUM_ON    11
#define	EC_TRIG_ACCUM_OFF   12
#define NECMD               13

#define	EV_NEXTTASK		0
#define	EV_SPILOOP		1
#define	EV_WF			2
#define	EV_SND			3
#define	EV_GPS			4
#define	EV_DPUMP		5
#define	EV_PRINTF		6
#define	EV_EXT          7
#define	EV_RX           8
#define	EV_WS           9
#define NEVT			10


// special profile to find performance problems
#if 0 && defined(PLATFORM_beaglebone_ai64)
	#define EV_MEAS
	#define EV_MEAS_NEXTTASK
	#define EV_MEAS_LATENCY
	#define EV_MEAS_DPUMP
	#define EV_MEAS_DPUMP_CHUNK
	#define EV_MEAS_SPI
	#define EV_MEAS_SPI_CMD
#endif

// measure general task switching times
#if 0
	#define EV_MEAS
    #define EV_MEAS_NEXTTASK
#endif

// measure sound runtimes
#if 0
	#define EV_MEAS
    #define EV_MEAS_NEXTTASK
    #define EV_MEAS_SND
#endif

// FAX extension latency
#if 0
	#define EV_MEAS
	#define EV_MEAS_FAX
#endif

// spi_lock has no owner
#if 0
	#define EV_MEAS
    #define EV_MEAS_LOCK
    #define EV_MEAS_NEXTTASK
#endif

// use when there's a crash that doesn't leave a backtrace for gdb
#if 0
	#define	EVENT_DUMP_WHILE_RUNNING
	#define EV_MEAS
	#define EV_MEAS_NEXTTASK
#endif

// measure where the time goes during latency issues
// also catch spi_lock with no owner
#if 0
	#define EV_MEAS
	#define EV_MEAS_NEXTTASK
	#define EV_MEAS_LATENCY
	#define EV_MEAS_LOCK
	#define EV_MEAS_SPI_CMD
#endif

// measure where the time goes during datapump latency issues
#if 0
	#define EV_MEAS
	#define EV_MEAS_NEXTTASK
	#define EV_MEAS_LATENCY
	#define EV_MEAS_DPUMP_LATENCY
    #define EV_WEB_SERVER
#endif

// measure where the time goes when getting sound underruns
#if 0
	#define EV_MEAS
	#define EV_MEAS_NEXTTASK
	#define EV_MEAS_DPUMP_CHUNK
#endif

// measure waterfall chunk processing
#if 0
	#ifdef SND_SEQ_CHECK
		#define EV_MEAS
		#define EV_MEAS_NEXTTASK
		#define EV_MEAS_WF_CHUNK
		#define EV_MEAS_DPUMP_CHUNK
	#endif
#endif

//#define EV_MEAS
#ifdef EV_MEAS
	void ev(int cmd, int event, int param, const char *s, const char *s2);
#else
	#define ev(c, e, p, s, s2)
#endif

//#define EV_MEAS_FAX
#if defined(EV_MEAS) && defined(EV_MEAS_FAX)
	#define evFAX(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evFAX(c, e, p, s, s2)
#endif

//#define EV_MEAS_LATENCY
#if defined(EV_MEAS) && defined(EV_MEAS_LATENCY)
	#define evLatency(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evLatency(c, e, p, s, s2)
#endif

//#define EV_MEAS_LOCK
#if defined(EV_MEAS) && defined(EV_MEAS_LOCK)
	#define evLock(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evLock(c, e, p, s, s2)
#endif

//#define EV_MEAS_PRINTF
#if defined(EV_MEAS) && defined(EV_MEAS_PRINTF)
	#define evPrintf(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evPrintf(c, e, p, s, s2)
#endif

//#define EV_MEAS_GPS
#if defined(EV_MEAS) && defined(EV_MEAS_GPS)
	#define evGPS(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evGPS(c, e, p, s, s2)
#endif

//#define EV_MEAS_NEXTTASK
#if defined(EV_MEAS) && defined(EV_MEAS_NEXTTASK)
	#define evNT(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evNT(c, e, p, s, s2)
#endif

//#define EV_MEAS_SPI_DEV
#if defined(EV_MEAS) && (defined(EV_MEAS_SPI_DEV) || defined(SPI_PUMP_CHECK))
	#define evSpiDev(c, e, p, s, s2) ev(c, e, p, s, s2)
	#define evSpiDevStmt(s) s
#else
	#define evSpiDev(c, e, p, s, s2)
	#define evSpiDevStmt(s)
#endif

//#define EV_MEAS_SPI
#if defined(EV_MEAS) && (defined(EV_MEAS_SPI) || defined(SPI_PUMP_CHECK))
	#define evSpi(c, e, p, s, s2) ev(c, e, p, s, s2)
	#define evSpiStmt(s) s
#else
	#define evSpi(c, e, p, s, s2)
	#define evSpiStmt(s)
#endif

//#define EV_MEAS_SPI_CMD
#if defined(EV_MEAS) && defined(EV_MEAS_SPI_CMD)
	#define evSpiCmd(c, e, p, s, s2) ev(c, e, p, s, s2)
	#define evSpiCmdStmt(s) s
#else
	#define evSpiCmd(c, e, p, s, s2)
	#define evSpiCmdStmt(s)
#endif

//#define EV_MEAS_WF
#if defined(EV_MEAS) && defined(EV_MEAS_WF)
	#define evWF(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evWF(c, e, p, s, s2)
#endif

//#define EV_MEAS_WF_CHUNK
#if defined(EV_MEAS) && defined(EV_MEAS_WF_CHUNK)
	#define evWFC(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evWFC(c, e, p, s, s2)
#endif

//#define EV_MEAS_SND
#if defined(EV_MEAS) && defined(EV_MEAS_SND)
	#define evSnd(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evSnd(c, e, p, s, s2)
#endif

//#define EV_MEAS_DPUMP
#if defined(EV_MEAS) && defined(EV_MEAS_DPUMP)
	#define evDP(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evDP(c, e, p, s, s2)
#endif

//#define EV_MEAS_DPUMP_CHUNK
#if defined(EV_MEAS) && defined(EV_MEAS_DPUMP_CHUNK)
	#define evDPC(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evDPC(c, e, p, s, s2)
#endif

//#define EV_WEB_SERVER
#if defined(EV_MEAS) && defined(EV_WEB_SERVER)
	#define evWS(c, e, p, s, s2) ev(c, e, p, s, s2)
#else
	#define evWS(c, e, p, s, s2)
#endif

char *evprintf(const char *fmt, ...);

#endif
