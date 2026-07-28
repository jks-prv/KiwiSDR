// Copyright (c) 2017-2026 John Seamons, ZL4VO/KF6VO

//
// TODO
//
// all
//    No leap second, UTC correction processing etc.
//
// JJY
//    Add hours, minutes field parity checking.
//    Minute 15/45: CW ID, year field replaced with service interruption field.
//
// RBU/RTZ
//    Decode second data bit so parity bits can be used.
//
// WWVB
//    Process six-minute sequences.
//    Do some of the weak signal integration scenarios?
//

var tc = {
   ext_name:   'timecode',    // NB: must match timecode.c:timecode_ext.name
   first_time: true,
   update: false,
   
   dw: 800,
   sh: 200,
   th: 20,
   oh: 150,
   start_point: 0,
   ref: 0,
   col: 0,
   
   state:      0,
   ACQ_PHASE:  0,
   ACQ_SYNC:   1,
   SYNCED:     2,
   MIN_MARK:   3,
   ACQ_DATA:   4,
   ACQ_DATA2:  5,

   dbug:       '',
   test:       0,
   force_rev:  0,

   sync_ph_done: 0,
   sync_ph_ct: 0,
   sync_ph:    0,
   sync_ph_p:  0,
   sync_ph_n:  0,
   
	srate:		0,
	srate_upd:  0,
	pb_cf:      500,
	interval:	0,
	
	_100Hz:     0,
	_10Hz:      0,
	_1Hz:		   0,
	mkr_per:		0,
	mkr:			0,
	
	trig:       0,
	sample_point: -1,
	ampl_last:  0,
	data:       0,
	data_ainv:  0,    // ampl phase invert
	
   df:         0,
   pll_phase:  0,
   pll_bw:     0,

   config:     0,
   // RBU: lsb stronger, RTZ: usb stronger
   sig:        { Beta:0,   JJY40:1, RTZ:2,   WWVBa:3, WWVBp:4, JJY60:5, MSF:6,   RBU:7,   BPCa:8,  BPCss:9, DCF77a:10,  DCF77ss:11, TDF:12,  WWV:13   },
   freq:       [ 25,       40,      50.1,    60,      60,      60,      60,      66.566,  68.5,    68.5,    77.5,       77.5,       162,     10000.1, ],
   pb:         [ 5,        5,       30,      5,       5,       5,       5,       30,      5,       5,       5,          5,          5,       5        ],
   pll_bw_i:   [ 100,      100,     100,     100,     5,       100,     100,     100,     100,     100,     100,        100,        5,       5        ],
   pll_off_i:  [ 500,      500,     500,     500,     0,       500,     500,     500,     500,     500,     500,        500,        0,       100      ],
   sigid_s:    [ 'beta',   'jjy',   'rus',   'wwvb',  'wwvb',  'jjy',   'msf',   'rus',   'bpc',   'bpc',   'dcf77',    'dcf77',    'tdf',   'wwv'    ],
   sync_phase: [ 2,        1,       1,       1,       2,       1,       1,       1,       1,       2,       1,          2,          2,       1        ],
   prev_sig:   -1,
   
   DISABLED: 0,
   ENABLED: 1,
   USE_FSK: 2,

   sig_s: [
      //                   enabled
      [ '25 kHz Beta-25',        0, 'Beta',     'https://en.wikipedia.org/wiki/Beta_(time_signal)' ],
      [ '40 kHz JJY-40',         1, 'JJY',      'https://en.wikipedia.org/wiki/JJY' ],
      [ '50 kHz RTZ',            1, 'RTZ',      'https://en.wikipedia.org/wiki/RTZ_(radio_station)' ],
      [ '60 kHz WWVB-ampl',      1, 'WWVB',     'https://en.wikipedia.org/wiki/WWVB' ],
      [ '60 kHz WWVB-phase',     1, 'WWVB',     'https://en.wikipedia.org/wiki/WWVB' ],
      [ '60 kHz JJY-60',         1, 'JJY',      'https://en.wikipedia.org/wiki/JJY' ],
      [ '60 kHz MSF',            1, 'MSF',      'https://en.wikipedia.org/wiki/Time_from_NPL_(MSF)' ],
      [ '66.666 kHz RBU',        1, 'RBU',      'https://en.wikipedia.org/wiki/RBU_(radio_station)' ],
      [ '68.5 kHz BPC-ampl',     1, 'BPC',      'https://en.wikipedia.org/wiki/BPC_(time_signal)' ],
      [ '68.5 kHz BPC-ss',       0, 'BPC',      'https://en.wikipedia.org/wiki/BPC_(time_signal)' ],
      [ '77.5 kHz DCF77-ampl',   1, 'DCF77',    'https://en.wikipedia.org/wiki/DCF77' ],
      [ '77.5 kHz DCF77-ss',     0, 'DCF77',    'https://en.wikipedia.org/wiki/DCF77' ],
      [ '162 kHz TDF',           1, 'TDF',      'https://en.wikipedia.org/wiki/TDF_time_signal' ],
      [ 'WWV/WWVH',              0, 'WWV/WWVH', 'https://en.wikipedia.org/wiki/WWV_(radio_station)' ],
      [ 'RWM',                   0, 'RWM',      'https://en.wikipedia.org/wiki/RWM' ],
      [ 'BPM',                   0, 'BPM',      'https://en.wikipedia.org/wiki/BPM_(time_service)' ],
      [ '5 MHz BPM-ss',          0, 'BPM',      'https://en.wikipedia.org/wiki/BPM_(time_service)' ],
      [ 'EFR Teleswitch (FSK)',  2, 'EFR',      'https://www.efr.de/en/efr-system/#/Technical-Data-forTransmitter-Stations' ]
   ],
   
   tct: {
	   bs:   [],
	   bp:   [],
	   prev: 0,
	   ct:   0,
   },
   
   //     jan feb mar apr may jun jul aug sep oct nov dec
   dim:  [ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 ],
   mo:   [ 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec', 'mo?' ],
   MpD:  24 * 60,
   MpY:  24 * 60 * 365,
   
   // used by individual decoders
   raw: [],
   no_modulation: 0,

   end: null
};

function timecode_main()
{
	ext_switch_to_client(tc.ext_name, tc.first_time, tc_recv);		// tell server to use us (again)
	if (!tc.first_time)
		tc_controls_setup();
	tc.first_time = false;
}

function tc_dmsg(s)
{
	if (s == undefined) {
		tc.dbug = '';
		tc.col = 0;
	} else {
      s = s.toString();
		tc.dbug += s;
		//console.log(tc.col +' >>>'+ s);
      if (s.includes('<br>')) {
         tc.col = 0;
		   //console.log('tc.col(1) = 0');
      } else {
         if (s[0] != '<')     // don't count HTML
            tc.col += s.length;
         if (tc.col > 100) {
            tc.dbug += '<br>';
            tc.col = 0;
		      //console.log('tc.col(2) = 0');
         }
      }
   }
	
	var el = w3_el('id-tc-scroll');
   var wasScrolledDown = w3_isScrolledDown(el);
	w3_innerHTML('id-tc-dbug', tc.dbug);
	if (wasScrolledDown) w3_scrollDown(el);
}

function tc_legend(which, s)
{
   if ((which.line & 3) == 0) {
      tc_dmsg('<span style="color:yellow">'+ s +'</span><br>');
   }
}

function tc_info(s)
{
	w3_innerHTML('id-tc-info', s);
}

function tc_stat(color, s)
{
   w3_color('id-tc-status', color);
	w3_innerHTML('id-tc-status', s);
}

function tc_stat2(color, s)
{
   w3_color('id-tc-status2', color);
	w3_innerHTML('id-tc-status2', s);
}

function tc_bcd(bits, offset, n_bits, dir)
{
   var val = 0;
   for (var i=0; i < n_bits; i++) val += bits[offset + (dir*i)]? [1,2,4,8,10,20,40,80,100,200][i] : 0;
   return val;
}

function tc_gap_bcd(bits, offset, n_bits, dir)
{
   var val = 0;
   for (var i=0; i < n_bits; i++) val += bits[offset + (dir*i)]? [1,2,4,8,0,10,20,40,80,0,100,200][i] : 0;
   return val;
}

function tc_recv(data)
{
   var i;
	var firstChars = arrayBufferToStringLen(data, 3);
	
	// process data sent from server/C by ext_send_msg_data()
	if (firstChars == "DAT") {
		var ba = new Uint8Array(data, 4);
		var cmd = ba[0];
		var nsamps = ba.length;

		if (cmd == 0) {
         for (i = 1; i < nsamps; i++) {
            var sample = ba[i] - 127;

            var ampl = -sample / 127.0;
            var ampl_abs = Math.abs(ampl);
            var ampl_sgn = (ampl > 0)? 1 : 0;

            if (tc.config == tc.sig.TDF) ampl *= 4;
            var ampl_t = ampl + (tc.ampl_last * 0.99);
            var ampl_offset_removed = ampl_t - tc.ampl_last;
            tc.ampl_last = ampl_t;

            var decision = undefined;
            tc._100Hz += 100 / tc.srate;
            
            // call decoding routines at 100 Hz (10 msec)
            if (tc._100Hz > 1) {
            
               switch (tc.config) {
         
               case tc.sig.WWVBa:
                  decision = wwvb_ampl(ampl_abs);
                  break;
         
               case tc.sig.WWVBp:
                  decision = wwvb_phase(ampl_sgn);
                  break;
         
               case tc.sig.WWV:
                  decision = wwv_ampl(ampl_abs);
                  break;
         
               case tc.sig.DCF77a:
                  decision = dcf77_ampl(ampl_abs);
                  break;
         
               case tc.sig.TDF:
                  decision = tdf_phase(ampl_offset_removed);
                  break;
      
               case tc.sig.MSF:
                  decision = msf_ampl(ampl_abs);
                  break;
         
               case tc.sig.BPCa:
                  decision = bpc_ampl(ampl_abs);
                  break;
         
               case tc.sig.JJY40:
               case tc.sig.JJY60:
                  decision = jjy_ampl(ampl_abs);
                  break;
         
               case tc.sig.RBU:
               case tc.sig.RTZ:
                  decision = rus_ampl(ampl_abs);
                  break;
         
               default:
                  break;
         
               }
               
               if (tc._10Hz++ >= 10) {
               /*
                  tc_info(
                     'mix '+ tc.pll.mix_re.toFixed(2).fieldWidth(10) +' '+ tc.pll.mix_im.toFixed(2).fieldWidth(10) +
                     ' lpf '+ tc.pll.lpf_re.toFixed(2).fieldWidth(10) +' '+ tc.pll.lpf_im.toFixed(2).fieldWidth(10) +
                     ' err '+ tc.pll.phase_err.toFixed(2).fieldWidth(6)
                  );
               */
                  tc._10Hz = 0;
               }
               
               if (tc._1Hz++ >= 100 && tc.sig_s[tc.config][1] != tc.USE_FSK) {
                  timecode_update_srate();
                  var s = 'cf '+ tc.pb_cf +
                     ', pll '+ tc.df.toFixed(2).withSign() +' Hz ' +
                     tc.pll_phase.toFixed(2).withSign() +' &phi; ' +
                     //', sr '+ tc.srate.toFixed(2) + ' ('+ tc.srate_upd.toUnits() +
                     //'), clk '+ (ext_adc_clock_Hz()/1e6).toFixed(6) +' ('+ ext_adc_gps_clock_corr().toUnits() +'), ' +
                     'S'+ tc.state;
                  var ampl_phase_meas = (tc.sync_phase[tc.config] != 2);
                  if (ampl_phase_meas) {
                     tc.sync_ph = tc.sync_ph_p / Math.max(1, (tc.sync_ph_p + tc.sync_ph_n));
                     var pt = tc.sync_ph_done? (tc.data_ainv? 'REV ' : 'IN ') : ((tc.sync_ph_ct / tc.srate).toFixed(0) +'s ');
                     s += ', ph '+ tc.sync_ph.toFixed(2) +' '+ pt + (tc.force_rev? ' R' : '');
                  }
                  tc_stat2('orange', s);
                  tc._1Hz = 0;
               }
            
               tc._100Hz -= 1;
            }
      
            // for now, plot every sample so we can see glitches
            switch (tc.config) {
      
            case tc.sig.WWV:
               scope_draw(
                  /* cyan */ (tc.trig <= tc.sample_point),
                  /* red  */ ampl_abs,
                  /* org  */ undefined,
                  /* blk  */ tc.data? -0.3 : -0.6,
                  /* lime */ 0,
                  /* trig */ 0
               );
               break;
      
            case tc.sig.WWVBa:
               scope_draw(
                  /* cyan */ (tc.trig >= 0 && tc.trig <= tc.sample_point),
                  /* red  */ ampl_abs,
                  /* org  */ undefined,
                  /* blk  */ tc.data? -0.3 : -0.6,
                  /* lime */ 0,
                  /* trig */ 0
               );
               break;
      
            case tc.sig.WWVBp:
               scope_draw(
                  /* cyan */ ampl_sgn,
                  /* red  */ (tc.trig == tc.sample_point)? { num:0.1, color:'blue' } : ampl_abs,
                  /* org  */ ampl >= 0? undefined : ampl,
                  /* blk  */ (tc.trig == tc.sample_point && isDefined(decision))? ((decision > 0)? 0.5:-0.5) : undefined,
                  // /* lime */ tc.start_point? 0 : { num:0, color:'purple' },
                  /* lime */ tc.data? 0 : { num:0, color:'purple' },
                  /* trig */ tc.trig == tc.sample_point
               );
               break;
      
            case tc.sig.DCF77a:
               scope_draw(
                  /* cyan */ (tc.trig <= tc.sample_point),
                  /* red  */ ampl_abs,
                  /* org  */ undefined,
                  /* blk  */ tc.data? -0.3 : -0.6,
                  /* lime */ 0,
                  /* trig */ 0
               );
               break;
      
            case tc.sig.TDF:
               scope_draw(
                  /* cyan */ 0,
                  /* red  */ (Math.abs(ampl_offset_removed) >= 0.3)?
                                 ampl_offset_removed : { num:ampl_offset_removed, color:'orange' },
                  /* org  */ (tc.trig == 1)? { num:1, color:'purple' } : undefined,
                  /* blk  */ (tc.trig > 1)? ((tc.trig == 2)? 0.7 : 0.9) : (tc.no_modulation? -0.9 : -0.7),
                  /* lime */ 0,
                  /* trig */ tc.mkr? ((tc.mkr == 1)? 1 : { num:1, color:'blue' }) : 0
               );
               tc.trig = 0;
               tc.mkr = 0;
               break;
      
            case tc.sig.MSF:
               scope_draw(
                  /* cyan */ (tc.trig <= tc.sample_point),
                  /* red  */ ampl_abs,
                  /* org  */ undefined,
                  /* blk  */ tc.data? -0.3 : -0.6,
                  /* lime */ 0,
                  /* trig */ 0
               );
               break;
      
            case tc.sig.BPCa:
               scope_draw(
                  /* cyan */ (tc.trig >= 0 && tc.trig <= tc.sample_point),
                  /* red  */ ampl_abs,
                  /* org  */ undefined,
                  /* blk  */ tc.data? -0.3 : -0.6,
                  /* lime */ 0,
                  /* trig */ 0
               );
               break;
      
            case tc.sig.JJY40:
            case tc.sig.JJY60:
               scope_draw(
                  /* cyan */ (tc.trig >= 0 && tc.trig <= tc.sample_point),
                  /* red  */ ampl_abs,
                  /* org  */ undefined,
                  /* blk  */ tc.data? -0.3 : -0.6,
                  /* lime */ 0,
                  /* trig */ 0
               );
               break;
      
            case tc.sig.RBU:
            case tc.sig.RTZ:
               scope_draw(
                  /* cyan */ (tc.trig >= 0 && tc.trig <= tc.sample_point),
                  /* red  */ ampl_abs,
                  /* org  */ undefined,
                  /* blk  */ tc.data? -0.3 : -0.6,
                  /* lime */ 0,
                  /* trig */ 0
               );
               break;
      
            default:
               break;
            }
      
            //if (tc.mkr) tc.mkr--;
            
            if (tc.state == tc.ACQ_PHASE) {
               if (tc.data) tc.sync_ph_p++; else tc.sync_ph_n++;
               tc.sync_ph_ct++;
               if (tc.sync_ph_ct >= 10 * tc.srate) {     // measure for 10 secs
                  tc.sync_ph_done = 1;
                  var phase = (tc.sync_ph >= 0.5)? 1:0;
                  if (phase != tc.sync_phase[tc.config]) {
                     tc.data_ainv ^= 1;
                     w3_call(tc.sigid_s[tc.config] +'_clr');
                     //tc_dmsg('[REV PHASE] ');
                  }
                  //tc_dmsg('[FIND SYNC] ');
                  tc.state = tc.ACQ_SYNC;
                  //console.log('FIND SYNC');
                  if (tc.test) {
                     //console.log('restart test file playback');
                     ext_send('SET test_reset');
                  }
                  tc_stat('yellow',
                     w3_inline('',
                        w3_icon('w3-text-aqua', 'fa-cog fa-spin', 20),
                        w3_div('w3-margin-L-8 w3-text-css-yellow', 'Looking for sync')
                     )
                  );
               }
            }
         }
		} else {
			console.log('tc_recv: DATA UNKNOWN cmd='+ cmd +' len='+ len);
		}
		
		return;
	}

	// process command sent from server/C by ext_send_msg() or ext_send_msg_encoded()
	var stringData = arrayBufferToString(data);
	var params = stringData.substring(4).split(" ");

	for (i = 0; i < params.length; i++) {
		var param = params[i].split("=");

		if (0 && param[0] != "keepalive") {
			if (isDefined(param[1]))
				console.log('tc_recv: '+ param[0] +'='+ param[1]);
			else
				console.log('tc_recv: '+ param[0]);
		}

		switch (param[0]) {

			case "ready":
            kiwi_load_js_dir('extensions/timecode/',
               ['wwvb.js', 'jjy.js', 'msf.js', 'dcf77.js', 'tdf.js', 'bpc.js', 'rus.js', '../../pkgs/js/scope.js'],
               'tc_controls_setup');
				break;

			case "df":
				tc.df = parseFloat(param[1]);
				break;

			case "phase":
				tc.pll_phase = parseFloat(param[1]);
				break;

			default:
				console.log('tc_recv: UNKNOWN CMD '+ param[0]);
				break;
		}
	}
}

function timecode_process_params(p) {
   //console.log('timecode_process_params p='+ p);
	if (p) {
	   var i, j;
	   p = p.split(',');
      for (i = 0, len = p.length; i < len; i++) {
         var a = p[i];
         if (i == 0) {
            var ps = a.toLowerCase();
            for (j = 0; j < tc.sig_s.length; j++) {
               if (/* enable */ tc.sig_s[j][1] && tc.sig_s[j][0].toLowerCase().includes(ps))
                  break;
            }
            if (j < tc.sig_s.length) {
               tc.config = j;
               w3_select_value('tc.config', j);
            }
         } else
         if (dbgUs && w3_ext_param('test', a).match) {
            setTimeout(function() { timecode_test_cb(); }, 2000);
         } else
         if (w3_ext_param('rev', a).match) {
            tc.force_rev = 1;
         }
      }
   }

	ext_send('SET draw=0');
	ext_send('SET display_mode=0');     // IQ
	ext_send('SET gain=0');
   tc_signal_menu_cb('tc.config', tc.config, false);
	ext_send('SET run=1');
}

function tc_controls_setup()
{
   tc.dh = tc.sh + tc.th + tc.oh;
   var data_html =
      time_display_html('tc') +

		w3_div(sprintf('id-tc-data|width:%dpx; height:%dpx; background-color:black; position:relative;', tc.dw, tc.dh),
			sprintf('<canvas id="id-tc-scope" width="%d" height="%d" style="position:absolute"></canvas>', tc.dw, tc.sh),
			w3_inline(sprintf('w3-absolute w3-width-full w3-font-13px w3-black|height:20.45px; top:%dpx/', tc.sh),
				w3_div('id-tc-status w3-margin-L-8'),
				w3_div('id-tc-status2 w3-margin-L-16')
			),
         w3_div(sprintf('id-tc-scroll w3-scroll w3-relative w3-font-11px w3-padding-LR-8 w3-575757-white|height:%dpx; top:%dpx', tc.oh, tc.sh + tc.th),
            '<pre id="id-tc-dbug"></pre>'
         )
		);
	
	tc.saved_setup = ext_save_setup();
   tc.save_agc_delay = ext_agc_delay(5000);
	tc.config = tc.sig.WWVBp;
   tc.pll_bw = tc.pll_bw_i[tc.config];

	var controls_html =
		w3_div('id-tc-controls w3-text-white|height:100%',
			w3_col_percent('',
				w3_div('w3-medium w3-text-aqua', '<b>Time station decoder</b>'), 85
			),
			w3_div('w3-margin-T-4', '<pre id="id-tc-info" style="margin:0"></pre>'),
			w3_inline('w3-margin-T-4/w3-margin-right',
			   w3_select_conditional('w3-text-red w3-width-auto', '', '', 'tc.config', tc.config, tc.sig_s, 'tc_signal_menu_cb'),
            w3_button('w3-padding-small w3-css-yellow', 'Re-sync', 'timecode_resync_cb')
			),
			w3_inline('w3-margin-T-8/w3-margin-right',
            w3_button('w3-padding-small w3-aqua', 'Reset PLL', 'timecode_reset_pll_cb'),
            //w3_checkbox('w3-label-inline w3-label-not-bold/', 'update Kiwi<br>date &amp; time', 'tc.update', tc.update, 'w3_bool_cb'),
			   w3_input('w3-padding-tiny w3-margin-L-8 w3-label-inline w3-label-not-bold|width:auto|size=3', 'pll bw:', 'tc.pll_bw', tc.pll_bw, 'timecode_pll_bw_cb'),
            dbgUs? w3_button('id-tc-test w3-padding-small w3-aqua', 'Test', 'timecode_test_cb') : ''
			),
         w3_div('id-tc-no-audio w3-margin-T-4 w3-text-css-lime w3-hide',
            'No audio will be heard (zero-IF mode).'
         )
		);
	
	ext_panel_show(controls_html, data_html, null);
	time_display_setup('tc');
	tc.sigid_s.forEach(function(sig) { w3_call(sig +'_init'); });
	timecode_environment_changed( {resize:1} );
	
	// our sample file is 12k only
	if (ext_nom_sample_rate() != 12000)
	   w3_disable('id-tc-test');
	
	ext_set_controls_width_height(300, 145);
	ext_set_data_height(tc.dh);
	timecode_process_params(ext_param());
}

function timecode_environment_changed(changed)
{
   //w3_console.log(changed, 'timecode_environment_changed');

   // don't do anything for changes.freq or changed.mode
   if (changed.ext_open) {
      timecode_process_params(extint.param);
   }
   
   if (changed.resize) {
      var el = w3_el('id-tc-data');
      // NB: For large displays this causes the desired effect of data panel centering.
      // The time display remains on the right side because left is applied to id-tc-data only.
      var width = tc.dw + kiwi.time_display_width;
      ext_set_data_width(width);
      var left = Math.max(0, (window.innerWidth - width) / 2);
      console.log('timecode resize left='+ left);

      el.style.left = px(left);
   }
}

function tc_signal_menu_cb(path, val, first)
{
   //console.log('tc_signal_menu_cb val='+ val +' first='+ first);
   if (first) return;
	val = +val;
	tc.config = val;
	w3_select_value(path, val);
	tc_info(w3_link('', tc.sig_s[tc.config][3], 'Time station info: '+ tc.sig_s[tc.config][2]));
   w3_call(tc.sigid_s[tc.prev_sig] +'_blur');
   
   if (tc.sig_s[tc.config][1] == tc.USE_FSK) {
	   scope_clr();
	   tc_stat('', '');
	   tc_stat2('', '');
      w3_hide('id-tc-no-audio');
      tc_dmsg();
      tc_dmsg('This station can be decoded using the FSK extension (see "Ham/Utility" menu).');
      return;
   }

   tc.prev_sig = val;
   timecode_update_srate();
	var phase_mode = (tc.pll_off_i[tc.config] == 0)? 1:0;
   w3_show_hide('id-tc-no-audio', phase_mode);
	
	// defaults
   ext_tune(tc.freq[val], 'cwn', ext_zoom.ABS, 12);
   var cwo = tc.pll_off_i[tc.config];
   ext_set_passband(cwo-tc.pb[val]/2, cwo+tc.pb[val]/2);
   ext_tune(tc.freq[val], 'cwn', ext_zoom.ABS, 12);      // in cw mode have to set freq again after pb change

   tc.pll_bw = tc.pll_bw_i[tc.config];
   timecode_pll_bw_cb('tc.pll_bw', tc.pll_bw, true, false);
	ext_send('SET pll_offset='+ cwo);
	ext_send('SET pll_mode=1 arg='+ (phase_mode? 2:1));   // PLL on, mode: carrier=1, BPSK=2
	var el = w3_el('id-tc-scope');
	opt = {
	      width: tc.dw,
         sec_per_sweep: 10,
         srate: tc.srate,
         single_shot: 0,
         background_color: '#f5f5f5'
   };

   w3_call(tc.sigid_s[tc.prev_sig] +'_focus');

	switch (val) {

	case tc.sig.WWVBa:
	case tc.sig.WWVBp:
		break;

	case tc.sig.WWV:
		break;

	case tc.sig.DCF77a:
		break;

	case tc.sig.MSF:
		break;

	case tc.sig.BPCa:
		opt.sec_per_sweep = 20;
		break;

	case tc.sig.JJY40:
	case tc.sig.JJY60:
		break;

	case tc.sig.RBU:
	case tc.sig.RTZ:
		break;

	case tc.sig.TDF:
		opt.sec_per_sweep = 3;
		break;
	
	default:
		opt.sec_per_sweep = 1;
		break;

	}
	
   scope_init(el, opt);
	timecode_resync_cb();
   var pb = ext_get_passband();
   tc.pb_cf = pb.low + (pb.high - pb.low)/2;
}

function timecode_reset(state)
{
   //console.log('timecode_reset state='+ state);
   tc_dmsg();
	//tc_dmsg('[MEAS PHASE] ');
	//tc_dmsg('test='+ tc.test +' ');

	if (state == false) {
	   if (tc.state != tc.ACQ_PHASE) tc.state = tc.ACQ_SYNC;
	} else
	if (isNumber(state)) {
	} else {
	   tc.state = tc.ACQ_SYNC;
	}
	
	if (tc.state == tc.ACQ_PHASE) {
	   tc.sync_ph_done = tc.sync_ph_ct = tc.sync_ph_p = tc.sync_ph_n = 0;
	   tc.data_ainv = 0;
	}

	tc.raw = [];
	tc.sample_point = -1;
	tc.data = 0;
	tc.ref = 0;
   w3_call(tc.sigid_s[tc.config] +'_clr');
	scope_clr();
	tc_stat('yellow',
	   w3_inline('',
	      w3_icon('w3-text-aqua', 'fa-cog fa-spin', 20),
	      w3_div('w3-margin-L-8 w3-text-css-yellow', (tc.state == tc.ACQ_PHASE)? 'Measuring phase' : 'Looking for sync')
	   )
	);
}

function timecode_resync_cb(path, val)
{
   var ampl_phase_meas = (tc.sync_phase[tc.config] != 2);
	tc.state = ampl_phase_meas? tc.ACQ_PHASE : tc.ACQ_SYNC;
	timecode_reset(tc.state);
	if (tc.test && ext_nom_sample_rate() == 12000)
	   ext_send('SET test='+ tc.sig_s[tc.config][2]);
}

function timecode_reset_pll_cb(path, val)
{
	ext_send('SET clear');
}

function timecode_pll_bw_cb(path, val, complete, first)
{
   console.log('timecode_pll_bw_cb path='+ path +' val='+ val +' first='+ first);
   if (first) return;
   val = +val;
   tc.pll_bw = val;
	w3_set_value(path, val);
	ext_send('SET pll_bandwidth='+ val);
}

function timecode_test_cb(path, val, first)
{
   if (ext_nom_sample_rate() != 12000) return;
   console.log('timecode_test_cb '+ tc.sig_s[tc.config][2]);
   timecode_resync_cb();
   tc.test = 1;
	ext_send('SET test='+ tc.sig_s[tc.config][2]);
}

function timecode_update_srate()
{
   var srate = ext_sample_rate();
   if (tc.srate != srate) {
      //console.log('timecode_update_srate');
      tc.srate = srate;
      tc.srate_upd++;
   }
}

function timecode_blur()
{
	ext_send('SET run=0');
	kiwi_clearInterval(tc.interval);
	ext_restore_setup(tc.saved_setup);
   ext_agc_delay(tc.save_agc_delay);
}

function timecode_help(show)
{
   if (show) {
      var s = 
         w3_text('w3-medium w3-bold w3-text-aqua', 'Timecode decoder help') +
         w3_div('w3-margin-T-8 w3-scroll-y|height:90%',
            w3_div('w3-margin-R-8',
               'To be supplied...'
            )
         );

      confirmation_show_content(s, 610, 300);
      w3_el('id-confirmation-container').style.height = '100%';   // to get the w3-scroll-y above to work
   }
   return true;
}

// called to display HTML for configuration parameters in admin interface
function timecode_config_html()
{
   ext_config_html(tc, 'timecode', 'Timecode', 'Timecode decoder configuration');
}
