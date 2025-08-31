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

// Copyright (c) 2015-2025 John Seamons, ZL4VO/KF6VO

#pragma once

#ifdef PLATFORM_beagleY_ai
 #define GPIO_HAT
#else
 #define GPIO_P8_P9
 #define HAS_BEAGLE_4_LEDS
#endif

#ifdef CPU_TDA4VM
 #define CPU_FREQ_NOM 2000000
#elif defined(CPU_AM67) || defined(AM62)
 #define CPU_FREQ_NOM 1400000
#else
 #define CPU_FREQ_NOM 1000000
#endif

#if defined(CPU_AM5729) || defined(CPU_BCM2837)
 #define HAS_CPU_FREQ
#endif

#ifdef CPU_AM3359
#else
 #define HAS_CPU_TEMP
#endif
