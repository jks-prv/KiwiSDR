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

// NOTE: updated 17-Dec-2025
// Rules enforced:
// - USABLE => enabled
// - NOT USABLE / DECOMMISSIONED / RETIRED => commented out (disable) with //
// - UNDER COMMISSIONING / TEST => commented out (disable) with //

SATELLITE Sats[] = {

    // SBAS PRN 120-158
    // en.wikipedia.org/wiki/GPS-aided_GEO_augmented_navigation
    // 11/04/2025 SBAS update based on information of www.gps.gov/pseudorandom-noise-code-assignments
    // PRN, G2 delay, G2 init
    // NB: listed first so better chance of obtaining a channel

    // WAAS (USA SBAS)
    {131, 1012, 00551, SBAS},  // Eutelsat 117 West B
    {133,  603, 01731, SBAS},  // SES-15
    {135,  359, 01216, SBAS},  // Intelsat Galaxy 30
  //{138,  386, 00450, SBAS},  // Ceased operational WAAS transmissions on May 17, 2022. en.wikipedia.org/wiki/Anik_F1R

    // UK-SBAS (United-Kingdom SBAS)
    {158,  904, 01542, SBAS},  // UK SBAS Testbed over Inmarsat 3F5

    // EGNOS (Europe SBAS)
    {121,  175, 01241, SBAS},  // Eutelsat 5WB en.wikipedia.org/wiki/Eutelsat_5_West_B
    {123,   21, 00232, SBAS},  // ASTRA 5B
    {126,  886, 01764, SBAS},  // Inmarsat 4F2
    {136,  595, 00740, SBAS},  // HOTBIRD 13G en.wikipedia.org/wiki/Hot_Bird#Hotbird_13G
  //{150,  853, 01041, SBAS},  // reserved

    // SDCM (RUSSIA SBAS)
    {125, 235, 01076, SBAS},   // Luch-5B Satellite en.wikipedia.org/wiki/Luch_5B
    {140, 456, 01653, SBAS},   // Luch-5V Satellite en.wikipedia.org/wiki/Luch_5V
    {141, 499, 01411, SBAS},   // Luch-5A Satellite en.wikipedia.org/wiki/Luch_5A

    // ALSBAS/ASAL (Algeria SBAS)
    {148,  163, 00335, SBAS},  // ALCOMSAT-1 en.wikipedia.org/wiki/Alcomsat-1

    // ASECNA/A-SBAS (Africa and Indian Ocean SBAS)
    {120,  145, 01106, SBAS},  // SES-12
    {147,  118, 00355, SBAS},  // NigComSat-1R

    // GAGAN (India SBAS)
    {127,  657, 00717, SBAS},  // GSAT-8 en.wikipedia.org/wiki/GSAT-8 
    {128,  634, 01532, SBAS},  // GSAT-10 en.wikipedia.org/wiki/GSAT-10
    {132,  176, 00520, SBAS},  // GSAT-15 en.wikipedia.org/wiki/GSAT-15

    // BDSBAS (China SBAS)  en.wikipedia.org/wiki/BeiDou#BeiDou-3
    {130,  355, 00341, SBAS},  // Compass-G1
    {143,  307, 01312, SBAS},  // Compass-G3
    {144,  127, 01060, SBAS},  // Compass-G2

    // KASS (South Korea SBAS)
    {134,  130, 00706, SBAS},  // MEASAT-3D en.wikipedia.org/wiki/MEASAT_Satellite_Systems

    // MSAS (Japan SBAS)
    {129,  762, 01250, SBAS},  // QZS-3
    {137,   68, 01007, SBAS},  // QZS-3
  //{139,  797, 00305, SBAS},  // QZS-7 - Launch planned (PRN assignment exists, not used here)

    // SPAN/SouthPAN (AUS/NZ SBAS)
    {122,   52, 00267, SBAS},  // Inmarsat 4F1

    // Navstar
    // www.navcen.uscg.gov/gps-constellation
    // PRN, G2 tap, G2 tap
    
    { 1,  2,  6, Navstar},
    { 2,  3,  7, Navstar},
    { 3,  4,  8, Navstar},
    { 4,  5,  9, Navstar},
    { 5,  1,  9, Navstar},
    { 6,  2, 10, Navstar},
    { 7,  1,  8, Navstar},
    { 8,  2,  9, Navstar},
    { 9,  3, 10, Navstar},
    {10,  2,  3, Navstar},
    {11,  3,  4, Navstar},
    {12,  5,  6, Navstar},
    {13,  6,  7, Navstar},
    {14,  7,  8, Navstar},
    {15,  8,  9, Navstar},
    {16,  9, 10, Navstar},
    {17,  1,  4, Navstar},
    {18,  2,  5, Navstar},
    {19,  3,  6, Navstar},
    {20,  4,  7, Navstar},
    {21,  5,  8, Navstar},
    {22,  6,  9, Navstar},
    {23,  1,  3, Navstar},
    {24,  4,  6, Navstar},
    {25,  5,  7, Navstar},
    {26,  6,  8, Navstar},
    {27,  7,  9, Navstar},
    {28,  8, 10, Navstar},
    {29,  1,  6, Navstar},
    {30,  2,  7, Navstar},
    {31,  3,  8, Navstar},
    {32,  4,  9, Navstar},

    // QZSS (Japan) prn(saif) = 183++, prn(std) = 193++
    // last checked: 17-Dec-2025
    // KiwiSDR typically tracks GPS-like L1 C/A. Keep only QZSS PRNs that transmit L1 C/A.
    // PRN, G2 delay, G2 init

//  {193, 339, 01050, QZSS},   // SVN1, QZS-1, terminated 15-Sep-2023
    {194, 208, 01607, QZSS},   // SVN2, QZS-2, L1 C/A
    {195, 711, 01747, QZSS},   // SVN4, QZS-4, L1 C/A
//  {196, 189, 01305, QZSS},   // SVN5, QZS-1R: L1C/L1C(B) (no L1 C/A) -> disable for L1 C/A-only tracking
    {199, 663, 00727, QZSS},   // SVN3, QZS-3, L1 C/A
//  {200, 942, 00147, QZSS},   // SVN7, QZS-6: L1C/L1C(B) (no L1 C/A) -> disable for L1 C/A-only tracking
//  {201, 173, 01206, QZSS},   // QZS-7: not launched / not operational on L1 C/A
//  {202, 900, 01045, QZSS},   // QZSS Test

    // Galileo E1B
    // last checked: 17-Dec-2025
    // Source: GSC Constellation Information (status) + NAGUs
    // PRN, (E1B codes derived separately)
    // Keep PRNs strictly in numeric order. No blank lines.

  //{ 1,0,0,E1B},    // GSAT0210 (E01) NOT USABLE
    { 2,0,0,E1B},    // GSAT0211 (E02) USABLE
    { 3,0,0,E1B},    // GSAT0212 (E03) USABLE
    { 4,0,0,E1B},    // GSAT0213 (E04) USABLE
    { 5,0,0,E1B},    // GSAT0214 (E05) USABLE
    { 6,0,0,E1B},    // GSAT0227 (E06) USABLE
    { 7,0,0,E1B},    // GSAT0207 (E07) USABLE
    { 8,0,0,E1B},    // GSAT0208 (E08) USABLE
    { 9,0,0,E1B},    // GSAT0209 (E09) USABLE
    {10,0,0,E1B},    // GSAT0224 (E10) USABLE
    {11,0,0,E1B},    // GSAT0101 (E11) USABLE
    {12,0,0,E1B},    // GSAT0102 (E12) USABLE
    {13,0,0,E1B},    // GSAT0220 (E13) USABLE
  //{14,0,0,E1B},    // GSAT0202 (E14) NOT USABLE
    {15,0,0,E1B},    // GSAT0221 (E15) USABLE
    {16,0,0,E1B},    // GSAT0232 (E16) USABLE
  //{17,0,0,E1B},    // unused
  //{18,0,0,E1B},    // GSAT0201 (E18) NOT USABLE
    {19,0,0,E1B},    // GSAT0103 (E19) USABLE
  //{20,0,0,E1B},    // GSAT0104 (E20) RETIRED/DECOMMISSIONED
    {21,0,0,E1B},    // GSAT0215 (E21) USABLE
  //{22,0,0,E1B},    // GSAT0204 (E22) NOT USABLE
    {23,0,0,E1B},    // GSAT0226 (E23) USABLE
  //{24,0,0,E1B},    // GSAT0205 (E24) DECOMMISSIONED
    {25,0,0,E1B},    // GSAT0216 (E25) USABLE
    {26,0,0,E1B},    // GSAT0203 (E26) USABLE
    {27,0,0,E1B},    // GSAT0217 (E27) USABLE
  //{28,0,0,E1B},    // GSAT0233 (E28) LAUNCHED 17-Dec-2025 05:01 UTC - UNDER COMMISSIONING (NAGU 2025061)
    {29,0,0,E1B},    // GSAT0225 (E29) USABLE
    {30,0,0,E1B},    // GSAT0206 (E30) USABLE
    {31,0,0,E1B},    // GSAT0218 (E31) USABLE
  //{32,0,0,E1B},    // GSAT0234 (E32) LAUNCHED 17-Dec-2025 05:01 UTC - UNDER COMMISSIONING (NAGU 2025061)
    {33,0,0,E1B},    // GSAT0222 (E33) USABLE
    {34,0,0,E1B},    // GSAT0223 (E34) USABLE
  //{35,0,0,E1B},    // unused
    {36,0,0,E1B},    // GSAT0219 (E36) USABLE
    
    {-1}
};
