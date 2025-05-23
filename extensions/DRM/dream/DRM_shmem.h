// Copyright (c) 2017-2025 John Seamons, ZL4VO/KF6VO

#pragma once

#include "shmem_config.h"

#ifdef DRM
    #ifdef MULTI_CORE
        //#define DRM_SHMEM_DISABLE_TEST
        #ifdef DRM_SHMEM_DISABLE_TEST
            #warning do not forget to remove DRM_SHMEM_DISABLE_TEST
            #define DRM_SHMEM_DISABLE
            #define RX_SHMEM_DISABLE
        #else
            // shared memory enabled
        #endif
    #else
        // normally shared memory disabled
        // but could be enabled for testing
        #define DRM_SHMEM_DISABLE
        #define RX_SHMEM_DISABLE
    #endif
#else
    #define DRM_SHMEM_DISABLE
#endif

#include "shmem.h"

#ifdef DRM_SHMEM_DISABLE
    extern drm_shmem_t *drm_shmem_p;
    #define DRM_SHMEM drm_shmem_p

    #define DRM_YIELD() NextTask("drm Y");
    #define DRM_YIELD_LOWER_PRIO() TaskSleepReasonUsec("drm YLP", 1000);
#else
    #define DRM_SHMEM (&shmem->drm_shmem)
    
    #ifdef MULTI_CORE
        // Processes run in parallel simultaneously and communicate w/ shmem mechanism.
        // But sleep a little bit to reduce cpu busy looping waiting for updates to shmem.
        // But don't sleep _too_ much else insufficient throughput.
        #define DRM_YIELD() kiwi_usleep(30000);
        //#define DRM_YIELD() kiwi_usleep(10000);
        #define DRM_YIELD_LOWER_PRIO() kiwi_usleep(1000);
    #else
        // experiment with shmem mechanism on uni-processors
        #define DRM_YIELD() kiwi_usleep(100);  // force process switch
    #endif
#endif

