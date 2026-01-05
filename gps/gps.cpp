//////////////////////////////////////////////////////////////////////////
// Homemade GPS Receiver
// Copyright (C) 2013 Andrew Holme
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// http://www.aholme.co.uk/GPS/Main.htm
//////////////////////////////////////////////////////////////////////////

#include "gps.h"
#include "printf.h"

#include <math.h>
#include <stdio.h>

GPS_UTC gps;

unsigned bin(const char *s, int n) {
    unsigned u = *s;
    while (--n) u += u + *++s;
    return u;
}

int parity(u4_t word) {
    u4_t par = word & ((1<<6)-1);
    int p = 0;
    for (int i=0; i<24; i++) p += (word >> (6+i)) & 1;
    return (p & 1) == par;
}

int preamble(u4_t word) {
    return (word >> 22) == PREAMBLE;
}
