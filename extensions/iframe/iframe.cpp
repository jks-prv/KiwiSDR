// Copyright (c) 2020 Kari Karvonen, OH1KK

#include "ext.h"	// all calls to the extension interface begin with "ext_", e.g. ext_register()

#include "kiwi.h"
#include "cfg.h"
#include "str.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>

#define IFRAME_DEBUG_MSG	false

// rx_chan is the receiver channel number we've been assigned, 0..rx_chans
// We need this so the extension can support multiple users, each with their own iframe[] data structure.

struct iframe_t {
	u1_t rx_chan;
};

static iframe_t iframe[MAX_RX_CHANS];

bool iframe_msgs(char *msg, int rx_chan)
{
	iframe_t *e = &iframe[rx_chan];
    e->rx_chan = rx_chan;	// remember our receiver channel number

	if (strcmp(msg, "SET ext_server_init") == 0) {
		ext_send_msg(rx_chan, IFRAME_DEBUG_MSG, "EXT ready");
		return true;
	}
    
    return false;
}

void iframe_close(int rx_chan)
{
    // do nothing
}

bool iframe_vars()
{
    bool up_cfg = false;
    
    // enable dx spots if no prior configuration
    cfg_default_object("iframe", "{}", &up_cfg);
    const char *s = cfg_string("iframe.url", NULL, CFG_OPTIONAL);
    const char *s2 = cfg_string("iframe.html", NULL, CFG_OPTIONAL);
    bool enabled = cfg_default_bool("iframe.enable", true, &up_cfg);
    if (kiwi_emptyStr(s) && kiwi_emptyStr(s2) && enabled) {
        cfg_set_int("iframe.src", 0);
	    cfg_set_string("iframe.url", "https://spots.kiwisdr.com");
	    cfg_set_string("iframe.title", "<span style=\\\"color:cyan\\\">Spots by <a href=\\\"http://kiwisdr.com\\\" target=\\\"_blank\\\">kiwisdr.com</a></span>");
        cfg_set_string("iframe.menu", "DX spots");
        cfg_set_string("iframe.help", "Clicking on a spot frequency will tune the Kiwi.");
        cfg_set_bool("iframe.allow_tune", true);
	    UPDATE_CFG_BREAK(up_cfg);
    } else {
        const char *s3 = cfg_string("iframe.title", NULL, CFG_OPTIONAL);
        if (kiwi_nonEmptyStr(s3) && strstr(s3, "sk6aw")) {
	        cfg_set_string("iframe.title", "<span style=\\\"color:cyan\\\">Spots by <a href=\\\"http://kiwisdr.com\\\" target=\\\"_blank\\\">kiwisdr.com</a></span>");
	        UPDATE_CFG_BREAK(up_cfg);
        }
        cfg_string_free(s3);
    }
    cfg_string_free(s);
    cfg_string_free(s2);
    
    return up_cfg;
}

void iframe_main();

ext_t iframe_ext = {
	"iframe",
	iframe_main,
	iframe_close,
	iframe_msgs,
};

void iframe_main()
{
	ext_register(&iframe_ext);
}
