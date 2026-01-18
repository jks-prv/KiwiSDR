
// Copyright (c) 2025-2026 John Seamons, ZL4VO/KF6VO
// All rights reserved.

#pragma once

#ifdef EXP
    #include "exp2.h"
#else
    struct rx_chan_exp_t {};
    struct wf_inst_exp_t {};
    
    #define rx_server_init_exp()
    #define c2s_sound_exp(rx_chan) false
    #define wf_compute_frame_exp(rx_chan)
    #define c2s_mon_exp(conn_mon) false
#endif
