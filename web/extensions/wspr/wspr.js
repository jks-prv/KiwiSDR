// KiwiSDR data demodulator UI
//
// Copyright (c) 2024 John Seamons, ZL4VO/KF6VO

// TODO
// sanity check upload data
// final wsprstat upload interval

// prefix all global vars with 'wspr_'
// ntype non-std 29 seen
// decode task shows > 100% in "-stats" task display
// update to WSJT-X merged version
// see screenshot: case where double peaks of same station, same freq didn't get filtered out?
// re-enable spot uploads

var wspr = {
   ext_name: 'wspr',    // NB: must match wspr.c:wspr_ext.name
   first_time: true,
   focus_interval: null,
   server_time_ms: 0,
   local_time_epoch_ms: 0,
   pie_size: 25,
   stack_decoder: 0,
   debug: 0,
   upload: true,
   upload_lockout: false,
   pie_interval: null,
   testing: false,
   
   hop_period: 20,
   //hop_period: 6,
   SYNC: true,
   NO_SYNC: false,
   
   // Order matches menu instantiation order and autorun wspr_main.cpp:wspr_cfs[]
   // See: wsprnet.org/drupal/node/7352
   // These are all passband center freqs with an offset of 1.5 kHz
   // Later, we compute our actual dial freq using the current BFO value (default 750 Hz).
   // dial freq = cf - bfo, cf aka "tx freq"
   // New entries can only be added at end due to limitations with autorun's wspr.autorun_u stored config value
   // 9999 entry for IWBP makes url param &ext=wspr,iwbp work due to freq range check.
   center_freqs: [
      137.5, 475.7, 1838.1, 3570.1, 3594.1, 5288.7, 5366.2, 7040.1, 10140.2, 14097.1, 18106.1, 21096.1, 24926.1, 28126.1, 6781.5, 13555.4, 9999
   ],
   
   freqs_s: {
      'lf':0, 'mf':1, '160m':2, '80m_ja':3, '80m':4, '60m':5, '60m_eu':6, '40m':7, '30m':8, '20m':9, '17m':10, '15m':11, '12m':12, '10m':13, 'ism_6':14, 'ism_13':15, 'iwbp':16
   },
   
   freqs_m: [
      'LF', 'MF', '160m', '80m_JA', '80m', '60m', '60m_EU', '40m', '30m', '20m', '17m', '15m', '12m', '10m', 'ISM_6', 'ISM_13', 'IWBP'
   ],
   
   // freqs on github.com/HB9VQQ/WSPRBeacon are cf - 1.5 kHz BFO (dial frequencies)
   // so we add 1.5 to those to get our cf values (same as regular WSPR wspr.center_freqs above)
   cf_IWBP: [ 1838.1, 3570.1, 5288.7, 7040.1, 10140.2, 14097.1, 18106.1, 21096.1, 24926.1, 28126.1 ],
   
   xvtr_center_freqs: [ 40680, 50294.5, 70092.5, 144490.5, 432301.5, 1296501.5, 10489569.5 ],
   xvtr_freqs_m: [ '8m', '6m', '4m', '2m', '440', '1296', 'QO-100' ],

   autorun_u: [
      'regular use', 'LF', 'MF', '160m', '80m_JA', '80m', '60m', '60m_EU',
      '40m', '30m', '20m', '17m', '15m', '12m', '10m',
      '8m', '6m', '4m', '2m', '440', '1296', 'QO-100', 'ISM_6', 'ISM_13', 'IWBP'
   ],
   
   // translates menu order to cfg value which then has to match order of wspr_main.cpp:wspr_cfs[]
   // assign new cfg values so as not to disturb existing values stored in cfg
   menu_i_to_cfg_i: [
      0,    //  0 regular use
      
   // +-- cfg value (no renumbering or reuse!)
   // |         +-- menu seq value  (use this in wspr_main.cpp:wspr_cfs)
   // |         |
      1,    //  1 LF
      2,    //  2 MF
      3,    //  3 160m
      4,    //  4 80m_JA
      5,    //  5 80m
      6,    //  6 60m
      7,    //  7 60m_EU
      8,    //  8 40m
      9,    //  9 30m
      10,   // 10 20m
      11,   // 11 17m
      12,   // 12 15m
      13,   // 12 12m
      14,   // 14 10m
      
      23,   // 15 8m
      15,   // 16 6m
      16,   // 17 4m
      17,   // 18 2m
      18,   // 19 440  70cm
      19,   // 20 1296 23cm
      24,   // 21 10G   3cm QO-100
      
      20,   // 22 ISM_6
      21,   // 23 ISM_13
      22,   // 24 IWBP
   ],

   PREEMPT_NO: 0,
   PREEMPT_YES: 1,
   preempt_u: ['no', 'yes'],
   
   sched_u: [ 'continuous',
      '00:00', '01:00', '02:00', '03:00', '04:00', '05:00', '06:00', '07:00', '08:00', '09:00', '10:00', '11:00',
      '12:00', '13:00', '14:00', '15:00', '16:00', '17:00', '18:00', '19:00', '20:00', '21:00', '22:00', '23:00',
   ],

   restart_highlight: '3px solid red',

   last_last: 0
};

var wspr_canvas_width = 1024;
//var wspr_canvas_height = 150;		// not currently used

function wspr_main()
{
	ext_switch_to_client(wspr.ext_name, wspr.first_time, wspr_recv);		// tell server to use us (again)
	if (!wspr.first_time)
		wspr_controls_setup();
	wspr.first_time = false;
}

var wspr_cmd_e = { WSPR_DATA:0 };
var wspr_cmd_d = { 0:"WSPR_DATA" };

var wspr_bins, wspr_startx;
var aw_ypos = 0, y_tstamp = -1, span = 0, over = 16;

function wspr_scroll()
{
	var aw_h = wccva.height;
	wccva.style.top=(aw_h-aw_ypos)+"px";
	wccvao.style.top=(-aw_ypos)+"px";
	aw_ypos++; if (aw_ypos>=aw_h) {
		aw_ypos=0;
		var tmp=wccvao; wccvao=wccva; wccva=tmp;
	}
}

var WSPR_F_BIN =			0x0fff;
var WSPR_F_DECODING =	0x1000;
var WSPR_F_DELETE =		0x2000;
var WSPR_F_DECODED =		0x4000;
var WSPR_F_IMAGE =		0x8000;

function wspr_recv(data)
{
   var s, i, el;
	var firstChars = arrayBufferToStringLen(data, 3);
	
	if (firstChars == "DAT") {
		var ba = new Uint8Array(data, 4);
		var cmd = ba[0];

		if (cmd == wspr_cmd_e.WSPR_DATA) {
			var o = 2;
			var blen = ba.length-o;
			var aw_h = wccva.height;
         
         if (ba[1]) {		// 2 min interval line
         	y_tstamp = aw_ypos+12;
            wccva.ct.fillStyle="white";
            wccva.ct.fillRect(wspr_startx-over, aw_ypos, blen*2+over*2, 1);
            wspr_scroll();
         }
         
         // FFT data
			var im = wccva.im;
			for (i=0; i < blen; i++) {
				var z = ba[o+i];
				if (z>255) z=255; if (z<0) z=0;
				for (j=0; j<2; j++) {
					var k = 4*(i*2+j+wspr_startx);
					im.data[k]=color_map_r[z];
					im.data[k+1]=color_map_g[z];
					im.data[k+2]=color_map_b[z];
					im.data[k+3]=255;
				}
			}
			wccva.ct.putImageData(im,0,aw_ypos);
			
			// handle context spanning of timestamp
			if ((y_tstamp >= aw_h) && (aw_ypos == (aw_h-1))) {
				span = 1;
			}
			//console.log('aw_ypos='+ aw_ypos +' aw_h='+ aw_h +' span='+ span +' y_tstamp='+ y_tstamp);
			
			if ((y_tstamp == aw_ypos) || span) {
				wccva.ct.fillStyle="white";
				wccva.ct.font="10px Verdana";
				var d = wspr_server_Date();
				wccva.ct.fillText(d.toUTCString().substr(17,5) +' UTC', wspr_startx+blen*2+over, y_tstamp);
				if (span) {
					y_tstamp -= aw_h;
					span = 0;
				} else {
					y_tstamp = -1;
				}
			}
         
         wspr_scroll();
		} else {
			console.log('wspr_recv: DATA UNKNOWN cmd='+ cmd +' len='+ (ba.length-1));
		}
		return;
	}
	
	var stringData = arrayBufferToString(data);
	var params = stringData.substring(4).split(" ");

	for (i=0; i < params.length; i++) {
		var param = params[i].split("=");

		if (0 && param[0] != "keepalive") {
			if (isDefined(param[1]))
				console.log('wspr_recv: '+ param[0] +'='+ param[1]);
			else
				console.log('wspr_recv: '+ param[0]);
		}

		switch (param[0]) {

			case "ready":
				var bfo = parseInt(ext_get_cfg_param('WSPR.BFO', wspr_bfo));
				//console.log('### bfo='+ bfo);
				if (!isNaN(bfo)) {
					//console.log('### set bfo='+ bfo);
					wspr_bfo = bfo;
				}
				ext_send('SET BFO='+ wspr_bfo.toFixed(0));
				wspr_controls_setup();		// needs wspr_startx
				ext_send('SET stack_decoder='+ wspr.stack_decoder);
				ext_send('SET debug='+ wspr.debug);
				break;

			case "WSPR_TIME_MSEC":
				wspr.server_time_ms = param[1] * 1000 + (+param[2]);
				wspr.local_time_epoch_ms = Date.now();
			   //console.log('WSPR_TIME_MSEC server: '+ (new Date(wspr.server_time_ms)).toUTCString() +
			   //   ' local: '+ (new Date(wspr.local_time_epoch_ms)).toUTCString());
				break;

			case "WSPR_SYNC":
				//console.log('WSPR: WSPR_SYNC');
				break;

			case "WSPR_STATUS":
				var status = parseInt(param[1]);
				//console.log('WSPR: WSPR_STATUS='+ status);
				wspr_set_status(status);
				break;

			case "WSPR_DECODED":
				s = decodeURIComponent(param[1]);
				//console.log('WSPR: '+ s);
				el = w3_el('id-wspr-decode');
				var wasScrolledDown = w3_isScrolledDown(el);
				el.innerHTML += s +'<br>';
				
				// only jump to bottom of updated list if it was already sitting at the bottom
				if (wasScrolledDown) w3_scrollDown(el);
				break;
			
			case "WSPR_UPLOAD":
				s = decodeURIComponent(param[1]);
				wspr_upload(wspr_report_e.SPOT, s);
				break;
			
			case "WSPR_PEAKS":
				s = decodeURIComponent(param[1]);
				var p, npk = 0;
				if (s != '') {
					p = s.split(':');
					if (p.length) npk = (p.length-1)/2;
				}
				//console.log('WSPR: '+ npk +' '+ s);
				var xscale = 2;

				for (i=0; i < npk; i++) {
					var bin0 = parseInt(p[i*2]);
					var flags = bin0 & ~WSPR_F_BIN;
					bin0 &= WSPR_F_BIN;
					if (flags & WSPR_F_DELETE) continue;
					var x = wspr_startx + bin0*xscale;
					if (x > wspr_canvas_width) break;
					var nextx;
					if (i < npk-1)
						nextx = wspr_startx + parseInt(p[(i+1)*2])*xscale;
					else
						nextx = wspr_canvas_width;
					if (nextx >= wspr_canvas_width)
						nextx = wspr_canvas_width + 256;
					var snr_call = p[i*2+1];
					var snr = snr_call.filterInt();
					var color;
					
					if (isNaN(snr)) {
						color = (flags & WSPR_F_IMAGE)? 'cl-wspr-image' : 'cl-wspr-call';
					} else {
						snr_call = snr.toFixed(0);
						color = (flags & WSPR_F_DECODING)? 'cl-wspr-decoding':'';
					}
					
					s +=
						w3_div('cl-wspr-mkr1|max-width:'+ (nextx-x) +'px; left:'+ (x-6) +'px; bottom:8px',
							w3_div('cl-wspr-mkr2',
								w3_div('cl-wspr-snr '+ color, snr_call)
							)
						) +
						w3_div('cl-wspr-line '+ color +'|width:1px; height:10px; position:absolute; left:'+ x +'px; bottom:0px;');
				}
				
				w3_el('id-wspr-peaks-labels').innerHTML = s;
				break;

			case "nbins":
				wspr_bins = parseInt(param[1]);
				
				// starting x position given that wspr display is centered in canvas
				// typically (1024 - 411*4)/2 = 101
				// remember that wspr_canvas is scaled to fit screen width
				wspr_startx = Math.round((wspr_canvas_width - wspr_bins*2)/2);
				break;

			case "bar_pct":
			   if (wspr.testing) {
               var pct = w3_clamp(parseInt(param[1]), 0, 100);
               //if (pct > 0 && pct < 3) pct = 3;    // 0% and 1% look weird on bar
               el = w3_el('id-wspr-bar');
               if (el) el.style.width = pct +'%';
               //console.log('bar_pct='+ pct);
            }
			   break;

			case "test_start":
			   wspr_test_cb('', 1);
			   break;

			case "test_done":
            if (wspr.testing) {
               w3_hide('id-wspr-bar-container');
               w3_show_inline('id-wspr-upload-container');
            }
			   break;

			default:
				console.log('wspr_recv: UNKNOWN CMD '+ param[0]);
				break;
		}
	}
}

var wspr_report_e = { STATUS:0, SPOT:1 };

var wspr_spectrum_A, wspr_spectrum_B, wspr_scale_canvas;
var wccva, wccva0;

var wspr_config = { deco:1 };
var wspr_config_okay = true;
var wspr_init_band = -1;

function wspr_controls_setup()
{
   var i;
   
   var data_html =
      time_display_html('wspr') +

      w3_div('id-wspr-peaks|width:1024px; height:30px; background-color:black; position:relative;',
      	w3_div('id-wspr-peaks-labels|width:1024px; height:30px; position:absolute;')
      ) +

   	w3_div('id-wspr-spectrum|width:1024px; height:150px; overflow:hidden; position:relative;',
			// two overlapping canvases to implement scrolling
   		'<canvas id="id-wspr-spectrum-A" width="1024" height="150" style="position:absolute">test</canvas>',
   		'<canvas id="id-wspr-spectrum-B" width="1024" height="150" style="position:absolute">test</canvas>'
   	) +
   	
      w3_div('id-wspr-scale|width:1024px; height:20px; background-color:black; position:relative;',
   		'<canvas id="id-wspr-scale-canvas" width="1024" height="20" style="position:absolute"></canvas>'
      );
   
   var call = ext_get_cfg_param_string('WSPR.callsign', '', EXT_NO_SAVE);
   if (call == undefined || call == null || call == '') {
   	call = '(not set)';
   	wspr_config_okay = false;
   }

   var grid = ext_get_cfg_param_string('WSPR.grid') || '';
   wspr.grid = grid;
   if (grid == '') {
      grid = '(not set)';
   	wspr_config_okay = false;
   }

   // re-define band menu if downconverter in use
   var r = ext_get_freq_range();
   if (r.lo_kHz > kiwi.freq_bb_max_kHz && r.hi_kHz > kiwi.freq_bb_max_kHz) {
      wspr.center_freqs = [];
      wspr.freqs_s = {};
      wspr.freqs_m = [];
      var j = 0;
      for (i = 0; i < wspr.xvtr_center_freqs.length; i++) {
         var f_kHz = wspr.xvtr_center_freqs[i];
         if (f_kHz >= r.lo_kHz && f_kHz <= r.hi_kHz) {
            wspr.center_freqs.push(f_kHz);
            var s = wspr.xvtr_freqs_m[i];
            wspr.freqs_m.push(s);
            wspr.freqs_s[s.toLowerCase()] = j++;
         }
      }
      if (wspr_init_band > 0) wspr_init_band = 0;
   }

	var controls_html =
	w3_div('id-wspr-controls',
	   w3_div('id-wspr-controls-top',
         w3_inline('w3-halign-space-between w3-margin-B-4|width:83%/',
            w3_div('w3-medium w3-text-aqua cl-viewer-label', '<b>WSPR<br>viewer</b>'),
            w3_select('w3-text-red', '', 'band', 'wspr_init_band', wspr_init_band, wspr.freqs_m, 'wspr_band_select_cb'),
            w3_button('w3-ext-btn w3-padding-smaller', 'stop', 'wspr_stop_start_cb'),
            w3_button('cl-w3-ext-btn w3-padding-smaller w3-css-yellow', 'clear', 'wspr_clear_cb'),
            w3_button('id-wspr-test cl-w3-ext-btn w3-padding-smaller w3-aqua||title="test spots NOT uploaded\nto wsprnet.org"',
               'test', 'wspr_test_cb', 1),
            w3_checkbox('id-wspr-upload-container cl-upload-checkbox/w3-label-inline w3-label-not-bold/',
               'upload<br>spots', 'wspr.upload', true, 'wspr_set_upload_cb'),
            w3_div('id-wspr-bar-container w3-progress-container w3-round-large w3-white w3-hide|width:70px; height:16px',
               w3_div('id-wspr-bar w3-progressbar w3-round-large w3-light-green|width:0%', '&nbsp;')
            )
         ),

         w3_inline('w3-halign-space-between/',
            w3_div('cl-wspr-pie|background-color:#575757',
               kiwi_pie('id-wspr-pie', wspr.pie_size, '#eeeeee', 'deepSkyBlue')
            ),
            w3_div('',
               w3_div('id-wspr-time cl-wspr-text'),
               w3_div('id-wspr-status cl-wspr-text')
            ),
            // FIXME: field validation
            w3_div('',
               w3_div('cl-wspr-text', 'BFO '+ wspr_bfo),
               w3_div('id-wspr-cf cl-wspr-text')
            ),
            w3_div('cl-wspr-text', 'reporter call '+ call),
            w3_div('id-wspr-rgrid cl-wspr-text', 'reporter grid '+ grid)
         ),
      
         w3_div('|background-color:lightGray; overflow:auto; width:100%; margin-top:5px; margin-bottom:0px; font-family:monospace; font-size:100%',
            '<pre style="display:inline"> UTC  dB   dT      Freq dF  Call   Grid    km  dBm</pre>'
            //                                                   dd  cccccc GGGG ddddd  nnn (n W)
         )
      ),
      
		w3_div('id-wspr-decode|white-space:pre; background-color:white; overflow:scroll; height:100px; width:100%; margin-top:0px; font-family:monospace; font-size:100%')
	);

	ext_panel_show(controls_html, data_html, null);
   var wh = waterfall_height();
   //var ch = (wh <= 546)? 240 : Math.round(wh * 0.44);    // scale control panel height on larger screens
   //var ch = Math.round(wh * 0.9);   // scale control panel height on larger screens
   var ch = (wh <= 225)? 203 : Math.round(wh * 0.9);     // scale control panel height on larger screens
   ext_set_controls_width_height(null, ch);
   var dh = ch - w3_el('id-wspr-controls-top').clientHeight - /* borders */ 20;
   w3_el('id-wspr-decode').style.height = px(dh);
   //console.log('WSPR wh='+ wh +' ch='+ ch +' dh='+ dh);
	time_display_setup('wspr');
	wspr.saved_mode = ext_get_mode();
	//wspr_resize();
	
	wspr_spectrum_A = w3_el('id-wspr-spectrum-A');
	wspr_spectrum_A.ct = wspr_spectrum_A.getContext("2d");
	wspr_spectrum_A.im = wspr_spectrum_A.ct.createImageData(1024, 1);

	wspr_spectrum_B = w3_el('id-wspr-spectrum-B');
	wspr_spectrum_B.ct = wspr_spectrum_B.getContext("2d");
	wspr_spectrum_B.im = wspr_spectrum_B.ct.createImageData(1024, 1);
	
	wccva = wspr_spectrum_A; wccvao = wspr_spectrum_B;

	wspr_scale_canvas = w3_el('id-wspr-scale-canvas');
	wspr_scale_canvas.ct = wspr_scale_canvas.getContext("2d");

   wspr.pie_interval = setInterval(wspr_draw_pie, 1000);
   wspr_draw_pie();
   wspr_draw_scale(100);
	
	// our sample file is 12k only
	if (ext_nom_sample_rate() != 12000)
	   w3_disable('id-wspr-test');
	
   wspr_reset();
   wspr_upload_timeout = setTimeout(function() {wspr_upload(wspr_report_e.STATUS);}, 1000);

	// set band and start if URL parameter present
	wspr_process_params(ext_param());
}

function wspr_process_params(p)
{
   //console.log('wspr_process_params p='+ p);
	if (p) {
      var r = ext_get_freq_range();
      p = p.toLowerCase().split(',');
      p.forEach(function(a, i) {
         var sel = wspr.freqs_s[a];
         if (i == 0 && isDefined(sel)) {
            console.log('<'+ a +'> '+ i +' sel='+ sel);
            var freq = wspr.center_freqs[sel];
            if (freq >= r.lo_kHz && freq <= r.hi_kHz)
               wspr_band_select_cb('wspr_init_band', sel, false);
            return;
         }
         if (a.startsWith('stack')) wspr.stack_decoder = 1; else
         if (a.startsWith('debug')) wspr.debug = 1; else
         if (a.startsWith('noupload')) {
            wspr.upload_lockout = true;
            wspr_set_upload_cb('', false);
         } else
         if (a.startsWith('help')) {
            ext_help_click();
         }
         //console.log('WSPR unknown URL param <'+ a +'>');
      });
	} else {
		// if reactivating, start up on same band
		if (wspr_init_band != -1)
			wspr_freq(wspr_init_band);
	}
}

function wspr_help(show)
{
   if (show) {
      var s = 
         w3_text('w3-medium w3-bold w3-text-aqua', 'WSPR viewer help') +
         w3_div('w3-margin-T-8 w3-scroll-y|height:90%',
            w3_div('w3-margin-R-8',
               'The WSPR viewer was the first Kiwi extension developed. ' +
               'It\'s more of a demonstration than a serious WSPR decoding utility. ' +
               'An older version of the WSJT-X <i>wsprd</i> decoder is used. Together with the limited ' +
               'processing power of the Beagle this means fewer decodes occur compared to the current WSJT-X. ' +
               'For serious decoding try <a href="http://wsprdaemon.org/index.html" target="_blank">wsprdaemon</a> ' +
               'which runs on a separate computer and makes connections to the Kiwi. ' +
               '<br><br>' +
         
               'The <i>band</i> menu contains the standard <a href="http://www.wsprnet.org" target="_blank">ham band frequencies</a> ' +
               'plus the ISM 6/13 MHz bands. Frequency hopping specified by the ' +
               '<a href="https://github.com/HB9VQQ/WSPRBeacon" target="_blank">International WSPR Beacon Project</a> (IWBP) is also supported. '+
               '<br><br>' +

               'About dial frequencies: By default the WSPR extension uses a BFO value of 750 Hz instead of the more traditional 1500 Hz ' +
               '(this value is set by the Kiwi owner on the admin page). ' +
               'This lower tone gives less listening fatigue. But it means the number shown in the Kiwi frequency entry box (dial frequency) ' +
               'must be reduced by 750 Hz to match the dial frequencies mentioned in the references above. ' +
               'The BFO value and center frequency (CF) of the passband are displayed in the WSPR control panel. ' +
               '<br><br>' +
               
               'Before spots will be uploaded to wsprnet.org the reporter <i>call</i> and <i>grid</i> must be set on the ' +
               'admin page (Extensions > WSPR) and <i>upload spots</i> checked. ' +
               'WSPR decoding still occurs even though spots are not being uploaded. ' +
               '<br><br>' +

               'Test button: First, select a band from the band menu. It doesn\'t matter which one. ' +
               'Then click the <i>test</i> button when the time clock just becomes fully blue (i.e. at the beginning of an even minute). ' +
               'A two minute test recording will be played back containing 8 spots. These spots will <b>not</b> be uploaded to wsprnet.org ' +
               '&nbsp;The recording was made with a BFO of 750 Hz, but will still decode even if the extension is configured for another BFO value. ' +
               '<br><br>' +
               
               'The decoder column values are the same as with other WSPR programs:' +
               '<ul>' +
                  '<li><i>dB</i> is signal-to-noise ratio (SNR), negative values meaning below the noise floor.</li>' +
                  '<li><i>dt</i> and <i>dF</i> indicate how far off the signal is in time and frequency.</li>' +
                  '<li><i>Freq</i> is the signal\'s passband frequency as seen in the spectrum display.</li>' +
                  '<li><i>km</i> is the signal\'s distance from the receiving Kiwi.</li>' +
                  '<li><i>dBm</i> is the transmit power reported by the WSPR beacon.</li>' +
               '</ul>' +

               'URL parameters: <br>' +
               'First parameter can be an entry from the band menu, e.g. <i>ext=wspr,40m</i> <br>' +
               '<br>'
            )
         );

      confirmation_show_content(s, 610, 300);
      w3_el('id-confirmation-container').style.height = '100%';   // to get the w3-scroll-y above to work
   }
   return true;
}

/*
function wspr_resize()
{
	var left = (window.innerWidth - 1024 - time_display_width()) / 2;
	w3_el('id-wspr-peaks').style.left = px(left);
	w3_el('id-wspr-spectrum').style.left = px(left);
	w3_el('id-wspr-scale').style.left = px(left);
}
*/

function wspr_band_select_cb(path, idx, first)
{
	//console.log('wspr_band_select_cb idx='+ idx +' path='+ path);
	idx = +idx;
	if (idx != -1 && !first) {
      w3_set_value(path, idx);
		wspr_init_band = idx;
		wspr_freq(idx);
	}
}

// automatically called on changes in the environment
function wspr_environment_changed(changed)
{
   //w3_console.log(changed, 'wspr_environment_changed');

   // don't do anything for changes.freq or changed.mode
   if (changed.ext_open) {
      wspr_process_params(extint.param);
   }
}

function wspr_focus()
{
   //console.log('### wspr_focus');
}

function wspr_blur()
{
	//console.log('### wspr_blur');
   ext_send('SET capture=0');
   kiwi_clearTimeout(wspr_upload_timeout);
   kiwi_clearInterval(wspr.pie_interval);
	ext_set_mode(wspr.saved_mode);
}


////////////////////////////////
// admin
////////////////////////////////

function wspr_input_grid_cb(path, val, first)
{
   val = val.trim();
   //console.log('wspr_input_grid_cb val='+ val);
	w3_string_set_cfg_cb(path, val);
	
	// need this because wspr_check_GPS_update_grid() runs asynch of server sending updated value via 10s status
	wspr.grid = val;
}

function wspr_config_html()
{
   var s =
      w3_div('w3-show-inline-block w3-width-full',
         w3_col_percent('w3-container/w3-margin-bottom',
            w3_divs('w3-restart',
               w3_input_get('', 'Reporter callsign', 'WSPR.callsign', 'w3_string_set_cfg_cb', '')
            ), 22,
            '', 3,
            w3_div('',
               w3_inline('w3-halign-space-between/',
                  w3_label('w3-bold', 'Reporter grid square '),
                  w3_button('id-wspr-grid-set cl-admin-check w3-blue w3-btn w3-round-large w3-margin-B-2 w3-hide', 'set from GPS')
               ),
               w3_input_get('', '', 'WSPR.grid', 'wspr_input_grid_cb', '', '4 or 6-character grid square location')
            ), 30,
            '', 3,
            w3_div('w3-restart',
               w3_input_get('', 'BFO Hz (multiple of 375 Hz)', 'WSPR.BFO', 'w3_num_set_cfg_cb', '', 'typically 750 Hz')
            ), 22,
            '', 3,
            w3_div('w3-restart',
               w3_input_get('', 'Test filename', 'WSPR.test_file', 'w3_string_set_cfg_cb', 'WSPR.test.au')
            ), 14,
            ''
         ),

         w3_col_percent('w3-container w3-margin-T-8 w3-margin-B-16/',
            w3_divs('w3-center w3-tspace-8',
               w3_switch_label('w3-center', 'Update grid continuously<br>from GPS?', 'Yes', 'No', 'cfg.WSPR.GPS_update_grid', cfg.WSPR.GPS_update_grid, 'wspr_GPS_update_grid_cb'),
               w3_text('w3-text-black w3-center',
                  'Useful for Kiwis in motion <br> (e.g. marine mobile)'
               )
            ), 22,
            '&nbsp;', 3,
            w3_divs('w3-center w3-tspace-8',
               w3_switch_label('w3-center', 'Log decodes to<br>syslog?', 'Yes', 'No', 'cfg.WSPR.syslog', cfg.WSPR.syslog, 'admin_radio_YN_cb'),
               w3_text('w3-text-black w3-center',
                  'Use with care as over time <br> filesystem can fill up.'
               )
            ), 22,
            '&nbsp;', 3,
            w3_divs('w3-center w3-tspace-8',
               w3_switch_label('w3-center', 'Log spot debug<br>info?', 'Yes', 'No', 'cfg.WSPR.spot_log', cfg.WSPR.spot_log, 'admin_radio_YN_cb'),
               w3_text('w3-text-black w3-center',
                  'Logs the actual upload commands<br>used to assist in spot debugging.'
               )
            ), 22
         ),
         
         '<hr>',
         w3_div('w3-container',
            w3_div('', '<b>Autorun</b>'),
            w3_div('w3-container',
               w3_div('w3-text-black', 'On startup automatically begins running the WSPR decoder on the selected band(s).<br>' +
                  'Channels available for regular use are reduced by one for each WSPR autorun enabled.<br>' +
                  'If Kiwi has been configured for a mix of channels with and without waterfalls then channels without waterfalls will be used first.<br><br>' +
                  
                  'Spot decodes are available in the Kiwi log (use "Log" tab above) and are listed on <a href="http://wsprnet.org/drupal/wsprnet/spots" target="_blank">wsprnet.org</a><br>' +
                  'The "Reporter" fields above must be set to valid values for proper spot entry into the <a href="http://wsprnet.org/drupal/wsprnet/spots" target="_blank">wsprnet.org</a> database.'),
               
               w3_div('w3-margin-T-10 w3-valign',
                  '<header class="id-wspr-warn-full w3-container w3-yellow"><h6>' +
                  'If your Kiwi is publicly listed you must <b>not</b> configure all the channels to use WSPR-autorun!<br>' +
                  '(unless at least one is set to preemptable) This defeats the purpose of making your Kiwi <br>' +
                  'public and public registration will be disabled until you make at least one channel available <br>' +
                  'for connection. Check the Admin Public tab.' +
                  '</h6></header>'
               ),
               
               w3_inline('w3-margin-top/',
                  w3_button('w3-blue', 'set all to regular use', 'wspr_autorun_all_regular_cb'),
                  w3_text('w3-margin-L-32 w3-text-teal w3-bold', 'Set all preemptable:'),
                  w3_button('w3-margin-L-8 w3-green', 'yes', 'wspr_autorun_all_preempt_cb', 1),
                  w3_button('w3-margin-L-8 w3-red', 'no', 'wspr_autorun_all_preempt_cb', 0),
                  w3_button('id-wspr-restart w3-margin-L-32 w3-aqua', 'autorun restart', 'wspr_autorun_restart_cb')
               ),
               
               w3_div('id-wspr-admin-autorun w3-margin-T-16')
            )
         )
      );

   ext_config_html(wspr, 'WSPR', 'WSPR', 'WSPR configuration', s);

	s = '';
	for (var i=0; i < rx_chans;) {
	   var s2 = '';
      var f1 = 'w3-margin-right w3-defer';
      var f2 = f1 +' w3-margin-T-8';
	   for (var j=0; j < 8 && i < rx_chans; j++, i++) {
	      var arun = w3_array_el_seq(wspr.menu_i_to_cfg_i, +cfg.WSPR['autorun'+ i]);
	      //console.log('WSPR.autorun'+ i +'='+ cfg.WSPR['autorun'+ i] +' arun='+ arun);
	      s2 +=
	         w3_div('',
	            w3_select(f1, 'Autorun '+ i, 'WSPR band', 'WSPR.autorun'+ i, arun, wspr.autorun_u, 'wspr_autorun_select_cb'),
	            w3_select_get_param(f2, '', 'preemptable?', 'WSPR.preempt'+ i, wspr.preempt_u, 'wspr_preempt_select_cb')
	            //w3_select_get_param(f2, '', 'start UTC', 'WSPR.start'+ i, wspr.sched_u, 'wspr_autorun_sched_cb', 0, 0),
	            //w3_select_get_param(f2, '', 'stop UTC', 'WSPR.stop'+ i, wspr.sched_u, 'wspr_autorun_sched_cb', 0, 1)
	         );
	   }
	   s += w3_inline('w3-margin-bottom/', s2);
	}
	w3_innerHTML('id-wspr-admin-autorun', s);
}

function wspr_GPS_update_grid_cb(path, idx, first)
{
	var enabled = w3_switch_idx2val(+idx);
	//console.log('wspr_GPS_update_grid_cb: first='+ first +' enabled='+ enabled);

	if (!first) {
	   ext_send("ADM get_gps_info");    // NB: must be sent as ADM command
	}
	
	admin_bool_cb(path, enabled, first);
}

function wspr_autorun_public_check()
{
   var num_autorun = 0;
	for (var i=0; i < rx_chans; i++) {
	   if (cfg.WSPR['autorun'+ i] != 0 && cfg.WSPR['preempt'+ i] == wspr.PREEMPT_NO)
	      num_autorun++;
	}
	if (cfg.WSPR.autorun != num_autorun)
	   ext_set_cfg_param('WSPR.autorun', num_autorun, EXT_SAVE);
	
	var full = (adm.kiwisdr_com_register && num_autorun >= rx_chans);
   w3_remove_then_add_cond('id-wspr-warn-full', full, 'w3-red', 'w3-yellow');
	if (full) {
      kiwisdr_com_register_cb('adm.kiwisdr_com_register', w3_SWITCH_NO_IDX);
	}
}

function wspr_autorun_restart_cb()
{
   console.log('wspr_autorun_restart_cb');
   wspr_autorun_public_check();
   w3_border('id-wspr-restart', '');
   ext_send("ADM wspr_autorun_restart");  // NB: must be sent as ADM command
}

function wspr_autorun_select_cb(path, idx, first)
{
   //console.log('wspr_autorun_select_cb path='+ path +' idx='+ idx +' cfg_i='+ wspr.menu_i_to_cfg_i[+idx]);
   admin_select_cb(path, wspr.menu_i_to_cfg_i[+idx], first);
   if (first) return;
   w3_border('id-wspr-restart', wspr.restart_highlight);
	var el = w3_el('id-kiwi-container');
	w3_scrollDown(el);   // keep menus visible
	
}

function wspr_preempt_select_cb(path, idx, first)
{
   //console.log('wspr_preempt_select_cb path='+ path +' idx='+ idx);
   admin_select_cb(path, idx, first);
   if (first) return;
   w3_border('id-wspr-restart', wspr.restart_highlight);
	var el = w3_el('id-kiwi-container');
	w3_scrollDown(el);   // keep menus visible
	
}

function wspr_autorun_sched_cb(path, idx, first, cbp)
{
   //console.log('wspr_autorun_sched_cb path='+ path +' idx='+ idx +' cbp='+ cbp +' first='+ first);
}

function wspr_autorun_all_regular_cb(path, idx, first)
{
   if (first) return;
   for (var i = 0; i < rx_chans; i++) {
      path = 'WSPR.autorun'+ i;
      w3_select_value(path, 0);
      admin_select_cb(path, 0, /* first: true => no save */ true);
   }
   ext_set_cfg_param('WSPR.autorun', 0, EXT_SAVE);
   w3_border('id-wspr-restart', wspr.restart_highlight);
	var el = w3_el('id-kiwi-container');
	w3_scrollDown(el);   // keep menus visible
}

function wspr_autorun_all_preempt_cb(path, idx, first)
{
   console.log('wspr_autorun_all_preempt_cb: idx='+ idx);
   if (first) return;
   var val = +idx;
   for (var i = 0; i < rx_chans; i++) {
      path = 'WSPR.preempt'+ i;
      w3_select_value(path, val);
      admin_select_cb(path, val, /* first: true => no save */ true);
   }
   ext_set_cfg_param('WSPR.preempt', 0, EXT_SAVE);
   w3_border('id-wspr-restart', wspr.restart_highlight);
	var el = w3_el('id-kiwi-container');
	w3_scrollDown(el);   // keep menus visible
}

function wspr_config_focus()
{
   //console.log('wspr_config_focus');
   wspr_autorun_public_check();
   wspr_check_GPS_update_grid();
   wspr.focus_interval = setInterval(function() { wspr_check_GPS_update_grid(); }, 10000);

   var el = w3_el('id-wspr-grid-set');
	if (el) el.onclick = function() {
	   wspr.single_shot_update = true;
	   ext_send("ADM get_gps_info");    // NB: must be sent as ADM command
	};
}

function wspr_config_blur()
{
   //console.log('wspr_config_blur');
   kiwi_clearInterval(wspr.focus_interval);
}

function wspr_check_GPS_update_grid()
{
   var auto = kiwi.GPS_auto_grid;
   var ok = isNonEmptyString(auto);
   //console.log('wspr_check_GPS_update_grid GPS_update_grid='+ cfg.WSPR.GPS_update_grid +' kiwi.GPS_auto_grid='+ auto +' w3_get_value(WSPR.grid)='+ w3_get_value('WSPR.grid') +' cfg.WSPR.grid='+ cfg.WSPR.grid);

   if (cfg.WSPR.GPS_update_grid && ok && wspr.grid != auto) {
      wspr.grid = auto;
      w3_set_value('id-WSPR.grid', wspr.grid);
      w3_input_change('WSPR.grid', 'wspr_input_grid_cb');
      //console.log('wspr_check_GPS_update_grid SET '+ wspr.grid);
   }
   if (kiwi.GPS_fixes) w3_show_inline_block('id-wspr-grid-set');
   wspr.single_shot_update = false;
}

// called from receipt of an "ADM get_gps_info_cb" in admin_recv()
// which is the callback to us sending an on-demand "ADM get_gps_info" above
function wspr_gps_info_cb()
{
   //console.log('wspr_gps_info_cb');
   if (!cfg.WSPR.GPS_update_grid && !wspr.single_shot_update) return;
   wspr.grid = kiwi.GPS_auto_grid;
   w3_set_value('id-WSPR.grid', wspr.grid);
   w3_input_change('WSPR.grid', 'wspr_input_grid_cb');
   wspr.single_shot_update = false;
}

var wspr_stop_start_state = 0;

function wspr_stop_start_cb(path, idx, first)
{
   if (wspr_stop_start_state == 0) {
      wspr_reset();
   } else {
      // startup if there is a band to start with
		if (wspr_init_band != -1) {
			wspr_freq(wspr_init_band);
	   }
   }
   
   wspr_stop_start_state ^= 1;
   w3_button_text(path, wspr_stop_start_state? 'start' : 'stop');
}

function wspr_test_cb(path, val, first)
{
   if (ext_nom_sample_rate() != 12000) return;
   val = +val;
   if (dbgUs) console.log('wspr_test_cb: val='+ val);
   wspr.testing = val;
   w3_show_hide('id-wspr-upload-container', !wspr.testing, 'w3-show-inline-new');
	w3_el('id-wspr-bar').style.width = '0%';
   w3_show_hide('id-wspr-bar-container', wspr.testing);
	ext_send('SET test='+ (val? 1:0));
}

function wspr_reset()
{
	//console.log('### wspr_reset');
	ext_send('SET capture=0');
	wspr_set_status(wspr_status.IDLE);
	
	wspr_set_upload_cb('', true);    // by default allow uploads unless manually unchecked
}

function wspr_clear_cb(path, idx, first)
{
	wspr_reset();
   wspr_test_cb('', 0);
	w3_el('id-wspr-decode').innerHTML = '';
	w3_el('id-wspr-peaks-labels').innerHTML = '';
}

var wspr_upload_timeout;

function wspr_draw_scale(cf)
{
	wspr_scale_canvas.ct.fillStyle="black";
	wspr_scale_canvas.ct.fillRect(0, 0, wspr_scale_canvas.width, wspr_scale_canvas.height);

	var y = 2;
	wspr_scale_canvas.ct.fillStyle="lime";
	var start = Math.round(50*(512.0/375.0));
	var width = Math.round(200*(512.0/375.0));
	wspr_scale_canvas.ct.fillRect(wspr_startx+start*2, y, width*2, 3);

	wspr_scale_canvas.ct.fillStyle="red";
	start = Math.round(150*(512.0/375.0));
	wspr_scale_canvas.ct.fillRect(wspr_startx + start*2-2, y, 5, 3);
	
	wspr_scale_canvas.ct.fillStyle="white";
	wspr_scale_canvas.ct.font="10px Verdana";
	var f;
	for (f=-150; f<=150; f+=50) {
		var bin = Math.round((f+150)*(512.0/375.0));
		var tf = f + cf;
		if (tf < 0) tf += 1000;
		var tcw = (tf < 10)? 4 : ((tf < 100)? 7:11);
		wspr_scale_canvas.ct.fillText(tf.toFixed(0), wspr_startx + (bin*2)-tcw, y+15);
	}
}

function wspr_set_upload_cb(path, checked)
{
	// remove old cookie use
	kiwi_storeDelete('wspr_upload');
	
	if (!wspr_config_okay || wspr.upload_lockout) checked = false;
	wspr.upload = checked;
	w3_checkbox_set('id-wspr.upload', checked);
	w3_color('id-wspr-upload-container', checked? "white":"black", checked? "inherit":"yellow");
}

// from WSPR-X via tcpdump: (how can 'rcall' have an un-%-escaped '/'?)
// GET /post?function=wsprstat&rcall=ZL/KF6VO&rgrid=RF82ci&rqrg=14.097100&tpct=0&tqrg=14.097100&dbm=0&version=0.8_r3058 HTTP/1.1
// GET /post?function=wspr&rcall=ZL/KF6VO&rgrid=RF82ci&rqrg=14.097100&date=140818&time=0808&sig=-25&dt=-0.2&drift=-1&tqrg=14.097018&tcall=VK6PG&tgrid=OF78&dbm=33&version=0.8_r3058 HTTP/1.1

function wspr_upload(type, s)
{
	var spot = (type == wspr_report_e.SPOT)? 1:0;
	var rcall = ext_get_cfg_param_string('WSPR.callsign', '', EXT_NO_SAVE);

   // Check for GPS-driven grid updates, i.e. those not coming from admin WSPR config changes.
   // These are delivered with the 10 second status updates if GPS-driven grid updates enabled.
   var auto = cfg.WSPR.GPS_update_grid;
   wspr.grid = (auto && isNonEmptyString(kiwi.GPS_auto_grid))? kiwi.GPS_auto_grid : cfg.WSPR.grid;
   var rgrid = wspr.grid;
	//console.log('wspr_upload: rcall=<'+ rcall +'> rgrid=<'+ rgrid +'>');
	var valid = wspr_rfreq && wspr_tfreq && (rcall != undefined) && (rgrid != undefined) && (rcall != null) && (rgrid != null) && (rcall != '') && (rgrid != '');

	// don't even report status if not uploading
	if (!valid || (wspr.upload == false)) {
		wspr_upload_timeout = setTimeout(function() {wspr_upload(wspr_report_e.STATUS);}, 1*60*1000);	// check again in another minute
		return;
	}
	
	var decode;
	if (spot) {
		decode = s.replace(/[\s]+/g, ' ').split(' ');		// remove multiple spaces before split()
	}
	
	var tqrg, dbm;
	
	var url = "http://wsprnet.org/post";
	//var url = "http://example.com/post";
	var request = kiwi_GETrequest(spot? "spot":"stat", url);
	kiwi_GETrequest_param(request, "function", spot? "wspr":"wsprstat");
	kiwi_GETrequest_param(request, "rcall", rcall);
	kiwi_GETrequest_param(request, "rgrid", rgrid);
	kiwi_GETrequest_param(request, "rqrg", wspr_rfreq.toFixed(6));
	
	if (spot) {
		var d = new Date();
		kiwi_GETrequest_param(request, "date",
			d.getUTCFullYear().toString().substr(2,2)+(d.getUTCMonth()+1).leadingZeros(2)+d.getUTCDate().leadingZeros(2));
		kiwi_GETrequest_param(request, "time", decode[0]);
		kiwi_GETrequest_param(request, "sig", decode[1]);
		kiwi_GETrequest_param(request, "dt", decode[2]);
		kiwi_GETrequest_param(request, "drift", decode[4]);
		tqrg = decode[3];
		dbm = decode[7];
	} else {
		kiwi_GETrequest_param(request, "tpct", "0");
		tqrg = wspr_tfreq.toFixed(6);
		dbm = "0";
	}
	
	kiwi_GETrequest_param(request, "tqrg", tqrg);

	if (spot) {
		kiwi_GETrequest_param(request, "tcall", decode[5]);
		kiwi_GETrequest_param(request, "tgrid", (decode[6] != 'no_grid')? decode[6] : '');
	}
	
	kiwi_GETrequest_param(request, "dbm", dbm);

	var version = "1.4 Kiwi";
	if (version.length <= 10) {
		kiwi_GETrequest_param(request, "version", version);
		kiwi_GETrequest_submit(request, { gc: kiwi_gc_wspr } );

		var now = new Date();
		//console.log('WSPR '+ (spot? 'SPOT':'STAT') +' '+ now.toUTCString() + (spot? (' <'+ s +'>'):''));
	}

	// report status every six minutes
	if (!spot) wspr_upload_timeout = setTimeout(function() {wspr_upload(wspr_report_e.STATUS);}, 6*60*1000);
}

var wspr_cur_status = 0;
var wspr_status = { 'NONE':0, 'IDLE':1, 'SYNC':2, 'RUNNING':3, 'DECODING':4 };
var wspr_status_text = { 0:'none', 1:'idle', 2:'sync', 3:'running', 4:'decoding' };
var wspr_status_color = { 0:'white', 1:'lightSkyBlue', 2:'violet', 3:'cyan', 4:'lime' };

function wspr_set_status(status)
{
	var el = w3_el('id-wspr-status');
	el.innerHTML = wspr_status_text[status];
	el.style.backgroundColor = wspr_status_color[status];
	
	wspr_cur_status = status;
}

function wspr_server_Date()
{
	return new Date(wspr.server_time_ms + (Date.now() - wspr.local_time_epoch_ms));
}

function wspr_draw_pie() {
	var d = wspr_server_Date();
	w3_el('id-wspr-time').innerHTML = d.toUTCString().substr(17,8) +' UTC';
	var min = d.getUTCMinutes();
   var wspr_secs = (min&1)*60 + d.getUTCSeconds() + 1;
   kiwi_draw_pie('id-wspr-pie', wspr.pie_size, wspr_secs / 120);

   // Check for GPS-driven grid updates, i.e. those not coming from admin WSPR config changes.
   // These are delivered with the 10 second status updates if GPS-driven grid updates enabled.
   var auto = cfg.WSPR.GPS_update_grid;
   wspr.grid = (auto && isNonEmptyString(kiwi.GPS_auto_grid))? kiwi.GPS_auto_grid : cfg.WSPR.grid;
   var rgrid = wspr.grid;
   //console.log('wspr_draw_pie GPS_update_grid='+ auto +' wspr.grid='+ rgrid +' kiwi.GPS_auto_grid='+ kiwi.GPS_auto_grid);
   w3_innerHTML('id-wspr-rgrid', 'reporter grid<br>'+ rgrid + (auto? ' (GPS)':''));
   
   // long-running decode abort happens at 1:40, so should be safe to switch freq at 1:50
   if (wspr.IWBP && wspr_secs == (60 + 50) /* 1:50 */) {
      var deco_hop = Math.floor((min % wspr.hop_period) / 2);
      var deco_cf = wspr.cf_IWBP[deco_hop];

      var hop = Math.floor(((min+1) % wspr.hop_period) / 2);
      var cf = wspr.cf_IWBP[hop];

      //console.log('WSPR IWBP deco: #'+ deco_hop +'/'+ min +'m='+ deco_cf +' capture: #'+ hop +'/'+ ((min+1)%60) +'m='+ cf);
      wspr_change_freq(cf, deco_cf, wspr.NO_SYNC);
   }
}

var wspr_rfreq=0, wspr_tfreq=0;
var wspr_bfo = 750;
var wspr_filter_bw = 300;

function wspr_freq(b)
{
   wspr_reset();

   var cf;
   if (wspr.freqs_m[b] == 'IWBP') {
      wspr.IWBP = true;
	   min = wspr_server_Date().getUTCMinutes();
      var hop = Math.floor((min % wspr.hop_period) / 2);
      cf = wspr.cf_IWBP[hop];
   } else {
      wspr.IWBP = false;
      cf = wspr.center_freqs[b];
   }
   wspr_change_freq(cf, cf, wspr.SYNC);
}

function wspr_change_freq(cf, deco_cf, sync)
{
	var dial_freq_kHz = cf - wspr_bfo/1000;
	var cfo = Math.round((cf - Math.floor(cf)) * 1000);

	var deco_dial_freq_kHz = deco_cf - wspr_bfo/1000;
	var deco_cfo = Math.round((deco_cf - Math.floor(deco_cf)) * 1000);

	w3_el('id-wspr-cf').innerHTML = 'CF '+ cf.toFixed(1);
	wspr_rfreq = wspr_tfreq = cf/1000;
	var r = ext_get_freq_range();
	var fo_kHz = dial_freq_kHz - r.offset_kHz;
	ext_tune(fo_kHz, 'usb', ext_zoom.MAX_IN);
	ext_set_passband(wspr_bfo-wspr_filter_bw/2, wspr_bfo+wspr_filter_bw/2);
	ext_tune(fo_kHz, 'usb', ext_zoom.MAX_IN);		// FIXME: temp hack so new passband gets re-centered
   wspr_draw_scale(cfo);

   ext_send('SET dialfreq='+ deco_dial_freq_kHz.toFixed(2) +' centerfreq='+ deco_cf.toFixed(2) +' cf_offset='+ deco_cfo);

	if (sync == wspr.SYNC) ext_send('SET capture=1');
   
   // promptly notify band change
   kiwi_clearTimeout(wspr_upload_timeout);
   wspr_upload_timeout = setTimeout(function() {wspr_upload(wspr_report_e.STATUS);}, 1000);
}
