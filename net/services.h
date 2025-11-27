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

// Copyright (c) 2022 John Seamons, ZL4VO/KF6VO

#pragma once

typedef enum { NAT_NO_DELETE, NAT_DELETE } nat_delete_e;
void UPnP_port(nat_delete_e nat_delete);

typedef enum { WAKEUP_REG, WAKEUP_REG_STATUS } wakeup_reg_e;
bool wakeup_reg_kiwisdr_com(wakeup_reg_e wakeup_reg);

#define FILE_DOWNLOAD_RELOAD        0
#define FILE_DOWNLOAD_DIFF_RESTART  1

void my_kiwi_register(bool reg = true, int root_pwd_unset = 0, int debian_pwd_default = 0);
void file_GET(void *param);

// net.proxy_status
#define PR_REG_OK           0
#define PR_NEW_ACCT_OK      1
#define PR_UPD_HOSTNAME_OK  2
#define PR_USER_HOST_BLANK  100
#define PR_BAD_USER_KEY     101
#define PR_HOST_INUSE       102
#define PR_INVALID_CHARS    103
#define PR_AUTO_NO_HOST     150
#define PR_AUTO_NO_USER     151
#define PR_AUTO_DUP_ACCT    152
#define PR_RUNNING          200
#define PR_PENDING          201
#define PR_NO_CONTACT       900
#define PR_INVALID_STATUS   901

void proxy_frpc_setup(const char *proxy_server, const char *user, const char *host, int port);
void proxy_frpc_restart();
