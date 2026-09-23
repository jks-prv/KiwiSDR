// Copyright (c) 2025-2026 John Seamons, ZL4VO/KF6VO
// All rights reserved.

#include "ext.h"	// all calls to the extension interface begin with "ext_", e.g. ext_register()

#include "kiwi.h"
#include "misc.h"
#include "mem.h"
#include "rx.h"
#include "net.h"
#include "ansi.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <sys/mman.h>

//#define DEBUG_MSG	true
#define DEBUG_MSG	false

// rx_chan is the receiver channel number we've been assigned, 0..rx_chans
// We need this so the extension can support multiple users, each with their own freedv[] data structure.

typedef struct {
	int rx_chan;
	int run;
	bool debug;

	bool task_created;
	tid_t tid;
	int rd_pos;

	bool test;
    s2_t *s2p;
} freedv_t;

static freedv_t freedv[MAX_RX_CHANS];

typedef struct {
    s2_t *s2p_start, *s2p_end;
    int tsamps;
} freedv_conf_t;

freedv_conf_t freedv_conf;

void freedv_close(int rx_chan)
{
	freedv_t *e = &freedv[rx_chan];
    rcprintf(rx_chan, "FreeDV: close rx_chan=%d task_created=%d\n", rx_chan, e->task_created);
    ext_unregister_receive_real_samps_task(rx_chan);

    #if 0
        if (e->task_created) {
            //rcprintf(rx_chan, "FreeDV: TaskRemove\n");
            TaskRemove(e->tid);
            e->task_created = false;
        }
    #endif

    ext_unregister_receive_cmds(e->rx_chan);
}

static void freedv_file_data(int rx_chan, int chan, int nsamps, TYPEMONO16 *samps, int freqHz)
{
    freedv_t *e = &freedv[rx_chan];

    if (!e->test) {
        return;
    }
    if (e->s2p >= freedv_conf.s2p_end) {
        e->test = false;
        return;
    }
    
    if (e->test) {
        for (int i = 0; i < nsamps; i++) {
            if (e->s2p < freedv_conf.s2p_end) {
                *samps++ = (s2_t) FLIP16(*e->s2p);
            }
            e->s2p++;
        }

        #if 0
        int pct = e->nsamps * 100 / freedv_conf.tsamps;
        e->nsamps += nsamps;
        pct += 3;
        if (pct > 100) pct = 100;
        ext_send_msg(rx_chan, false, "EXT bar_pct=%d", pct);
        #endif
    }

}

// NB: this is being called in the context of the snd, wf or mon thread
bool freedv_receive_cmds(u2_t key, char *cmd, int rx_chan)
{
    if (strncmp(cmd, "SET rev_txt=", 12) == 0) {
	    freedv_t *e = &freedv[rx_chan];
        //real_printf(BLUE "%d" NORM " ", strlen(cmd)); fflush(stdout);
        //real_printf("FreeDV (%d) %s\n", strlen(cmd), cmd); fflush(stdout);
        ext_send_msg_encoded(rx_chan, DEBUG_MSG, "EXT", "reporter_json", &cmd[12]);

        // still encoded from mon reception of "SET rev_txt="
        //ext_send_msg(rx_chan, DEBUG_MSG, "EXT reporter_json=%s", &cmd[12]);
        return true;
    }

    return false;
}

bool freedv_msgs(char *msg, int rx_chan)
{
	freedv_t *e = &freedv[rx_chan];
    e->rx_chan = rx_chan;	// remember our receiver channel number
	int i, n;
    char *cmd_p;
	
	//printf("### freedv_msgs RX%d <%s>\n", rx_chan, msg);
	
	if (strcmp(msg, "SET ext_server_init") == 0) {
        ext_send_msg_encoded(rx_chan, DEBUG_MSG, "EXT", "ready", "<bvwk>7:2:``g3g37a03;7e1be322f36aa16;2,%s", net.unique_id);
		return true;
	}
	
	if (strcmp(msg, "SET freedv_setup") == 0) {
	    e->debug = kiwi.dbgUs;
        conn_t *conn = rx_channels[rx_chan].conn;

		if (freedv_conf.tsamps != 0) {
            ext_register_receive_real_samps(freedv_file_data, rx_chan);
		}

        #if 0
            if (!e->task_created) {
                e->tid = CreateTaskF(freedv_task, TO_VOID_PARAM(rx_chan), EXT_PRIORITY, CTF_RX_CHANNEL | (rx_chan & CTF_CHANNEL));
                e->task_created = true;
            }
    
            ext_register_receive_real_samps_task(e->tid, rx_chan);
        #endif
        
		ext_register_receive_cmds(freedv_receive_cmds, rx_chan);
		return true;
	}

	if (strcmp(msg, "SET freedv_close") == 0) {
		//rcprintf(rx_chan, "freedv_close\n");
		freedv_close(rx_chan);
		return true;
	}

	int start;
	if (sscanf(msg, "SET freedv_start=%d", &start) == 1) {
	    wf_inst_t *wf = &WF_SHMEM->wf_inst[rx_chan];
	    if (!wf) return true;
	    if (start) {
            wf->want_rtn_snd = true;
	    } else {
            wf->want_rtn_snd = false;
        }
		return true;
	}

	int test;
	if (sscanf(msg, "SET freedv_test=%d", &test) == 1) {
		rcprintf(rx_chan, "FreeDV test=%d\n", test);
	    wf_inst_t *wf = &WF_SHMEM->wf_inst[rx_chan];
	    if (!wf) return true;
		if (test) {
            wf->want_rtn_snd = true;
            e->s2p = freedv_conf.s2p_start;
            e->test = true;
        } else {
            wf->want_rtn_snd = false;
            e->test = false;
        }
		return true;
	}

	return false;
}

void FreeDV_main();

ext_t freedv_ext = {
	"FreeDV",
	FreeDV_main,
	freedv_close,
	freedv_msgs,
	EXT_NEW_VERSION
};

bool FreeDV_vars() { return ext_vars(&freedv_ext, "freedv"); }

void FreeDV_main()
{
	ext_register(&freedv_ext);

    const char *fn = "FreeDV.test.au";
    if (!fn || *fn == '\0') return;
    char *fn2;
    asprintf(&fn2, "%s/samples/%s", DIR_CFG, fn);
    //cfg_string_free(fn);
    printf("FreeDV: mmap %s\n", fn2);
    int fd = open(fn2, O_RDONLY);
    if (fd < 0) {
        printf("FreeDV: open failed\n");
        return;
    }
    off_t fsize = kiwi_file_size(fn2);
    kiwi_asfree(fn2);
    char *file = (char *) mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file == MAP_FAILED) {
        printf("FreeDV: mmap failed\n");
        return;
    }
    close(fd);
    int words = fsize/2;
    freedv_conf.s2p_start = (s2_t *) file;
    u4_t off = *(freedv_conf.s2p_start + 3);
    off = FLIP16(off);
    printf("FreeDV: off=%d size=%ld\n", off, fsize);
    off /= 2;
    freedv_conf.s2p_start += off;
    words -= off;
    freedv_conf.s2p_end = freedv_conf.s2p_start + words;
    freedv_conf.tsamps = words / NIQ;
}
