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

// Copyright (c) 2023-2025 John Seamons, ZL4VO/KF6VO

#include "types.h"
#include "config.h"
#include "kiwi.h"
#include "rx.h"
#include "rx_util.h"
#include "str.h"
#include "mem.h"
#include "misc.h"
#include "coroutines.h"
#include "debug.h"
#include "printf.h"
#include "leds.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <stdlib.h>

bool backup_in_progress;

static int sd_cmd(conn_t *conn, const char *cmd, const char *kill1, const char *kill2, const char *cleanup,
    bool from_admin, bool change_espeed)
{
	char *sb, *sb2;
	char *cmd_p, *buf_m;
	
    backup_in_progress = true;  // NB: must be before rx_server_kick() to prevent new connections
    rx_server_kick(KICK_ALL);      // kick everything (including autorun) off to speed up copy
    
    bool restore_espeed_10M = false;
    if (change_espeed && kiwi.current_espeed == ESPEED_10M) {
        mprintf("Ethernet: temporary switch from 10 to 100 mbps\n");
        non_blocking_cmd_system_child("kiwi.ethtool",
            stprintf("ethtool -s eth0 speed %d duplex full", 100), NO_WAIT);
        TaskSleepReasonSec("espeed change", 5);
        restore_espeed_10M = true;
    } else {
        // if this delay isn't here the subsequent non_blocking_cmd_popen() hangs for
        // MINUTES, if there is a user connection open, for reasons we do not understand
        TaskSleepReasonSec("kick delay", 5);
    }
    
    // clear user list on status tab
    sb = rx_users(IS_ADMIN);
    send_msg(conn, false, "MSG user_cb=%s", kstr_sp(sb));
    kstr_free(sb);
    
    #define NBUF 256
    char *buf = (char *) kiwi_malloc("sd_cmd", NBUF);
    int i, n, err;
    
    sd_copy_in_progress = true;
    if (kiwi_nonEmptyStr(cleanup)) system(cleanup);

    non_blocking_cmd_t p;
    p.cmd = cmd;
    //real_printf("sd_cmd: non_blocking_cmd_popen..\n");
    non_blocking_cmd_popen(&p);
    //real_printf("sd_cmd: ..non_blocking_cmd_popen\n");
    for (i = n = 0; n >= 0; i++) {
        n = non_blocking_cmd_read(&p, buf, NBUF);
        //real_printf("sd_cmd: n=%d\n", n);
        if (n > 0) {
            //real_printf("sd_cmd: mprintf %d %d <%s>\n", n, strlen(buf), buf);
            mprintf("%s", buf);
        }
        TaskSleepMsec(250);
        u4_t now = timer_sec();
        if ((now - conn->keepalive_time) > 5) {
            send_msg(conn, false, "MSG keepalive");
            conn->keepalive_time = now;
        }
        
        // if admin connection lost stop command
        if (conn->kick) {
            if (debian_ver >= 10) {
                if (kiwi_nonEmptyStr(kill1)) system(stprintf("pkill %s", kill1));
                if (kiwi_nonEmptyStr(kill2)) system(stprintf("pkill %s", kill2));
            } else {
                if (kiwi_nonEmptyStr(kill1)) system(stprintf("ps laxww|grep %s|awk '{print $3}'|xargs -n 1 kill", kill1));
                if (kiwi_nonEmptyStr(kill2)) system(stprintf("ps laxww|grep %s|awk '{print $3}'|xargs -n 1 kill", kill2));
            }
            if (kiwi_nonEmptyStr(cleanup)) system(cleanup);
            cprintf(conn, "sd_cmd: KICKED\n");
            break;
        }
    }
    err = non_blocking_cmd_pclose(&p);
    //real_printf("sd_cmd: err=%d\n", err);
    sd_copy_in_progress = false;
    
    err = (err < 0)? err : WEXITSTATUS(err);
    mprintf("sd_cmd: system returned %d\n", err);
    kiwi_free("sd_cmd", buf);
    #undef NBUF

    if (restore_espeed_10M) {
        mprintf("Ethernet: restored to 10 mbps\n");
        non_blocking_cmd_system_child("kiwi.ethtool",
            stprintf("ethtool -s eth0 speed %d duplex full", 10), NO_WAIT);
        TaskSleepReasonSec("espeed change", 5);
    }

    backup_in_progress = false;
    if (from_admin) rx_autorun_restart_victims(true);
    //real_printf("sd_cmd: sd_done=%d\n", err);
    return err;
}

#define SD_CMD_DIR "cd /root/" REPO_NAME "/tools; "
#define SD_CMD_OLD SD_CMD_DIR "./kiwiSDR-make-microSD-flasher-from-eMMC.sh"
#define SD_CMD_NEW SD_CMD_DIR "cp /etc/beagle-flasher/%s-emmc-to-microsd /etc/default/beagle-flasher; ./%s-flasher.sh"

void sd_backup(conn_t *conn, bool from_admin)
{
	#if 0
        if (!kiwi.dbgUs && (debian_maj == 11 || (debian_maj == 12 && debian_min < 4))) {
            send_msg(conn, SM_NO_DEBUG, "%s sd_done=87", from_admin? "ADM":"MFG");
            return;
        }
    #endif

    // On BBAI-64, backup script only supports dual-partition setups (i.e. /boot/firmware on p1)
    if (kiwi.platform == PLATFORM_BBAI_64 && (
        !kiwi_file_exists("/boot/firmware") ||
        !kiwi_file_exists("/boot/firmware/extlinux") ||
        !kiwi_file_exists("/boot/firmware/extlinux/extlinux.conf")
        )) {
        send_msg(conn, SM_NO_DEBUG, "%s sd_done=88", from_admin? "ADM":"MFG");
        return;
    }
    
    led_task_stop();
    
    char *cmd;
    const char *platform = platform_s[kiwi.platform];
    bool D11_plus = (debian_ver >= 11);
    asprintf(&cmd, D11_plus? SD_CMD_NEW : SD_CMD_OLD, platform, platform);
    cprintf(conn, "sd_backup: kiwi.platform=%d <%s>\n", kiwi.platform, cmd);
    int err = sd_cmd(conn, cmd, "rsync", NULL, NULL, from_admin, false);
    kiwi_asfree(cmd);
    send_msg(conn, SM_NO_DEBUG, "%s sd_done=%d", from_admin? "ADM":"MFG", err);
    
    led_task_start();
}

#define UPG_CMD "cd /root/" REPO_NAME "/tools; ./kiwiSDR-make-Debian-11-flasher.sh"
#define UPG_CLEANUP "cd /root; rm -f *.sha *.img.xz"

void sd_upgrade(conn_t *conn)
{
    //#define UPG_TEST
    #ifdef UPG_TEST
    #else
        if (debian_maj >= 11) {
            send_msg(conn, SM_NO_DEBUG, "ADM sd_done=90");
            return;
        }
    
        if (kiwi.platform != PLATFORM_BBG_BBB) {
            send_msg(conn, SM_NO_DEBUG, "ADM sd_done=91");
            return;
        }
    #endif
    
    char *cmd;
    asprintf(&cmd, UPG_CMD);
    cprintf(conn, "sd_upgrade: kiwi.platform=%d <%s>\n", kiwi.platform, cmd);
    int err = sd_cmd(conn, cmd, "curl", "xzcat", UPG_CLEANUP, true, true);
    kiwi_asfree(cmd);
    if (err == 0) err = -1;
    send_msg(conn, SM_NO_DEBUG, "ADM sd_done=%d", err);
}
