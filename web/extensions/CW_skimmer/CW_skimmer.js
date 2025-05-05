// Copyright (c) 2018 John Seamons, ZL4VO/KF6VO

var cws = {
   ext_name: 'CW_skimmer',    // NB: must match cw_skimmer.cpp:cw_skimmer_ext.name
   first_time: true,
   height: 400,
   max_chars: 120,
   
   wspace: true,
   test: 0,
   texts: [],

   pwr_calc: 0,
   pwr_calc_s: ['avg_ratio', 'avg_bottom', 'threshold'],
   filter_neighbors: 1,

   log_mins: 0,
   log_interval: null,

   last_last: 0
};

function CW_skimmer_main()
{
	ext_switch_to_client(cws.ext_name, cws.first_time, cw_skimmer_recv);		// tell server to use us (again)
	if (!cws.first_time)
		cw_skimmer_controls_setup();
	cws.first_time = false;
}

function cw_skimmer_recv(data)
{
	var firstChars = arrayBufferToStringLen(data, 3);
	
	// process data sent from server/C by ext_send_msg_data()
	if (firstChars == "DAT") {
		var ba = new Uint8Array(data, 4);
		var cmd = ba[0];
		var o = 1;
		var len = ba.length-1;

		console.log('cw_skimmer_recv: DATA UNKNOWN cmd='+ cmd +' len='+ len);
		return;
	}
	
	// process command sent from server/C by ext_send_msg() or ext_send_msg_encoded()
	var stringData = arrayBufferToString(data);
	var params = stringData.substring(4).split(" ");

	for (var i=0; i < params.length; i++) {
		var param = params[i].split("=");

		if (0 && param[0] != "keepalive") {
			if (isDefined(param[1]))
				console.log('cw_skimmer_recv: '+ param[0] +'='+ param[1]);
			else
				console.log('cw_skimmer_recv: '+ param[0]);
		}

		switch (param[0]) {

			case "ready":
				cw_skimmer_controls_setup();
				break;

			case "cw_chars":
				cw_skimmer_output_chars(param[1]);
				break;

			case "bar_pct":
			   if (cws.test) {
               var pct = w3_clamp(parseInt(param[1]), 0, 100);
               //if (pct > 0 && pct < 3) pct = 3;    // 0% and 1% look weird on bar
               var el = w3_el('id-cw-bar');
               if (el) el.style.width = pct +'%';
               //console.log('bar_pct='+ pct);
            }
			   break;

			default:
				console.log('cw_skimmer_recv: UNKNOWN CMD '+ param[0]);
				break;
		}
	}
}

function cw_skimmer_output_chars(input)
{
   // split cfreq from input like "a1234" to a and 1234
   input = kiwi_decodeURIComponent('CW', input);
   //console.log(input);
   var msg = {
      text: input.charAt(0),
      freq: parseInt(input.substring(1)),
      wpm: parseInt(input.substring(1).split(',')[1])
   };

   // Current time
   var now = Date.now();
   
   // Modify or add a new entry
   var j = cws.texts.findIndex(function(x) { return x.freq >= msg.freq; });
   if (j < 0) {
      // Append a new entry
      if (msg.text.trim().length > 0) {
         cws.texts.push({ freq: msg.freq, wpm: msg.wpm, text: msg.text, ts: now });
      }
   } else if (cws.texts[j].freq == msg.freq) {
      // Update existing entry
      cws.texts[j].text = (cws.texts[j].text + msg.text).slice(-cws.max_chars);
      cws.texts[j].ts   = now;
   } else {
     // Insert a new entry
      if (msg.text.trim().length > 0) {
         cws.texts.splice(j, 0, { freq: msg.freq, wpm: msg.wpm, text: msg.text, ts: now });
      }
   }
   
   // Generate table body
   var body = '';
   for (j = 0 ; j < cws.texts.length ; j++) {
      // Limit the lifetime of entries depending on their length
      var cutoff = 5000 * cws.texts[j].text.length;
      if (now - cws.texts[j].ts >= cutoff) {
         cws.texts.splice(j--, 1);
      } else {
         var f = Math.floor(cws.texts[j].freq / 100.0) / 10.0 + freq_car_Hz / 1e3;
         body +=
            w3_table_row('|color:black;background:'+ (j&1? '#E0FFE0':'#FFFFFF'),
               w3_table_cells('cl-cws-data', f.toFixed(1), cws.texts[j].wpm, cws.texts[j].text)
            );
      }
   }
   
   w3_innerHTML('id-cws-table', body);		// overwrites last status msg
}

function cw_skimmer_controls_setup()
{
   var do_test = false, do_help = false;
	var p = ext_param();
	if (p) {
      p = p.split(',');
      p.forEach(
         function(a, i) {
            console.log('p'+ i +'='+ a);
            var r;
            if (i == 0 && isNumber(a)) {
               cws.pboff_locked = a;
            } else
            if (w3_ext_param('test', a).match) {
               do_test = true;
            } else
            if (w3_ext_param('help', a).match) {
               do_help = true;
            }
         }
      );
   }
	
   var data_html =
      time_display_html('cw') +
      
      w3_div('id-cw-data|left:150px; width:1044px; height:'+ px(cws.height) +'; overflow:hidden; position:relative; background-color:mediumBlue;',
			w3_div('id-cw-skimmer-msg w3-text-output w3-scroll-down w3-small w3-text-black|width:1024px; position:absolute; overflow-x:hidden;',
            w3_table('',
               w3_table_row('', 
                  w3_table_heads('cl-cws-freq', 'Freq'), w3_table_heads('cl-cws-wpm', 'WPM'), w3_table_heads('cl-cws-text', 'Text')
               ),
               w3_table_body('id-cws-table')
            )
			)
      );

	var controls_html =
		w3_div('id-cw-controls w3-text-white',
			w3_divs('',
            w3_inline('',
				   w3_div('w3-medium w3-text-aqua', '<b>CW skimmer</b>'),
               w3_div('w3-margin-L-16', 'Luarvique KC1TXE')
				),
				w3_div('', '<b><a href="https://github.com/luarvique/csdr-cwskimmer" target="_blank">CSDR based CW skimmer</a></b> &copy; 2025'),
            
				w3_inline('w3-margin-T-10/w3-margin-between-16',
               w3_button('w3-padding-smaller', 'Clear', 'cws_clear_button_cb', 0),
               w3_button('id-cw-test w3-padding-smaller w3-green', 'Test', 'cws_test_cb', 0),
               w3_div('id-cw-bar-container w3-progress-container w3-round-large w3-white w3-hide|width:70px; height:16px',
                  w3_div('id-cw-bar w3-progressbar w3-round-large w3-light-green|width:0%', '&nbsp;')
               )
            ),

				w3_inline('w3-margin-T-10/w3-margin-between-16',
               w3_select('w3-text-red', '', 'power calc', 'cws.pwr_calc', cws.pwr_calc, cws.pwr_calc_s, 'cws_pwr_calc_cb'),
               w3_checkbox('/w3-label-inline w3-label-not-bold/', 'filter neighbors', 'cws.filter_neighbors', cws.filter_neighbors, 'cws_filter_neighbors_cb')
				)
			)
		);
	
	//cws.saved_setup = ext_save_setup();
	//ext_set_mode('cw');
	ext_panel_show(controls_html, data_html, null);
	time_display_setup('cw');

   ext_set_data_height(cws.height);
	ext_set_controls_width_height(350, 125);
	
	// our sample file is 12k only
	if (ext_nom_sample_rate() != 12000)
	   w3_disable('id-cw-test');

   ext_send('SET cws_start');

   cws_clear_button_cb();
	if (do_test) cws_test_cb();
	if (do_help) extint_help_click();
}

function cws_clear_button_cb(path, idx, first)
{
   if (first) return;
   cws.texts = [];
   w3_innerHTML('id-cws-table', '');
}

function cws_send_params()
{
   ext_send('SET cws_params='+ cws.pwr_calc +','+ (cws.filter_neighbors? 1:0));
}

function cws_pwr_calc_cb(path, idx, first)
{
   if (first) return;
   cws.pwr_calc = +idx;
   cws_send_params();
}

function cws_filter_neighbors_cb(path, checked, first)
{
   cws.filter_neighbors = checked;
   cws_send_params();
}

/*
function cws_log_mins_cb(path, val)
{
   cws.log_mins = w3_clamp(+val, 0, 24*60, 0);
   console.log('cws_log_mins_cb path='+ path +' val='+ val +' log_mins='+ cws.log_mins);
	w3_set_value(path, cws.log_mins);

   kiwi_clearInterval(cws.log_interval);
   if (cws.log_mins != 0) {
      console.log('CW logging..');
      cws.log_interval = setInterval(function() { cws_log_cb(); }, cws.log_mins * 60000);
   }
}

function cws_log_cb()
{
   for (var j = 0 ; j < cws.texts.length ; j++) {
      cws.log_txt += kiwi_remove_escape_sequences(kiwi_decodeURIComponent('CW', cws.texts[j].text));
      cws.log_txt += "\n";
   }

   var txt = new Blob([cws.log_txt], { type: 'text/plain' });
   var a = document.createElement('a');
   a.style = 'display: none';
   a.href = window.URL.createObjectURL(txt);
   a.download = kiwi_timestamp_filename('cws.', '.log.txt');
   document.body.appendChild(a);
   console.log('cws_log: '+ a.download);
   a.click();
   window.URL.revokeObjectURL(a.href);
   document.body.removeChild(a);
}
*/

function cws_test_cb()
{
   //console_nv('cws_test_cb', 'cws.test');
	if (ext_nom_sample_rate() != 12000) return;
   cws.test = cws.test? 0:1;    // if already running stop if clicked again
   w3_colors('id-cw-test', '', 'w3-red', cws.test);
	w3_el('id-cw-bar').style.width = '0%';
   w3_show_hide('id-cw-bar-container', cws.test);
   ext_send('SET cws_test='+ cws.test);
}

function CW_skimmer_blur()
{
	ext_set_data_height();     // restore default height
	ext_send('SET cws_stop');
   kiwi_clearInterval(cws.log_interval);
	//ext_restore_setup(cws.saved_setup);
}

// called to display HTML for configuration parameters in admin interface
function CW_skimmer_config_html()
{
   ext_config_html(cws, 'cws', 'CW skimmer', 'CW skimmer configuration');
}

function CW_skimmer_help(show)
{
   if (show) {
      var s = 
         w3_text('w3-medium w3-bold w3-text-aqua', 'CW skimmer help') +
         w3_div('w3-margin-T-8 w3-scroll-y|height:90%',
            w3_div('w3-margin-R-8',
               'Automatically decodes multiple CW signals within the current passband.'
            )
         );
      confirmation_show_content(s, 600, 350);
      w3_el('id-confirmation-container').style.height = '100%';   // to get the w3-scroll-y above to work
   }
   return true;
}
