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

#pragma once

#include "peri.h"
#include "spi.h"
#include "options.h"

enum SPI_SEL {
    SPI_FPGA=0,  // Load FPGA
    SPI_BOOT=1,  // Load embedded CPU image
    SPI_HOST=2   // Host messaging
};

typedef struct {
    SPI_SEL sel;
    SPI_MOSI *mosi;
    int tx_xfers;
    SPI_MISO *miso;
    int rx_xfers;
} spi_dev_ipc_t;

typedef struct {
    u64_t spi_bytes;
    spi_dev_ipc_t spi_dev_ipc;
    SPI_MISO dpump_miso;
    SPI_MISO gps_search_miso, gps_channel_miso[GPS_MAX_CHANS], gps_clocks_miso, gps_iqdata_miso, gps_glitches_miso[2];
    SPI_MOSI gps_e1b_code_mosi;
    SPI_MISO misc_miso[2];
    SPI_MISO spi_junk_miso, pingx_miso;
    SPI_MOSI spi_tx[N_SPI_TX];
    SPI_MOSI misc_mosi;

    #ifdef OPTION_WF_CONSOLIDATE_SPI
        SPI_MISO wf_miso[MAX_RX_CHANS][NWF_NXFER];      // FIXME: check perf if [NWF_NXFER] => [16]
    #else
        SPI_MISO wf_miso[MAX_RX_CHANS];
    #endif
} spi_shmem_t;

#include "shmem_config.h"

// list all CPUs as a reminder that the effect of SPI SHMEM needs to be evaluated for each
#ifdef CPU_AM67
    // shared memory enabled
#elif CPU_TDA4VM
    // shared memory enabled
#elif CPU_AM5729
    //#define SPI_SHMEM_DISABLE_TEST
    #ifdef SPI_SHMEM_DISABLE_TEST
        #warning dont forget to remove SPI_SHMEM_DISABLE_TEST
        //#define SPI_SHMEM_DISABLE
    #else
        // shared memory enabled
    #endif
#elif CPU_BCM2837
    #define SPI_SHMEM_DISABLE
#elif CPU_AM3359
    #define SPI_SHMEM_DISABLE
#else
    #error must define SPI_SHMEM_DISABLE per CPU
#endif

#include "shmem.h"

#ifdef SPI_SHMEM_DISABLE
    #define SPI_SHMEM   spi_shmem_p
    extern spi_shmem_t  *spi_shmem_p;
#else
    #define SPI_SHMEM   (&shmem->spi_shmem)
#endif

#ifdef SPI_8
	#define	SPI_BPW	8
#endif
#ifdef SPI_16
	#define	SPI_BPW	16
#endif
#ifdef SPI_32
	#define	SPI_BPW	32
#endif

#define SPI_SETUP_MODE      0
#define SPI_KIWISDR_1_MODE  0
#define SPI_KIWISDR_2_MODE  1

void spi_dev_init(int spi_clkg, int spi_speed);
void spi_dev_init2();
void spi_dev_mode(int spi_mode);
void spi_dev(SPI_SEL sel, SPI_MOSI *mosi, int tx_xfers, SPI_MISO *miso, int rx_xfers);
