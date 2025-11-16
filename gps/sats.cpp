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
// http://www.holmea.demon.co.uk/GPS/Main.htm
//////////////////////////////////////////////////////////////////////////

#include "gps.h"

// FIXME: update periodically as new sats are commissioned

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
    {158,  904, 01542, SBAS}, // UK SBAS Testbed over Inmarsat 3F5 www.inmarsat.com/en/news/latest-news/government/2021/first-uk-generated-national-satnav-signal-to-be-delivered.html

    // EGNOS (Europe SBAS)
    {121,  175, 01241, SBAS}, // Eutelsat 5WB en.wikipedia.org/wiki/Eutelsat_5_West_B
    {123,   21, 00232, SBAS}, // ASTRA 5B
    {126,  886, 01764, SBAS}, // Inmarsat 4F2
    {136,  595, 00740, SBAS}, // HOTBIRD 13G en.wikipedia.org/wiki/Hot_Bird#Hotbird_13G
  //{150,  853, 01041, SBAS}, // reserved

    // SDCM (RUSSIA SBAS)
    {125, 235, 01076, SBAS}, // Luch-5B Satellite en.wikipedia.org/wiki/Luch_5B
    {140, 456, 01653, SBAS}, // Luch-5V Satellite en.wikipedia.org/wiki/Luch_5V
    {141, 499, 01411, SBAS}, // Luch-5A Satellite en.wikipedia.org/wiki/Luch_5A

    // ALSBAS/ASAL (Algeria SBAS)
    {148,  163, 00335, SBAS}, // ALCOMSAT-1 en.wikipedia.org/wiki/Alcomsat-1

     // ASECNA/A-SBAS (Africa and Indian Ocean SBAS)
    {120,  145, 01106, SBAS}, // SES-12
    {147,  118, 00355, SBAS}, // NigComSat-1R

    // GAGAN (India SBAS)
    {127,  657, 00717, SBAS}, // GSAT-8 en.wikipedia.org/wiki/GSAT-8 
    {128,  634, 01532, SBAS}, // GSAT-10 en.wikipedia.org/wiki/GSAT-10
    {132,  176, 00520, SBAS}, // GSAT-15 en.wikipedia.org/wiki/GSAT-15

    // BDSBAS (China SBAS)  en.wikipedia.org/wiki/BeiDou#BeiDou-3
    {130,  355, 00341, SBAS}, // G1 en.wikipedia.org/wiki/Compass-G1
    {143,  307, 01312, SBAS}, // G3
    {144,  127, 01060, SBAS}, // G2

    // KASS (South Korea SBAS)
    {134,  130, 00706, SBAS}, // MEASAT-3D en.wikipedia.org/wiki/MEASAT_Satellite_Systems

    // MSAS (Japan SBAS)
    {129,  762, 01250, SBAS}, // QZS-3
    {137,   68, 01007, SBAS}, // QZS-3
  //{139,  797, 00305, SBAS}, // QZS-7 - Launch planned

    // SPAN/SouthPAN (AUS/NZ SBAS)
    {122,   52, 00267, SBAS}, // Inmarsat 4F1

    // reserved
    //{124,  237, 01617, SBAS},

    // unallocated
    //{142,  883, 01644, SBAS},
    //{145,  211, 01560, SBAS},
    //{146,  121, 00035, SBAS},
    //{149,  628, 01254, SBAS},
    //{151,  484, 00142, SBAS},
    //{152,  289, 01641, SBAS},
    //{153,  811, 01504, SBAS},
    //{154,  202, 00751, SBAS},
    //{155, 1021, 01774, SBAS},
    //{156,  463, 00107, SBAS},
    //{157,  568, 01153, SBAS},


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
    // last checked: 5-Nov-2025
    // sys.qzss.go.jp/dod/en/constellation.html [PNT L1 C/A entries]
    // PRN, G2 delay, G2 init

//  {193, 339, 01050, QZSS},   // SVN1, QZS-1, terminated 15-Sep-2023
    {194, 208, 01607, QZSS},   // SVN2, QZS-2
    {195, 711, 01747, QZSS},   // SVN4, QZS-4
    {196, 189, 01305, QZSS},   // SVN5, QZS-1R
//  {197, 263, 00540, QZSS},   //
//  {198, 537, 01363, QZSS},   //
    {199, 663, 00727, QZSS},   // SVN3, QZS-3
    {200, 942, 00147, QZSS},   // SVN7, QZS-6
//  {201, 173, 01206},
//  {202, 900, 01045},


    // Galileo E1B
    // last checked: 08-March-2025
    // www.gsc-europa.eu/system-service-status/constellation-information
    // en.wikipedia.org/wiki/List_of_Galileo_satellites
    // PRN, (E1B codes derived separately)

//  { 1, 0, 0, E1B},    // gsat0210 removed from active service
    { 2, 0, 0, E1B},    // gsat0211
    { 3, 0, 0, E1B},    // gsat0212
    { 4, 0, 0, E1B},    // gsat0213
    { 5, 0, 0, E1B},    // gsat0214
    { 6, 0, 0, E1B},    // gsat0227 Active - NAGU 2024034 - www.gsc-europa.eu/notice-advisory-to-galileo-users-nagu-2024034
    { 7, 0, 0, E1B},    // gsat0207
    { 8, 0, 0, E1B},    // gsat0208
    { 9, 0, 0, E1B},    // gsat0209
    {10, 0, 0, E1B},    // gsat0224
    {11, 0, 0, E1B},    // gsat0101
    {12, 0, 0, E1B},    // gsat0102
    {13, 0, 0, E1B},    // gsat0220
//  {14, 0, 0, E1B},    // gsat0202 unavailable
    {15, 0, 0, E1B},    // gsat0221
    {16, 0, 0, E1B},    // gsat0232 - Launched 2024-09-17 - Active NAGU 2025004 www.gsc-europa.eu/notice-advisory-to-galileo-users-nagu-2025004
//  {17, 0, 0, E1B},    // 
//  {18, 0, 0, E1B},    // gsat0201 unavailable
    {19, 0, 0, E1B},    // gsat0103
//  {20, 0, 0, E1B},    // gsat0104 decommissioned satellite - NAGU 2024015 - www.gsc-europa.eu/notice-advisory-to-galileo-users-nagu-2024015
    {21, 0, 0, E1B},    // gsat0215
//  {22, 0, 0, E1B},    // gsat0204 removed from active service
    {23, 0, 0, E1B},    // gsat0226 - Launched 2024-09-17 - Active NAGU 2025003 - www.gsc-europa.eu/notice-advisory-to-galileo-users-nagu-2025003
    {24, 0, 0, E1B},    // gsat0205
    {25, 0, 0, E1B},    // gsat0216
    {26, 0, 0, E1B},    // gsat0203
    {27, 0, 0, E1B},    // gsat0217
//  {28, 0, 0, E1B},    // 
    {29, 0, 0, E1B},    // gsat0225 Active - NAGU 2024033 - www.gsc-europa.eu/notice-advisory-to-galileo-users-nagu-2024033
    {30, 0, 0, E1B},    // gsat0206
    {31, 0, 0, E1B},    // gsat0218
//  {32, 0, 0, E1B},    // 
    {33, 0, 0, E1B},    // gsat0222
    {34, 0, 0, E1B},    // gsat0223
//  {35, 0, 0, E1B},    // 
    {36, 0, 0, E1B},    // gsat0219
    
    {-1}
};
