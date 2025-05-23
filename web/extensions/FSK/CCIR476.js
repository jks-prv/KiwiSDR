/***************************************************************************
 *   Copyright (C) 2011 by Paul Lutus                                      *
 *   lutusp@arachnoid.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/**
 *
 * @author lutusp
 */

function CCIR476() {
   var t = this;
   t.start_bit = 1;

   console.log('FSK encoder: CCIR476');
   t.code_character32 = 0x6a;
   t.LETTERS = 0x5a;
   t.FIGURES = 0x36;
   t.code_alpha = 0x0f;
   t.code_beta = 0x33;
   t.code_char32 = 0x6a;
   t.code_rep = 0x66;
   t.char_bell = 0x07;

   t.valid_codes = [];
   t.code_ltrs = []; t.ltrs_code = [];
   t.code_figs = []; t.figs_code = [];
   t.shift = false;
   t.alpha_phase = false;
   t.strict_mode = false;

   var BEL = '\07';
   var ALF = BET = FGS = LTR = REP = C32 = '_';   // documentation only

   // codes assigned so always 4 mark bits (=1) and 3 space bits (=0) for error detection
   // hence framing name of "4/7"
   t.ltrs = [
    // x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xa   xb   xc   xd   xe   xf
      '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', ALF,    // 0x
      '_', '_', '_', '_', '_', '_', '_', 'J', '_', '_', '_', 'F', '_', 'C', 'K', '_',    // 1x
      '_', '_', '_', '_', '_', '_', '_', 'W', '_', '_', '_', 'Y', '_', 'P', 'Q', '_',    // 2x
      '_', '_', '_', BET, '_', 'G', FGS, '_', '_', 'M', 'X', '_', 'V', '_', '_', '_',    // 3x
      '_', '_', '_', '_', '_', '_', '_', 'A', '_', '_', '_', 'S', '_', 'I', 'U', '_',    // 4x
      '_', '_', '_', 'D', '_', 'R', 'E', '_', '_', 'N', LTR, '_', ' ', '_', '_', '_',    // 5x
      '_', '_', '_', 'Z', '_', 'L', REP, '_', '_', 'H', C32, '_', '\n', '_', '_', '_',   // 6x
      '_', 'O', 'B', '_', 'T', '_', '_', '_', '\r', '_', '_', '_', '_', '_', '_', '_'    // 7x
   ];

   t.figs = [
    // x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xa   xb   xc   xd   xe   xf
      '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', ALF,    // 0x
      '_', '_', '_', '_', '_', '_', '_', '\'', '_', '_', '_', '!', '_', ':', '(', '_',   // 1x
      '_', '_', '_', '_', '_', '_', '_', '2', '_', '_', '_', '6', '_', '0', '1', '_',    // 2x
      '_', '_', '_', BET, '_', '&', FGS, '_', '_', '.', '/', '_', ';', '_', '_', '_',    // 3x
      '_', '_', '_', '_', '_', '_', '_', '-', '_', '_', '_', BEL, '_', '8', '7', '_',    // 4x
      '_', '_', '_', '$', '_', '4', '3', '_', '_', ',', LTR, '_', ' ', '_', '_', '_',    // 5x
      '_', '_', '_', '"', '_', ')', REP, '_', '_', '#', C32, '_', '\n', '_', '_', '_',   // 6x
      '_', '9', '?', '_', '5', '_', '_', '_', '\r', '_', '_', '_', '_', '_', '_', '_'    // 7x
   ];

   // tables are small enough we don't bother with a hash map/table
   for (var code = 0; code < 128; code++) {
      if (t.check_bits(code)) {
         t.valid_codes[code] = true;
         var ltrv = t.ltrs[code];
         if (ltrv != '_') {
            t.code_ltrs[code] = ltrv;
            t.ltrs_code[ltrv] = code;
         }
         var figv = t.figs[code];
         if (figv != '_') {
            t.code_figs[code] = figv;
            t.figs_code[figv] = code;
         }
      }
   }  
}

CCIR476.prototype.reset = function() {
   this.shift = false;
}

CCIR476.prototype.get_nbits = function() {
   return 7;
}

CCIR476.prototype.get_msb = function() {
   return 0x40;
}

/*
   CCIR476.prototype.char_to_code = function(ch, ex_shift) {
      ch = String.toUpperCase(ch);
      // default: return -ch
      int code = -ch;
      // avoid unnececessary shifts
      if (ex_shift && figs_code.containsKey(ch)) {
         code = figs_code.get(ch);
      } else if (!ex_shift && ltrs_code.containsKey(ch)) {
         code = ltrs_code.get(ch);
      } else {
         if (figs_code.containsKey(ch)) {
            shift = true;
            code = figs_code.get(ch);
         } else if (ltrs_code.containsKey(ch)) {
            shift = false;
            code = ltrs_code.get(ch);
         }
      }
      return code;
   }
*/

CCIR476.prototype.code_to_char = function(code, shift) {
   var t = this;
   var ch = shift? t.code_figs[code] : t.code_ltrs[code];
   if (ch == undefined)
      ch = -code;    // default: return negated code
   //console.log('code=0x'+ code.toString(16) +' sft='+ TF(shift) +' char='+ ch +'(0x'+ ch.toString(16) +')');
   return ch;
}

CCIR476.prototype.check_bits = function(v) {
   var _v = v;
   var bc = 0;
   while (v != 0) {
      bc++;
      v &= v - 1;
   }
   //console.log('check_bits v=0x'+ _v.toString(16) +' bc='+ bc +' '+ (bc == 4));
   return bc == 4;
}

// two phases: alpha and rep
// marked during sync by code_alpha and code_rep
// then for data: rep phase character is sent first,
// then, three chars later, same char is sent in alpha phase
CCIR476.prototype.process_char = function(code, fixed_start, cb) {
   var t = this;
   var success = t.check_bits(code);
   var tally = 0;
   var chr = -1;

   // force phasing with the two phasing characters
   if (code == t.code_rep) {
      t.alpha_phase = false;
   } else if (code == t.code_alpha) {
      t.alpha_phase = true;
   }

   if (!t.alpha_phase) {
      t.c1 = t.c2;
      t.c2 = t.c3;
      t.c3 = code;
   } else { // alpha channel
      if (t.strict_mode) {
         if (success && t.c1 == code) {
            chr = code;
         }
      } else {
         if (success) {
            chr = code;
         } else if (t.check_bits(t.c1)) {
            chr = t.c1;
            //console.log('FEC replacement: 0x'+ code.toString(16) +' -> 0x'+ t.c1.toString(16));
         }
      }
      if (chr == -1) {
         tally = -1;
         //console.log('fail all options: 0x'+ code.toString(16) +' -> 0x'+ t.c1.toString(16));
      } else {
         tally = 1;

         switch (chr) {
            case t.code_rep:
               break;
            case t.code_alpha:
               break;
            case t.code_beta:
               break;
            case t.code_char32:
               break;
            case t.LETTERS:
               t.shift = false;
               break;
            case t.FIGURES:
               t.shift = true;
               break;
            default:
               chr = t.code_to_char(chr, t.shift);
               if (chr < 0) {
                  console.log('missed this code: 0x'+ Math.abs(chr).toString(16));
               } else {
                  cb(chr.toString());
               }
               break;
         } // switch

      }
      //parent.debug_p(String.format("compare: %x = %x, %s", code, t.c1, (code == t.c1) ? "YES" : "NO"));
   } // alpha channel

   // alpha/rep phasing
   t.alpha_phase = !t.alpha_phase;
   return { success:success, tally:tally };
}
