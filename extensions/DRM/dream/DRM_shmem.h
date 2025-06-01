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
#else
    #define DRM_SHMEM (&shmem->drm_shmem)
#endif
