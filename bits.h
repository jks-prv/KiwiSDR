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

#include "types.h"

typedef u4_t wires;

typedef u1_t wire;
typedef u1_t wire2;
typedef u1_t wire3;
typedef u1_t wire4;
typedef u1_t wire5;
typedef u1_t wire6;
typedef u1_t wire7;
typedef u1_t wire8;

typedef u2_t wire9;
typedef u2_t wire10;
typedef u2_t wire11;
typedef u2_t wire12;
typedef u2_t wire13;
typedef u2_t wire14;
typedef u2_t wire15;
typedef u2_t wire16;

typedef u4_t wire20;
typedef u4_t wire32;

typedef u64_t wire40;

typedef u1_t reg;
typedef u1_t reg2;
typedef u1_t reg3;
typedef u1_t reg4;
typedef u1_t reg5;
typedef u1_t reg6;
typedef u1_t reg7;
typedef u1_t reg8;

typedef u2_t reg9;
typedef u2_t reg10;
typedef u2_t reg11;
typedef u2_t reg12;
typedef u2_t reg13;
typedef u2_t reg14;
typedef u2_t reg15;
typedef u2_t reg16;

typedef u4_t reg32;

// NB: using e.g. "Bn" here instead of "BTn" conflicts with system include files

#define BT0      0x00000001
#define BT1      0x00000002
#define BT2      0x00000004
#define BT3      0x00000008

#define BT4      0x00000010
#define BT5      0x00000020
#define BT6      0x00000040
#define BT7      0x00000080

#define BT8      0x00000100
#define BT9      0x00000200
#define BT10     0x00000400
#define BT11     0x00000800

#define BT12     0x00001000
#define BT13     0x00002000
#define BT14     0x00004000
#define BT15     0x00008000

#define BT16     0x00010000
#define BT17     0x00020000
#define BT18     0x00040000
#define BT19     0x00080000
#define BT20     0x00100000

#define BT31     0x80000000

// bits
#define b0(w)   (((w) >> 0) & 1)
#define b1(w)   (((w) >> 1) & 1)
#define b2(w)   (((w) >> 2) & 1)
#define b3(w)   (((w) >> 3) & 1)
#define b4(w)   (((w) >> 4) & 1)
#define b5(w)   (((w) >> 5) & 1)
#define b6(w)   (((w) >> 6) & 1)
#define b7(w)   (((w) >> 7) & 1)

#define b8(w)   (((w) >> 8) & 1)
#define b9(w)   (((w) >> 9) & 1)
#define b10(w)  (((w) >> 10) & 1)
#define b11(w)  (((w) >> 11) & 1)
#define b12(w)  (((w) >> 12) & 1)
#define b13(w)  (((w) >> 13) & 1)
#define b14(w)  (((w) >> 14) & 1)
#define b15(w)  (((w) >> 15) & 1)

#define b16(w)  (((w) >> 16) & 1)
#define b17(w)  (((w) >> 17) & 1)
#define b18(w)  (((w) >> 18) & 1)
#define b19(w)  (((w) >> 19) & 1)
#define b20(w)  (((w) >> 20) & 1)
#define b31(w)  (((w) >> 31) & 1)
#define b32(w)  (((w) >> 32) & 1)
#define b39(w)  (((w) >> 39) & 1)

#define bn(w,b)     ( ((w) & (1 << (b)))? 1:0 )
#define bf(f,s,e)   ( ((f) >> (e)) & ((1U << ((s)-(e)+1)) - 1)  )
#define bf64(f,s,e) ( ((f) >> (e)) & ((1ULL << ((s)-(e)+1)) - 1LL)  )

#define b7_0(w)     ((w) & 0xff)
#define b15_8(w)    (((w) >> 8) & 0xff)

#define BIT(w,b)    bn(w,b)
#define BIS(w,b)    (w) |= 1 << (b)
#define BIC(w,b)    (w) &= ~(1 << (b))
#define BIW(w,b)    ( ((w) & ~(1 << (b))) | (1 << (b)) )

// extract bytes
#define	BYTE3(i)    (((i) >> 24) & 0xff)
#define	BYTE2(i)    (((i) >> 16) & 0xff)
#define	BYTE1(i)    (((i) >>  8) & 0xff)
#define	BYTE0(i)    (((i) >>  0) & 0xff)

// field combine

#define FC2(f1,s1, f0,s0) \
    ((f1) << (s1) | (f0) << (s0))

#define FC3(f2,s2, f1,s1, f0,s0) \
    ((f2) << (s2) | (f1) << (s1) | (f0) << (s0))

#define FC4(f3,s3, f2,s2, f1,s1, f0,s0) \
    ((f3) << (s3) | (f2) << (s2) | (f1) << (s1) | (f0) << (s0))

#define FC5(f4,s4, f3,s3, f2,s2, f1,s1, f0,s0) \
    ((f4) << (s4) | (f3) << (s3) | (f2) << (s2) | (f1) << (s1) | (f0) << (s0))

#define FC6(f5,s5, f4,s4, f3,s3, f2,s2, f1,s1, f0,s0) \
    ((f5) << (s5) | (f4) << (s4) | (f3) << (s3) | (f2) << (s2) | (f1) << (s1) | (f0) << (s0))

#define FC7(f6,s6, f5,s5, f4,s4, f3,s3, f2,s2, f1,s1, f0,s0) \
    ((f6) << (s6) | (f5) << (s5) | (f4) << (s4) | (f3) << (s3) | (f2) << (s2) | (f1) << (s1) | (f0) << (s0))

#define FC8(f7,s7, f6,s6, f5,s5, f4,s4, f3,s3, f2,s2, f1,s1, f0,s0) \
    ((f7) << (s7) | (f6) << (s6) | (f5) << (s5) | (f4) << (s4) | (f3) << (s3) | (f2) << (s2) | (f1) << (s1) | (f0) << (s0))

#define m0(w)   ((w)? BT0 : 0)
#define m1(w)   ((w)? BT1 : 0)
#define m2(w)   ((w)? BT2 : 0)
#define m3(w)   ((w)? BT3 : 0)
#define m4(w)   ((w)? BT4 : 0)
#define m5(w)   ((w)? BT5 : 0)
#define m6(w)   ((w)? BT6 : 0)
#define m7(w)   ((w)? BT7 : 0)

#define m8(w)   ((w)? BT8 : 0)
#define m9(w)   ((w)? BT9 : 0)
#define m10(w)  ((w)? BT10 : 0)
#define m11(w)  ((w)? BT11 : 0)
#define m12(w)  ((w)? BT12 : 0)
#define m13(w)  ((w)? BT13 : 0)
#define m14(w)  ((w)? BT14 : 0)
#define m15(w)  ((w)? BT15 : 0)

#define m16(w)  ((w)? BT16 : 0)
#define m17(w)  ((w)? BT17 : 0)
#define m18(w)  ((w)? BT18 : 0)
#define m19(w)  ((w)? BT19 : 0)
#define m20(w)  ((w)? BT20 : 0)
#define m31(w)  ((w)? BT31 : 0)

// mask field
#define mf(s,e)  ( ((1 << (s+1)) - 1) & ~((1 << (e)) - 1) )

// bit field set
#define bfs(dst,s,e,src)  ( ((dst) & ~mf(s,e)) | ((src) << (e)) )

// bit set
#define bset_v(dst,v,src)  ( ((dst) & ~(v)) | (((src) & (v))? (v) : 0) )
#define bset_n(dst,b,src)  ( ((dst) & ~mf(b,b)) | (((src) & (1 << (b)))? (1 << (b)) : 0) )
