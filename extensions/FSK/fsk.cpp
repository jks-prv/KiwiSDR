// Copyright (c) 2017 John Seamons, ZL4VO/KF6VO

#include "ext.h"	// all calls to the extension interface begin with "ext_", e.g. ext_register()

#include "kiwi.h"
#include "misc.h"
#include "mem.h"
#include "web.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <sys/mman.h>

//#define DEBUG_MSG	true
#define DEBUG_MSG	false

typedef struct {
    s2_t *s2p_start, *s2p_end;
    int tsamps;
    char *udp_url;
	void *udp;
} fsk_t;

static fsk_t fsk;


// rx_chan is the receiver channel number we've been assigned, 0..rx_chans
// We need this so the extension can support multiple users, each with their own fsk_chan[] data structure.

typedef struct {
	int rx_chan;
	bool test;

    s2_t *s2p, *s22p, *s2px;
    int nsamps;
} fsk_chan_t;

static fsk_chan_t fsk_chan[MAX_RX_CHANS];

static void fsk_file_data(int rx_chan, int chan, int nsamps, TYPEMONO16 *samps, int freqHz)
{
    fsk_chan_t *e = &fsk_chan[rx_chan];

    if (!e->test) return;
    if (e->s2p >= fsk.s2p_end) {
        //printf("fsk_file_data test_done\n");
        ext_send_msg(rx_chan, false, "EXT test_done");
        e->test = false;
        return;
    }
    
    // Pushback sample file.
    if (e->test) {
        for (int i = 0; i < nsamps; i++) {
            if (e->s2p < fsk.s2p_end) {
                *samps++ = (s2_t) FLIP16(*e->s2p);
            }
            e->s2p++;
        }

        int pct = e->nsamps * 100 / fsk.tsamps;
        e->nsamps += nsamps;
        pct += 3;
        if (pct > 100) pct = 100;
        //ext_send_msg(rx_chan, false, "EXT bar_pct=%d", pct);
    }

}

bool fsk_msgs(char *msg, int rx_chan)
{
	fsk_chan_t *e = &fsk_chan[rx_chan];
    e->rx_chan = rx_chan;	// remember our receiver channel number
	int n;
	
	//printf("### fsk_msgs RX%d <%s>\n", rx_chan, msg);
	
	if (strcmp(msg, "SET ext_server_init") == 0) {
        #ifdef MG_VERSION_OLD
            #warning FSK: no UDP support for older Mongoose versions
            printf("FSK: no UDP support\n");
        #else
            if (fsk.udp == NULL && ext_auth(rx_chan) == AUTH_LOCAL) {
                const char *fn = cfg_string("fsk.udp", NULL, CFG_OPTIONAL);
                if (kiwi_nonEmptyStr(fn)) {
                    asprintf(&fsk.udp_url, "udp://%s", fn);
                    fsk.udp = udp_connect(fsk.udp_url);
                    printf("FSK: UDP connect %s\n", fsk.udp_url);
                    kiwi_asfree(fsk.udp_url);
                }
                cfg_string_free(fn);
            }
        #endif

		ext_send_msg(rx_chan, DEBUG_MSG, "EXT ready");
		return true;
	}

	if (strcmp(msg, "SET test") == 0) {
        e->s2p = e->s2px = e->s22p = fsk.s2p_start;
        e->test = true;
        
        // misuse ext_register_receive_real_samps() to pushback audio samples from the test file
        ext_register_receive_real_samps(fsk_file_data, rx_chan);
		return true;
	}
	
	char *udp_text_m;
    n = sscanf(msg, "SET udp_text=%256m[^\n]", &udp_text_m);
    if (n == 1) {
        if (fsk.udp) {
            size_t sl = strlen(udp_text_m);
            //printf("FSK: UDP send (%d) %s\\n\n", sl, kiwi_str_ASCII_static(udp_text_m));
            udp_send(fsk.udp, stprintf("%s\n", udp_text_m), sl + 1);
        }
        kiwi_asfree(udp_text_m);
        return true;
    }

	return false;
}

void fsk_close(int rx_chan)
{
	fsk_chan_t *e = &fsk_chan[rx_chan];
    ext_unregister_receive_real_samps(e->rx_chan);
}

bool FSK_vars() { return false; }

void FSK_main();

ext_t fsk_ext = {
	"FSK",
	FSK_main,
	fsk_close,
	fsk_msgs,
	EXT_NEW_VERSION,
	EXT_FLAGS_HEAVY
};

void FSK_main()
{
	ext_register(&fsk_ext);

    const char *fn = cfg_string("fsk.test_file", NULL, CFG_OPTIONAL);
    if (!fn || *fn == '\0') return;
    char *fn2;
    asprintf(&fn2, "%s/samples/%s", DIR_CFG, fn);
    cfg_string_free(fn);
    printf("FSK: mmap %s\n", fn2);
    int fd = open(fn2, O_RDONLY);
    if (fd < 0) {
        printf("FSK: open failed\n");
        return;
    }
    off_t fsize = kiwi_file_size(fn2);
    kiwi_asfree(fn2);
    char *file = (char *) mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file == MAP_FAILED) {
        printf("FSK: mmap failed\n");
        return;
    }
    close(fd);
    int words = fsize/2;
    fsk.s2p_start = (s2_t *) file;
    u4_t off = *(fsk.s2p_start + 3);
    off = FLIP16(off);
    printf("FSK: size=%ld\n", fsize);
    off /= 2;
    fsk.s2p_start += off;
    words -= off;
    fsk.s2p_end = fsk.s2p_start + words;
    fsk.tsamps = words;
}
