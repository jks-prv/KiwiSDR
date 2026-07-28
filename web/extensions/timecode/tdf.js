// Copyright (c) 2017-2026 John Seamons, ZL4VO/KF6VO

var tdf = {
   arm: 0,
   mod_threshold: 0.3,
   no_modulation: 0,
   slice_period: 150,
   slice_threshold: 5,
   modct: 0,
   sec: 0,
   msec: 0,
   line: 0,
   dcnt: 0,
   prev: [],
   
   end: null
};

// see: en.wikipedia.org/wiki/TDF_time_signal
function tdf_legend()
{
   tc_legend(tdf, '0 LS hhhh 000000 HH 0 ASN 01 mmmmmmm p hhhhhh p dddddd www mmmmm yyyyyyyy p 0');
}

function tdf_decode(bits)
{
   dcf77_decode(bits);     // yes they really are the same format!
}

function tdf_clr()
{
   var t = tdf;
   
   t.arm = t.no_modulation = t.dcnt = t.modct = t.line = t.sec = t.msec = 0;
}

// called at 100 Hz (10 msec)
function tdf_phase(ampl)
{
   var t = tdf;
   var abs_ampl = Math.abs(ampl);
   
   if (abs_ampl < t.mod_threshold) {
      // no modulation

      if (tc.no_modulation == 100) {      // 59th second -- period of no phase transitions
         t.arm = 1;
         t.modct = 0;
      }
      tc.no_modulation++;
   } else {
      // modulation
      
      // sync when armed and the first modulation occurs 
      if (t.arm && abs_ampl >= t.mod_threshold) {
         if (t.modct == 0) {
            scope_clr();
            tc.mkr = 1;
            t.sec = 0;
            t.msec = 0;
            if (tc.state == tc.ACQ_SYNC) {
               tc_stat('cyan', 'Found sync');
               tdf_legend();
            } else {
               tdf_decode(tc.raw);   // for the minute just reached
               t.line++;
               tdf_legend();
            }
            tc.raw = [];
            t.dcnt = 0;
            tc.state = tc.ACQ_DATA;
         }
         t.arm = 0;
      }

      tc.no_modulation = 0;
      t.modct++;
   }

   t.msec += 10;

   if (t.msec == 1000) {
      t.sec++;
      t.msec = 0;
      t.modct = 0;
   }

   // after slice_period msec the modct versus slice_threshold determines if one or zero received
   if (tc.state == tc.ACQ_DATA && t.msec == t.slice_period) {
      tc.mkr = 2;
      //tc_dmsg(t.modct +' ');
      var b = (t.modct >= t.slice_threshold)? 1:0;
      tc.trig = b+2;
      tc.raw[t.sec] = b;

      // highlight differences from last period
      var s;
      if (b != t.prev[t.dcnt] && t.line) {
         s = '<span style="color:lime">'+ b +'</span>';
      } else {
         s = b;
      }
      t.prev[t.dcnt] = b;
      tc_dmsg(s);
      if ([0,2,6,12,14,15,18,20,27,28,34,35,41,44,49,57,58].includes(t.dcnt)) tc_dmsg(' ');
      t.dcnt++;
      if (t.dcnt == 60) t.dcnt = 0;
   } else
      tc.trig = 1;
}
