// Copyright (c) 2016-2025 John Seamons, ZL4VO/KF6VO

// TODO
//		input range validation
//		NTP status?

var admin = {
   current_tab_name: '',
   ext_configs_done: false,
   console_open: false,
   status_interval: null,
   
   pending_restart: false,
   pending_reboot: false,
   pending_power_off: false,
   confirm_cb_func: null,
   cancel_cb_func: null,
   
   long_running: false,
   is_multi_core: false,
   
   wb_u: [ '72k', '108k', '144k', '192k', '204k', '240k', '300k' ],
   
   update_interval: null,
   status: {},
   
   pie_size: 25,
   mode_fmt: 'w3-margin-left w3-valign w3-padding-L-8 w3-border w3-border-light-blue w3-center',
   
   spectral_inversion_lockout: false,
   
   users_list: null,
   users_seq: 0,
   users_sort: 0,
   exp_vis: [],
   
   FRPC_EXISTING:    0,
   FRPC_NEW:         1,
   FRPC_UPDATE_HOST: 2,
   FRPC_PROXY_UPD:   3,
   
   _last_: 0
};


////////////////////////////////
// status
////////////////////////////////

// so can be called from browser console easily
function za(zoom)
{
   if (dbgUs) ext_send('SET zoom_all='+ zoom);
   return zoom;
}

function status_html()
{
   var s2 = admin_sdr_mode?
      (
         w3_div('w3-section',
            w3_text('w3-text-black',
               'Your Kiwi <i>may</i> restart during the nightly update window for the following reasons. ' +
               'The restart will not occur when there are active user connections. <br>' +
               'But <b>will</b> occur if there are only "background" connections such as FT8/WSPR autorun, kiwirecorder (e.g. wsprdaemon) etc. <br>' +
               'To prevent <i>any</i> restarts disable all of the restart sources listed below until all of the icons are grey in color.'
            ),
            w3_div('w3-container',
               w3_inline('', w3_icon('id-rst-daily', 'fa-square', 16, 'grey'), w3_text('w3-margin-left w3-text-black', 'Control tab: Daily restart')),
               w3_inline('', w3_icon('id-rst-comm',  'fa-square', 16, 'grey'), w3_text('w3-margin-left w3-text-black', 'DX tab: Automatically download community database')),
               w3_inline('', w3_icon('id-rst-swupd', 'fa-square', 16, 'grey'), w3_text('w3-margin-left w3-text-black', 'Update tab: Automatically install software updates')),
               w3_inline('', w3_icon('id-rst-ipbl',  'fa-square', 16, 'grey'), w3_text('w3-margin-left w3-text-black', 'Network tab: Automatically download IP blacklist'))
            )
         ) +
         '<hr>'
      ) : '';

   var s3 = admin_sdr_mode?
		(
         w3_div('id-msg-errors') + 
         w3_div('w3-section',
            w3_inline('',
               w3_div('', 'Realtime response histograms:'),
               w3_button('w3-padding-smaller w3-aqua|margin-left:10px', 'Reset', 'status_dpump_hist_reset_cb')
            ),
            w3_div('w3-container',
               w3_div('id-status-dp-hist'),
               w3_div('id-status-in-hist')
            )
         ) +
         '<hr>'
      ) : '';
   
	var s =
      w3_divs('id-status w3-hide/w3-container',
         w3_div('id-problems') +
         w3_div('id-msg-config') +
         w3_div('id-msg-debian') +
         w3_div('id-msg-gps') +
         w3_div('id-msg-snr') +
         w3_div('id-msg-antsw w3-hide') +
         w3_div('', 'Browser: '+ navigator.userAgent) +
         '<hr>' +
         w3_div('id-msg-stats-cpu') +
         w3_div('id-msg-stats-xfer') +
         '<hr>' +
         w3_div('id-users-list') +
         '<hr>' +
         s2 + s3
      );
   
	return s;
}

function status_focus()
{
   if (admin_sdr_mode) {
      w3_colors('id-rst-daily', 'grey', 'lime', adm.daily_restart);
      w3_colors('id-rst-comm',  'grey', 'lime', adm.dx_comm_auto_download);
      w3_colors('id-rst-swupd', 'grey', 'lime', adm.update_install);
      w3_colors('id-rst-ipbl',  'grey', 'lime', adm.ip_blacklist_auto_download);
   }

   if (kiwi.test_cfg_save_seq) {
      cfg_save_json('test_cfg_save_seq 1', 'cfg');
      setTimeout(function() { cfg_save_json('test_cfg_save_seq 2', 'cfg'); }, 100);
      setTimeout(function() { cfg_save_json('test_cfg_save_seq 3', 'cfg'); }, 200);
      setTimeout(function() { cfg_save_json('test_cfg_save_seq 4', 'cfg'); }, 300);
   }

   kiwi_clearInterval(admin.status_interval);
	admin.status_interval =
	   setInterval(
	      function() {
	         msg_send('SET xfer_stats');
	         ext_send('ADM antsw_GetCurrentAnt');
	         
	         var hide = true;
	         if (ant_sw && isNonEmptyString(ant_sw.status) && isFunction(ant_switch_disabled) && !ant_switch_disabled()) {
	            w3_innerHTML('id-msg-antsw', 'Antennas selected: '+ ant_sw.status);
	            hide = false;
	         }
	         w3_hide2('id-msg-antsw', hide);
	      },
	   1000);
}

function status_blur()
{
   kiwi_clearInterval(admin.status_interval);
}

function status_xfer_cb(audio_dropped, underruns, seq_errors, dp_resets, dp_in_hist_resets, dp_hist_cnt, dp_hist, in_hist_cnt, in_hist)
{
   var i, s;
   if (audio_dropped == undefined) return;
   
	var el = w3_el('id-msg-errors');
	if (el) el.innerHTML = 'Stats: '+
	   'dropped '+ audio_dropped.toUnits() +
	   ', underruns '+ underruns.toUnits() +
	   ', sequence '+ seq_errors.toUnits() +
	   ', realtime_D '+ dp_resets.toUnits() +
	   ', realtime_S '+ dp_in_hist_resets.toUnits();

	el = w3_el('id-status-dp-hist');
	if (el) {
	   s = 'Datapump: ';
		for (i = 0; i < dp_hist_cnt; i++) {
		   s += (i? ' ':'') + dp_hist[i].toUnits();
		}
      el.innerHTML = s;
	}

	el = w3_el('id-status-in-hist');
	if (el) {
	   s = 'SoundInQ: ';
		for (i = 0; i < in_hist_cnt; i++) {
		   s += (i? ' ':'') + in_hist[i].toUnits();
		}
      el.innerHTML = s;
	}
}

function status_dpump_hist_reset_cb(id, idx)
{
	ext_send('SET dpump_hist_reset');
}

function status_user_kick_cb(id, idx)
{
   console.log('status_user_kick_cb='+ idx);
	ext_send('SET user_kick='+ idx);
}


////////////////////////////////
// mode
////////////////////////////////

var mode_icon_snd12 = w3_icon('w3-text-blue', 'fa-volume-up', 28) +'&nbsp;';
var mode_icon_snd20 = w3_icon('w3-text-red', 'fa-volume-up', 28) +'&nbsp;';
var mode_icon_fft   = w3_icon('w3-text-green', 'fa-bar-chart', 28) +'&nbsp;';
var mode_icon_wf    = w3_icon('w3-text-amber', 'fa-area-chart', 28) +'&nbsp;';

function mode_html()
{
   var bw = 245, bwpx = px(bw);
   var pw = 113, pwpx = px(pw);
   var ci = 0;
   
   var max = (admin.is_multi_core);
   var wb = 0;
   
   var s1 = '';
   if (max)
      s1 += w3_nav(admin_colors[ci++] +' w3-border w3-padding-xxlarge w3-restart', 'Max channels', 'id-sidenav-fw', kiwi.RX14_WF0, 'firmware_sel_cb', (adm.firmware_sel == kiwi.RX14_WF0));
   if (wb)
      s1 += w3_nav(admin_colors[ci++] +' w3-border w3-padding-xxlarge w3-restart', 'Wideband output', 'id-sidenav-fw', kiwi.RX_WB, 'firmware_sel_cb', (adm.firmware_sel == kiwi.RX_WB));

   var s2 = '';
   if (max)
      s2 += w3_div('id-fw-14ch w3-flex w3-padding-TB-6');
   if (wb)
      s2 += w3_div('id-fw-wb w3-flex w3-padding-TB-6');

	var s =
	w3_div('id-mode w3-hide',
		w3_div('w3-container',
         w3_div('w3-flex w3-margin-B-8',
            //w3_div('w3-text-teal|width:'+ pwpx, ' '),
            w3_div('w3-text-teal w3-center w3-bold|width:'+ bwpx, 'select FPGA mode'),
            w3_div('id-fw-hdr w3-flex w3-margin-left')
         ),
         
         w3_div('',
            w3_sidenav('id-sidenav-fw|width:'+ bwpx +';border-collapse:collapse',
               w3_nav(admin_colors[ci++] +' w3-border w3-padding-xxlarge w3-restart', 'Kiwi classic', 'id-sidenav-fw', kiwi.RX4_WF4, 'firmware_sel_cb', (adm.firmware_sel == kiwi.RX4_WF4)),
               w3_nav(admin_colors[ci++] +' w3-border w3-padding-xxlarge w3-restart', 'More receivers', 'id-sidenav-fw', kiwi.RX8_WF2, 'firmware_sel_cb', (adm.firmware_sel == kiwi.RX8_WF2)),
               w3_nav(admin_colors[ci++] +' w3-border w3-padding-xxlarge w3-restart', 'More bandwidth', 'id-sidenav-fw', kiwi.RX3_WF3, 'firmware_sel_cb', (adm.firmware_sel == kiwi.RX3_WF3)),
               s1
            ),
            w3_div('w3-margin-left w3-left',
               w3_div('id-fw-4ch w3-flex w3-padding-TB-6'),
               w3_div('id-fw-8ch w3-flex w3-padding-TB-6'),
               w3_div('id-fw-3ch w3-flex w3-padding-TB-6'),
               s2
            )
         ),
         
		   w3_div('w3-clear', ' '),      // don't quite understand why this is needed, but it is
		   w3_div('w3-margin-T-16', '<hr>'),

         w3_div('w3-text-black',
            w3_text('w3-bold w3-margin-B-8 w3-text-teal', 'Trade-offs: receiver channels, audio bandwidth and waterfalls'),

            w3_div('w3-flex w3-valign-center', w3_div('|width:40px', mode_icon_snd12), w3_div('', 'Audio output, 12 kHz max bandwidth')),
            w3_div('w3-flex w3-valign-center', w3_div('|width:40px', mode_icon_snd20), w3_div('', 'Audio output, 20 kHz max bandwidth')),
            w3_div('w3-flex w3-margin-B-8 w3-valign-center', w3_div('|width:40px', mode_icon_wf),  w3_div('', 'Tuneable waterfall/spectrum, 30 MHz bandwidth, 14-level zoom')),
            w3_div('w3-flex w3-margin-B-8 w3-valign-center', w3_div('|width:40px', mode_icon_fft), w3_div('', 'Audio FFT display, 12/20 kHz max bandwidth'))
         ),
		   w3_div('w3-margin-T-16', '<hr>'),
   
         w3_div('w3-width-half w3-text-black',
            'Description of the different modes:' +
            '<ul>' +
            
            '<li><b>Kiwi classic</b><br>' +
            'The original Kiwi FPGA firmware, with its 4 tuneable audio/waterfall receiver channels and 12 GPS channels. ' +
            'The other modes were developed later on, where some of the waterfalls and GPS channels are traded for ' +
            'adding more audio channels.' +
            '<br><br></li>' +
            
            '<li><b>More receivers</b><br>' +
            'Having more receiver channels per Kiwi is especially important with the features that are channel intensive. ' +
            'Like the TDoA service, WSPR/FT8 autorun and external connections via the kiwirecorder program for other software such ' +
            'as wsprdaemon. When these kinds of connections are made in "more receivers" mode channels rx2 - rx7 will be used first. ' +
            'Leaving rx0 and rx1 available for normal browser connections where it is desirable to view the waterfall. ' +
            'However rx0 and rx1 will be used last if necessary. A user connection on rx2 - rx7 will show an audio-bandwidth FFT in place of ' +
            'the usual waterfall. This works because it requires no additional FPGA resources.' +
            '<br><br></li>' +

            '<li><b>More bandwidth</b><br>' +
            'In this mode the audio bandwidth is increased from 12 to 20 kHz. ' +
            'This supports wide passbands for hi-fidelity listening of AM BCB and SW stations. ' +
            'And also wide IQ bandwidths for external applications processing large parts of the spectrum. ' +
            'In exchange the number of channels must drop from four to three.' +
            '</li>' +
            
            (max?
               ('<br><li><b>Max channels</b><br>' +
               'For custom Kiwi setups using more powerful BeagleBones (e.g. BBAI, BBAI-64) this mode ' +
               'maximizes the number of receiver channels. The trade off is that no waterfalls are available. ' +
               '</li>')
               : '') +
            
            '</ul>'
         ),
         w3_div('w3-margin-T-16', '<hr>')
      )
	);
	
	if (wb) setTimeout(function() { mode_wb_srate(); }, 1);
	return s;
}

function mode_wb_srate_cb(path, idx, first)
{
   admin_select_cb(path, idx, first);
   w3_innerHTML('id-fw-wb-srate', parseInt(admin.wb_u[adm.wb_sel]) +' kHz');
   if (!first && adm.firmware_sel == kiwi.RX_WB) w3_restart_cb();
}

function mode_wb_srate()
{
   var s =
      w3_div(admin.mode_fmt +'|width:834px;height:58px',
         w3_div('w3-font-32px w3-margin-R-4', '🌀'),
         'Wideband output &nbsp;'+ w3_div('id-fw-wb-srate')
      ) +
      w3_select('w3-center w3-text-teal w3-margin-L-32//', 'Wideband bandwidth', '', 'adm.wb_sel', adm.wb_sel, admin.wb_u, 'mode_wb_srate_cb');
   w3_innerHTML('id-fw-wb', s);
}

function mode_focus()
{
   //console.log('mode_focus');
   var i, s;
   var iwpx = px(90);
   
   s = '';
   for (i = 0; i < 8; i++) s += w3_div('w3-margin-left w3-bold w3-center|width:'+ iwpx, 'rx'+ i);
   w3_innerHTML('id-fw-hdr', s);

   //var rx12wf = w3_div('w3-margin-left w3-border w3-border-light-blue w3-center|width:'+ iwpx, mode_icon_snd12, mode_icon_fft, '<br>', mode_icon_wf);
   var rx12_wf  = w3_div('w3-margin-left w3-border w3-border-light-blue w3-center|width:'+ iwpx, mode_icon_snd12, '<br>', mode_icon_wf);
   var rx20_wf  = w3_div('w3-margin-left w3-border w3-border-light-blue w3-center|width:'+ iwpx, mode_icon_snd20, '<br>', mode_icon_wf);
   
   var rx12_afft = w3_div('w3-margin-left w3-border w3-border-light-blue w3-center|width:'+ iwpx, mode_icon_snd12, '<br>', mode_icon_fft);
   
   s = '';
   for (i = 0; i < 4; i++) s += rx12_wf;
   //for (i = 4; i < 8; i++) s += w3_div('w3-margin-left|width:50px', '&nbsp;');
   w3_innerHTML('id-fw-4ch', s);

   s = '';
   for (i = 0; i < 2; i++) s += rx12_wf;
   for (i = 2; i < 8; i++) s += rx12_afft;
   w3_innerHTML('id-fw-8ch', s);

   s = '';
   for (i = 0; i < 3; i++) s += rx20_wf;
   w3_innerHTML('id-fw-3ch', s);

   if (admin.is_multi_core) {
      s = rx12_afft + w3_div(admin.mode_fmt +'|width:728px', '14 rx channels total...');
      w3_innerHTML('id-fw-14ch', s);
   }
}

function firmware_sel_cb_focus(path)
{
   var firmware_sel = +path;
   console.log('firmware_sel_cb_focus path='+ path);
	ext_set_cfg_param('adm.firmware_sel', firmware_sel, EXT_SAVE);
}


////////////////////////////////
// control
////////////////////////////////

var daily_restart_u = { 0: 'no', 1: 'restart server', 2: 'reboot Beagle' };

function control_html()
{
   var i;
   
   // Let cfg.ext_api_nchans retain values > rx_chans if it was set when another configuration
   // was used. Just clamp the menu value to the current rx_chans;
	var ext_api_ch = ext_get_cfg_param('ext_api_nchans', -1);
	if (ext_api_ch == -1) ext_api_ch = rx_chans;      // has never been set
	var ext_api_nchans = Math.min(ext_api_ch, rx_chans);
   var ext_api_chans_u = { 0:'none' };
   for (i = 1; i <= rx_chans; i++)
      ext_api_chans_u[i] = i.toFixed(0);

	var s1 =
		w3_inline('w3-container w3-halign-space-between/',
         w3_inline('w3-flex-col w3-gap-10/',
            w3_button('w3-aqua', 'KiwiSDR server restart', 'admin_restart_cb'),
            w3_button('w3-blue', 'Beagle reboot', 'admin_reboot_cb'),
            w3_button('w3-red', 'Beagle power off', 'admin_power_off_cb')
         ),
         
			w3_div('w3-center',
            w3_select('w3-center//w3-width-auto', 'Daily restart?', '', 'adm.daily_restart', adm.daily_restart, daily_restart_u, 'admin_select_cb'),
				w3_div('w3-text-black w3-tspace-8',
					"Set if you're having problems with the server<br>after it has run for a period of time.<br>" +
					"Restart occurs at the same time as updates (0100-0600 Local)<br> and will wait until there are no active user connections."
				)
			),

         w3_divs('w3-center/w3-tspace-8',
            w3_select('w3-width-auto', 'Number of simultaneous channels available<br>for connection by non-Kiwi apps',
               '', 'ext_api_nchans', ext_api_nchans, ext_api_chans_u, 'admin_select_cb'),
            w3_div('w3-text-black',
               'If you want to limit incoming connections from <br> non-Kiwi apps like kiwirecorder set this value. <br>' +
               'This overrides similar value in TDoA extension settings.'
            )
         )
      );

	var s2 =
		'<hr>' +
		w3_third('w3-container', '',
         w3_inline('w3-halign-space-around/',
            w3_switch_label('w3-center', 'Enable user<br>connections?', 'Yes', 'No', 'adm.server_enabled', adm.server_enabled, 'server_enabled_cb'),
            w3_divs('w3-center/w3-margin-T-8',
               w3_div('', '<b>Close all active<br>user connections</b>'),
               w3_button('w3-red', 'Kick', 'control_user_kick_cb')
            )
         ),

			w3_divs('w3-restart w3-center/w3-tspace-8',
            w3_switch_label('w3-center', 'Disable waterfalls/spectrum?', 'Yes', 'No', 'cfg.no_wf', cfg.no_wf, 'admin_radio_YN_cb'),
				w3_div('w3-text-black',
				   'Set "yes" to save Internet bandwidth by preventing <br>' +
				   'the waterfall and spectrum from being displayed.'
				)
			),
			
			w3_divs('', 
            w3_checkbox_get_param('//w3-label-inline', 'Require name/callsign entry when connecting', 'require_id', 'admin_bool_cb', false),
            w3_checkbox_get_param('w3-margin-T-8//w3-label-inline', 'Prevent multiple connections from <br> the same IP address', 'adm.no_dup_ip', 'admin_bool_cb', false),
            w3_checkbox_get_param('w3-margin-T-8//w3-label-inline w3-restart', 'Non-Kiwi connections (kiwirecorder, TDoA) <br> can preempt autorun processes', 'any_preempt_autorun', 'admin_bool_cb', true)
         )
		) +

      w3_half('w3-margin-T-20', 'w3-container',
         w3_div('',
            w3_input_get('', 'Reason if disabled', 'reason_disabled', 'reason_cb', '', 'will be shown to users attempting to connect'),
            w3_divs('w3-margin-top/',
               '<label><b>Disabled reason HTML preview</b></label>',
               w3_div('id-reason-disabled-preview w3-text-black w3-background-pale-aqua', '')
            )
         ),
         w3_div('',
            w3_input_get('', 'Reason if kicked', 'reason_kicked', 'reason_cb', '', 'will be shown to users when kicked'),
            w3_divs('w3-margin-top/',
               '<label><b>Kicked reason HTML preview</b></label>',
               w3_div('id-reason-kicked-preview w3-text-black w3-background-pale-aqua', '')
            )
         )
      );
	
	var n_camp = ext_get_cfg_param('n_camp', -1);
	console.log('rx_chans='+ rx_chans +' n_camp='+ n_camp +' max_camp='+ max_camp);
   var n_camp_u = [ 'disable camping' ];
   for (i = 1; i <= max_camp; i++)
      n_camp_u[i] = i.toFixed(0);
   var snr_interval_u = [ 'disable', 'hourly', '4 hours', '6 hours', '24 hours', '1 min', '5 min', '10 min', 'custom' ];

	var s3 =
		'<hr>' +
		w3_col_percent('w3-margin-bottom w3-text-teal/w3-container',
			w3_div('',
				w3_input_get('', 'Inactivity time limit (minutes, 0 = no limit)', 'inactivity_timeout_mins', 'admin_int_cb'),
				w3_div('w3-text-black', 'Connections from the local network are exempt.')
			), 30,
			w3_div('',
				w3_input_get('', '24hr per-IP addr time limit (min, 0 = no limit)', 'ip_limit_mins', 'admin_int_cb'),
				w3_div('w3-text-black', 'Connections from the local network are exempt.')
			), 30,
			w3_div('',
				w3_input_get('', 'User exemption password', 'adm.tlimit_exempt_pwd', 'w3_string_set_cfg_cb'),
				w3_div('w3-text-black',
				   'Password users can give to override time limits, etc. <br>' +
				   'To specify in URL: my_kiwi:8073/?pwd=<i>password</i> <br>' +
				   'Or: my_kiwi:8073/?password=<i>password</i> <br>' +
               'Also password for RF attenuator and antenna switch.'
            )
			)
		) +

		'<hr>' +
		w3_col_percent('w3-margin-bottom w3-text-teal/w3-container',
         w3_divs('w3-restart/w3-center w3-tspace-8',
            w3_select('w3-width-auto', 'Number of audio campers per channel', '', 'n_camp', n_camp, n_camp_u, 'admin_select_cb'),
            w3_div('w3-text-black',
               'Reduce this value if your Kiwi is experiencing <br>' +
               'performance problems from too many audio campers.'
            )
         ), 30,
         
			w3_divs('/w3-center w3-tspace-8',
            w3_select('w3-width-auto', 'SNR measurement interval', '', 'cfg.snr_meas_interval_hrs', cfg.snr_meas_interval_hrs, snr_interval_u, 'control_snr_interval_cb'),
				w3_text('w3-text-black w3-center',
				   'Enables automatic sampling of <br>' +
				   'signal-to-noise ratio (SNR) at the specified interval. <br>' +
				   'Access SNR data in JSON format using <br>' +
				   'URL of the form: <i>my_kiwi:8073/snr</i>'
				),
				w3_div('id-snr-remain w3-margin-top w3-bold')
			), 30,
			
			w3_divs('/w3-tspace-8',
			   w3_div('',
               w3_checkbox_get_param('//w3-label-inline', 'Timestamp SNR with local time', 'snr_local_time', 'admin_bool_cb', true),
               w3_checkbox_get_param('//w3-label-inline', 'Also measure ham bands and AM BCB', 'snr_meas_ham', 'admin_bool_cb', false),
               w3_checkbox_get_param('//w3-label-inline', 'Measure on antenna change (after 5 second delay)', 'snr_meas_ant_sw', 'admin_bool_cb', false),
               w3_input_get('w3-margin-top//', 'Custom interval (min)', 'snr_meas_custom_min', 'admin_int_cb'),
               w3_inline('w3-margin-T-8',
                  w3_text('w3-text-teal w3-bold', 'Custom band:'),
                  w3_link('w3-link-darker-color w3-margin-L-4', '//forum.kiwisdr.com/index.php?p=/discussion/comment/21044/#Comment_21044', 'more info')
               ),
               w3_inline('w3-valign w3-gap-16 w3-halign-space-between/',
                  w3_input_get('', 'Freq lo (kHz)', 'snr_meas_custom_lo', 'admin_int_cb'),
                  w3_input_get('', 'Freq hi (kHz)', 'snr_meas_custom_hi', 'admin_int_cb'),
                  w3_input_get('', 'Zoom', 'snr_meas_custom_zoom', 'admin_int_cb')
               )
            ),
            w3_inline('w3-margin-top w3-valign/',
               w3_button('w3-aqua', 'Measure SNR now', 'control_snr_measure_cb'),
               w3_div('id-msg-snr-now w3-margin-left w3-text-black')
            )
         )
		) +
		'<hr>';

   return w3_div('id-control w3-text-teal w3-hide', s1 + (admin_sdr_mode? (s2 + s3) : ''));
}

function control_focus()
{
	w3_innerHTML('id-reason-disabled-preview', admin_preview_status_box('disabled_preview_1', cfg.reason_disabled));
	w3_innerHTML('id-reason-kicked-preview', admin_preview_status_box('kicked_preview_1', cfg.reason_kicked));
}

function server_enabled_cb(path, idx, first)
{
	idx = +idx;
	var enabled = (idx == 0);
	//console.log('server_enabled_cb: first='+ first +' enabled='+ enabled);

	if (!first) {
		ext_send('SET server_enabled='+ (enabled? 1:0));
	}
	
	admin_bool_cb(path, enabled, first);
}

function control_user_kick_cb(id, idx)
{
	ext_send('SET user_kick=-1');
}

function control_snr_measure_cb(id, idx)
{
   kiwi.snr_measuring = true;
   w3_innerHTML('id-msg-snr-now', w3_icon('', 'fa-refresh fa-spin', 24) + ' &nbsp; measuring..');
	ext_send('SET snr_meas');
}

function control_snr_interval_cb(path, idx, first)
{
   idx = +idx;
   //console.log('control_snr_interval_cb path='+ path +' idx='+ idx +' first='+ first);
   if (first) return;
   admin_select_cb(path, idx, first);
   var min = (idx == kiwi.SNR_CUSTOM)? cfg.snr_meas_custom_min : kiwi.snr_intervals_min[idx];
   ext_send('SET snr_interval='+ (min * 60));
   var s = idx? ('Next measurement in '+ min +' min') : 'Measurement disabled';
   w3_innerHTML('id-snr-remain', s);
}

function reason_cb(path, val)
{
	w3_string_set_cfg_cb(path, val);
	w3_el('id-reason-disabled-preview').innerHTML = admin_preview_status_box('disabled_preview_2', cfg.reason_disabled);
	w3_el('id-reason-kicked-preview').innerHTML = admin_preview_status_box('kicked_preview_2', cfg.reason_kicked);
}


////////////////////////////////
// connect
////////////////////////////////

var connect = {
   focus_seen: 0,
   focus_query: false,
   NOT_IP:0, IS_IP:1, LOCAL_IP:-1, 
   timeout: null
};

// REMEMBER: cfg.server_url is what's used in kiwisdr.com registration
// cfg.sdr_hu_dom_sel is just the selector of kiwi.{NAM,DUC,PUB,SIP,REV}
// Both these are cfg parameters stored in kiwi.json, so don't get confused.

// cfg.{sdr_hu_dom_sel(num), sdr_hu_dom_name(str), sdr_hu_dom_ip(str), server_url(str)}

var duc_update_i = { 0:'5 min', 1:'10 min', 2:'15 min', 3:'30 min', 4:'60 min' };
var duc_update_v = { 0:5, 1:10, 2:15, 3:30, 4:60 };

function connect_rev_user() { return (adm.rev_auto? adm.rev_auto_user : adm.rev_user); }
function connect_rev_host() { return (adm.rev_auto? adm.rev_auto_host : adm.rev_host); }

function connect_html()
{
   // remove old references to kiwisdr.example.com so empty field message shows
   var s = ext_get_cfg_param('server_url');
   if (s == 'kiwisdr.example.com') {
      ext_set_cfg_param('cfg.server_url', '', EXT_SAVE);
      s = '';
   }
   
   // fix unexpected fixed IP when proxy mode selected
   if (cfg.sdr_hu_dom_sel == kiwi.REV && (s == '' || s == '103.156.230.194')) {
      var server_url = connect_rev_host();
      if (server_url != '') server_url += '.'+ adm.proxy_server;
      console.log('connect_html REV RESET server_url='+ server_url);
      ext_set_cfg_param('cfg.server_url', server_url, EXT_SAVE);
   }
   
   if (ext_get_cfg_param('sdr_hu_dom_name') == 'kiwisdr.example.com')
      ext_set_cfg_param('cfg.sdr_hu_dom_name', '', EXT_SAVE);

   var ci = 0;
   var s1 =
		w3_div('w3-valign',
			w3_header('w3-container w3-yellow/', 5,
            'If you are not able to make an incoming connection from the Internet to your Kiwi because ' +
            'of problems <br> with your router or Internet Service Provider (ISP) then please consider using the KiwiSDR ' +
            '<a href='+ dq('http://'+ admin.proxy_host) +' target="_blank">reverse proxy service</a>.'
         )
		) +
		
      '<hr>' +
      w3_divs('w3-container/w3-tspace-8',
         w3_label('w3-bold', 'What domain name or IP address will people use to connect to your KiwiSDR?<br>' +
            'If you are listing on rx.kiwisdr.com this information will be part of your entry.<br>' +
            'Click one of the five options below and enter any additional information:<br><br>'),
         
         // (n/a anymore) w3-static because w3-sidenav does a position:fixed which is no good here at the bottom of the page
         // w3-left to get float:left to put the input fields on the right side
         // w3-sidenav-full-height to match the height of the w3_input() on the right side
		   w3_sidenav('id-sidenav-dom w3-margin-R-16',
		      w3_nav(admin_colors[ci++] +' w3-border', 'Domain Name', 'id-sidenav-dom', 'connect_dom_nam', 'connect_dom_nam', (cfg.sdr_hu_dom_sel == kiwi.NAM)),
		      w3_nav(admin_colors[ci++] +' w3-border', 'DUC Domain', 'id-sidenav-dom', 'connect_dom_duc', 'connect_dom_duc', (cfg.sdr_hu_dom_sel == kiwi.DUC)),
		      w3_nav(admin_colors[ci++] +' w3-border', 'Reverse Proxy', 'id-sidenav-dom', 'connect_dom_rev', 'connect_dom_rev', (cfg.sdr_hu_dom_sel == kiwi.REV)),
		      w3_nav(admin_colors[ci++] +' w3-border', 'Public IP', 'id-sidenav-dom', 'connect_dom_pub', 'connect_dom_pub', (cfg.sdr_hu_dom_sel == kiwi.PUB)),
		      w3_nav(admin_colors[ci++] +' w3-border', 'Specified IP', 'id-sidenav-dom', 'connect_dom_sip', 'connect_dom_sip', (cfg.sdr_hu_dom_sel == kiwi.SIP))
		   ),
		   
		   w3_divs('w3-padding-L-16/w3-padding-T-1',
		      w3_inline('',
               w3_div('id-dom-field w3-show-inline-block|width:70%;', w3_input_get('', '', 'sdr_hu_dom_name', 'connect_dom_name_cb', '',
                  'Enter domain name that you will point to Kiwi public IP address, e.g. kiwisdr.my_domain.com (don\'t include port number)')),
               w3_div('id-dom-error w3-margin-L-32 w3-padding-LR-8 w3-red w3-hide')
            ),
            w3_div('id-connect-duc-dom w3-padding-TB-8'),
            w3_div('id-connect-rev-dom w3-padding-TB-8'),
            w3_div('id-connect-pub-ip w3-padding-TB-8'),
		      w3_inline('',
               w3_div('id-ip-field w3-show-inline-block|width:70%;', w3_input_get('', '', 'sdr_hu_dom_ip', 'connect_dom_ip_cb', '',
                  'Enter known public IP address of the Kiwi (don\'t include port number or use a local ip address)')),
               w3_div('id-ip-error w3-margin-L-32 w3-padding-LR-8 w3-red w3-hide')
            )
         ),
         
		   w3_div('w3-margin-T-16', 
            w3_label('id-connect-url-label w3-show-inline-block w3-margin-R-16 w3-text-teal') +
			   w3_div('id-connect-url w3-show-inline-block w3-text-black w3-background-pale-aqua')
         )
      );

   var s2 =
		'<hr>' +
      w3_div('w3-container w3-text-teal|width:80%',
         w3_input_get('', 'Next Kiwi URL redirect', 'adm.url_redirect', 'w3_url_set_cfg_cb'),
         w3_div('w3-text-black',
            'Use this setting to get multiple Kiwis to respond to a single URL.<br>' +
            'When all the channels of this Kiwi are busy further connection attempts ' +
            'will be redirected to the above URL.<br>' +
            'Example: Your Kiwi is known as "mykiwi.com:8073". ' +
            'Configure another Kiwi to use port 8074 and be known as "mykiwi.com:8074".<br>' +
            'On the port 8073 Kiwi set the above field to "http://mykiwi.com:8074".<br>' +
            'On the port 8074 Kiwi leave the above field blank.<br>' +
            'Configure the port 8074 Kiwi as normal (i.e. router port open, dynamic DNS, proxy etc.)<br><br>' +
            '<b>CAUTION:</b> Do not create a cycle by redirecting Kiwis like: A -> B -> A &nbsp;' +
            'Always create a chain that stops redirecting at the end: A -> B <br>' +
            'This prevents the browser from going into a loop when all channels on all Kiwis are full.'
         )
		);

   var s3 =
		'<hr>' +
		w3_divs('/w3-tspace-8',
         w3_div('w3-container w3-valign',
            w3_header('w3-container w3-yellow/', 6,
               'Please read these instructions before use: ' +
               '<a href="http://kiwisdr.com/info#id-net-duc" target="_blank">dynamic DNS update client (DUC)</a>'
            )
         ),

			w3_col_percent('w3-text-teal/w3-container',
			   w3_div('w3-text-teal w3-bold', 'Dynamic DNS update client (DUC) configuration'), 50,
				w3_div('w3-text-teal w3-bold w3-center w3-light-grey', 'Account at noip.com'), 50
			),
			
			w3_col_percent('w3-text-teal/w3-container',
				w3_div(), 50,
				w3_input_get('', 'Username or email', 'adm.duc_user', 'w3_string_set_cfg_cb', '', 'required'), 25,
				w3_input_get('', 'Password', 'adm.duc_pass', 'w3_string_set_cfg_cb', '', 'required'), 25
			),
			
			w3_col_percent('w3-text-teal/w3-container',
				w3_switch_label('w3-center', 'Enable DUC at startup?', 'Yes', 'No', 'adm.duc_enable', adm.duc_enable, 'connect_DUC_enabled_cb'
				), 20,
				
				w3_div('w3-center',
				   w3_select('w3-width-auto', 'Update', '', 'adm.duc_update', adm.duc_update, duc_update_i, 'admin_select_cb')
				), 10,
				
				w3_div('w3-center w3-tspace-8',
					w3_button('w3-aqua', 'Click to (re)start DUC', 'connect_DUC_start_cb'),
					w3_div('w3-text-black',
						'After changing username or password click to test changes.'
					)
				), 20,
				
				w3_input_get('', 'Host (e.g. xyz.ddns.net)', 'adm.duc_host', 'connect_DUC_host_cb', '', 'required'), 50
			),
			
			w3_div('w3-container',
            w3_label('w3-show-inline-block w3-margin-R-16 w3-text-teal', 'Status:') +
				w3_div('id-net-duc-status w3-show-inline-block w3-text-black w3-background-pale-aqua', '')
			)
		) +
      '<hr>';
	
	var auto_s = '';
	if (kiwi.model != kiwi.KiwiSDR_1) {
      if (isNonEmptyString(adm.rev_auto_user) && isNonEmptyString(adm.rev_auto_host)) {
         auto_s =
            w3_div('w3-center w3-tspace-8',
               w3_switch_label('w3-center', 'Automatic configuration?', 'Yes', 'No', 'adm.rev_auto', adm.rev_auto, 'connect_auto_proxy_cb', 'clicked'),
               w3_div('w3-text-black',
                  'KiwiSDR 2 and later have the proxy service <br>' +
                  'enabled by default using the Kiwi serial <br>' +
                  'number as the host name. Set to <x1>No</x1> to <br>' +
                  'choose your own host name.'
                  )
            );
      } else {
         auto_s =
            w3_div('w3-text-black',
               'The automatic proxy configuration <br> for this Kiwi seems to be missing. <br>' +
               'Please contact support@kiwisdr.com <br><br> Manual proxy setup is shown.'
            );
         connect_auto_proxy_cb('adm.rev_auto', w3_switch_val2idx(false));
         
      }
	}

   var proxy_s =
      '<hr>' +
      w3_divs('/w3-tspace-16',
			w3_col_percent('w3-container/',
            w3_div('w3-valign',
               w3_header('w3-container w3-yellow/', 6,
                  'Please read these instructions before use: ' +
                  '<a href='+ dq('http://'+ admin.proxy_host) +' target="_blank">reverse proxy service</a>'
               )
            ), 50,

            w3_text('id-proxy-menu w3-margin-left w3-valign w3-nopad w3-width-min w3-red w3-hide',
               'When adding or making changes to the proxy user key or host name fields <br>' +
               'you must make sure "Reverse proxy" is selected in the menu at the top <br>' +
               'of the page AND click the \"re-register\" button below. <br><br>' +

               'NOTE: If you are connected to the admin page <i>using</i> a proxy connection <br>' +
               'then after clicking \"re-register\" you will be immediately reconnected to <br>' +
               'the admin page using any host name change you have made.'
            )
         ),

			w3_col_percent('w3-text-teal/w3-container',
			   w3_div('w3-text-teal w3-bold', 'Reverse proxy configuration'), 50,
				w3_div('id-proxy-hdr w3-text-teal w3-bold w3-center w3-light-grey',
				   'Proxy information for '+ admin.proxy_host), 50
			),
		
			w3_half('w3-text-teal', 'w3-container',
            w3_inline('w3-halign-space-around/',
               w3_div('w3-center w3-tspace-8',
                  w3_button('w3-aqua', 'Click to (re)register', 'connect_rev_register_cb'),
                  w3_div('w3-text-black',
                     'After changing user key or<br>host name click to register proxy.'
                  )
               ),
               auto_s
			   ),
			   
            w3_divs('/w3-tspace-16',
            
               // user key
               w3_input_get('id-proxy-user//|width:70%', 'User key: (see instructions)', 'adm.rev_user', 'connect_rev_user_cb', '', 'required'),

               w3_div('id-proxy-auto-user w3-hide/',
                  w3_text('w3-bold w3-text-teal', 'User key:'),
                  w3_div('',
                     w3_text('w3-text-black', adm.rev_auto_user),
                     w3_text('w3-text-black', '(automatically generated)')
                  )
               ),

               // host name
               w3_inline('id-proxy-host w3-valign-end/',
                  w3_input_get('|width:70%/', "Host name: a-z, 0-9, -, _<br>(all lower case, no leading '-' or digit, see instructions)",
                     'adm.rev_host', 'connect_rev_host_cb', '', 'required'
                  ),
                  w3_div('id-connect-proxy_server w3-margin-L-8 w3-show-inline-block')
               ),

               w3_div('id-proxy-auto-host w3-hide/',
                  w3_text('w3-bold w3-text-teal', 'Host name:'),
                  w3_div('',
                     w3_text('w3-text-black w3-padding-0', adm.rev_auto_host),
                     w3_text('id-connect-proxy_server2 w3-text-black'),
                     w3_text('w3-text-black', '(automatically generated)')
                  )
               )
            )
			),
		
         w3_half('w3-margin-top w3-margin-bottom w3-text-teal', 'w3-container',
			//w3_div('w3-container',
            w3_label('w3-text-top w3-show-inline-block w3-margin-R-16 w3-text-teal', 'Status:') +
				w3_div('id-connect-rev-status w3-show-inline-block w3-text-black w3-background-pale-aqua', ''),
				
            w3_div('w3-restart|width:70%;',
               w3_input_get('id-proxy-server', 'Proxy server hostname', 'adm.proxy_server', 'connect_proxy_server_cb'),
               w3_div('w3-text-black',
                  'Change <b>only</b> if you have implemented a private proxy server. <br>' +
                  'Set to '+ dq(admin.proxy_host) +' for the default proxy service.'
               )
            )
			)
		);

	return w3_div('id-connect w3-text-teal w3-hide', s1 + proxy_s + s2 + s3);
}

function connect_focus()
{
   connect.focus_seen = 1;
   connect_update_url();
   w3_el('id-proxy-hdr').innerHTML = 'Proxy information for '+ adm.proxy_server;
   ext_send('SET DUC_status_query');
	
   w3_hide('id-proxy-menu');
	if (cfg.sdr_hu_dom_sel == kiwi.REV) {
	   console.log('connect_focus rev_status_query');
	   connect.focus_query = true;
	   ext_send('SET rev_status_query');
	}
	
	connect_auto_proxy_cb('adm.rev_auto', w3_switch_val2idx(adm.rev_auto));
}

function connect_blur()
{
   connect.focus_seen = 0;
}

function connect_auto_proxy_cb(path, idx, first, cb_param)
{
	var enabled = (+idx == w3_SWITCH_YES_IDX);
	console.log('connect_auto_proxy_cb: path='+ path +' first='+ first +' enabled='+ enabled);
	admin.last_rev_user = connect_rev_user();
	admin.last_rev_host = connect_rev_host();
	console.log('connect_auto_proxy_cb: last_rev_user|host='+ admin.last_rev_user +'|'+ admin.last_rev_host);
	admin_bool_cb(path, enabled, first);
	
	w3_hide2('id-proxy-user', enabled);
	w3_hide2('id-proxy-auto-user', !enabled);
	w3_hide2('id-proxy-host', enabled);
	w3_hide2('id-proxy-auto-host', !enabled);
	
	if (cb_param && cb_param == 'clicked') {
	   connect_rev_register_cb();
      connect_rev_usage();
   }
}

function connect_update_url()
{
   var ok, ok_color = [ 'w3-override-yellow', 'w3-background-pale-aqua' ];
   
   ok = (adm.duc_host && adm.duc_host != '')? 1:0;
	w3_innerHTML('id-connect-duc-dom', 'Use domain name from DUC configuration below: ' +
	   w3_div('w3-show-inline-block w3-text-black '+ ok_color[ok], ok? adm.duc_host : '(none currently set)'));

   var rev_host = connect_rev_host();
   ok = (rev_host != '')? 1:0;
   var rev_host_fqdn = ok? (rev_host +'.'+ adm.proxy_server) : '(none currently set)';
	w3_innerHTML('id-connect-rev-dom', 'Use domain name from reverse proxy configuration below: ' +
	   w3_div('w3-show-inline-block w3-text-black '+ ok_color[ok], rev_host_fqdn));
	w3_el('id-connect-proxy_server').innerHTML = '.'+ adm.proxy_server;
	w3_el('id-connect-proxy_server2').innerHTML = '.'+ adm.proxy_server;

   ok = config_net.pub_ip? 1:0;
	w3_innerHTML('id-connect-pub-ip', 'Public IP address detected by Kiwi: ' +
	   w3_div('w3-show-inline-block w3-text-black '+ ok_color[ok], ok? config_net.pub_ip : '(no public IP address detected)'));

   var host = decodeURIComponent(cfg.server_url);
   var host_and_port = host;
   
   //console.log('connect_update_url: sdr_hu_dom_sel='+ cfg.sdr_hu_dom_sel +' REV='+ kiwi.REV +' host='+ host_and_port +' port_ext='+ adm.port_ext);

   if (cfg.sdr_hu_dom_sel != kiwi.REV) {
      host_and_port += ':'+ adm.port_ext;
      w3_set_label('Based on above selection, and external port from Network tab, the URL to connect to your Kiwi is:', 'id-connect-url');
   } else {
      // using proxy
      // proxy URLs no longer require the constant ":8073" to be specified
      /*
      if (admin.proxy_port != 80) {
         host_and_port += ':'+ admin.proxy_port;
         if (adm.port_ext != admin.proxy_port)
            host_and_port += ' (proxy always uses port '+ admin.proxy_port +' even though your external port is '+ adm.port_ext +')';
      }
      */
      w3_set_label('Based on the above selection the URL to connect to your Kiwi is:', 'id-connect-url');
   }
   
   ok = (host != '');
   w3_flag_cond('id-connect-url', !ok, ok? host_and_port : '(incomplete information, fill-in field above)');
}

function connect_stop_proxy()
{
   ext_send('SET stop_proxy');
}

function connect_my_kiwi_register()
{
   // wait for prior ext_set_cfg_param() to update on server
   setTimeout(function() { ext_send('SET my_kiwi_register'); }, 3000);
}

function connect_dom_nam_focus(ok)
{
   var server_url = (ok == false)? '' : cfg.sdr_hu_dom_name;
   console.log('connect_dom_nam_focus ok='+ ok +' server_url='+ server_url);
	ext_set_cfg_param('cfg.server_url', server_url, EXT_NO_SAVE);
	ext_set_cfg_param('cfg.sdr_hu_dom_sel', kiwi.NAM, EXT_SAVE);
	connect_update_url();
	connect_stop_proxy();
	connect_my_kiwi_register();
	if (server_url != '') w3_restart_cb();
}

function connect_dom_duc_focus()
{
   var server_url = adm.duc_host;
   console.log('connect_dom_duc_focus server_url='+ server_url);
	ext_set_cfg_param('cfg.server_url', server_url, EXT_NO_SAVE);
	ext_set_cfg_param('cfg.sdr_hu_dom_sel', kiwi.DUC, EXT_SAVE);
	connect_update_url();
	connect_stop_proxy();
	connect_my_kiwi_register();
	if (server_url != '') w3_restart_cb();
}

function connect_dom_rev_focus(check_restart)
{
   var server_url = connect_rev_host();
   if (server_url != '') server_url += '.'+ adm.proxy_server;
   console.log('connect_dom_rev_focus server_url='+ server_url);
	ext_set_cfg_param('cfg.server_url', server_url, EXT_NO_SAVE);
	ext_set_cfg_param('cfg.sdr_hu_dom_sel', kiwi.REV, EXT_SAVE);
	connect_update_url();
	connect_my_kiwi_register();
	if (check_restart && server_url != '') w3_restart_cb();
}

function connect_dom_pub_focus()
{
   var server_url = config_net.pub_ip;
   console.log('connect_dom_pub_focus server_url='+ server_url);
	ext_set_cfg_param('cfg.server_url', server_url, EXT_NO_SAVE);
	ext_set_cfg_param('cfg.sdr_hu_dom_sel', kiwi.PUB, EXT_SAVE);
	connect_update_url();
	connect_stop_proxy();
	connect_my_kiwi_register();
	if (server_url != '') w3_restart_cb();
}

function connect_dom_sip_focus(ok)
{
   var server_url = (ok == false)? '' : cfg.sdr_hu_dom_ip;
   console.log('connect_dom_sip_focus ok='+ ok +' server_url='+ server_url);
	ext_set_cfg_param('cfg.server_url', server_url, EXT_NO_SAVE);
	ext_set_cfg_param('cfg.sdr_hu_dom_sel', kiwi.SIP, EXT_SAVE);
	connect_update_url();
	connect_stop_proxy();
	connect_my_kiwi_register();
	if (server_url != '') w3_restart_cb();
}


// domain name

function connect_dom_name_cb(path, val, first)
{
   var ok = true;
	var dom = connect_remove_port_and_local_ip(path, val, first);
   //console.log('$connect_dom_name_cb <'+ dom +'>');
	if (isNonEmptyString(dom)) {
	   // accept public IP in addition to a domain name
	   var isIP = kiwi_inet4_d2h(dom);
	   if (isIP) {   
	      var isPublicIP = kiwi_inet4_d2h(dom, { no_local_ip:1 });
         connect_domain_check_cb(isPublicIP? 1:-1);
         if (!isPublicIP) ok = false;
      } else {
         console.log('SET domain_check='+ dom);
         //ext_send_new('SET domain_check='+ dom);
         ext_send('SET domain_check='+ encodeURIComponent(dom));
      }
	} else {
      connect_domain_check_cb(1);
   }

   if (cfg.sdr_hu_dom_sel == kiwi.NAM) {     // if currently selected option update the value
      connect_dom_nam_focus(ok);
   }
}

function connect_domain_check_cb(found)
{
   var ok;
   //console.log('$connect_domain_check_cb found='+ found);
   var el_field = w3_el('id-dom-field');
   var el_error = w3_el('id-dom-error');
   
   switch (found) {
      case 1:
         w3_hide(el_error);
         el_field.style.width = '70%';
         ok = true;
         break;
      case 0: case -1:
         w3_innerHTML(el_error,
            found? 'Error: must not be a local IP address' : 'Error: domain name lookup failed');
         w3_show(el_error);
         el_field.style.width = '40%';
         ok = false;
         break;
   }

   if (cfg.sdr_hu_dom_sel == kiwi.NAM) {     // if currently selected option update the value
      connect_dom_nam_focus(ok);
   }
}


// specified IP

function connect_dom_ip_cb(path, val, first)
{
   var ok = false;
	var ip_rem_port = connect_remove_port_and_local_ip(path, val, first);
   var isIP = kiwi_inet4_d2h(ip_rem_port);
   var isPublicIP = kiwi_inet4_d2h(ip_rem_port, { no_local_ip:1 });
   //console.log('$val='+ val +' ip_rem_port='+ ip_rem_port +' isIP='+ isIP +' isPublicIP='+ isPublicIP);
   var el_field = w3_el('id-ip-field');
   var el_error = w3_el('id-ip-error');
   
   if (isIP) {
      if (isPublicIP) {
         connect_ip_check_cb(connect.IS_IP);
         ok = true;
      } else {
         connect_ip_check_cb(connect.LOCAL_IP);
      }
   } else {
         connect_ip_check_cb((val == '')? connect.IS_IP : connect.NOT_IP);
   }

   if (cfg.sdr_hu_dom_sel == kiwi.SIP)     // if currently selected option update the value
      connect_dom_sip_focus(ok);
}

function connect_ip_check_cb(status)
{
   //console.log('$connect_ip_check_cb status='+ status);
   var el_field = w3_el('id-ip-field');
   var el_error = w3_el('id-ip-error');
   
   switch (status) {
      case connect.IS_IP:
         w3_hide(el_error);
         el_field.style.width = '80%';
         break;
      case connect.LOCAL_IP: case connect.NOT_IP:
         w3_innerHTML(el_error,
            (status == connect.LOCAL_IP)? 'Error: must not be a local IP address' : 'Error: not an IP address');
         w3_show(el_error);
         el_field.style.width = '40%';
         break;
   }
}

function connect_remove_port_and_local_ip(el, s, first, check_ip)
{
	var state = { bad:0, number:1, alpha:2, remove:3 };
	var st = state.bad;
	
	s = s.replace('http://', '').replace('https://', '');
	var sl = s.length;
	
	for (var i = sl-1; i >= 0; i--) {
		var c = s.charAt(i);
		if (c >= '0' && c <= '9') {
			st = state.number;
			continue;
		}
		if (c == ':') {
			if (st == state.number)
				st = state.remove;
			break;
		}
		st = state.alpha;
		if (c == ']')
		   break;      // start of escaped ipv6 with embedded ':'s
	}
	
	if (st == state.remove) {
		s = s.substr(0,i);
	}
	
	if (check_ip && check_ip['always'] && kiwi_inet4_d2h(s, { no_local_ip:1 }) == null) s = '';
	else
	
	// only check for local ip if entry is a valid ip to begin with (i.e. allows domain name)
	if (check_ip && check_ip['if_ip'] && kiwi_inet4_d2h(s) != null) {
	   if (kiwi_inet4_d2h(s, { no_local_ip:1 }) == null) s = '';
	}
	
	w3_string_set_cfg_cb(el, s, first);
	admin_set_decoded_value(el);
	return s;
}


// DUC

function connect_DUC_enabled_cb(path, idx, first)
{
	idx = +idx;
	var enabled = (idx == 0);
	//console.log('connect_DUC_enabled_cb: first='+ first +' enabled='+ enabled);

	if (!first) {
		//?(enabled? 1:0);
	}
	
	admin_bool_cb(path, enabled, first);
}

function connect_DUC_start_cb(id, idx)
{
	// decode stored json values because we recode below to encode spaces of composite string
	var s = '-u '+ sq(decodeURIComponent(adm.duc_user)) +' -p '+ sq(decodeURIComponent(adm.duc_pass)) +
	   ' -H '+ sq(decodeURIComponent(adm.duc_host)) +' -U '+ duc_update_v[adm.duc_update];
	console.log('start DUC: '+ s);
	w3_innerHTML('id-net-duc-status', w3_icon('', 'fa-refresh fa-spin', 24) + '&nbsp; Getting status from DUC server...');
	ext_send('SET DUC_start args='+ encodeURIComponent(s));
}

function connect_DUC_host_cb(path, val, first)
{
	var rem_port = connect_remove_port_and_local_ip(path, val, first);
   w3_string_set_cfg_cb(path, rem_port, first);
   if (cfg.sdr_hu_dom_sel == kiwi.DUC)     // if currently selected option update the value
      connect_dom_duc_focus();
   else
      connect_update_url();
}

function connect_DUC_status_cb(status)
{
	status = +status;
	console.log('DUC_status='+ status);
	var s;
	
	switch (status) {
		case 0:   s = 'DUC started successfully'; break;
		case 100: s = 'Incorrect username or password'; break;
		case 101: s = 'No hosts defined on your account at noip.com; please correct and retry'; break;
		case 102: s = 'Please specify a host'; break;
		case 103: s = 'Host given isn\'t defined on your account at noip.com; please correct and retry'; break;
		case 300: s = 'DUC start failed'; break;
		case 301: s = 'DUC enabled and running'; break;
		default:  s = 'DUC internal error: '+ status; break;
	}
	
	w3_el('id-net-duc-status').innerHTML = s;
}


// reverse proxy

function connect_rev_usage()
{
   w3_show('id-proxy-menu');
   //w3_scrollDown('id-kiwi-container');
}

function connect_rev_register_cb(id, idx)
{
   var auto = adm.rev_auto? 1:0;
   var user = connect_rev_user();
   var host = connect_rev_host();
   if (user == '' || host == '') {
      connect_rev_status_cb(100);
      return;
   }
   
   kiwi_clearTimeout(connect.timeout);
	w3_innerHTML('id-connect-rev-status', w3_icon('', 'fa-refresh fa-spin', 24) + '&nbsp; Getting status from proxy server...');

   // update info on proxy server
	var s = 'user='+ user +' host='+ host +' auto='+ auto;
	console.log('rev register: '+ s);
	ext_send('SET rev_register reg='+ admin.FRPC_PROXY_UPD +' '+ s);
}

function connect_rev_user_cb(path, val, first)
{
	admin.last_rev_user = connect_rev_user();
	console.log('connect_rev_user_cb: last_rev_user|host='+ admin.last_rev_user +'|'+ admin.last_rev_host);
   w3_clearInnerHTML('id-connect-rev-status');
   w3_string_set_cfg_cb(path, val, first);
   connect_rev_register_cb();
   connect_rev_usage();
}

function connect_rev_host_cb(path, val, first)
{
	console.log('connect_rev_host_cb: path='+ path +' val=<'+ val +'>');
	admin.last_rev_host = connect_rev_host();
	console.log('connect_rev_host_cb: last_rev_user|host='+ admin.last_rev_user +'|'+ admin.last_rev_host);
	var check_restart = true;
   connect_rev_usage();
   if (val[0] >= '0' && val[0] <= '9') {
      w3_innerHTML('id-connect-rev-status', 'First host name character cannot be a digit');
      val = admin_set_decoded_value('adm.rev_host');     // restore previous value
      check_restart = false;
   } else {
      w3_clearInnerHTML('id-connect-rev-status');
   }
   w3_string_set_cfg_cb(path, val, first);
   if (cfg.sdr_hu_dom_sel == kiwi.REV) {     // if currently selected option update the value
      connect_dom_rev_focus(check_restart);
      connect_rev_register_cb();
   } else {
      connect_update_url();
   }
}

function connect_rev_status_cb(status)
{
   if (!connect.focus_seen) return;
	status = +status;
	console.log('rev_status='+ status);
	var s, error = false;
	
	var auto = adm.rev_auto? 1:0;
   var user = connect_rev_user();
   var host = connect_rev_host();
   console_nv('$connect_rev_status_cb', {status}, {auto}, {user}, {host});
   if (!auto && (status == 200 || status == 201) && (user == '' || host == '')) {
      status = 100;
   }
	
	if (status >= 0 && status <= 99) {     // okay
	   console.log('$okay');
      if (cfg.sdr_hu_dom_sel == kiwi.REV) connect_dom_rev_focus(true);
   } else
   
	if (!(status >= 200 && status <= 299)) {     // error
	   error = true;
	   console.log('$error');
      ext_set_cfg_param('cfg.server_url', '', EXT_SAVE);
   }
   
	switch (status) {
		case   0: s = 'Existing account, registration successful'; break;       // FRPC_EXISTING
		case   1: s = 'New account, registration successful'; break;            // FRPC_NEW
		case   2: s = 'Updating host name, registration successful'; break;     // FRPC_UPDATE_HOST

		case 100: s = 'User key or host name field blank'; break;
		case 101: s = 'User key invalid. Did you email your user/API key to support@kiwisdr.com as per the instructions?'; break;
		case 102: s = 'Host name already in use; please choose another and retry'; break;
		case 103: s = 'Invalid characters in user key or host name field (use a-z, 0-9, -, _)'; break;

		case 150: s = 'No auto account user key. Please contact support@kiwisdr.com'; break;
		case 151: s = 'No auto account host name. Please contact support@kiwisdr.com'; break;
		case 152: s = 'Auto account duplicate. Please contact support@kiwisdr.com'; break;

		case 200: s = 'Reverse proxy enabled and running'; break;
		case 201: s = 'Reverse proxy enabled and pending'; break;

		case 900: s = 'Problem contacting proxy server; please check Internet connection'; break;
		case 901: s = 'Proxy server returned invalid status data?'; break;
		default:  s = 'Reverse proxy internal error: '+ status; break;
	}
	
	w3_innerHTML('id-connect-rev-status', s);
	connect_update_url();
	
	// if pending keep checking
	if (status == 201) {
	   connect.timeout = setTimeout(
	      function() {
	         ext_send('SET rev_status_query');
	         //console.log('setTimeout 5000: SET rev_status_query');
	      }, 5000
	   );
	}
	
	// If this admin connection is on a proxy connection then it needs to be reconnected
	// because frpc will be restarted using the new user and/or host value.
   var cur_hp = kiwi_remove_protocol(kiwi_host_port()).split('.');
	var admin_is_proxy_conn = kiwi_host().includes('proxy.kiwisdr.com');    // FIXME: alt proxy servers?
	var focus_query = connect.focus_query;
   
   // these are only valid if admin_is_proxy_conn since kiwi_host_port() could be a local IP otherwise
   var host_changed = (cur_hp[0] != host);
	var reload_auto = ( auto && host_changed &&  status == 0);
	var reload_man  = (!auto && host_changed && (status >= 0 && status <= 2));

   console.log('connect_rev_status_cb: auto='+ auto +' user='+ user +' host='+ host +'|'+ cur_hp[0] +
      ' reload_auto|man='+ reload_auto +'|'+ reload_man +' focus_query='+ focus_query +' error='+ error +' admin_is_proxy_conn='+ admin_is_proxy_conn);

   if (admin_is_proxy_conn) {
      if (reload_auto || reload_man) {
         // update host name while proxy connected

         console_nv('$connect_rev_status_cb RELOAD ADMIN CONN', {reload_auto}, {reload_man});
         cur_hp[0] = host;
         kiwi.reload_url = kiwi_SSL() + cur_hp.join('.') +'/admin';
         s = sprintf('SET rev_register reg=%d user=%s host=%s auto=%d', admin.FRPC_UPDATE_HOST, user, host, auto);
         console.log(s);
         
         // wait long enough for in-flight save_config() to finish before doing the rev_register
         // which will close the admin connection immediately by having stopped frpc
         //ext_send(s);
         //ext_send_after_cfg_save(s);      // doesn't work in this case
         setTimeout(function() { ext_send(s); }, 3000);

         wait_then_reload_page(10, 'You changed the Kiwi\'s host name. <br>' +
            'Will reconnect to new name at <x1>'+ kiwi.reload_url +'</x1>');
      }
	} else {
	   // !admin_is_proxy_conn
	   
      // setup frpc.ini and restart frpc if new account or host name updated
      var user_or_host_changed = (user != admin.last_rev_user || host != admin.last_rev_host);
      if (status == admin.FRPC_EXISTING && user_or_host_changed) {
         console.log('status => FRPC_UPDATE_HOST(2) because user_or_host_changed');
	      console.log('user|host='+ user +'|'+ host +' last_rev_user|host='+ admin.last_rev_user +'|'+ admin.last_rev_host);
         status = admin.FRPC_UPDATE_HOST;
      }
      var proxy_setup = (status >= admin.FRPC_EXISTING && status <= admin.FRPC_UPDATE_HOST);
      var isProxy = (cfg.sdr_hu_dom_sel == kiwi.REV);
      console.log('!admin_is_proxy_conn: status='+ status +' rev='+ TF(isProxy) +' focus_query='+ TF(focus_query) +' proxy_setup='+ TF(proxy_setup));
      if (isProxy && !focus_query && proxy_setup) {
         s = sprintf('SET rev_register reg=%d user=%s host=%s auto=%d', status, user, host, auto);
         console.log(s);
         ext_send(s);
      } else {
         console.log('did NOT send a rev_register');
      }
   }
   
   connect.focus_query = false;
}

function connect_proxy_server_cb(path, val)
{
   val = val.trim();
   if (val == '') {
      val = 'proxy.kiwisdr.com';
      w3_set_value('id-proxy-server', val);
   }
	w3_string_set_cfg_cb(path, val);
	connect_update_url();
   connect_rev_register_cb();
   connect_rev_usage();
}


////////////////////////////////
// users
////////////////////////////////

function users_html()
{
   var s =
      w3_div('id-users w3-container w3-hide',
         w3_inline('w3-container/w3-margin-top',
            w3_text('w3-text-teal w3-bold', 'All users since Kiwi restart'),
            w3_button('w3-margin-left w3-aqua', 'Clear list', 'users_clear_cb')
         ),
         w3_div('w3-container w3-margin-top w3-margin-bottom w3-card-8 w3-round-xlarge w3-pale-blue',
            w3_table('id-users-table w3-margin-bottom w3-table-6-8 w3-striped-except-hidden')
         )
      );
   return s;
}

function users_focus()
{
	admin.users_interval = kiwi_setInterval(
	   function() {
	      users_get_list();
	   }, 30000
	);
}

function users_blur()
{
	kiwi_clearInterval(admin.users_interval);
}

function users_get_list()
{
   ext_send("SET get_user_list");
}

function users_clear_cb(path, idx)
{
   admin.users_new = [];
   ext_send("SET user_list_clear");
}

function users_expand_cb(path, idx)
{
   var i = +idx;
   admin.exp_vis[i] ^= 1;
   var vis = admin.exp_vis[i];
   console.log('users_expand_cb i='+ i +' vis='+ vis);
   w3_innerHTML('id-users-icon-'+ i,
      w3_icon('', 'fa-chevron-circle-'+ (vis? 'down' : 'right'), 20, vis? 'red' : 'blue'));
   w3_els('id-users-'+ i, function(el, i) { w3_hide2(el, !vis); } );
}

/*
function users_sort_ip4(a, b)
{
   a = a.split('.');
   if (a.length < 4) return 0;
   b = b.split('.');
   if (b.length < 4) return 0;
   
   for (var i = 0; i < 4; i++) {
      if (+a[i] < +b[i]) return -1;
      if (+a[i] > +b[i]) return +1;
   }
   return 0;
}
*/

function users_sort_cb(path, idx)
{
   admin.users_sort = +idx;
   console.log('users_sort_cb='+ admin.users_sort);
   admin.users_list.sort(users_sort);
   users_list(admin.users_list);
}

function users_sort(a, b)
{
   switch (admin.users_sort) {
      case 0: return kiwi_sort_numeric(a.s, b.s);

      case 1: return kiwi_sort_ignore_case(a.i, b.i);
      case 2: return kiwi_sort_ignore_case(b.i, a.i);

      //case 3: return users_sort_ip4(a.a[0].ip, b.a[0].ip);
      //case 4: return users_sort_ip4(b.a[0].ip, a.a[0].ip);
      case 3: return kiwi_sort_ignore_case(a.a[0].ip, b.a[0].ip);
      case 4: return kiwi_sort_ignore_case(b.a[0].ip, a.a[0].ip);

      case 5: return kiwi_sort_ignore_case(a.a[0].g, b.a[0].g);
      case 6: return kiwi_sort_ignore_case(b.a[0].g, a.a[0].g);

      case 7: return kiwi_sort_numeric(a.t, b.t);
      case 8: return kiwi_sort_numeric(b.t, a.t);

      default: return 0;
   }
}

function users_list(ar)
{
   var i, j;
   
   var detail = function(ar, i) {
      var a = ar[i];
      var o = kiwi_dhms(+ar[i].t);
      return { ip: a.ip, geo: a.g, dhms: o.dhms };
   };
   
   i = 1;
   var fmt = 'w3-margin-L-8 w3-padding-tiny';
   var ud = function(id) {
      var s = id;
      s += w3_button(fmt, w3_icon('', 'fa-caret-up', 20, 'blue'), 'users_sort_cb', i++);
      s += w3_button(fmt, w3_icon('', 'fa-caret-down', 20, 'blue'), 'users_sort_cb', i++);
      return s;
   };
   
   var s =
      w3_table_row('',
         w3_table_heads('',
            w3_button('w3-padding-tiny||title="sort in order of\nconnection time"',
               w3_icon('', 'fa-clock-o', 20, 'blue') + w3_icon('w3-margin-L-4', 'fa-caret-down', 20, 'blue'),
               'users_sort_cb', 0),
            ud('name/callsign'), ud('IP address'), ud('location'), ud('connect time'), 'notes'
         )
      );

   fmt = ' w3-padding-tiny||title="name&slash;callsign used\nmultiple IP addresses"';
   if (ar) ar.forEach(function(u,i) {
      var multi = (u.a.length > 1);
      if (isUndefined(admin.exp_vis[i])) admin.exp_vis[i] = 0;
      admin.exp_vis[i] = 0;   //jksx
      var vis = admin.exp_vis[i];
      var notes = '';
      //if (+u.c > 1) notes += ' ['+ u.c +' conns]';
      if (multi) notes += ' ['+ kiwi_dhms(+u.t).dhms +' total]';
      var d = detail(u.a, 0);
      s += w3_table_row('',
         w3_table_cells('', multi? w3_button('id-users-icon-'+ i + fmt, w3_icon('', 'fa-chevron-circle-right', 20, 'blue'), 'users_expand_cb', i) : '',
            dq(u.i), d.ip, d.geo, d.dhms, notes)
      );
      for (j = 1; j < u.a.length; j++) {
         d = detail(u.a, j);
         s += w3_table_row(sprintf('id-users-%d %s', i, vis? '' : 'w3-hide'),
            w3_table_cells('', '', '', d.ip, d.geo, d.dhms, '&nbsp;'));
      }
   });
   
   w3_innerHTML('id-users-table', s);
}

function users_list_cb(s)
{
   admin.users_seq++;
   //if (admin.users_seq > 1) return;
   var o;
   try {
      o = JSON.parse(s);
   } catch(ex) {
      console.log(ex);
      console.log(s);
      return;
   }
   //console.log(o);
   var isEnd = false;
   o.forEach(
      function(a,i) {
         if (isDefined(a.end)) {
            if (!isArg(admin.users_new)) admin.users_new = [];
            admin.users_list = admin.users_new;
            console.log('users_list: n='+ admin.users_list.length +' sort='+ admin.users_sort);
            if (admin.users_sort) admin.users_list.sort(users_sort);
            users_list(admin.users_list);
            isEnd = true;
         } else {
            //console.log(a);
            if (+a.s == 0) admin.users_new = [];
            admin.users_new.push(a);
         }
      }
   );
   if (!isEnd) users_get_list();
}


////////////////////////////////
// all in admin_sdr.js
// config
// webpage
// public
// dx
////////////////////////////////


////////////////////////////////
// update
//		auto reload page when build finished?
////////////////////////////////

function update_html()
{
	var s =
	w3_div('id-update w3-hide',
		w3_div('w3-margin-bottom',
         w3_half('w3-container', '',
		      w3_div('id-msg-update', '&nbsp;')
		      /*
            , w3_divs('w3-tspace-8',
               w3_switch_label('w3-label-inline w3-label-left/w3-text-teal/', 'Update only when major version number changes?', 'Yes', 'No', 'adm.update_major_only', adm.update_major_only, 'admin_radio_YN_cb'),
               w3_text('w3-text-black',
                  'The major version number will change (e.g. 1.xxx to 2.0) only when a <br>' +
                  'stable version is declared. Set this option to <x1>Yes</x1> to only update when <br>' +
                  'stable versions are released.')
            )
            */
         )
      ) +

		'<hr>' +
		w3_div('w3-margin-bottom',
         w3_half('w3-container', 'w3-text-teal',
            w3_switch_label('w3-label-inline w3-label-left', 'Automatically check for software updates?', 'Yes', 'No', 'adm.update_check', adm.update_check, 'admin_radio_YN_cb'),
            w3_switch_label('w3-label-inline w3-label-left', 'Automatically install software updates?', 'Yes', 'No', 'adm.update_install', adm.update_install, 'admin_radio_YN_cb')
         ),
         w3_half('w3-container w3-tspace-16', 'w3-text-teal',
            w3_div('',
               w3_select('/w3-label-inline/w3-width-auto', 'After a restart', '', 'adm.restart_update', adm.restart_update, restart_update_u, 'admin_select_cb')
            ),
            w3_div('',
               w3_select('/w3-label-inline/w3-width-auto', 'After an update', '', 'adm.update_restart', adm.update_restart, update_restart_u, 'admin_select_cb')
            )
         )
		) +

		w3_half('w3-container', 'w3-text-teal',
			w3_div('w3-valign',
				'<b>Check for software update </b> ' +
				w3_button('w3-aqua w3-margin', 'Check now', 'update_check_now_cb')
			),
			w3_div('w3-valign',
				'<b>Force software build </b> ' +
				w3_button('w3-aqua w3-margin', 'Build now', 'update_build_now_cb')
			)
		) +

		'<hr>' +
		w3_inline('w3-halign-space-between w3-margin-bottom w3-text-teal w3-restart/w3-container',
         w3_divs('w3-tspace-8',
            w3_switch_label('w3-label-left', 'Disable recent changes?',
               'Yes', 'No', 'disable_recent_changes', cfg.disable_recent_changes, 'admin_radio_YN_cb'),
            w3_text('w3-text-black', 'Currently:<br><ul><li>The Firefox audio hang workaround.</li></ul>')
         ),
         ''
      ) +

		'<hr>'
	);
	return s;
}

var restart_update_u = { 0: 'install updates', 1: 'delay updates until overnight' };
var update_restart_u = { 0: 'restart server', 1: 'reboot Beagle' };

function update_check_now_cb(id, idx)
{
	ext_send('SET force_check=1 force_build=0');
	w3_el('id-msg-update').innerHTML =
	   w3_icon('', 'fa-refresh fa-spin', 24) + ' &nbsp; Checking for software update..';
}

function update_build_now_cb(id, idx)
{
	ext_send('SET force_check=1 force_build=1');
	w3_el('id-msg-update').innerHTML = w3_icon('', 'fa-refresh fa-spin', 24) +
      ' &nbsp; Monitor build progress using console tab, "monitor build progress" button.';

	if (adm.update_restart == 0)
	   w3_show_block('id-build-restart');
	else
	   w3_show_block('id-build-reboot');
}


////////////////////////////////
// backup
////////////////////////////////

function backup_html()
{
   sd_backup_init();
   
	var s =
      w3_div('id-backup w3-hide',

         w3_div('id-sd-backup-container', 
            w3_inline_percent('w3-margin-right/w3-container w3-valign',
               w3_div('',
                  w3_div('w3-margin-bottom w3-text-teal w3-bold',
                     'Backup complete contents of KiwiSDR by writing Beagle filesystem onto a user provided SD card'),
                  w3_button('w3-aqua', 'Click to write backup SD card', 'sd_backup_click_cb', /* backup */ 0)
               ), 22,

               w3_div('w3-margin-L-64',
                  w3_div('w3-container w3-text w3-red',
                     'WARNING: after SD card is written immediately remove from Beagle.<br>Otherwise on next reboot Beagle will be re-flashed from SD card.'),

                  w3_inline('|margin-top:45px/',
                     w3_div('id-sd-progress-container w3-progress-container w3-round-large w3-css-lightGray w3-show-inline-block',
                        w3_div('id-sd-progress w3-progressbar w3-round-large w3-light-green w3-width-zero',
                           w3_div('id-sd-progress-text w3-container')
                        )
                     ),
            
                     w3_inline('/w3-margin-left',
                        w3_div('id-sd-backup-time'),
                        w3_div('id-sd-backup-icon'),
                        w3_div('id-sd-backup-msg')
                     )
                  )
               ), 56,
               
               w3_div('',
                  w3_div('w3-margin-bottom w3-text-teal w3-bold',
                     'Create a Debian 11 upgrade SD card which includes all Kiwi customizations in kiwi.config directory.'),
                  w3_button('w3-aqua', 'Click to write upgrade SD card', 'sd_backup_click_cb', /* upgrade */ 1)
               )
            ),
            '<hr>',

            w3_div('id-output-msg w3-container w3-text-output w3-scroll-down w3-small w3-margin-B-16')
         )
      );
	return s;
}

function backup_focus()
{
	w3_width_height('id-sd-progress-container', 200);
	w3_width_height('id-output-msg', null, 400);
	
   sd_backup_focus();
}

function backup_blur()
{
   sd_backup_blur();
}


////////////////////////////////
// network
////////////////////////////////

var network = {
   auto_nat_color:   null,
   nat_status_interval: null,
   show_updating: true,
   
   ip_blacklist_file_base: 'kiwisdr.com/ip_blacklist/ip_blacklist3.cjson',
   ip_blacklist_check_mtime: true,
   bl_timeout: null,
   
   // this ordering gives a remapping of the old 0/1 values: 100M(0) => auto, 10M(1) => same
   ethernet_speed_s: [ ['auto', 1], ['10 Mbps', 1], ['100 Mbps', 1] ],
   ESPEED_10M: 1,
   ESPEED_ENA: 1,
   ethernet_mtu_s: [ '1500 (default)', '1440', '1400' ],
   
   restart_delay_s: [ 'no delay', 30, 45, 60, 90, 120, 150, 180 ],

   bl_port_s: [ 'default (yes)', 'yes', 'no' ],
   
   __last__: null
};

function network_html()
{
   // check for interference between simultaneous cfg/admcfg saves
   //ext_set_cfg_param('cfg.snr_local_time', true, EXT_SAVE)
   //ext_set_cfg_param('adm.ip_blacklist_auto_download', false, EXT_SAVE)
   
   if (!isDefined(adm.ip_address)) {
      console.log('network_html: adm.ip_address is undefined? -- initializing');
      adm.ip_address = {};
      adm.ip_address.use_static = false;
      adm.ip_address.ip = '';
      adm.ip_address.netmask = '';
      adm.ip_address.gateway = '';
      adm.ip_address.dns1 = '';
      adm.ip_address.dns2 = '';
   }

   network.ip_blacklist_file = 'http://'+ network.ip_blacklist_file_base;
   network.ip_blacklist_file_SSL = kiwi_SSL() + network.ip_blacklist_file_base;
   network.ip_blacklist_file_SSL_mtime = kiwi_SSL() + network.ip_blacklist_file_base +'.mtime';

   var commit_use_static = ext_get_cfg_param('adm.ip_address.commit_use_static');
   console.log('commit_use_static='+ commit_use_static);
   
   // on reload use last committed value in case commit transaction never completed
   if (isNoArg(commit_use_static))
      commit_use_static = false;    // default to DHCP if there has never been a commit
   
   // if commit value differs from current setting the update must have failed -- fix it
   if (adm.ip_address.use_static != commit_use_static) {
      ext_set_cfg_param('adm.ip_address.use_static', commit_use_static, EXT_SAVE);
      w3_switch_set_value('adm.ip_address.use_static', w3_switch_val2idx(!commit_use_static));
   }
   
   // check once per admin page load
   if (network.ip_blacklist_check_mtime) {
      network.ip_blacklist_double_fault = false;
      //kiwi_ajax(network.ip_blacklist_file_SSL_mtime, 'network_blacklist_mtime_cb', 0, -2000);
      kiwi_ajax(network.ip_blacklist_file_SSL_mtime, 'network_blacklist_mtime_cb', 0, 10000);
      network.ip_blacklist_check_mtime = false;
   }

   var spd_s;
   if (kiwi.platform == kiwi.PLATFORM_BBAI_64) {
      network.ethernet_speed_s[network.ESPEED_10M][network.ESPEED_ENA] = 0;
      spd_s = '10 Mbps setting not available <br> on BBAI-64.';
   } else {
      spd_s = 'Select 10 Mbps to reduce <br> Ethernet spurs. Try changing <br> while looking at waterfall.';
   }

	var s1 =
		w3_div('id-net-auto-nat-msg w3-valign w3-hide') +

		w3_div('id-net-reboot w3-container',
			w3_inline('w3-halign-space-between w3-margin-bottom w3-text-teal/',
			   w3_divs('w3-valign w3-flex-col/w3-tspace-6',
					w3_input_get('w3-restart', 'Internal port', 'adm.port', 'admin_int_cb'),
					w3_input_get('w3-restart', 'External port', 'adm.port_ext', 'admin_int_cb'),
					w3_input_get('', 'Debian hostname', 'adm.hostname', 'network_hostname_cb', 'kiwisdr')
				),
				w3_divs('id-net-ssl-vis w3-hide/ w3-center w3-restart',
					w3_switch_label_get_param('id-net-ssl w3-center', 'Enable HTTPS/SSL on<br>network connections?',
					   'Yes', 'No', 'adm.use_ssl', true, false, 'network_use_ssl_cb')
				),
				w3_switch_label('w3-center', 'Auto add NAT rule<br>on firewall / router?', 'Yes', 'No', 'adm.auto_add_nat', adm.auto_add_nat, 'network_auto_nat_cb'),
            w3_switch_label_get_param('w3-center', 'IP address<br>(only static IPv4 for now)',
               'DHCP', 'Static', 'adm.ip_address.use_static', 0, false, 'network_use_static_cb'),
            w3_divs('w3-center/',
               w3_select_conditional('w3-width-auto', 'Ethernet interface speed', '', 'ethernet_speed', cfg.ethernet_speed, network.ethernet_speed_s, 'network_ethernet_speed'),
               w3_div('w3-text-black', spd_s)
            ),
            w3_divs('w3-center/',
               w3_select('w3-width-auto', 'Ethernet interface MTU', '', 'ethernet_mtu', cfg.ethernet_mtu, network.ethernet_mtu_s, 'network_ethernet_mtu'),
               w3_div('w3-text-black',
                  'Select 1440 when having <br> connection problems <br> using 4G networks.')
            )
			),
			
			w3_div('id-net-ssl-container w3-restart w3-hide',
            w3_inline('w3-halign-space-between w3-margin-bottom w3-text-teal/',
               w3_input_get('', 'Local port (HTTP)', 'adm.port_http_local', 'admin_int_cb')
            )
         ),
			
			w3_div('id-net-static w3-hide',
			   w3_div('',
               w3_third('w3-margin-B-8 w3-text-teal', 'w3-container',
                  w3_input_get('', 'IP address (n.n.n.n where n = 0..255)', 'adm.ip_address.ip', 'network_ip_address_cb', ''),
                  w3_input_get('', 'Netmask (n.n.n.n where n = 0..255)', 'adm.ip_address.netmask', 'network_netmask_cb', ''),
                  w3_input_get('', 'Gateway (n.n.n.n where n = 0..255)', 'adm.ip_address.gateway', 'network_gw_address_cb', '')
               ),
               w3_third('w3-margin-B-8 w3-text-teal', 'w3-container',
                  w3_div('id-network-check-ip w3-green'),
                  w3_div('id-network-check-nm w3-green'),
                  w3_div('id-network-check-gw w3-green')
               ),
               w3_third('w3-valign w3-margin-bottom w3-text-teal', 'w3-container',
                  w3_input_get('', 'DNS-1 (n.n.n.n where n = 0..255)', 'adm.ip_address.dns1', 'net_set_dns_cb', ''),
                  w3_input_get('', 'DNS-2 (n.n.n.n where n = 0..255)', 'adm.ip_address.dns2', 'net_set_dns_cb', ''),
                  w3_div('',
                     w3_label('', '<br>') +     // makes the w3-valign above work for button below
                     w3_button('w3-show-inline w3-aqua', 'Use well-known public DNS servers', 'net_public_dns_cb')
                  )
               )
            ),
            w3_text('w3-margin-left w3-text-black', 'If DNS fields are blank the DNS servers specified by your router\'s DHCP will be used.')
			)
		);
	
	var s2 =
		'<hr>' +
		w3_div('id-net-config w3-container') +

		'<hr>' +
      w3_div('w3-container w3-text-teal',
         w3_inline('w3-valign w3-halign-space-between/',
            w3_div('',
               w3_div('', 
                  w3_label('w3-show-inline w3-bold w3-text-teal', 'Check if your external router port is open:') +
                  w3_button('w3-show-inline w3-aqua|margin-left:10px', 'Check port open', 'net_port_open_cb')
               ),
               w3_text('w3-block w3-text-black',
                  'Does kiwisdr.com successfully connect to your Kiwi using these URLs?<br>' +
                  'If both respond "NO" then check the NAT port mapping on your router.<br>' +
                  'If first responds "NO" and second "YES" then domain name of the first<br>' +
                  'isn\'t resolving to the ip address of the second. Check DNS.'),
               w3_div('', 
                  w3_label('id-net-check-port-dom-q w3-show-inline-block w3-margin-LR-16 w3-text-teal') +
                  w3_div('id-net-check-port-dom-s w3-show-inline-block w3-text-black w3-background-pale-aqua')
               ),
               w3_div('', 
                  w3_label('id-net-check-port-ip-q w3-show-inline-block w3-margin-LR-16 w3-text-teal') +
                  w3_div('id-net-check-port-ip-s w3-show-inline-block w3-text-black w3-background-pale-aqua')
               )
            ),
         
            w3_div('w3-center w3-text-teal',
               w3_switch_label('w3-center', 'Register this Kiwi on my.kiwisdr.com<br>on each reboot?',
                  'Yes', 'No', 'adm.my_kiwi', adm.my_kiwi, 'network_my_kiwi_cb'),
               w3_text('w3-block w3-center w3-text-black',
                  'Registering on <a href="http://my.kiwisdr.com" target="_blank">my.kiwisdr.com</a> <br>' +
                  'allows the local ip address of Kiwis to be easily discovered. <br>' +
                  'Set to "no" if you don\'t want your Kiwi <br>' +
                  'sending information to kiwisdr.com. Defaults to "yes".'
               )
            ),
         
            w3_div('w3-center w3-text-teal',
               w3_select('w3-width-auto', 'Power on restart delay (secs)', '', 'adm.restart_delay', adm.restart_delay, network.restart_delay_s, 'admin_select_cb'),
               w3_text('w3-block w3-center w3-text-black',
                  'On a power on of the Kiwi, how long to <br>' +
                  'wait before starting the server. <br>' +
                  'Gives time for network equipment <br>' +
                  'such as a router to boot and stabilize. <br>' +
                  'Increase value if e.g. proxy service <br>' +
                  'doesn\'t work after a power cycle.'
               )
            )
         )
      );

   var s3 =
		'<hr>' +
      w3_div('w3-container w3-text-teal',
         w3_inline('w3-valign w3-halign-space-between/',
            w3_div('',
               w3_text('w3-bold w3-text-teal', 'IP address blacklist'),
               w3_text('w3-text-black w3-show-block',
                  'IP addresses/ranges listed here are<br>' +
                  'blocked from accessing your Kiwi. <br>' +
                  'Use CIDR notation for ranges, e.g.<br>' +
                  '"ip/24" is netmask "255.255.255.0".<br>' +
                  'More information at the ' +
                  w3_link('w3-link-darker-color', 'http://forum.kiwisdr.com/index.php?p=/discussion/2352/call-for-ip-address-blacklist-contributions/p1', 'Kiwi forum') +'.'
               )
            ),
            
            w3_divs('/w3-center w3-tspace-8',
                  w3_div('',
                     w3_div('w3-margin-T-16', '<b>Download IP address blacklist?</b>'),
                     w3_text('id-ip-blacklist-new w3-text-red w3-hide', 'New blacklist available'),
                     w3_inline('w3-valign w3-halign-space-evenly w3-margin-T-10/',
                        w3_button('id-ip-blacklist-download w3-aqua', 'Download', 'network_download_button_cb'),
                        w3_button('w3-aqua', 'Clear', 'network_download_clear_cb')
                     )
                  ),
               w3_text('w3-text-black w3-center',
                  'Downloads a standard blacklist definition from<br>' +
                  w3_link('w3-link-darker-color', network.ip_blacklist_file, 'kiwisdr.com') +
                  '. A local, writeable blacklist<br>can be entered below.'
               )
            ),
            
            w3_switch_label('w3-center w3-margin-B-24', 'Automatically download<br>IP blacklist?',
               'Yes', 'No', 'adm.ip_blacklist_auto_download', adm.ip_blacklist_auto_download, 'admin_radio_YN_cb'),
            
            w3_div('w3-center w3-text-teal',
               //w3_switch_label('w3-center w3-margin-B-24 w3-restart', 'Limit blacklist to Kiwi<br>local port number?',
               //   'Yes', 'No', 'adm.ip_blacklist_port_only', adm.ip_blacklist_port_only, 'admin_radio_YN_cb'),
               w3_select('w3-width-auto w3-restart', 'Limit blacklist to Kiwi<br>local port number?', '',
                  'adm.ip_blacklist_port', adm.ip_blacklist_port, network.bl_port_s, 'admin_select_cb'),
               w3_text('w3-block w3-center w3-text-black',
                  'Set "yes" for special cases e.g. <br>' +
                  'when a local service such as DDNS <br>' +
                  'must talk to a hosting provider <br>' +
                  'located in a blacklisted IP range.'
               )
            )
         ),
         
         
         w3_inline('w3-valign w3-margin-top/', 
            w3_text('w3-text-teal', 'Blacklist status:'),
            w3_div('id-ip-blacklist-status w3-margin-left w3-text-black w3-background-pale-aqua', '')
         ),
         
         w3_textarea_get_param('w3-margin-T-16//w3-light-grey|width:100%|readonly',
            'Downloaded blacklist (read-only)',
            'adm.ip_blacklist', 8, 100, '', ''
         ),
         
         w3_textarea_get_param('w3-margin-T-32//w3-input-any-change|width:100%',
            w3_div('w3-flex w3-valign',
               w3_text('w3-bold  w3-text-teal', 'Local blacklist (writeable)'),
               w3_button('w3-margin-left w3-aqua', 'Save', 'network_user_blacklist_save_cb')
            ) +
            w3_text('w3-text-black w3-margin-T-6',
               'Always add whitelist entries ("+" character before ip address) after corresponding ip range, ' +
               'e.g. 1.2.3.0/24 +1.2.3.22'
            ),
            'adm.ip_blacklist_local', 8, 100, 'network_user_blacklist_cb', ''
         )
      ) +
      '<hr>';

	return w3_div('id-network w3-hide', s1 + s2 + s3);
}

function network_hostname_cb(path, s, first)
{
   if (first) return;
   s = s.trim();
   //console.log('network_hostname_cb path='+ path +' s='+ s);
   if (s == '') s = 'kiwisdr';
   w3_set_value(path, s);
   w3_string_set_cfg_cb(path, s);
	ext_send('SET hostname='+ encodeURIComponent(s));
}

function network_my_kiwi_cb(path, idx, first)
{
	idx = +idx;
	var enabled = (idx == 0);
	//console.log('network_my_kiwi_cb: first='+ first +' enabled='+ enabled);
	if (!first) ext_send('SET my_kiwi='+ (enabled? 1:0));
	admin_bool_cb(path, enabled, first);
}

function network_ssl_container_init()
{
   var show = dbgUs && (debian_ver >= 10);
   w3_hide2('id-net-ssl-vis', !show);

   var use_ssl = show && adm.use_ssl;
   var s = '';
   if (use_ssl) s = ' (HTTPS)';
   w3_innerHTML('id-adm.port-label', 'Internal port'+ s);
   w3_innerHTML('id-adm.port_ext-label', 'External port'+ s);
   w3_hide2('id-net-ssl-container', !use_ssl);
}

function network_use_ssl_cb(path, idx, first)
{
   if (first) return;
	var use_ssl = (+idx == 0);
	console.log('network_use_ssl_cb use_ssl='+ use_ssl);
   admin_bool_cb(path, use_ssl);
   network_ssl_container_init();
}

function network_download_button_cb(id, idx, first)
{
   if (first) return;
   w3_unflag('id-ip-blacklist-status', 'updating..'+ w3_icon('w3-margin-left', 'fa-refresh fa-spin', 24));
   //kiwi_ajax(network.ip_blacklist_file_SSL, 'network_download_blacklist_cb', 0, -2000);
   kiwi_ajax(network.ip_blacklist_file_SSL, 'network_download_blacklist_cb', 0, 10000);
}

function network_download_clear_cb(id, idx, first)
{
   kiwi_clearTimeout(network.bl_timeout);
   ext_send('SET network_ip_blacklist_clear');
   network_ip_blacklist_set('adm.ip_blacklist', '');
   w3_int_set_cfg_cb('adm.ip_blacklist_mtime', 0);
}

function network_user_blacklist_save_cb(path)
{
   //console.log('network_user_blacklist_save_cb');
   var el = w3_el('id-adm.ip_blacklist_local');
   //console.log('val='+ el.value);
   network_ip_blacklist_set('adm.ip_blacklist_local', el.value);
   w3_schedule_highlight(el);
}

function network_user_blacklist_cb(path, val)
{
   //console.log('network_user_blacklist_cb val='+ val);
   network_ip_blacklist_set('adm.ip_blacklist_local', val);
}

function network_download_blacklist_cb(bl)
{
   var fault = false;
   
   if (!bl) {
      console.log('network_download_blacklist_cb: bl='+ bl);
      fault = true;
   } else
   
   if (bl.AJAX_error) {
      console.log('network_download_blacklist_cb: '+ bl.AJAX_error);
      console.log(bl);
      fault = true;
   } else
   if (!isArray(bl)) {
      console.log('network_download_blacklist_cb: not array');
      console.log(bl);
      fault = true;
   }
   
   if (fault) {
      if (network.ip_blacklist_double_fault) {
         console.log('network_download_blacklist_cb: default blacklist fetch FAILED');
         console.log(bl);
         w3_flag('id-ip-blacklist-status', 'download failed: could not contact kiwisdr.com or find default file');
         return;
      } else {
         w3_flag('id-ip-blacklist-status', 'download failed, using default..'+ w3_icon('w3-margin-left', 'fa-refresh fa-spin', 24));
         network.show_updating = false;
      }
      
      // load the default blacklist if unable to contact kiwisdr.com
      var url = kiwi_url_origin() +'/kiwi/ip_blacklist.default.cjson';
      console.log('network_download_blacklist_cb: using default station list '+ url);
      network.ip_blacklist_double_fault = true;
      kiwi_ajax(url, 'network_download_blacklist_cb', 0, /* timeout */ 10000);
      return;
   }
   
   network.ip_blacklist_double_fault = false;
   //console.log('network_download_blacklist_cb:');
   //console.log(bl);
   
   var ip_bl_s = network_blacklist_validate(bl);
   network_ip_blacklist_set('adm.ip_blacklist', ip_bl_s);
   network.show_updating = true;

   // silently fail if kiwisdr.com can't be contacted for the mtime check
   //kiwi_ajax(network.ip_blacklist_file_SSL_mtime, 'network_blacklist_mtime_cb', 1, -2000);
   kiwi_ajax(network.ip_blacklist_file_SSL_mtime, 'network_blacklist_mtime_cb', 1, 10000);
}

function network_blacklist_validate(bl)
{
   //console.log('network_blacklist_validate bl=...');
   //console.log(bl);
   var bl_debug = false;
   var n = [];
   
   bl.forEach(function(s, i) {
      if (!isString(s)) return;
      var a = s.split('/');

      var nmd;
      if (a.length == 1)
         nmd = 32;
      else
         nmd = +a[1];
      var nm = (~((1<<(32-nmd))-1)) & 0xffffffff;
      //console.log(a[0]);
      //console.log('nm '+ nmd +' '+ nm.toHex(8));
      
      // NB: if a[0] begins with '+' due to being a whitelist entry, kiwi_inet4_d2h() still works
      // because the first component of the ip address string is just treated as a positive number
      var ip1 = kiwi_inet4_d2h(a[0], { no_local_ip:1 });
      if (ip1 == null) return;
      var ip = ip1 & nm;
      if (ip1 != ip)
         console.log('ip/netmask mismatch: '+ kiwi_ip_str(ip1) +'|'+ ip1.toHex(8) +' '+
            kiwi_ip_str(ip) +'|'+ ip.toHex(8) +' '+ nm.toHex(8) +'/'+ nmd);

      n.push( { ip: ip, nm: nm, nmd: nmd, whitelist: (a[0][0] == '+')? 1:0, del: 0 } );
   });
   
   // sort largest netmask first
   n.sort(function(a, b) { return a.nm - b.nm; } );
   
   if (bl_debug) n.forEach(function(o, i) {
      console.log('sorted: #'+ i +' '+ (o.whitelist? '+':'') + kiwi_ip_str(o.ip) +'|'+ o.ip.toHex(8) +' '+ o.nm.toHex(8) +'/'+ o.nmd);
   });
   
   // remove duplicates
   var nn = kiwi_dup_array(n);
   if (bl_debug) console.log('blacklist: #'+ nn.length +' entries pre duplicate check');
   n.forEach(function(oi, i) {
      var stop = false;
      nn.forEach(function(oj, j) {
         if (stop || j <= i || oj.ip == 0) return;
         if ((oj.ip & oi.nm) == oi.ip && (!oj.whitelist && !oi.whitelist)) {
            console.log('blacklist: #'+ j +' '+ kiwi_ip_str(oj.ip) +'|'+ oj.ip.toHex(8) +' '+ oj.nm.toHex(8) +'/'+ oj.nmd +
               ' is a subset of #'+
               i +' '+ kiwi_ip_str(oi.ip) +'|'+ oi.ip.toHex(8) +' '+ oi.nm.toHex(8) +'/'+ oi.nmd);
            oj.del = 1;
            stop = true;
         }
      });
      if ((kiwi_ip_loopback & oi.nm) == oi.ip && !oi.whitelist) {
         console.log('blacklist: #'+ i +' '+ kiwi_ip_str(oi.ip) +'|'+ oi.ip.toHex(8) +' '+ oi.nm.toHex(8) +'/'+ oi.nmd +
            ' is a superset of the loopback address 127.0.0.1!');
         oi.del = 1;
      }
   });
   
   nn = kiwi_array_iter_dup(nn, function(el,i) { return el.del? undefined : el; });
   
   // remove matches on reserved addresses
   
   console.log('blacklist: #'+ nn.length +' entries post duplicate check');
   if (bl_debug) nn.forEach(function(o, i) {
      console.log('final: #'+ i +' '+ (o.whitelist? '+':'') +
         kiwi_ip_str(o.ip) +'|'+ o.ip.toHex(8) +' '+ o.nm.toHex(8) +'/'+ o.nmd + (o.del? ' DELETE':''));
   });

   var ip_bl_s = '';
   nn.forEach(function(o, i) {
      ip_bl_s = w3_sb(ip_bl_s, (o.whitelist? '+':'') + kiwi_ip_str(o.ip) +'/'+ o.nmd);
   });
   
   return ip_bl_s;
}

function network_blacklist_mtime_cb(mt, update)
{
   var fault = false;
   
   if (isNoArg(mt)) {
      console.log('network_blacklist_mtime_cb: mt='+ mt);
      fault = true;
   } else
   
   if (isObject(mt) && mt.AJAX_error && mt.AJAX_error == 'timeout') {
      console.log('network_blacklist_mtime_cb: TIMEOUT');
      fault = true;
   }

   if (fault) {
      console.log(mt);
      return;
   }
   //console.log(mt);
   
   //if (dbgUs) console.log(mt);
   var mtime = parseInt(mt);
   if (dbgUs) console.log('network_blacklist_mtime_cb: '+ (update? 'UPDATE' : 'AVAIL') +
      ' mtime='+ mtime +' adm.ip_blacklist_mtime='+ adm.ip_blacklist_mtime);
   
   if (update) {
      // new blacklist downloaded -- update file mtime into our configuration
      w3_remove_then_add('id-ip-blacklist-download', 'w3-red', 'w3-aqua');
      w3_hide2('id-ip-blacklist-new', true);
      w3_int_set_cfg_cb('adm.ip_blacklist_mtime', mtime);
   } else {
      // update download button ui if new blacklist available
      if (adm.ip_blacklist_mtime < mtime) {
         w3_remove_then_add('id-ip-blacklist-download', 'w3-aqua', 'w3-red');
         w3_hide2('id-ip-blacklist-new', false);
      }
   }
}

function network_ip_blacklist_set(path, val)
{
   network.bl_path = path;
   network.bl_val = val;
   network.bad_local_ip = false;
   
   // "network_ip_blacklist_locked" returned which then calls network_ip_blacklist_set2() below
   ext_send('SET network_ip_blacklist_lock');
}

function network_ip_blacklist_set2(path, val)
{
   //console.log('network_ip_blacklist_set path='+ path +' val='+ val);
   
	var re = /([^,;\s]+)/gm;
	var ar = val.match(re);
	//console.log(ar);
	if (ar == null) ar = [];
	var ar2 = [];

   var s = '';
   ar.forEach(function(v, i) {
      if (kiwi_inet4_d2h(v, { no_local_ip:1, no_local_overlap:1 }) == null) {
         network.bad_local_ip = true;
         return;    // filter out bad and local ip addresses
      }
      s += v +' ';
      ar2.push(v);
   });
   w3_set_value(path, s);
   w3_string_set_cfg_cb(path, s);

   if (path.endsWith('ip_blacklist_local')) {
	   network.ip_blacklist_local = network_blacklist_validate(ar2).split(' ');
	   
	   // make sure network.ip_blacklist is valid
	   ar = decodeURIComponent(adm.ip_blacklist).match(re);
	   if (ar == null) ar = [];
	   network.ip_blacklist = ar;
	} else {
	   network.ip_blacklist = ar2;

	   // make sure network.ip_blacklist_local is valid
	   ar = decodeURIComponent(adm.ip_blacklist_local).match(re);
	   if (ar == null) ar = [];
	   network.ip_blacklist_local = ar;
	}

   ext_send('SET network_ip_blacklist_start');
   if (network.show_updating) {
      w3_unflag('id-ip-blacklist-status', 'updating..'+ w3_icon('w3-margin-left', 'fa-refresh fa-spin', 24));
   }
   
   network.seq = 0;
	network.ip_address_error = false;
	console.log('ip_blacklist:');
	console.log(network.ip_blacklist);
	console.log('ip_blacklist_local:');
	console.log(network.ip_blacklist_local);
	
   ext_send('SET network_ip_blacklist_disable');
   network_ip_blacklist_send( {idx:0, global:1} );
}

// Send rate limited requests to invoke iptables on server side.
// Rate limited so as not to disturb realtime.
function network_ip_blacklist_send(p)
{
   var rate = admin.status.ip_set? 20 : 250;      // rate limit due to iptables overhead
   if (p.global) {
      if (p.idx == network.ip_blacklist.length) {
         network_ip_blacklist_send( {idx:0, global:0} );
      } else {
         ext_send('SET network_ip_blacklist='+ encodeURIComponent(network.ip_blacklist[p.idx]));
         network.bl_timeout = setTimeout(function() { network_ip_blacklist_send( {idx:p.idx+1, global:1} ); }, rate);
      }
   } else {
      if (p.idx == network.ip_blacklist_local.length) {
         ext_send('SET network_ip_blacklist_enable');
         network.bl_timeout = null;
      } else {
         ext_send('SET network_ip_blacklist='+ encodeURIComponent(network.ip_blacklist_local[p.idx]));
         network.bl_timeout = setTimeout(function() { network_ip_blacklist_send( {idx:p.idx+1, global:0} ); }, rate);
      }
   }
}

function network_ip_blacklist_status(status, ip)
{
	console.log('network_ip_blacklist_status #'+ network.seq +' status='+ status +' ip='+ ip);
	network.seq++;
	if (status == 0) return;
	network.ip_address_error = true;
   w3_flag('id-ip-blacklist-status', 'ip address error: '+ dq(ip));
}

function network_ethernet_speed(path, idx, first)
{
   idx = +idx;
	//console.log('network_ethernet_speed path='+ path +' idx='+ idx +' first='+ first);
   if (first) return;
   admin_select_cb(path, idx, first);
}

function network_ethernet_mtu(path, idx, first)
{
   idx = +idx;
	//console.log('network_ethernet_mtu path='+ path +' idx='+ idx +' first='+ first);
   if (first) return;
   admin_select_cb(path, idx, first);
}

function network_port_open_init()
{
   // proxy always uses a fixed port number
   w3_do_when_rendered('id-net-check-port-dom-q',
      function() {
         var el = w3_el('id-net-check-port-dom-q');
         var port = (cfg.sdr_hu_dom_sel == kiwi.REV)? admin.proxy_port : adm.port_ext;
         el.innerHTML =
            (cfg.server_url != '')?
               'http://'+ cfg.server_url +':'+ port
                  :
               '(incomplete: on "connect" tab please use a valid setting in menu)';


         w3_el('id-net-check-port-dom-s').innerHTML = '';
         w3_el('id-net-check-port-ip-s').innerHTML = '';
      }
   );
   // REMINDER: w3_do_when_rendered() returns immediately
   
   w3_do_when_cond(
      function() {
         if (isEmptyString(config_net.pub_ip)) {
            msg_send('SET GET_CONFIG');
            return false;
         }
         return true;
      },
      function() {
         w3_innerHTML('id-net-check-port-ip-q',
            'http://'+ config_net.pub_ip +':'+ adm.port_ext);
      }
   );
   // REMINDER: w3_do_when_cond() returns immediately
}

function network_focus()
{
   network_static_init();
	network_port_open_init();
	network_ssl_container_init();
	admin_update_start();
	
	// in case a javascript error during development never cleared the lock
   ext_send('SET network_ip_blacklist_clear');
}

function network_blur()
{
	kiwi_clearInterval(network.nat_status_interval);
	admin_update_stop();
}

function network_auto_nat_status_poll()
{
	ext_send('SET auto_nat_status_poll');
}

function network_auto_nat_cb(path, idx, first)
{
   if (first) return;
   idx = +idx;
	var auto_nat = (idx == w3_SWITCH_YES_IDX)? 1:0;
	//console.log('network_auto_nat_cb: path='+ path +' auto_nat='+ auto_nat);
   admin_radio_YN_cb(path, idx);
   ext_send_after_cfg_save('SET auto_nat_set');    // server inspects adm.auto_add_nat to add or delete NAT
   if (auto_nat && network.nat_status_interval == null) {
      //console.log('auto_nat_status_poll START');
	   network.nat_status_interval = kiwi_setInterval(network_auto_nat_status_poll, 2000);
   } else
   if (!auto_nat && network.nat_status_interval != null) {
      //console.log('auto_nat_status_poll STOP');
	   kiwi_clearInterval(network.nat_status_interval);
	   network.nat_status_interval = null;
   }
}

function network_check_port_status_cb(status)
{
   console.log('network_check_port_status_cb status='+ status.toHex());
   if (status < 0) {
      w3_el('id-net-check-port-dom-s').innerHTML = 'Error checking port status';
      w3_el('id-net-check-port-ip-s').innerHTML = 'Error checking port status';
   } else {
      var dom_status = status & 0xf0;
      var ip_status = status & 0x0f;
      w3_el('id-net-check-port-dom-s').innerHTML = dom_status? 'NO' : 'YES';
      w3_el('id-net-check-port-ip-s').innerHTML = ip_status? 'NO' : 'YES';
   }
}

function net_port_open_cb()
{
   w3_el('id-net-check-port-dom-s').innerHTML = w3_icon('', 'fa-refresh fa-spin', 20);
   w3_el('id-net-check-port-ip-s').innerHTML = w3_icon('', 'fa-refresh fa-spin', 20);
	ext_send('SET check_port_open');
}

function network_dhcp_static_update_cb(path, idx)
{
   var use_static = adm.ip_address.use_static;
	if (use_static) {
	   // NB: for D11+ must always use these two together and in this order because of how server-side works
      ext_send('SET dns dns1=x'+ encodeURIComponent(adm.ip_address.dns1) +' dns2=x'+ encodeURIComponent(adm.ip_address.dns2));
      ext_send('SET static_ip='+ kiwi_ip_str(network_ip) +' static_nb='+ network_nm.nm +' static_nm='+ kiwi_ip_str(network_nm) +' static_gw='+ kiwi_ip_str(network_gw));
	} else {
		ext_send('SET use_DHCP');
	}

   ext_set_cfg_param('adm.ip_address.commit_use_static', use_static, EXT_SAVE);
   admin_confirm_cancel_cb();
   
   if (debian_ver <= 9)
      w3_reboot_cb();      // show reboot button after confirm button pressed
   else
      // Debian 10 and above use connmanctl/networkctl which has immediate effect (no reboot required)
		wait_then_reload_page(10, 'Waiting for configuration change');
}

function network_dhcp_static_confirm_cb()
{
   network_dhcp_static_update_cb();
}

function network_dhcp_static_cancel_cb()
{
   var use_static_restore = adm.ip_address.use_static? false:true;
   ext_set_cfg_param('adm.ip_address.use_static', use_static_restore, EXT_SAVE);
   w3_switch_set_value('adm.ip_address.use_static', w3_switch_val2idx(!use_static_restore));
}

function network_static_init()
{
   w3_do_when_rendered('id-net-static',
      function() {
		   var use_static = ext_get_cfg_param('adm.ip_address.use_static', false);
		   network_use_static_cb('adm.ip_address.use_static', use_static, /* first */ true);
	   }
	);
   // REMINDER: w3_do_when_rendered() returns immediately
}

function network_use_static_cb(path, idx, first)
{
	idx = +idx;
	//console.log('network_use_static_cb idx='+ idx);
	var dhcp = (idx == 0);
	
	// only show IP fields if in static mode
	w3_hide2('id-net-static', dhcp);

	//console.log('network_use_static_cb: first='+ first +' dhcp='+ dhcp);

	// when mode is changed decide if update button needs to appear
	if (!first) {
		if (dhcp) {
			admin_confirm_show('Are you sure? Click to update interface DHCP/static IP configuration',
			   network_dhcp_static_confirm_cb, network_dhcp_static_cancel_cb);
		} else {
			network_show_update(false);	// show based on prior static info (if any)
		}
	} else {
		// first time, fill-in the fields with the configured values
		network_ip_address_cb('adm.ip_address.ip', adm.ip_address.ip, true);
		network_netmask_cb('adm.ip_address.netmask', adm.ip_address.netmask, true);
		network_gw_address_cb('adm.ip_address.gateway', adm.ip_address.gateway, true);
	}
	
	admin_bool_cb(path, dhcp? 0:1, first);
}

function network_ip_nm_check(val, ip)
{
	var rexp = '^([0-9]*)\.([0-9]*)\.([0-9]*)\.([0-9]*)$';
	var p = new RegExp(rexp).exec(val);
	var a, b, c, d;
	
	if (p != null) {
		//console.log('regexp p='+ p);
		a = parseInt(p[1]);
		a = (a > 255)? NaN : a;
		b = parseInt(p[2]);
		b = (b > 255)? NaN : b;
		c = parseInt(p[3]);
		c = (c > 255)? NaN : c;
		d = parseInt(p[4]);
		d = (d > 255)? NaN : d;
	}
	
	if (p == null || isNaN(a) || isNaN(b) || isNaN(c) || isNaN(d)) {
		ip.ok = false;
	} else {
		ip.ok = true; ip.a = a; ip.b = b; ip.c = c; ip.d = d;
	}
	
	return ip.ok;
}

network_ip = { ok:false, a:null, b:null, c:null, d:null };
network_nm = { ok:false, a:null, b:null, c:null, d:null };
network_gw = { ok:false, a:null, b:null, c:null, d:null };

function network_show_update(first)
{
	//console.log('network_show_update: first='+ first);

	if (!first && network_ip.ok && network_nm.ok && network_gw.ok) {
		//console.log('network_show_update: SHOW');
      admin_confirm_show('Are you sure? Click to update interface DHCP/static IP configuration',
         network_dhcp_static_confirm_cb, network_dhcp_static_cancel_cb);
	} else {
		admin_confirm_cancel_cb();
	}
}

function network_show_check(id, name, path, val_str, ip, first, check_func)
{
	if (val_str != '') {
		var el = w3_el(id);
		var check = network_ip_nm_check(val_str, ip), check2 = true;
		
		if (check == true && check_func != undefined) {
			check2 = check_func(val_str, ip);
		}
	
		if (check == false || check2 == false) {
			el.innerHTML = 'bad '+ name +' entered';
			w3_remove(el, 'w3-green');
			w3_add(el, 'w3-red');
		} else {
			el.innerHTML = name +' okay, check: '+ ip.a +'.'+ ip.b +'.'+ ip.c +'.'+ ip.d;
			w3_remove(el, 'w3-red');
			w3_add(el, 'w3-green');
			w3_string_set_cfg_cb(path, val_str, first);
		}

		network_show_update(first);		// when a field is made good decide if update button needs to be shown
	}
}

function network_ip_address_cb(path, val, first)
{
	network_show_check('network-check-ip', 'IP address', path, val, network_ip, first);
}

function network_netmask_cb(path, val, first)
{
	network_nm.nm = -1;
	network_show_check('network-check-nm', 'netmask', path, val, network_nm, first,
		function(val_str, ip) {
			var ip_host = kiwi_inet4_d2h(val_str);
			if (ip_host == null) { ip.ok = false; return false; }
			ip.nm = 0;		// degenerate case: no one-bits in netmask at all
			for (var i = 0; i < 32; i++) {
				if (ip_host & (1<<i)) {		// first one-bit hit
					ip.nm = 32-i;
					for (; i < 32; i++) {
						if ((ip_host & (1<<i)) == 0) {
							ip.nm = -1;		// rest of bits weren't ones like they're supposed to be
							ip.ok = false;
							return false;
						}
					}
				}
			}
			ip.ok = true;
			return true;
		});

	if (network_nm.nm != -1)
		w3_el('network-check-nm').innerHTML += ' (/'+ network_nm.nm +')';
}

function network_gw_address_cb(path, val, first)
{
	network_show_check('network-check-gw', 'gateway', path, val, network_gw, first);
}

function net_set_dns_cb(path, s)
{
   //console.log('net_set_dns_cb path='+ path +' s='+ s);
   w3_string_set_cfg_cb(path, s);
	network_show_update(false);
}

function net_public_dns_cb(id, idx)
{
	w3_string_set_cfg_cb('adm.ip_address.dns1', '1.1.1.1');
	w3_set_value('adm.ip_address.dns1', '1.1.1.1');
	w3_string_set_cfg_cb('adm.ip_address.dns2', '8.8.8.8');
	w3_set_value('adm.ip_address.dns2', '8.8.8.8');
	network_show_update(false);
}


////////////////////////////////
// GPS
//		tracking tasks aren't stopped when !enabled
////////////////////////////////

var pin = {
    green: w3_div('cl-leaflet-marker cl-legend-marker|background-color:lime'),
      red: w3_div('cl-leaflet-marker cl-legend-marker|background-color:red'),
   yellow: w3_div('cl-leaflet-marker cl-legend-marker|background-color:yellow')
};

var _gps = {
   focus: false,
   leaflet: true,
   gps_map_loaded: false,
   pkgs_maps_js: [ 'pkgs_maps/pkgs_maps.js', 'pkgs_maps/pkgs_maps.css' ],
   gmap_js: ['http://maps.googleapis.com/maps/api/js?key='],

   RSSI:0, AZEL:1, POS:2, MAP:3, IQ:4,
   IQ_data: null,
   iq_ch: 0,
   map_init: 0,
   map_needs_height: 0,
   map_locate: 0,
   map_mkr: [],
   legend_sep: w3_inline('', pin.green, 'Navstar/QZSS only', pin.yellow, 'Galileo only', pin.red, 'all sats'),
   legend_all: w3_inline('', pin.green, 'all sats (Navstar/QZSS/Galileo)')
};

//var E1B_offset_i = [ '-1', '-3/4', '-1/2', '-1/4', '0', '+1/4', '+1/2', '+3/4', '+1' ];

function gps_html()
{
	var s =
	w3_div('id-gps w3-hide|line-height:1.5',
	   w3_inline('w3-valign w3-halign-space-between/',
         w3_div('w3-valign w3-text-teal',
            w3_text('w3-text-teal w3-bold w3-small', 'Acquire'),
            w3_div('w3-flex-col w3-valign-start w3-margin-L-4',
               w3_checkbox('w3-label-inline w3-label-not-bold w3-small/w3-small', 'Navstar', 'adm.acq_Navstar', adm.acq_Navstar, 'gps_acq_cb'),
               w3_inline('',
                  w3_checkbox('w3-label-inline w3-label-not-bold w3-small/w3-small', 'QZSS', 'adm.acq_QZSS', adm.acq_QZSS, 'gps_acq_cb'),
                  w3_checkbox('w3-label-inline w3-label-not-bold w3-small w3-margin-left/w3-small/', 'Priority', 'adm.QZSS_prio', adm.QZSS_prio, 'gps_acq_cb')
               ),
               w3_checkbox('w3-label-inline w3-label-not-bold w3-small/w3-small', 'Galileo', 'adm.acq_Galileo', adm.acq_Galileo, 'gps_acq_cb')
            )
         ),

         w3_div('w3-valign w3-text-teal',
            w3_checkbox('w3-label-inline w3-small/w3-small', 'Acquire<br>if Kiwi<br>busy? [n]', 'adm.always_acq_gps', adm.always_acq_gps, 'w3_bool_set_cfg_cb')
         ),

         w3_div('w3-valign w3-text-teal',
            w3_checkbox('w3-label-inline w3-small/w3-small', 'Set date<br>from GPS?', 'adm.gps_set_date', adm.gps_set_date, 'w3_bool_set_cfg_cb')
         ),

         w3_div('w3-valign w3-text-teal',
            w3_checkbox('w3-label-inline w3-small/w3-small', 'Include<br>alerted sats in<br>solutions? [n]', 'adm.include_alert_gps', adm.include_alert_gps, 'w3_bool_set_cfg_cb')
         ),

         w3_div('w3-valign w3-text-teal',
            w3_checkbox('w3-label-inline w3-small/w3-small', 'Include<br>Galileo in<br>solutions? [y]', 'adm.include_E1B', adm.include_E1B, 'w3_bool_set_cfg_cb')
         ),

         w3_div('w3-valign w3-text-teal',
            w3_checkbox('w3-label-inline w3-small/w3-small', 'Use<br>Kalman<br>filter? [y]', 'adm.use_kalman_position_solver', adm.use_kalman_position_solver, 'w3_bool_set_cfg_cb')
         ),

         w3_div('w3-valign w3-hcenter w3-text-teal',
            w3_div('w3-margin-right', '<b>Select<br>Graph</b>') +
            w3_radio_button('w3-margin-R-4', 'RSSI', 'adm.rssi_azel_iq', adm.rssi_azel_iq == _gps.RSSI, 'gps_graph_cb'),
            w3_radio_button('w3-margin-R-4', 'Az/El', 'adm.rssi_azel_iq', adm.rssi_azel_iq == _gps.AZEL, 'gps_graph_cb'),
            w3_radio_button('w3-margin-R-4', 'Pos', 'adm.rssi_azel_iq', adm.rssi_azel_iq == _gps.POS, 'gps_graph_cb'),
            w3_radio_button('w3-margin-R-4', 'Map', 'adm.rssi_azel_iq', adm.rssi_azel_iq == _gps.MAP, 'gps_graph_cb'),
            w3_radio_button('', 'IQ', 'adm.rssi_azel_iq', adm.rssi_azel_iq == _gps.IQ, 'gps_graph_cb')
         ),

         w3_divs('w3-hcenter w3-text-teal/w3-center',
            w3_div('id-gps-pos-scale w3-center w3-hide w3-small',
               '<b>Scale</b> ',
               w3_select('w3-margin-L-5 w3-text-red', '', '', '_gps.pos_scale', 10-1, '1:20', 'gps_pos_scale_cb')
            ),
            w3_div('id-gps-iq-ch w3-center w3-hide w3-small',
               '<b>Chan</b> ',
               w3_select('w3-margin-L-5 w3-text-red', '', '', '_gps.iq_ch', 0, '1:12', 'gps_iq_ch_cb')
            )
         )
      ) +

	   w3_div('w3-valign',
         w3_div('id-gps-loading-maps w3-container w3-section w3-card-8 w3-round-xlarge w3-pale-blue|width:100%',
            'loading map...'
         ),
         w3_div('id-gps-channels w3-container w3-section w3-card-8 w3-round-xlarge w3-pale-blue|width:100%',
            w3_table('id-gps-ch w3-table-6-8 w3-striped')
         ),
         w3_div('id-gps-azel-container w3-hide',
            w3_div('w3-hcenter w3-relative',
               w3_img('id-gps-azel-graph|position:absolute; top:-2px', 'gfx/gpsEarth.png', 400, 400),
               '<canvas id="id-gps-azel-canvas" width="400" height="400" style="position:absolute"></canvas>'
            )
         ),
         w3_div('id-gps-map-container',
            w3_div('||id="id-gps-map"', ''),
            w3_div('id-gps-map-legend w3-small w3-margin-left', '')
         )
		) +

		w3_div('w3-container w3-section w3-card-8 w3-round-xlarge w3-pale-blue',
			w3_table('id-gps-info w3-table-6-8')
		)
	);
	return s;
}

function gps_acq_cb(path, val, first)
{
   if (first) return;
   console.log('gps_acq_cb path='+ path +' val='+ val);
   w3_bool_set_cfg_cb(path, val);
}

function gps_graph_cb(id, idx, first)
{
   idx = +idx;
   //console.log('gps_graph_cb idx='+ idx);
   admin_int_cb(id, idx, first);
   gps_iq_ch_cb('', _gps.iq_ch);

   w3_show_hide('id-gps-pos-scale', idx == _gps.POS);
   w3_show_hide('id-gps-iq-ch', idx == _gps.IQ);
   
   // id-gps-channels and id-gps-map-container are separated like this because the id-gps-ch inside id-gps-channels
   // is being updated all the time (@ 1 Hz) and we don't want the map to be effected by this.
   // But the id-gps-map-container needs to have its height set somehow. So we set the map_needs_height flag and
   // let the height be set the next time id-gps-ch is rendered.
   
   var el_ch = w3_el('id-gps-channels');
   if (idx == _gps.AZEL || idx == _gps.MAP) {
      el_ch.style.width = '65%';
      w3_el('id-gps-azel-container').style.width = '35%';
      w3_el('id-gps-map-container').style.width = '35%';
      _gps.map_needs_height = 1;
   } else {
      el_ch.style.width = '100%';
   }
   w3_show_hide('id-gps-azel-container', idx == _gps.AZEL);
   w3_show_hide('id-gps-map-container', idx == _gps.MAP);
   
   gps_update_admin_cb();     // redraw immediately to keep display from glitching
   if (idx == _gps.AZEL) ext_send("SET gps_az_el_history");
}

function gps_E1B_offset_cb(path, idx, first)
{
   idx = +idx;
	if (idx == -1)
	   idx = 4;
	//console.log('gps_E1B_offset_cb idx='+ idx +' path='+ path+' first='+ first);
}

function gps_pos_scale_cb(path, idx, first)
{
   idx = +idx;
	if (idx == -1 || first)
	   idx = 10-1;
	_gps.pos_scale = idx + 1;
	//console.log('gps_pos_scale_cb idx='+ idx +' path='+ path+' first='+ first +' pos_scale='+ _gps.pos_scale);
}

function gps_iq_ch_cb(path, idx, first)
{
   _gps.iq_ch = idx = +idx;
   
   // channel # is biased at 1 so zero indicates "off" (no sampling)
   // make sure IQ data isn't being unnecessarily requested unless GPS tab is in focus and IQ button selected
	var ch = (_gps.focus && adm.rssi_azel_iq == _gps.IQ)? (idx+1) : 0;
	console.log('gps_iq_ch_cb idx='+ idx +' path='+ path+' first='+ first +' ch='+ ch +' IQ-button='+ TF(adm.rssi_azel_iq == _gps.IQ) +' focus='+ TF(_gps.focus));
   ext_send('SET gps_IQ_data_ch='+ ch);
   _gps.IQ_data = null;    // blank display until new data arrives
}

function gps_gain_cb(path, idx, first)
{
   idx = +idx;
	if (idx == -1 || first)
	   idx = 0;
	_gps.gain = idx + 1;
	//console.log('gps_gain_cb idx='+ idx +' path='+ path+' first='+ first +' gain='+ _gps.gain);
   ext_send('SET gps_gain='+ _gps.gain);
}

var gps_interval, gps_azel_interval;
var gps_has_lat_lon, gps_az_el_history_running = false;

function gps_schedule_azel()
{
   //console.log('gps_schedule_azel running='+ gps_az_el_history_running);
   if (gps_az_el_history_running == false) {
      gps_az_el_history_running = true;
      ext_send("SET gps_az_el_history");
      gps_azel_interval = setInterval(function() {ext_send("SET gps_az_el_history");}, 60000);
   }
}

function gps_focus(id)
{
   _gps.focus = true;
   if (!_gps.gps_map_loaded) {
      kiwi_load_js(_gps.leaflet? _gps.pkgs_maps_js : _gps.gmap_js, 'gps_focus2');
      _gps.gps_map_loaded = true;
   } else {
      gps_focus2(id);
   }
}

function gps_focus2(id)
{
   w3_hide('id-gps-loading-maps');
   gps_schedule_azel();
   
	// only get updates while the gps tab is selected
	ext_send("SET gps_update");
	gps_interval = setInterval(function() {ext_send("SET gps_update");}, 1000);
	gps_graph_cb('adm.rssi_azel_iq', adm.rssi_azel_iq, true);
}

function gps_blur(id)
{
	kiwi_clearInterval(gps_interval);
	kiwi_clearInterval(gps_azel_interval);
   gps_az_el_history_running = false;
	ext_send("SET gps_IQ_data_ch=0");
   _gps.focus = false;
}

var gps_nsamp;
var gps_nsats;
var gps_now;
var gps_prn;
var gps_az = null;
var gps_el = null;
var gps_qzs3_az = 0;
var gps_qzs3_el = 0;
var gps_shadow_map = null;

function gps_az_el_history_cb(obj)
{
   gps_nsats = obj.n_sats;
   gps_nsamp = obj.n_samp;
   gps_now = obj.now;
   gps_prn = new Array(gps_nsats);
   gps_az = new Array(gps_nsamp*gps_nsats); gps_az.fill(0);
   gps_el = new Array(gps_nsamp*gps_nsats); gps_el.fill(0);
   
   var sat_i, sat;
   var n_sat = obj.sat_seen.length;
   //console.log('gps_nsamp='+ gps_nsamp +' n_sat='+ n_sat +' alen='+ gps_az.length);
   for (sat_i = 0; sat_i < n_sat; sat_i++) {
      sat = obj.sat_seen[sat_i];
      gps_prn[sat] = obj.prn_seen[sat_i];
   }

   for (var samp = 0; samp < gps_nsamp; samp++) {
      for (sat_i = 0; sat_i < n_sat; sat_i++) {
         var obj_i = samp*n_sat + sat_i;
         az = obj.az[obj_i];
         el = obj.el[obj_i];

         sat = obj.sat_seen[sat_i];
         var azel_i = samp*gps_nsats + sat;
         gps_az[azel_i] = az;
         gps_el[azel_i] = el;

         //console.log('samp='+ samp +' sat_i='+ sat_i +' obj_i='+ obj_i +' sat='+ sat +' prn='+ obj.prn_seen[sat_i] +' az='+ az +' el='+ el);
      }
   }

   gps_qzs3_az = obj.qzs3.az;
   gps_qzs3_el = obj.qzs3.el;
   gps_shadow_map = kiwi_dup_array(obj.shadow_map);
   //for (az=0; az<90; az++) gps_shadow_map[az] = (az < 45)? 0x0000ffff:0xffff0000;
   gps_update_azel();
}

var SUBFRAMES = 5;
var max_rssi = 1;

var refresh_icon = w3_icon('', 'fa-refresh', 20);

var sub_colors = [ 'w3-red', 'w3-green', 'w3-blue', 'w3-yellow', 'w3-orange' ];

var gps_canvas;
var gps_last_good_el = [];
var gps_rssi_azel_iq_s = [ 'RSSI', 'Az/el', 'Position solution map', 'Map', 'LO PLL IQ' ];

function gps_update_admin_cb()
{
   if (!gps) return;

	var i, el, x, y, z, zw, s, cn, az, axis, scale, len, color;
	
	s =
		w3_table_row('',
			w3_table_heads('w3-right-align', 'chan', 'acq', '&nbsp;PRN', 'SNR', 'eph age', 'hold', 'wdog'),
			w3_table_heads('w3-center', 'status', 'subframe'),
			w3_table_heads('w3-right-align', 'ov', 'az', 'el'),
			(adm.rssi_azel_iq == _gps.RSSI)? null : w3_table_heads('w3-right-align', 'RSSI'),
         (adm.rssi_azel_iq == _gps.AZEL || adm.rssi_azel_iq == _gps.MAP)? null : 
            w3_table_heads('w3-center|width:35%',
               ((adm.rssi_azel_iq == _gps.IQ && _gps.iq_ch)? ('Channel '+ _gps.iq_ch +' ') : '') + gps_rssi_azel_iq_s[adm.rssi_azel_iq]
            )
		);
	
      for (cn=0; cn < gps.ch.length; cn++) {
         s += w3_table_row('id-gps-ch-'+ cn, '');
      }

      s += w3_table_row('','&nbsp;');

	w3_el("id-gps-ch").innerHTML = s;
	
	var soln_color = (gps.stype == 0)? 'w3-green' : ((gps.stype == 1)? 'w3-yellow':'w3-red');

	for (cn=0; cn < gps.ch.length; cn++) {
		var ch = gps.ch[cn];

		if (ch.rssi > max_rssi)
			max_rssi = ch.rssi;
		
		var prn = 0;
		var prn_pre = '';
		if (ch.prn != -1) {
		   if (ch.prn_s != 'N') prn_pre = ch.prn_s;
		   prn = ch.prn;
      }
      //if (cn == 3) console.log('ch04 ch.prn='+ ch.prn +' ch.prn_s='+ ch.prn_s +' snr='+ ch.snr);
      
		//var ch_soln_color = (adm.plot_E1B && ch.prn_s == 'E')? 'w3-yellow' : soln_color;
		var ch_soln_color = soln_color;

      var unlock = ch.alert? 'A' : ((ch.ACF == 1)? '+' : ((ch.ACF == 2)? '-':'U'));
	
		var cells =
			w3_table_cells('w3-right-align', cn+1) +
			w3_table_cells('w3-center', (cn == gps.FFTch)? refresh_icon:'') +
			w3_table_cells('w3-right-align',
				prn? (prn_pre + prn):'',
				ch.snr? ch.snr:''
				//ch.rssi? ch.gain:'',
			) +
			w3_table_cells('w3-right-align'+ (ch.old? ' w3-text-red w3-bold':''), ch.age) +
			w3_table_cells('w3-right-align',
				ch.hold? ch.hold:'',
				ch.rssi? ch.wdog:''
			) +
			w3_table_cells('w3-center',
				'<span class="w3-tag '+ (ch.alert? ((ch.alert == 1)? 'w3-red':'w3-green') : (ch.unlock? 'w3-yellow':'w3-white')) +'">' +
				   unlock +'</span>' +
				'<span class="w3-tag '+ (ch.parity? 'w3-yellow':'w3-white') +'">P</span>' +
				'<span class="w3-tag '+ (ch.soln? ch_soln_color : 'w3-white') +'">S</span>'
			);
	
		var sub = '';
		var has_subframes = false;
		for (i = SUBFRAMES-1; i >= 0; i--) {
			var sub_color;
			if (ch.sub_renew & (1<<i)) {
				sub_color = 'w3-grey';
			} else {
			   var subframe = ch.sub & (1<<i);
				sub_color = subframe? sub_colors[i]:'w3-white';
				if (subframe) has_subframes = true;
			}
			sub += '<span class="w3-tag '+ sub_color +'">'+ (i+1) +'</span>';
		}
		cells +=
			w3_table_cells('w3-center', sub);
	
      cells +=
         w3_table_cells('w3-right-align',
            ch.novfl? ch.novfl:'',
            ch.el? ch.az:'',
            ch.el? ch.el:'',
            (adm.rssi_azel_iq == _gps.RSSI)? null : (ch.rssi? ch.rssi : '')
         );

	   if (adm.rssi_azel_iq == _gps.RSSI) {
         var pct = ((ch.rssi / max_rssi) * 100).toFixed(0);
         color = has_subframes? 'w3-light-green' : 'w3-red';
         cells +=
            w3_table_cells('',
               w3_div('w3-progress-container w3-round-xlarge w3-white',
                  w3_div('w3-progressbar w3-round-xlarge '+ color +'|width:'+ pct +'%',
                     w3_div('w3-container w3-text-white', ch.rssi)
                  )
               )
            );
      } else
	   if (adm.rssi_azel_iq != _gps.RSSI || adm.rssi_azel_iq != _gps.AZEL || adm.rssi_azel_iq != _gps.MAP) {
         if (cn == 0) {
            cells +=
               w3_table_cells('|vertical-align:top;position:relative;|rowspan='+ gps.ch.length,
                  w3_div('w3-hcenter',
                     '<canvas id="id-gps-canvas" width="400" height="400" style="position:absolute; z-index:2; pointer-events:none"></canvas>'
                  )
               );
         }
      }

		w3_el('id-gps-ch-'+ cn).innerHTML = cells;
	}

	s =
		w3_table_row('',
			w3_table_heads('', 'acq', 'track', 'good', 'fixes', 'f/min', 'run', 'TTFF', 'UTC offset',
				'ADC clock', 'lat', 'lon', 'alt', 'map')
		) +
		
		w3_table_row('',
			w3_table_cells('',
				gps.acq? 'yes':'paused',
				gps.track? gps.track:'',
				gps.good? gps.good:'',
				gps.fixes? gps.fixes.toUnits():'',
				gps.fixes_min,
				gps.run,
				gps.ttff? gps.ttff:'',
			//	gps.gpstime? gps.gpstime:'',
				gps.utc_offset? gps.utc_offset:'',
				gps.adc_clk.toFixed(6) +' '+
				w3_span('w3-round-xlarge w3-padding-LR-6 '+ (gps.is_corr? 'w3-green':'w3-red'), gps.adc_corr.toUnits()),
				gps.lat? gps.lat:'',
				gps.lat? gps.lon:'',
				gps.lat? gps.alt:'',
				gps.lat? gps.map:''
			)
		);
	w3_el("id-gps-info").innerHTML = s;

   gps_canvas = w3_el('id-gps-canvas');
   if (gps_canvas == null) return;
   gps_canvas.ctx = gps_canvas.getContext("2d");
   var ctx = gps_canvas.ctx;
   
   if (adm.rssi_azel_iq == _gps.RSSI) return;

   if (_gps.map_needs_height) {
      var h = css_style_num(w3_el('id-gps-channels'), 'height');
      if (h) {
         w3_el('id-gps-azel-container').style.height = px(h - 24);
         w3_el('id-gps-map').style.height = px(h - 24);
         _gps.a = enc(gps.a);
         _gps.map_needs_height = 0;
      }
   }
      

   ////////////////////////////////
   // MAP
   ////////////////////////////////

   var latlon;
   if (adm.rssi_azel_iq == _gps.MAP) {

      if (!_gps.map_init && !_gps.map_needs_height) {
         if (_gps.leaflet) {
            var map_tiles;
            maxZoom = 19;
            var server_e = { MapTiler_Vector:0, MapTiler_Raster_512:1, MapTiler_Raster_256:2, OSM_Raster:3 };
            var server = server_e.OSM_Raster;

            // MapTiler vector tiles using LeafletGL/MapBoxGL
            if (server == server_e.MapTiler_Vector) {
               map_tiles = function(map_style) {
                  return L.mapboxGL({
                     attribution: '<a href="https://www.maptiler.com/license/maps/" target="_blank">&copy; MapTiler</a> <a href="https://www.openstreetmap.org/copyright" target="_blank">&copy; OpenStreetMap contributors</a>',
                     accessToken: 'not-needed',
                     style: 'https://api.maptiler.com/maps/'+ map_style +'/style.json'+ _gps.a
                  });
               };
            }

            // MapTiler 512/256 px raster tiles
            if (server == server_e.MapTiler_Raster_512 || server == server_e.MapTiler_Raster_256) {
               var slash_256 = (server == server_e.MapTiler_Raster_256)? '/256':'';
               map_tiles = function(map_style) {
                  return L.tileLayer(
                     (map_style == 'hybrid')?
                        'https://api.maptiler.com/maps/'+ map_style + slash_256 +'/{z}/{x}/{y}{r}.jpg'+ _gps.a
                     :
                        'https://api.maptiler.com/maps/'+ map_style + slash_256 +'/{z}/{x}/{y}.png'+ _gps.a, {
                     tileSize: (server == server_e.MapTiler_Raster_256)? 256 : 512,
                     zoomOffset: (server == server_e.MapTiler_Raster_256)? 0 : -1,
                     attribution: '<a href="https://www.maptiler.com/copyright/" target="_blank">&copy; MapTiler</a> <a href="https://www.openstreetmap.org/copyright" target="_blank">&copy; OpenStreetMap contributors</a>',
                     crossOrigin: true
                  });
               };
            }

            // OSM raster tiles
            if (server == server_e.OSM_Raster) {
               map_tiles = function() {
                  maxZoom = 18;
                  return L.tileLayer(
                     'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                     tileSize: 256,
                     zoomOffset: 0,
                     attribution: '<a href="https://www.openstreetmap.org/copyright" target="_blank">&copy; OpenStreetMap contributors</a>',
                     crossOrigin: true
                  });
               };
            }

            var sat_map = map_tiles('hybrid');
            _gps.map = L.map('id-gps-map',
               {
                  maxZoom: maxZoom,
                  minZoom: 1,
               }
            ).setView([0, 0], 1);
            sat_map.addTo(_gps.map);

            // MapTiler map choices
            if (server != server_e.OSM_Raster) {
               L.control.layers(
                  {
                     'Satellite': sat_map,
                     'Basic': map_tiles('basic'),
                     'Bright': map_tiles('bright'),
                     'Positron': map_tiles('positron'),
                     'Street': map_tiles('streets'),
                     'Topo': map_tiles('topo')
                  },
                  null
               ).addTo(_gps.map);
            }

         } else {
            latlon = new google.maps.LatLng(0, 0);
            var map_div = w3_el('id-gps-map');
            _gps.map = new google.maps.Map(map_div,
               {
                  zoom: 1,
                  center: latlon,
                  navigationControl: false,
                  mapTypeControl: false,
                  streetViewControl: false,
                  mapTypeId: google.maps.MapTypeId.SATELLITE
               });
         }
         _gps.map_init = 1;
      }
      
      w3_innerHTML('id-gps-map-legend', adm.plot_E1B? _gps.legend_sep : _gps.legend_all);

      if (!_gps.MAP_data || !_gps.map_init) return;
         
      if (!_gps.map_locate) {
         if (_gps.leaflet) {
            _gps.map.setView([_gps.MAP_data.ref_lat, _gps.MAP_data.ref_lon], 15, { duration: 0, animate: false });
         } else {
            latlon = new google.maps.LatLng(_gps.MAP_data.ref_lat, _gps.MAP_data.ref_lon);
            _gps.map.panTo(latlon);
            _gps.map.setZoom(18);
         }
         _gps.map_locate = 1;
      }
      
      var mlen = _gps.MAP_data.MAP.length;
      //console.log('mlen='+ mlen);
      for (var j=0; j < mlen; j++) {
         var mp = _gps.MAP_data.MAP[j];
         //console.log(mp);
         color = (mp.nmap == 0)? (_gps.leaflet? 'lime':'green') : ((mp.nmap == 1)? 'red':'yellow');
         var mkr;
         if (_gps.leaflet) {
            var icon =
               L.divIcon({
                  className: "fooLM",
                  iconAnchor: [12, 12],
                  labelAnchor: [-6, 0],
                  popupAnchor: [0, -36],
                  html: '<span class="cl-leaflet-marker" style="background-color:'+ color +';"/>',
               });
            mkr = L.marker([mp.lat, mp.lon], { 'icon':icon, 'opacity':1.0 });
            mkr.addTo(_gps.map);
            _gps.map_mkr.push(mkr);
            while (_gps.map_mkr.length > 12) {
               _gps.map_mkr.shift().remove();
            }
         } else {
            latlon = new google.maps.LatLng(mp.lat, mp.lon);
            mkr = new google.maps.Marker({
               position:latlon,
               //label: mp.nmap? 'G':'N',
               icon: 'http://maps.google.com/mapfiles/ms/icons/'+ color +'-dot.png',
               map:_gps.map
            });
            _gps.map_mkr.push(mkr);
            while (_gps.map_mkr.length > 12) {
               _gps.map_mkr.shift().setMap(null);
            }
         }
      }
      _gps.MAP_data = null;

      return;
   }


   ////////////////////////////////
   // IQ
   ////////////////////////////////

   if (adm.rssi_azel_iq == _gps.IQ) {
      axis = 400;
      ctx.fillStyle = 'hsl(0, 0%, 90%)';
      ctx.fillRect(0,0, axis, axis);
      
      // crosshairs
      ctx.fillStyle = 'yellow';
      ctx.fillRect(axis/2,0, 1, axis);
      ctx.fillRect(0,axis/2, axis, 1);
      
      if (!_gps.IQ_data) return;
      ctx.fillStyle = 'black';
      var magnify = 8;
      scale = (axis/2) / 32768.0 * magnify;
      len = _gps.IQ_data.IQ.length;

      for (i=0; i < len; i += 2) {
         var I = _gps.IQ_data.IQ[i];
         var Q = _gps.IQ_data.IQ[i+1];
         if (I == 0 && Q == 0) continue;
         x = Math.round(I*scale + axis/2);
         y = Math.round(Q*scale + axis/2);
         if (x < 0 || x >= axis) {
            x = (x < 0)? 0 : axis-1;
         }
         if (y < 0 || y >= axis) {
            y = (y < 0)? 0 : axis-1;
         }
         ctx.fillRect(x,y, 2,2);
      }
      
      return;
   }

   
   ////////////////////////////////
   // POS
   ////////////////////////////////

   var clamp;
   if (adm.rssi_azel_iq == _gps.POS) {
      axis = 400;
      ctx.fillStyle = 'hsl(0, 0%, 90%)';
      ctx.fillRect(0,0, axis, axis);
      
      if (!_gps.POS_data) return;
      ctx.fillStyle = 'black';
      var fs = 0.001000 * _gps.pos_scale;
      scale = (axis/2) / fs;
      len = _gps.POS_data.POS.length;
      clamp = 0;

      //ctx.globalAlpha = 0.5;
      ctx.globalAlpha = 1;
      var x0min, x0max, y0min, y0max, x1min, x1max, y1min, y1max;
      x0min = y0min = x1min = y1min = Number.MAX_VALUE;
      x0max = y0max = x1max = y1max = Number.MIN_VALUE;
      for (i=0; i < len; i += 2) {
         if (!adm.plot_E1B && i >= len/2) break;
         ctx.fillStyle = (i < len/2)? "DeepSkyBlue":"black";
         var lat = _gps.POS_data.POS[i];
         if (lat == 0) continue;
         var lon = _gps.POS_data.POS[i+1];
         lat -= _gps.POS_data.ref_lat;
         lon -= _gps.POS_data.ref_lon;
         x = Math.round(lon*scale + axis/2);
         y = Math.round(-lat*scale + axis/2);
         if (x < 0 || x >= axis) {
            x = (x < 0)? 0 : axis-1;
            clamp++;
         }
         if (y < 0 || y >= axis) {
            y = (y < 0)? 0 : axis-1;
            clamp++;
         }
         var bs = 8;
         if (i < len/2) {
            ctx.fillRect(x-2,y-2, 5,5);
            if (x+bs > x0max) x0max = x+bs; else if (x-bs < x0min) x0min = x-bs;
            if (y+bs > y0max) y0max = y+bs; else if (y-bs < y0min) y0min = y-bs;
         } else {
            ctx.fillRect(x,y-2, 1,5);
            ctx.fillRect(x-2,y, 5,1);
            if (x+bs > x1max) x1max = x+bs; else if (x-bs < x1min) x1min = x-bs;
            if (y+bs > y1max) y1max = y+bs; else if (y-bs < y1min) y1min = y-bs;
         }
      }
      //ctx.globalAlpha = 1.0;
      
      // bboxes
      if (x0max >= axis) x0max = axis-1;
      if (x0min < 0) x0min = 0;
      if (y0max >= axis) y0max = axis-1;
      if (y0min < 0) y0min = 0;
      line_stroke(ctx, 0, 1, 'DeepSkyBlue', x0min,y0min, x0max,y0min);
      line_stroke(ctx, 0, 1, 'DeepSkyBlue', x0min,y0max, x0max,y0max);
      line_stroke(ctx, 1, 1, 'DeepSkyBlue', x0min,y0min, x0min,y0max);
      line_stroke(ctx, 1, 1, 'DeepSkyBlue', x0max,y0min, x0max,y0max);

      if (adm.plot_E1B) {
         if (x1max >= axis) x1max = axis-1;
         if (x1min < 0) x1min = 0;
         if (y1max >= axis) y1max = axis-1;
         if (y1min < 0) y1min = 0;
         line_stroke(ctx, 0, 1, 'black', x1min,y1min, x1max,y1min);
         line_stroke(ctx, 0, 1, 'black', x1min,y1max, x1max,y1max);
         line_stroke(ctx, 1, 1, 'black', x1min,y1min, x1min,y1max);
         line_stroke(ctx, 1, 1, 'black', x1max,y1min, x1max,y1max);
      }
      
      // text
      x = 16;
      var xi = 12;
      y = axis - 16*2;
      var yi = 18;
      var yf = 4;
      var fontsize = 15;
      ctx.font = fontsize +'px Courier';

      ctx.fillStyle = "DeepSkyBlue";
      ctx.fillRect(x-2,y-2-yf, 5,5);
      x += xi;
      ctx.fillStyle = 'black';
      ctx.fillText((adm.plot_E1B? ' w/o Galileo span: ':'All sats span: ')+
         _gps.POS_data.y0span.toFixed(0).fieldWidth(4) +'m Ylat '+ _gps.POS_data.x0span.toFixed(0).fieldWidth(4) +'m Xlon', x,y);

      if (adm.plot_E1B) {
         x -= xi;
         y += yi;
         ctx.fillStyle = 'black';
         ctx.fillRect(x,y-2-yf, 1,5);
         ctx.fillRect(x-2,y-yf, 5,1);
         x += xi;
         ctx.fillText('with Galileo span: '+ _gps.POS_data.y1span.toFixed(0).fieldWidth(4) +'m Ylat '+ _gps.POS_data.x1span.toFixed(0).fieldWidth(4) +'m Xlon', x,y);
      }

      //if (clamp) console.log('gps POS clamp='+ clamp);
      return;
   }
}

function gps_update_azel()
{
   ////////////////////////////////
   // AZEL
   ////////////////////////////////

   //console.log('gps_update_azel');

   if (adm.rssi_azel_iq != _gps.AZEL || gps_el == null) return;

   var gps_azel_canvas = w3_el('id-gps-azel-canvas');
   if (gps_azel_canvas == null) return;
   gps_azel_canvas.ctx = gps_azel_canvas.getContext("2d");
   var ctx = gps_azel_canvas.ctx;
   var az_rad, r;

   var gW = 400;
   var gD = 360;
   var gHD = gD/2;
   var gM = (gW-gD)/2;
   var gO = gHD + gM;
   ctx.clearRect(0, 0, gW, gW);
   
   if (adm.rssi_azel_iq == _gps.AZEL && gps_shadow_map) {
      ctx.fillStyle = "cyan";
      ctx.globalAlpha = 0.1;
      z = 4;
      zw = z*2 + 1;
      
      for (az = 0; az < 360; az++) {
         az_rad = az * Math.PI / gHD;
         var elm = gps_shadow_map[az];
         for (var n = 0, b = 1; n < 32; n++, b <<= 1) {
            if (elm & b) {
               el = n/31 * 90;
               r = (90 - el)/90 * gHD;
               x = Math.round(r * Math.sin(az_rad));
               y = Math.round(r * Math.cos(az_rad));
               ctx.fillRect(x+gO-z-1, gO-y-z-1, zw+2, zw+2);
            }
         }
      }
      ctx.globalAlpha = 1;
   }
   
   ctx.strokeStyle = "black";
   ctx.miterLimit = 2;
   ctx.lineJoin = "circle";
   ctx.font = "13px Verdana";

   if (gps_qzs3_el > 0) {
      az_rad = gps_qzs3_az * Math.PI / gHD;
      r = (90 - gps_qzs3_el)/90 * gHD;
      x = Math.round(r * Math.sin(az_rad));
      y = Math.round(r * Math.cos(az_rad));
      x += gO;
      y = gO - y;
      //console.log('QZS-3 az='+ gps_qzs3_az +' el='+ gps_qzs3_el +' x='+ x +' y='+ y);
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.arc(x,y,10, 0,2*Math.PI);
      ctx.stroke();

      // legend
      x = 12;
      y = 30;
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.arc(x,y,10, 0,2*Math.PI);
      ctx.stroke();
      var xo = 16;
      var yo = 4;
      ctx.fillStyle = 'black';
      ctx.lineWidth = 1;
      ctx.fillText('QZS-3', x+xo, y+yo);
   }

   ctx.fillStyle = "black";
   
   // img & canvas alignment target
   //ctx.fillRect(100, 200, 200, 1);
   //ctx.fillRect(200, 100, 1, 200);

   for (var sat = 0; sat < gps_nsats; sat++) gps_last_good_el[sat] = -1;
   
   for (var off = gps_nsamp-10; off >= -1; off--) {
      for (sat = 0; sat < gps_nsats; sat++) {
         var loff = (off == -1)? gps_last_good_el[sat] : off;
         if (off == -1 && loff == -1) continue;
         var m = gps_now - loff;
         if (m < 0) m += gps_nsamp;
         i = m*gps_nsats + sat;
         az = gps_az[i];
         el = gps_el[i];
         if (el == 0) continue;
         gps_last_good_el[sat] = off;
         
         az_rad = az * Math.PI / gHD;
         r = (90 - el)/90 * gHD;
         x = Math.round(r * Math.sin(az_rad));
         y = Math.round(r * Math.cos(az_rad));

         if (off == -1) {
            var prn = gps_prn[sat];
            var tw = ctx.measureText(prn).width;
            var tof = 8;
            var ty = 5;
            var toff = (az <= 180)? (-tw-tof) : tof;
            ctx.fillStyle = (loff > 1)? "pink" : "yellow";
            ctx.lineWidth = 3;   // rule of thumb: font size 13px /3 = 4.333
            ctx.strokeText(prn, x+toff+gO, gO-y+ty);
            ctx.lineWidth = 1;
            ctx.fillText(prn, x+toff+gO, gO-y+ty);

            z = 3;
            zw = z*2 + 1;
            ctx.fillStyle = "black";
            ctx.fillRect(x+gO-z-1, gO-y-z-1, zw+2, zw+2);
            ctx.fillStyle = (loff > 1)? "red" : "yellow";
            ctx.fillRect(x+gO-z, gO-y-z, zw, zw);
         } else {
            ctx.fillStyle = "black";
            ctx.fillRect(x+gO, gO-y, 2, 2);
         }
      }
   }
}


////////////////////////////////
// log
////////////////////////////////

var log = { };
var nlog = 256;

function log_html()
{
	var s =
	w3_div('id-log w3-text-teal w3-hide',
		w3_div('w3-container',
		   w3_inline('w3-valign w3-halign-space-between/',
		      w3_div('',
               w3_label('w3-show-inline', 'KiwiSDR server log (scrollable list, first and last set of messages)'),
               w3_button('w3-aqua|margin-left:10px', 'Log state', 'log_state_cb'),
               w3_button('w3-aqua|margin-left:10px', 'Log IP blacklist', 'log_blacklist_cb'),
               w3_button('w3-blue|margin-left:10px', 'Clear Histogram', 'log_clear_hist_cb')
            ),
            w3_switch_label('w3-label-inline w3-label-left', 'Log connections from local ip addresses?',
               'Yes', 'No', 'adm.log_local_ip', adm.log_local_ip, 'admin_radio_YN_cb')
         ),
			w3_div('id-log-msg w3-margin-T-8 w3-text-output w3-small w3-text-black', '')
		)
	);
	return s;
}

function log_setup()
{
	var el = w3_el('id-log-msg');
	var s = '<pre>';
		for (var i = 0; i < nlog; i++) {
			if (i == nlog/2) s += '<code id="id-log-not-shown"></code>';
			s += '<code id="id-log-'+ i +'"></code>';
		}
	s += '</pre>';
	el.innerHTML = s;

	ext_send('SET log_update=1');
}

function log_state_cb(id, idx)
{
	ext_send('SET log_state');
}

function log_blacklist_cb(id, idx)
{
	ext_send('SET log_blacklist');
}

function log_clear_hist_cb(id, idx)
{
	ext_send('SET log_clear_hist');
}


function log_resize()
{
	var el = w3_el('id-log-msg');
	if (!el) return;
	var hdr_height = w3_el("id-admin-top").clientHeight + w3_el("id-admin-nav").clientHeight;
	var log_height = window.innerHeight - hdr_height - (kiwi_isMobile()? 100 : 90);
	el.style.height = px(log_height);
}

var log_interval;

function log_focus(id)
{
	log_resize();
	log_update();
	log_interval = setInterval(log_update, 3000);
}

function log_blur(id)
{
	kiwi_clearInterval(log_interval);
}

function log_update()
{
	ext_send('SET log_update=0');
}


////////////////////////////////
// console
////////////////////////////////

function console_html()
{
   // must set "inline_returns" since pty output lines are terminated with \r\n instead of \n alone
   // otherwise the \r overwrite logic in kiwi_output_msg() will be triggered
   //console.log('$console SETUP');
   //kiwi_trace();
   admin.console = {
      scroll_only_at_bottom: true, inline_returns: true, process_return_alone: false, remove_returns: false,
      rows: 10, cols: 140,
      show_cursor: true,
      is_char_oriented: false,
      paste_state: 0
   };
   
   // reminder: use "&m" to simulate mobile on desktop
   admin.console.isMobile = kiwi_isMobile();
   //admin.console.isMobile = true;
   admin.console.always_char_oriented = admin.console.isMobile? false : true;
   
   kiwi.CONSOLE_KEY_ORIG = true;
   var console_msg_psa = kiwi.CONSOLE_KEY_ORIG? '' : ' w3-no-caret||contenteditable="true" spellcheck="false"';
   
   var dbg = (0 && dbgUs);

	var s =
	w3_div('id-console w3-text-teal w3-hide',
		w3_div('w3-container',
		   w3_div('',
            w3_label('w3-show-inline', 'Beagle Debian console'),
            w3_button('id-console-connect-btn w3-aqua|margin-left:10px', 'Connect', 'console_connect_cb'),

            (dbg)?
               w3_button('w3-aqua|margin-left:16px', 'ANSI', 'console_cmd_cb', 'console_input_cb|cd tools;mr')
               :
               w3_button('w3-green|margin-left:32px', 'monitor build progress', 'console_cmd_cb',
                  'console_input_cb|tail -fn 500 /root/build.log'),

            w3_button('w3-aqua|margin-left:16px|title="Type \'q\' to stop htop"',
               'htop', 'console_cmd_cb', 'console_input_cb|htop'),
            
            w3_button('w3-yellow|margin-left:16px', 'disk free', 'console_cmd_cb', 'console_input_cb|df -h .'),

            w3_button('w3-red|margin-left:16px|' +
               'title="CAUTION: This will delete all but\n10 MB of your system log file"',
               'clean logs', 'console_cmd_cb', 'console_input_cb|df -h .; journalctl --vacuum-size=10M; df -h .'),

            (dbg)?
               w3_button('w3-aqua|margin-left:16px', 'nano j', 'console_cmd_cb', 'console_input_cb|nano j')
               :
               w3_button('id-console-reclone w3-red|margin-left:16px', 're-clone sources', 'console_reclone_confirm'),

            w3_button('w3-blue|margin-left:16px', 'check github', 'console_cmd_cb',
               'console_input_cb|cdp; git show origin:Makefile &vbar; head -n 2'),

            w3_button('w3-blue|margin-left:16px', 'ping DNS', 'console_cmd_cb',
               'console_input_cb|ping -c3 1.1.1.1; ping -c3 8.8.8.8'),

            w3_button('w3-blue|margin-left:16px', 'ping kiwisdr', 'console_cmd_cb',
               'console_input_cb|ping -c3 kiwisdr.com')
         ),
         
			w3_div('id-console-msg w3-margin-T-8 w3-text-output w3-scroll-always-y w3-scroll-down w3-small w3-text-black ' +
			   'cl-admin-console-color' + console_msg_psa,
			   '<pre><code id="id-console-msgs"></code></pre>'
			),
			
			w3_div('',
            admin.console.always_char_oriented?
               w3_text('id-console-debug w3-text-black w3-margin-T-8',
                  kiwi_isWindows()?
                     'Windows: Type <x1>control-v</x1> twice (quickly) for clipboard paste. Once to get a normal <x1>control-v</x1>. ' +
                     'Control-w alternatives: nano <x1>fn-f6</x1>, bash <x1>esc</x1> <x1>control-h</x1> (see ' +
                     w3_link('w3-link-darker-color',
                        'https://forum.kiwisdr.com/index.php?p=/discussion/2927/windows-and-running-nano-text-editor-in-admin-console#p1',
                        'forum') +')'
                  :
                     'Mac: Type <x1>command-v</x1> for clipboard paste.'
               )
            :
               w3_div('id-console-line',
                  admin.console.isMobile?
                     w3_inline('w3-margin-T-8/',
                        w3_input('//id-console-line-input w3-input-any-change',
                           '', 'console_input', '', 'console_input_cb|console_key_cb', 'enter shell command'),
                        w3_inline('w3-margin-L-16/',
                           w3_button('w3-yellow', '^C', 'console_ctrl_button_cb', 'c'),
                           w3_button('w3-blue|margin-left:10px', '^D', 'console_ctrl_button_cb', 'd'),
                           w3_button('w3-red|margin-left:10px', '^\\', 'console_ctrl_button_cb', '\x3c'),
                           w3_button('w3-blue|margin-left:10px', '^P', 'console_ctrl_button_cb', 'p'),
                           w3_button('w3-blue|margin-left:10px', '^N', 'console_ctrl_button_cb', 'n')
                        )
                     )
                  :
                     w3_div('w3-margin-T-8',
                        w3_input('id-console-line-input w3-input-any-key', '', 'console_input', '',
                           'console_input_cb|console_key_cb', 'enter shell command'),
                        w3_text('id-console-debug w3-text-black w3-margin-T-8',
                           'Control characters (^C, ^D, ^\\) and empty lines may now be typed directly into shell command field.')
                     )
               )
         )
		)
	);
	return s;
}

function console_reclone_confirm(cmd)
{
   admin_confirm_show('Really re-clone?',
      function() {
         console_cmd_cb('console_reclone_confirm',
            'console_input_cb|cd /root; rm -rf '+ admin.repo_dir +'; git clone https://github.com/'+ admin.repo_git);
         admin_confirm_cancel_cb();
      }
   );
   
}

function console_is_char_oriented(is_char_oriented)
{
   is_char_oriented = isDefined(is_char_oriented)? is_char_oriented : admin.console.always_char_oriented;
   admin.console.is_char_oriented = is_char_oriented;
}

function console_input_cb(path, val)
{
   //console.log('console_input_cb val='+ val);
   if (admin.console.is_char_oriented) {
	   //console.log('console_input_cb IGNORED due to admin.console.is_char_oriented');
	   return;
   }
   
	//console.log('console_input_cb '+ val.length +' <'+ val +'>');
	ext_send('SET console_w2c='+ encodeURIComponent(val +'\n'));
   //alert('console_input_cb (LINE): '+ val);
   w3_set_value(path, '');    // erase input field
   w3_scrollDown('id-console-msg');    // scroll to bottom on input
}

function console_key_cb(ev, called_from_w3_input)
{
   called_from_w3_input = called_from_w3_input || false;
   //console.log('console_key_cb called_from_w3_input='+ called_from_w3_input);
	//event_dump(ev, 'console_key_cb', 1);
	//console.log(ev);
   if (!ev || !ev.key) return;
   var k = ev.key;
   var ord_k = ord(k);
   var ctrl_k = ord_k & 0x1f;
   var ok, ctrl_or_arrow;
   
   // NB: send SS3 versions of cursor keys (e.g. \x1bOA) instead of
   // CSI version (e.g. \x1b[A) since htop requires it
   // see: stackoverflow.com/questions/13585131/keyboard-transmit-mode-in-vt100-terminal-emulator

   if (!admin.console.is_char_oriented) {
   
      // line-oriented
      if (!called_from_w3_input) return;     // ignore the global keydown events
      var ctrl_s = k.toUpperCase();
      ctrl_or_arrow = true;
      if (ev.ctrlKey && 'CDPN\\'.includes(ctrl_s)) ; else
      if (k == 'ArrowUp')    k = '\x1bOA'; else
      if (k == 'ArrowDown')  k = '\x1bOB'; else
      if (k == 'ArrowRight') k = '\x1bOC'; else
      if (k == 'ArrowLeft')  k = '\x1bOD'; else
         ctrl_or_arrow = false;

      if (ctrl_or_arrow) {
         if (ev.ctrlKey) {
            //console.log('console_key_cb LINE: CTRL ^'+ ctrl_s +'('+ ctrl_k +') w3_input='+ called_from_w3_input);
            if (ctrl_k <= 0xff) {
               ext_send('SET console_oob_key='+ ctrl_k);
            //alert('console_key_cb LINE: '+ ctrl_k);
            }
         } else {
            //console.log('console_key_cb LINE: k='+ JSON.stringify(k) +' w3_input='+ called_from_w3_input);
	         ext_send('SET console_w2c='+ encodeURIComponent(k));
            //alert('console_key_cb LINE: '+ k);
         }
         w3_scrollDown('id-console-msg');    // scroll to bottom on input
      }
   } else
   
   if (!ev.metaKey) {
   
      // character-oriented
      if (called_from_w3_input) return;      // ignore the w3_input keydown events
	   //event_dump(ev, 'CHAR:', 1);
      var k2 = k, redo = false;
      ok = true;

      if (k.length == 1)      { k2 = k; } else
      if (k == 'Enter')       { k2 = '\r';   redo = true; } else
      if (k == 'Backspace')   { k2 = '\x7f'; redo = true; } else
      if (k == 'Tab')         { k2 = '\t';   redo = true; } else
      if (k == 'Escape')      { k2 = '\x1b'; redo = true; } else
      if (k == 'ArrowUp')     { k2 = '\x1bOA'; } else
      if (k == 'ArrowDown')   { k2 = '\x1bOB'; } else
      if (k == 'ArrowRight')  { k2 = '\x1bOC'; } else
      if (k == 'ArrowLeft')   { k2 = '\x1bOD'; } else
      if (k == 'F1')          { k2 = '\x1bOP'; } else
      if (k == 'F2')          { k2 = '\x1bOQ'; } else
      if (k == 'F3')          { k2 = '\x1bOR'; } else
      if (k == 'F4')          { k2 = '\x1bOS'; } else
      if (k == 'F5')          { k2 = '\x1b[15~'; } else
      if (k == 'F6')          { k2 = '\x1b[17~'; } else
      if (k == 'F7')          { k2 = '\x1b[18~'; } else
      if (k == 'F8')          { k2 = '\x1b[19~'; } else
      if (k == 'F9')          { k2 = '\x1b[20~'; } else
      if (k == 'F10')         { k2 = '\x1b[21~'; } else
         ok = false;
      
      if (redo) { ord_k = ord(k2); ctrl_k = ord_k & 0x1f; }
      var ctrl = ev.ctrlKey;
      
      // hack to get clipboard paste on Windows without sacrificing ctrl-v (nano "page next")
      if (kiwi_isWindows() && ctrl && k == 'v') {
         //console.log('Windows ctrl-v paste_state='+ admin.console.paste_state);
         
         if (admin.console.paste_state == 0) {
            admin.console.paste_timeout = setTimeout(
               function() {
                  // happened once within window -- treat as regular ctrl-v
                  ext_send('SET console_oob_key='+ (ord('v') & 0x1f));
                  admin.console.paste_state = 0;
               }, 500
            );
            admin.console.paste_state = 1;
            ok = false;
         } else {
            // happened twice within window -- let paste event through
            kiwi_clearTimeout(admin.console.paste_timeout);
            admin.console.paste_state = 0;
            return;
         }
      }

      if (0 && dbgUs) {
         var ctrl1 = (k2.length == 1 && ord_k < 32);  // not ev.ctrlKey but still single char with ord_k < 32
         var esc = (k2.length > 1);
         var del = (k2.length == 1 && ord_k == 127);
         var k_s = ctrl? k2.toUpperCase() : (ctrl1? ('\\'+ chr(ord_k + 96)) : (del? 'del' : (esc? k : k2)));
         var ord_s =  esc? '' : ( '('+ (ctrl? ctrl_k : ord_k) +')' );
         console.log('console_key_cb CHAR: '+ (ctrl? 'CTRL ^':'') + k_s + ord_s +
            ' len='+ k.length +'|'+ k2.length +' w3_input='+ called_from_w3_input +' ok='+ ok);
      }

      if (ok) {
         if (ctrl) {
            if (ctrl_k <= 0xff) {
               ext_send('SET console_oob_key='+ ctrl_k);
               //alert('console_key_cb CHAR: '+ ctrl_k);
            }
         } else {
            // htop requires multi-char sequences (e.g. arrow keys) to be written to server-side pty together
            // so use console_w2c= instead of repeated console_oob_key=
	         ext_send('SET console_w2c='+ encodeURIComponent(k2));
            //alert('console_key_cb CHAR: '+ k2);
         }
         admin.console.must_scroll_down = true;
      }
   }

   // prevent Firefox "quick find" popup from appearing with ' and / keys
   // but let meta keys go through for things like page reload (meta-R), cut/paste etc.
   if (!ev.metaKey)
      ev.preventDefault();

	// prevent dom parents from seeing bubbling event
	// Don't call cancelEvent() because that does a stopImmediatePropagation() which prevents the
	// subsequent keyup & change/input events from occurring.
	//console.log('console_key_cb stopPropagation');
	ev.stopPropagation();
}

// paste single char every 10 msec
function console_paste_char(i)
{
   var c = admin.console.pasted_text[i];
   //console.log(i +'>>> '+ c);
   ext_send('SET console_w2c='+ encodeURIComponent(c));
   i++;
   if (i < admin.console.pasted_text_len) {
	   setTimeout(function(i) { console_paste_char(i); }, 10, i);
	}
}

// paste multiple chars every 10 msec
function console_paste_chars(s)
{
   var sl = Math.min(s.length, 32);
   var s1 = s.slice(0, sl);
   var s2 = s.slice(sl);
   ext_send('SET console_w2c='+ encodeURIComponent(s1));
   if (s2 != '') {
	   setTimeout(function(s) { console_paste_chars(s); }, 10, s2);
	}
}

function console_paste_cb(ev)
{
	//event_dump(ev, 'console_paste_cb', 1);
	//console.log(ev);
   if (admin.console.is_char_oriented) {
	   var s = ev.clipboardData.getData('text');
      //admin.console.pasted_text = s.split('');
      //admin.console.pasted_text_len = admin.console.pasted_text.length;
      //console.log(admin.console.pasted_text);
      //console.log(admin.console.pasted_text_len);
      //console_paste_char(0);
      console_paste_chars(s);
   }
	ev.preventDefault();
}

function console_ctrl_button_cb(id, ch)
{
   console.log('console_ctrl_button_cb ch='+ ord(ch));
   ext_send('SET console_oob_key='+ (ord(ch) & 0x1f));
   //alert('console_ctrl_button_cb: '+ ch);
}

function console_calc_rows_cols(init)
{
	var el = w3_el('id-console-msg');
   var h_msgs = el.clientHeight - /* margins +5 */ 25;
   var h_msg = 15.6;
   var h_ratio = h_msgs / h_msg;
   var rows = Math.floor(h_ratio);
   if (rows < 1) rows = 1;

   var w_msgs = el.clientWidth - /* margins +5 */ 25;
   var w_msg = 7.4;
   var w_ratio = w_msgs / w_msg;
   var cols = w3_clamp(Math.floor(w_ratio), 1, 256);

   if (0 && dbgUs)
      w3_innerHTML('id-console-debug', 'h_msgs='+ h_msgs +' rows: '+ h_ratio.toFixed(2) +' <x1>'+ rows +'</x1>  ' +
         'w_msgs='+ w_msgs +' cols: '+ w_ratio.toFixed(2) +' <x1>'+ cols +'</x1>');

   if (init || rows != admin.console.rows || cols != admin.console.cols) {
      //console_nv('$console_calc_rows_cols', {init}, {rows}, {cols});
      //kiwi_trace('$');
      kiwi_clearTimeout(admin.resize_timeout);
      admin.resize_timeout = setTimeout(
         function() {
            admin.console.resized = true;
            // let server-side know so it can send a TIOCSWINSZ to libcurses et al
            ext_send('SET console_rows_cols='+ rows +','+ cols);
            admin.console.rows = rows;
            admin.console.cols = cols;
         }, init? 1:1000
      );
   }
}

function console_open()
{
   ext_send('SET console_open');
   console_calc_rows_cols(1);
   console_is_char_oriented();
   admin.console_open = true;
   console_focus();
}

function console_connect_cb(id)
{
   //console.log('console_connect_cb id='+ id);
   if (admin.console_open) return;
   console_open();
}

function console_cmd_cb(id, cb_param)
{
   //console.log('console_cmd_cb id='+ id +' cb_param='+ cb_param);
   var delay = 1;
   
   if (!admin.console_open) {
      console_open();
      delay = 1000;
   }
   
	setTimeout(function() {
	   var a = cb_param.split('|');
	   var cb = a[0];
	   var cmd = a[1];
      //console.log('console_cmd_cb: cmd='+ cmd);
      if (admin.console.is_char_oriented) {
         cmd = cmd.replace('&vbar;', '|');
         ext_send('SET console_w2c='+ encodeURIComponent(cmd +'\n'));
         w3_scrollDown('id-console-msg');    // scroll to bottom on input
      } else {
	      w3_input_force('id-console-line-input', cb, cmd);
	   }
	}, delay);
}

function console_resize()
{
	var el = w3_el('id-console-msg');
	if (!el) return;
	var hdr_height = w3_el("id-admin-top").clientHeight + w3_el("id-admin-nav").clientHeight;
	var adj = admin.console.always_char_oriented? 115 : (admin.console.isMobile? 140 : 150);
	var console_height = window.innerHeight - hdr_height - adj;
   //if (kiwi_isMobile()) alert('cr '+ console_height +'='+ window.innerHeight +'-'+ hdr_height +'-'+ adj);
	el.style.height = px(console_height);
	
	if (kiwi_isMobile()) {
	   // mobile: leave at laptop width, but scale input bar
	   
	   var m = ext_mobile_info();
	   var input_field_size = m.phone? 32 : (m.iPad? 48 : 64);
	   //alert('ifs='+ input_field_size);
	   w3_el('id-console_input').size = input_field_size;
	} else {
	   // desktop: full width
	   var console_width = window.innerWidth - 65;
	   el.style.width = px(console_width);
	}
	
	//w3_innerHTML('id-console-debug', window.innerHeight +' '+ hdr_height +' '+ console_height);

   console_calc_rows_cols(0);
}

function console_focus(id)
{
   //console.log('console_focus console_open='+ admin.console_open);
   
   if (admin.console_open) {
      if (kiwi.CONSOLE_KEY_ORIG) {
      
         // console_key_cb() has to be a document keydown event so console window immediately has focus
         // when the console tab is selected. But since it's being dynamically added/removed by the
         // console focus/blur routines the global admin_navkey_cb() must be removed because otherwise
         // it will be first in the event dispatch list. Put it back when another tab is selected.
         // Downside: There is (currently) no key assigned that will blur the console giving
         // keydown events back to admin_navkey_cb()
         if (admin.admin_navkey_cb_added) {
            document.removeEventListener("keydown", admin_navkey_cb, w3.BUBBLING);
            admin.admin_navkey_cb_added = false;
         }
         document.addEventListener("keydown", console_key_cb, w3.BUBBLING);
         document.addEventListener("paste", console_paste_cb, w3.BUBBLING);
      } else {
      
         // This is an alternative to the above where the keydown event is attached to the
         // id-console-msg div. But the div must have contenteditable set to receive such events.
         // And being contenteditable has some side-effects (i.e. w3-no-caret spellcheck="false")
         // Another requirement is that when the console is selected a id-console-msg focus
         // must be made to match CONSOLE_KEY_ORIG behavior.
         // So in the end this scheme really has no advantage over the CONSOLE_KEY_ORIG option.
         var tgt = w3_el('id-console-msg');
         tgt.addEventListener("keydown", console_key_cb, w3.BUBBLING);
         tgt.addEventListener("paste", console_paste_cb, w3.BUBBLING);
         tgt.focus();
      }
   }
   
	console_resize();
}

function console_blur(id)
{
   //console.log('console_blur console_open='+ admin.console_open);
   
   if (admin.console_open) {
      if (kiwi.CONSOLE_KEY_ORIG) {
         document.removeEventListener("keydown", console_key_cb, w3.BUBBLING);
         document.removeEventListener("paste", console_paste_cb, w3.BUBBLING);
         if (!admin.admin_navkey_cb_added) {
            document.addEventListener("keydown", admin_navkey_cb, w3.BUBBLING);
            admin.admin_navkey_cb_added = true;
         }
      } else {
         var tgt = w3_el('id-console-msg');
         tgt.removeEventListener("keydown", console_key_cb, w3.BUBBLING);
         tgt.removeEventListener("paste", console_paste_cb, w3.BUBBLING);
      }
   }
}


////////////////////////////////
// extensions, in admin_sdr.js
////////////////////////////////


////////////////////////////////
// security
////////////////////////////////

function security_html()
{
   // Let cfg.chan_no_pwd retain values > rx_chans if it was set when another configuration
   // was used. Just clamp the menu value to the current rx_chans;
	var chan_no_pwd = ext_get_cfg_param('chan_no_pwd', 0);
	chan_no_pwd = Math.min(chan_no_pwd, rx_chans - 1);
   var chan_no_pwd_u = { 0:'none' };
   for (var i = 1; i < rx_chans; i++)
      chan_no_pwd_u[i] = i.toFixed(0);

	var s1 =
	/*
		w3_div('w3-valign',
			w3_header('w3-container w3-yellow/', 5,
            'Passwords are now stored in an encrypted format. After the page is reloaded the ' +
            'password fields will show "(encrypted)" instead of showing the passwords in the clear. <br>' +
            'As before, you may change passwords at any time and also set an empty password if, for example, ' +
            'you want to allow user connections without needing a password. <br>' +
            'If below "Admin auto-login from local net even if password set" is set to "No", ' +
            '<i>and you forget the admin password</i>, then you\'ll have no way to bring up the admin page. <br>' +
            'In that case the only way to recover is to ssh/PuTTY into Debian on the Beagle and remove the password encryption files manually.'
         )
		) +
	*/

		w3_inline_percent('w3-container/w3-hspace-16 w3-text-teal',
			w3_div('',
            w3_switch_label('', 'User auto-login from local net<br>even if password set?',
               'Yes', 'No', 'adm.user_auto_login', adm.user_auto_login, 'admin_radio_YN_cb')
			), 25,

			w3_div('w3-center',
				w3_select('w3-width-auto', 'Number of channels<br>not requiring a password<br>even if password set',
					'', 'chan_no_pwd', chan_no_pwd, chan_no_pwd_u, 'admin_select_cb'),
				w3_div('w3-margin-T-8 w3-text-black',
					'Set this and a password to create two sets of channels. ' +
					'Some that have open-access requiring no password and some that are password protected.'
				)
			), 30,

			w3_div(''), 1,

			w3_div('',
				w3_input('w3-encrypted', 'User password', 'adm.user_password', '', 'security_set_upw_cb',
					'No password set: unrestricted Internet access to SDR')
			), 40
		) +

		'<hr>' +
		w3_inline_percent('w3-container/w3-hspace-16 w3-text-teal',
			w3_div('',
            w3_switch_label('', 'Admin auto-login from local net<br>even if password set?',
               'Yes', 'No', 'adm.admin_auto_login', adm.admin_auto_login, 'admin_radio_YN_cb')
			), 25,

			w3_div('w3-text-teal',
				''
			), 30,

			w3_div(''), 1,

			w3_div('',
				w3_input('w3-encrypted', 'Admin password', 'adm.admin_password', '', 'security_set_apw_cb',
					'No password set: no admin access from Internet allowed')
			), 40
		);

   var s2 =
		'<hr>' +
		w3_inline_percent('w3-container/w3-hspace-16 w3-text-teal',
			w3_div('',
			   // Don't erase password here if set to "No" in case it is immediately switched back to "Yes".
			   // Password will be erased at next admin login if set "No".
            w3_switch_label('', 'Save admin password in<br> browser local storage?',
               'Yes', 'No', 'adm.admin_save_pwd', adm.admin_save_pwd, 'security_save_pwd_cb')
			), 25,

			w3_div('w3-text-black',
				'Default is "No". Set to "Yes" to save the admin password in browser local storage. ' +
				'It will be used for subsequent admin connections. ' +
				'Note that local storage is different from browser cookie storage. ' +
				'The admin password will never be sent as part of any cookie transmission.'
			), 33
		);

   var s3 =
      /*
		'<hr>' +
		w3_inline_percent('w3-container/w3-hspace-16 w3-text-teal',
			w3_div('',
            w3_switch_label('', 'Restrict console connections <br> to the local network?',
               'Yes', 'No', 'adm.console_local', adm.console_local, 'admin_radio_YN_cb')
			), 25,

			w3_div('w3-text-black',
				'Set to "Yes" (default) to restrict use of the console tab ' +
				'to connections from the local network. This is an important ' +
				'security enhancement. You might want to set to "No" if the ' +
				'Kiwi has difficulty determining your local network address.'
			), 33
		) +
		*/

		'<hr>' +
		w3_inline_percent('w3-container/w3-hspace-16',
			w3_div('w3-text-teal w3-bold', 'Restrict console connections <br> to the local network?'), 25,
			w3_div('w3-text-black',
			   'This feature is now enabled by creating a file in the Kiwi configuration directory. See the ' +
            w3_link('w3-link-darker-color', 'http://kiwisdr.com/info/#id-opt-dot', 'Operating Information') +
			   ' for more information.'
			), 33
		) +

		'<hr>' +
		w3_inline_percent('w3-container/w3-hspace-16 w3-text-teal',
			w3_div('',
            w3_switch_label('', 'Allow GPS timestamp information <br> to be sent on the network?',
               'Yes', 'No', 'adm.GPS_tstamp', adm.GPS_tstamp, 'admin_radio_YN_cb')
			), 25,

			w3_div('w3-text-black',
				'Set to "No" to prevent timestamp information from your GPS ' +
				'(assuming it is working) from being used by applications on the Internet ' +
				'such as the TDoA service. You would only do this if you had some concern ' +
				'about your publicly-listed Kiwi participating in these kinds of projects. '
			), 33,

			w3_div('w3-text-black'), 1,

			w3_div('w3-text-black',
				'However we expect most Kiwi owners will want to participate and we encourage ' +
				'you to do so. Your precise GPS location is not revealed by the timestamp information. ' +
				'For more discussion please see the ' +
				w3_link('w3-link-darker-color', 'http://forum.kiwisdr.com/discussion/1218/participation-of-kiwis-in-the-tdoa-process/p1', 'Kiwi forum') +'.'
			), 33
		);

   var s4 =
		'<hr>' +
		w3_inline_percent('w3-container/w3-hspace-16 w3-text-teal',
			w3_div('',
            w3_switch_label('', 'Automatically reload admin page <br> if server stops responding?',
               'Yes', 'No', 'adm.admin_keepalive', adm.admin_keepalive, 'admin_radio_YN_cb')
			), 25,

			w3_div('w3-text-black',
			   'Default "Yes". If set to "No" only a warning message will be displayed if the Kiwi server ' +
			   'or network closes the connection.'
			), 33,

			w3_div('w3-text-black'), 1,

			w3_div('w3-text-black',
			   ''
			), 33
		) +
		'<hr>';

	return w3_div('id-security w3-hide', s1 + s2 + s3 + s4);
}

function security_focus(id)
{
	admin_set_decoded_value('adm.user_password');
	admin_set_decoded_value('adm.admin_password');
	//w3_el('id-security-json').innerHTML = w3_div('w3-padding w3-scroll', JSON.stringify(cfg));
}

function security_set_upw_cb(path, val, first)
{
   adm.user_pwd_seq = +adm.user_pwd_seq + 1;
   w3_string_set_cfg_cb(path, val, first);
}

function security_set_apw_cb(path, val, first)
{
   adm.admin_pwd_seq = +adm.admin_pwd_seq + 1;
   w3_string_set_cfg_cb(path, val, first);
}

function security_save_pwd_cb(path, idx, first)
{
	var enabled = (+idx == w3_SWITCH_YES_IDX);
	//console.log('security_save_pwd_cb: first='+ first +' enabled='+ enabled);
   w3_string_set_cfg_cb(path, enabled, first);
   if (enabled)
      kiwi_storeWrite('admin', adm.admin_password);
   else
      kiwi_storeDelete('admin');
}


////////////////////////////////
// admin
////////////////////////////////

var admin_colors = [
	'w3-hover-red',
	'w3-hover-blue',
	'w3-hover-purple',
	'w3-hover-black',
	'w3-hover-aqua',
	'w3-hover-pink',
	'w3-hover-yellow',
	'w3-hover-amber',
	'w3-hover-green',
	'w3-hover-orange',
	'w3-hover-grey',
	'w3-hover-lime',
	'w3-hover-indigo',
	'w3-hover-brown',
	'w3-hover-teal',
	'w3-hover-blue-grey',
	'w3-hover-deep-orange'
];

function admin_main()
{
	ext_send("SET browser="+ navigator.userAgent);
	window.addEventListener('resize', admin_resize, w3.BUBBLING);
}

//var arseq = 0;
function admin_resize()
{
   var adm = w3_el("id-admin");
      var top = w3_el("id-admin-top");
      var scr = w3_el("id-admin-scr");
         var con1 = w3_el("id-admin-con1");
            var nav = w3_el("id-admin-nav");
            if (isUndefined(top)) return;
            var hdr_height = top.clientHeight + nav.clientHeight;
            var con2 = w3_el('id-admin-con2');
               // ...

   // top bar is fixed at the width of the screen so the "user page" button is always visible
   top.style.width = px(window.innerWidth - /* L/R margins */ 32);
   
   // There are a couple bits of magic here:
   
   // The X scroll for screen widths less-than-laptop only works when the w3-scroll is
   // one div level above where the div minWidth is set.
   w3_add(scr, 'w3-scroll');
   con1.style.minWidth = '1465px';     // 1496px (development laptop width) - 31px = 1465px

   // The Y scroll only works when the height is set in the *same* div as the w3-scroll.
   // "footer slop" is a compromise between iPhone/iPad devices.
   w3_add(con2, 'w3-scroll');
   var adj = /* header margin bottom */ 16 + /* x_scrollbar_height */ kiwi_scrollbar_width() + /* footer slop */ 16;
   //if (kiwi_isMobile()) alert('ar wh='+ window.innerHeight +' hh='+ hdr_height +' adj='+ adj);
   con2.style.height = px(window.innerHeight - hdr_height - adj);

	//mdev_log('#'+ arseq +' wh='+ window.innerWidth +'|'+ window.innerHeight +' hh|ch='+ hdr_height +'|'+ w3_el("id-admin-con2").clientHeight);

   /*
   mdev_log('#'+ arseq +
      ' CON1 sc h='+ con1.scrollHeight +'|'+ con1.clientHeight +' w='+ con1.scrollWidth +'|'+ con1.clientWidth +
      ' NAV sc h='+ nav.scrollHeight +'|'+ nav.clientHeight +' w='+ nav.scrollWidth +'|'+ nav.clientWidth +
      ' CON2 sc h='+ con2.scrollHeight +'|'+ con2.clientHeight +' w='+ con2.scrollWidth +'|'+ con2.clientWidth
   );
   */
   
   /*
	console.log('admin_resize('+ arseq +'): ADM h='+ adm.scrollHeight +'|'+ adm.clientHeight +' w='+ adm.scrollWidth +'|'+ adm.clientWidth);
	console.log('admin_resize('+ arseq +'): CON1 h='+ con1.scrollHeight +'|'+ con1.clientHeight +' w='+ con1.scrollWidth +'|'+ con1.clientWidth);
	console.log('admin_resize('+ arseq +'): NAV h='+ nav.scrollHeight +'|'+ nav.clientHeight +' w='+ nav.scrollWidth +'|'+ nav.clientWidth);
	console.log('admin_resize('+ arseq +'): CON2 h='+ con2.scrollHeight +'|'+ con2.clientHeight +' w='+ con2.scrollWidth +'|'+ con2.clientWidth);
   console.log('admin_resize hh='+ hdr_height);
   */

   //arseq++;
	log_resize();
	console_resize();
}

function kiwi_ws_open(conn_type, cb, cbp)
{
	return open_websocket(conn_type, cb, cbp, admin_msg, admin_recv, null, admin_close);
}

function admin_draw(sdr_mode)
{
	var ci = 0;
	
   if (sdr_mode)
      admin.tabs = [
         'Status', 'Mode',    'Control',    'Users', 'Connect',
         'Config', 'Webpage', 'Public',     'DX',
         'Update', 'Backup',  'Network',
         'GPS',
         'Log',    'Console', 'Extensions', 'Security'
      ];
   else
      admin.tabs = [
         'GPS',
         'Status', 'Mode',    'Control',    'Users', 'Connect',
         'Update', 'Backup',  'Network',
         'Log',    'Console', 'Extensions', 'Security'
      ];
   
   var s = '';
   admin.tabs.forEach(
      function(tab,i) {
         s += w3_nav(admin_colors[ci++], tab, 'id-navbar-admin', tab.toLowerCase(), 'admin_nav');
      }
   );

	var hdr =
		w3_div('id-admin-nav w3-margin-B-16 w3-margin-R-16',
			w3_navbar('id-navbar-admin w3-border w3-light-grey', s) +
	
			w3_divs('id-confirm w3-hide/w3-valign',
			   w3_header('w3-show-inline-block w3-container w3-red/id-confirm-msg', 5) +
				w3_div('w3-show-inline-block', w3_button('w3-green w3-margin-L-16', 'Confirm', 'admin_confirm_cb')) +
				w3_div('w3-show-inline-block', w3_button('w3-yellow w3-margin-L-16', 'Cancel', 'admin_confirm_cancel_cb'))
			) +
			
			w3_divs('id-restart w3-hide/w3-valign',
			   w3_header('w3-show-inline-block w3-container w3-red/', 5, 'Restart required for changes to take effect') +
				w3_div('w3-show-inline-block', w3_button('w3-green w3-margin-L-16', 'KiwiSDR server restart', 'admin_restart_now_cb')) +
				w3_div('w3-show-inline-block', w3_button('w3-yellow w3-margin-L-16', 'Cancel', 'admin_restart_cancel_cb'))
			) +
			
			w3_divs('id-reboot w3-hide/w3-valign',
			   w3_header('w3-show-inline-block w3-container w3-red/', 5, 'Reboot required for changes to take effect') +
				w3_div('w3-show-inline-block', w3_button('w3-green w3-margin-L-16', 'Beagle reboot', 'admin_reboot_now_cb')) +
				w3_div('w3-show-inline-block', w3_button('w3-yellow w3-margin-L-16', 'Cancel', 'admin_reboot_cancel_cb'))
			) +
			
			w3_div('id-build-restart w3-valign w3-hide',
			   w3_header('w3-container w3-blue/', 5, 'Server will restart after build')
			) +

			w3_div('id-build-reboot w3-valign w3-hide',
			   w3_header('w3-container w3-red/', 5, 'Beagle will reboot after build')
			) +

			w3_div('id-admin-closed w3-valign w3-hide',
			   w3_header('w3-container w3-red/', 5, 'Warning: Admin connection closed')
			)
		);
	
	if (sdr_mode)
	   s = status_html();
	else
	   s = gps_html() + status_html();

	s +=
		mode_html() +
		control_html() +
      users_html() +
		connect_html();

	if (sdr_mode)
	   s +=
		   config_html() +
         webpage_html() +
         kiwi_reg_html() +
         dx_html();

   s +=
		update_html() +
		backup_html() +
		network_html() +
		(sdr_mode? gps_html() : '') +
		log_html() +
		console_html() +
		(sdr_mode? extensions_html() : '') +
		security_html();

	w3_innerHTML('id-kiwi-container',
	   w3_div('id-admin w3-margin-L-16',
		   w3_inline_percent('id-admin-top/',
			   w3_header('w3-container w3-teal/id-mdev-msg', 5,
			      'Admin interface'+
			      w3_text('w3-margin-L-32 w3-padding-B-2 w3-font-14px', "Type 'h' or '?' for help")
			   ), 95,
			   w3_button('w3-aqua w3-margin-left', 'User page', 'admin_user_page_cb')
			),
			
			w3_div('id-admin-scr',
            w3_div('id-admin-con1',
               hdr,
               w3_div('id-admin-con2', s)
            )
         )
	   )
	);

   admin_resize();
   setTimeout(function() { admin_resize(); }, 500);
	log_setup();
	stats_init();

	if (sdr_mode) {
	   users_init( { admin:1 } );
	   config_init();
	   dx_html_init();
	   //gps_focus();
	} else {
	   gps_focus();
	}

	w3_show_block('id-admin');
	var nav_def = sdr_mode? 'status' : 'gps';
	
	admin.init = true;
	   var help = false;
	   var tab = kiwi_url_param(0, null);
	   if (tab) tab = tab.split(',')[0];
	   if (isNonEmptyString(tab)) {
	      if (tab == 'help') help = true;
	      var found = false;
	      admin.tabs.forEach(
	         function(s,i) {
	            s = s.toLowerCase();
	            if (!found && s.startsWith(tab)) {
	               //console.log('last_admin_navbar='+ s);
	               kiwi_storeWrite('last_admin_navbar', s);
	               found = true;
	            }
	         }
	      );
	   }
	   
	   if (kiwi_storeRead('last_admin_navbar') == 'sdr_hu') kiwi_storeWrite('last_admin_navbar', 'public');
      w3_click_nav('id-navbar-admin', kiwi_toggle(toggle_e.FROM_COOKIE | toggle_e.SET, nav_def, nav_def, 'last_admin_navbar'), 'admin_nav');

      // setup "user connection style" confirmation panel for admin help
      confirmation_panel_init();
	   init_panel_toggle(ptype.POPUP, 'id-confirmation', false, popt.CLOSE);
      confirmation_panel_init2();
      if (help) admin_show_help();
	admin.init = false;
	
	// navigate navbar using keys
   document.addEventListener("keydown", admin_navkey_cb, w3.BUBBLING);
   admin.admin_navkey_cb_added = true;

	setTimeout(function() { setInterval(status_periodic, 5000); }, 1000);
}

function admin_user_page_cb() { kiwi_open_or_reload_page({ tab:1 }); }

function admin_nav_focus(id, cb_arg)
{
   if (confirmation && confirmation.close_cb)
      confirmation.close_cb();
   id = id.toLowerCase();
   //console.log('admin_nav_focus id='+ id);
   admin.current_tab_name = id;
   w3_click_nav('id-navbar-admin', id, id, null, 'admin_nav_focus');
   kiwi_storeWrite('last_admin_navbar', id);
}

function admin_nav_blur(id, cb_arg)
{
   //console.log('admin_nav_blur id='+ id);
   w3_call(id +'_blur');
}

function admin_navkey_cb(ev) {
	//event_dump(ev, 'admin_navkey_cb', 1);
   if (!isString(ev.key) || any_modifier_key_except_shift(ev)) return;
   var k = ev.key.toLowerCase();

   if (k == 'h' || k == '?') {
      admin_show_help();
      return;
   }
   if (k == 'escape') {
      confirmation.close_cb();
      return;
   }
   if (k == '@') {
	   ext_send('SET log_state');
      return;
   }
   
   if (k == 'enter' && admin.current_tab_name == 'console' && !admin.console_open) {
      w3_schedule_highlight('id-console-connect-btn');
      console_open();
   }
   
   var tabs = admin.tabs;
   var i = w3_array_el_seq(tabs, admin.current_tab_name, { toLower:1 }), j;
   var e = tabs.length - 1;
   if (k.length == 1 && k >= 'a' && a <= 'z') {    // lcase/ucase char match next/prev
      dir = (ev.key == k.toUpperCase())? -1:1;
      i = w3_wrap(i, 0, e);
      j = w3_wrap(i + dir, 0, e);
      var looping = 0;
      while (j != i) {
         //console_nv('kd', {dir}, {e}, {j}, {i});
         if (tabs[j].toLowerCase()[0] == k) {
            admin_nav_focus(tabs[j]);
            break;
         }
         j = w3_wrap(j + dir, 0, e);
         if (looping > 32) {
            admin_nav_focus(tabs[0]);
            break;
         }
         looping++;
      }
      //console.log('char dir='+ dir +' j='+ j +' '+ tabs[j]);
   } else
   if (k.startsWith('arrow')) {     // left/right arrow
      dir = (k[5] == 'l')? -1 : ((k[5] == 'r')? 1:0);
      j = w3_wrap(i + dir, 0, e);
      if (j != i) admin_nav_focus(tabs[j]);
   }
}

function admin_show_help()
{
   var s =
      w3_text('w3-medium w3-bold w3-text-aqua', 'Admin interface help') +
      w3_div('w3-margin-T-8 w3-scroll-y|height:90%',
         w3_div('w3-margin-R-8',
            w3_inline('w3-padding-tiny',
               'See the ' +
               w3_link('w3-link-darker-color w3-bold', '//kiwisdr.com/info/#id-config', 'Kiwi documentation') +
               ' for more information. <br> Use the escape key, or click <x1>X</x1> at upper right, to dismiss this panel.'
            ),
            
            w3_inline('w3-padding-tiny w3-bold w3-text-aqua  w3-margin-T-8', 'URL parameters'),
            w3_inline('w3-padding-tiny',
               'The URL connecting to the admin page can specify which tab to initially select. ' +
               'And if the <x1>Extensions</x1> tab is selected a second parameter can select the  ' +
               'entry in the extensions list. The tab names can be abbreviated.' +
               '<br>Example:<br>' +
               w3_text('|color:orange', 'my_kiwi:8073/admin?users &nbsp; my_kiwi:8073/admin?ext,ft8')
            ),
            
            w3_inline('w3-padding-tiny w3-bold w3-text-aqua  w3-margin-T-8', 'Navigation bar shortcuts'),
            w3_inline('w3-padding-tiny',
               'In addition to clicking on the navigation bar tabs you can also use keyboard shortcuts. ' +
               'Type the beginning character of a tab name to switch to it, e.g. "d" for the <x1>DX</x1> tab. ' +
               'If more than one tab is matched they will cycle left-to-right, ' +
               'e.g. "s" selects between <x1>Status</x1> and <x1>Security</x1>. ' +
               'Typing a capital letter cycles right-to-left. The left/right arrow keys also work. ' +
               'On the <x1>Extensions</x1> tab the up/down arrow keys cycle the extension menu on the left side.'
            ),

            w3_inline('w3-padding-tiny w3-bold w3-text-aqua w3-margin-T-8', 'Console tab'),               
            w3_inline('w3-padding-tiny',
               'Shortcut: Instead of clicking on the <x1>Connect</x1> button use the return key.' +
               '<br><br>' +
               'One quirk to the navigation shortcuts is the console tab.' +
               'When the console is not connected the shortcuts work as expected. ' +
               'But when connected the shortcut keys don\'t function. This is because the console ' +
               'needs to intercept every key. Consider what happens when you run a screen-based text editor ' +
               'like "nano". It makes use of essentially all keys, including control characters and arrow keys. ' +
               'So if the console is open and you are on another tab and cycle into <x1>Console</x1> by using "c" ' +
               'the cycling will "lock" as soon as the console tab is hit. Because the open console now captures ' +
               'all the keystrokes. In this case simply use the mouse to move on to another tab.'
            ),
            
            w3_inline('w3-padding-tiny w3-bold w3-text-aqua w3-margin-T-8', 'Other shortcuts'),               
            w3_inline_percent('w3-padding-tiny', '@', 15, 'Same as "Log state" button on Log tab (debugging aid)')
         )
      );
   confirmation_show_content(s, 600, 625);
   w3_el('id-confirmation-container').style.height = '100%';   // to get the w3-scroll-y above to work
}

function admin_close()
{
   // don't show message if reload countdown running
   kiwi_clearTimeout(admin.keepalive_timeoout);
   if (kiwi.no_reopen_retry) {
	      w3_hide('id-kiwi-msg-container');      // in case password entry panel is being shown
         w3_show_block('id-kiwi-container');
         wait_then_reload_page(0, 'Server has closed connection.');
   } else
   if (isUndefined(adm.admin_keepalive) || adm.admin_keepalive == true) {
      if (!admin.reload_rem && !admin.long_running) {
	      w3_hide('id-kiwi-msg-container');      // in case password entry panel is being shown
         w3_show_block('id-kiwi-container');
         //kiwi_show_msg('Server has closed connection.');
         //if (dbgUs) console.log('admin close'); else
            wait_then_reload_page(60, 'Server has closed connection. Will retry.');
      }
   } else {
      //console.log('ignoring admin keepalive (websocket close)');
      w3_show_block('id-admin-closed');
      w3_scrollTop('id-kiwi-container');
   }
}

function admin_update_start()
{
	ext_send_after_cfg_save("SET admin_update");
	admin.update_interval = setInterval(function() {ext_send("SET admin_update");}, 5000);
}

function admin_update_stop()
{
	kiwi_clearInterval(admin.update_interval);
}

function admin_update(p)
{
	var i;
	var json = decodeURIComponent(p);
	//console.log('admin_update='+ json);
   var obj = kiwi_JSON_parse('admin_update', json);
	if (obj) admin.status = obj;
	
	// rx.kiwisdr.com registration status
	if (adm.kiwisdr_com_register && admin.status.kiwisdr_com != undefined && admin.status.kiwisdr_com != '') {
	   w3_innerHTML('id-kiwisdr_com-reg-status', 'rx.kiwisdr.com registration: successful');
	}
	
	// GPS has had a solution, show buttons
	if (admin.status.lat != undefined) {
		w3_show_inline_block('id-webpage-grid-set');
		w3_show_inline_block('id-public-grid-set');
		w3_show_inline('id-webpage-gps-set');
		w3_show_inline('id-public-gps-set');

		w3_show_inline_block('id-wspr-grid-set');
		w3_show_inline_block('id-ft8-grid-set');
	}
}

// Process replies to our messages sent via ext_send('SET ...')
// As opposed to admin_recv() below that processes unsolicited messages sent from C code.
// #msg-proc

var gps = null;

function admin_msg(param)
{
   //console.log('admin_msg: '+ param[0]);
   switch (param[0]) {

		case "gps_update_cb":
			//console.log('gps_update_cb='+ param[1]);
         gps = kiwi_JSON_parse('gps_update_cb', decodeURIComponent(param[1]));
         w3_call('gps_update_admin_cb');
			break;

		case "gps_IQ_data_cb":
         _gps.IQ_data = kiwi_JSON_parse('gps_IQ_data_cb', decodeURIComponent(param[1]));
			break;

      case "gps_POS_data_cb":
         _gps.POS_data = kiwi_JSON_parse('gps_POS_data_cb', decodeURIComponent(param[1]));
         break;

      case "gps_MAP_data_cb":
         _gps.MAP_data = kiwi_JSON_parse('gps_MAP_data_cb', decodeURIComponent(param[1]));
         break;

      case "gps_az_el_history_cb":
         var gps_az_el = kiwi_JSON_parse('gps_az_el_history_cb', decodeURIComponent(param[1]));
         if (gps_az_el) w3_call('gps_az_el_history_cb', gps_az_el);
         break;

		case "dx_size":
			dx_size(param[1]);
			break;
		
		case "admin_mkr":
			var mkr = param[1];
			var obj = kiwi_JSON_parse('admin_mkr', mkr);
			if (obj) dx_render(obj);
			break;
		
		case "mkr_search_pos":
		   dx_search_pos_cb(param[1]);
		   break;

		case "keepalive":
		   // The keepalive sent by the backup function doesn't contain the additional info
         if (param.length == 2) {
            var p = param[1].split(',');
            if (p.length == 2) {
               kiwi.uptime = +p[0];
               kiwi.isLocal_ip = +p[1];
            }
         }
		   kiwi_clearTimeout(admin.keepalive_timeoout);
		   if (adm.admin_keepalive) {
		      //console.log('admin keepalive');
            admin.keepalive_rx_time = Date.now();
            admin.keepalive_timeoout = setTimeout(function() {
         
               // in case the timeout somehow didn't get cancelled (which shouldn't happen)
               var diff = Date.now() - admin.keepalive_rx_time;
               if (diff > 75000)
                  admin_close();
            }, 90000);
         } else {
            //console.log('ignoring admin keepalive (server)');
         }
		   break;

		default:
		   return false;
   }
   
   return true;
}

var log_msg_idx, log_msg_not_shown = 0;
var admin_sdr_mode = 1;

// Process replies to our messages sent via ext_send('ADM ...')
// after calling admin_main(), server will download cfg and adm state to us, then send 'init' message
function admin_recv(data)
{
   var param, el, s;
	var stringData = arrayBufferToString(data);
	var params = stringData.substring(4).split(" ");

	//console.log('admin_recv: '+ stringData);

	for (var i=0; i < params.length; i++) {
		param = params[i].split("=");

		//console.log('admin_recv: '+ param[0]);
		switch (param[0]) {     // #msg-proc

			case "admin_sdr_mode":
				admin_sdr_mode = (+param[1])? 1:0;
				break;
			
			case "proxy_url":
			   s = kiwi_remove_protocol(decodeURIComponent(param[1])).split(':');
			   admin.proxy_host = s[0];
			   admin.proxy_port = s[1];
			   console.log('PROXY '+ admin.proxy_host +':'+ admin.proxy_port);
			   break;

			case "is_multi_core":
				admin.is_multi_core = true;
				break;

			case "init":
		      // rx_chan == rx_chans for admin connections (e.g. 4 when ch = 0..3 for user connections)
				rx_chans = rx_chan = +param[1];
				//console.log("ADMIN init rx_chans="+rx_chans);
            admin_draw(admin_sdr_mode);
            ext_send('SET extint_load_extension_configs');
            ant_switch_config_html();
				break;

			case "repo_dir":
			   admin.repo_dir = decodeURIComponent(param[1]);
			   w3_title('id-console-reclone', 'CAUTION: Do not use unless git clone in\n'+ admin.repo_dir +' has become corrupted');
				break;

			case "repo_git":
			   admin.repo_git = decodeURIComponent(param[1]);
				break;

			case "get_gps_info_cb":
				param = decodeURIComponent(param[1]);
				//console.log('get_gps_info_cb: func='+ func +' param='+ param);
            var gps_info = kiwi_JSON_parse('get_gps_info_cb', param);
				//console.log(gps_info);
            kiwi.GPS_auto_grid = gps_info.grid;
				var func = admin_sdr.ext_cur_nav +'_gps_info_cb';
				w3_call(func, param);
				break;

			case "ext_call":
			   // assumes that '=' is a safe delimiter to split func/param
				var ext_call = decodeURIComponent(param[1]).split('=');
				var ext_func = ext_call[0];
				var ext_param = (ext_call.length > 1)? ext_call[1] : null;
				//console.log('ext_call: func='+ ext_func +' param='+ ext_param);
				w3_call(ext_func, ext_param);
				break;

			case "ext_configs_done":
				admin.ext_configs_done = true;
				break;

			case "admin_update":
				admin_update(param[1]);
				break;

			case "auto_nat":
				var p = +param[1];
				//console.log('auto_nat='+ p);
				el = w3_el('id-net-auto-nat-msg');
				var msg, color, type = 'add', stop = true, err = false;
				
				switch (p) {
					case 0: break;
					case 1: msg = 'succeeded'; color = 'w3-green'; break;
					case 2: msg = 'no device found'; color = 'w3-orange'; err = true; break;
					case 3: msg = 'rule already exists'; color = 'w3-yellow'; err = true; break;
					case 4: msg = 'command failed'; color = 'w3-red'; err = true; break;
					case 5: msg = 'pending'; color = 'w3-yellow'; stop = false; break;
					case 6: msg = 'pending'; color = 'w3-yellow'; type = 'delete'; stop = false; break;
					case 7: msg = 'succeeded'; color = 'w3-green'; type = 'delete'; break;
					default: break;
				}
				
				if (p && el) {
					el.innerHTML = w3_header('w3-container/', 5, 'Automatic '+ type +' of NAT rule on firewall / router: '+ msg);
					w3_remove_then_add(el, network.auto_nat_color, color);
					network.auto_nat_color = color;
					w3_show_block(el);
					if (stop) {
                  //console.log('auto_nat_status_poll STATUS OK STOP');
                  kiwi_clearInterval(network.nat_status_interval);
                  network.nat_status_interval = null;
                  if (err) {
                     //console.log('auto_nat_status_poll ERR => OFF');
					      w3_switch_set_value('adm.auto_add_nat', w3_SWITCH_NO_IDX);
					   }
					}
					if (err) setTimeout(function() { w3_hide(el); }, 5000);
				}
				break;

			case "log_msg_not_shown":
				log_msg_not_shown = parseInt(param[1]);
				if (log_msg_not_shown) {
					el = w3_el('id-log-not-shown');
					el.innerHTML = '---- '+ log_msg_not_shown.toString() +' lines not shown ----\n';
				}
				break;

			case "log_msg_idx":
				log_msg_idx = parseInt(param[1]);
				break;

			case "log_msg_save":
				el = w3_el('id-log-'+ log_msg_idx);
				if (!el) break;
				var el2 = w3_el('id-log-msg');
				var wasScrolledDown = w3_isScrolledDown(el2);
				s = kiwi_decodeURIComponent('log_msg_save', param[1]).replace(/</g, '&lt;').replace(/>/g, '&gt;');
				el.innerHTML = s;

				// only jump to bottom of updated list if it was already sitting at the bottom
				w3_scrollDown(el2, wasScrolledDown);
				break;

			case "log_update":
				log_update(param[1]);
				break;

			case "sd_done":
				sd_done(parseFloat(param[1]));
				break;

			case "domain_check_result":
				connect_domain_check_cb(parseFloat(param[1]));
				break;

			case "DUC_status":
				connect_DUC_status_cb(parseFloat(param[1]));
				break;

         // in response to us sending "SET rev_status_query" or "SET rev_register reg=..." with reg == FRPC_PROXY_UPD
			case "rev_status":
				connect_rev_status_cb(parseFloat(param[1]));
				break;

			case "check_port_status":
				network_check_port_status_cb(parseInt(param[1]));
				break;
				
			case "console_c2w":
		      // kiwi_output_msg() does decodeURIComponent()
		      admin.console.s = param[1];
		      //console.log('console_c2w:');
		      //console.log(admin.console);
				kiwi_output_msg('id-console-msgs', 'id-console-msg', admin.console);
				break;

			case "console_done":
			   console.log('## console_done');
				break;

			case "config_clone_status":
				config_clone_status_cb(parseInt(param[1]));
				break;
				
			case "network_ip_blacklist_status":
			   p = decodeURIComponent(param[1]).split(',');
				network_ip_blacklist_status(parseInt(p[0]), p[1]);
				break;
				
			case "network_ip_blacklist_enabled":
            if (!network.ip_address_error) {
               w3_flag_cond('id-ip-blacklist-status', network.bad_local_ip,
                  network.bad_local_ip? 'an improperly included local or loopback IP address was removed from the local blacklist!' : 'updated');
            }
				break;
			
			case "network_ip_blacklist_locked":
			   network_ip_blacklist_set2(network.bl_path, network.bl_val);
				break;
			
			case "network_ip_blacklist_busy":
            if (!network.ip_address_error)
               w3_flag('id-ip-blacklist-status', 'busy');
				break;
			
         case "user_list":
            //console.log(param[1]);
            users_list_cb(kiwi_decodeURIComponent('user_list', param[1]));
            break;

         case "xfer_stats_cb":   // in response to "SET xfer_stats"
            //console.log('xfer_stats_cb='+ param[1]);
            var o = kiwi_JSON_parse('xfer_stats_cb', param[1]);
            if (o) {
               //console.log(o);
				   status_xfer_cb(o.ad, o.au, o.ae, o.ar, o.ar2, o.an, o.ap, o.an2, o.ai);
            }
            break;
         
         case "rem_console_local":
            console.log('rem_console_local');
            delete adm.console_local;
            cfg_save_json('rem_console_local', 'adm.console_local', 'DELETE');
            break;

			default:
            if (param[0].startsWith('antsw_')) {
               if (ant_switch_admin_msg(param))
                  break;
            }
            
				console.log('ADMIN UNKNOWN: '+ param[0] +'='+ param[1]);
				break;
		}
	}
}

// callback when a control has w3-restart property
function w3_restart_cb()
{
	w3_show_block('id-restart');
	admin_resize();
}

// callback when a control has w3-reboot property
function w3_reboot_cb()
{
	w3_show_block('id-reboot');
	admin_resize();
}

function admin_restart_now_cb()
{
	ext_send('SET restart');
	wait_then_reload_page(60, 'Restarting KiwiSDR server');
}

function admin_restart_cancel_cb()
{
	w3_hide('id-restart');
	admin_resize();
	w3_call(admin.current_tab_name +'_restart_cancel_cb');
}

function admin_reboot_now_cb()
{
	ext_send('SET reboot');
	wait_then_reload_page(90, 'Rebooting Beagle');
}

function admin_reboot_cancel_cb()
{
	w3_hide('id-reboot');
	admin_resize();
}


// confirmation interface

function admin_confirm_show(msg, cb_func, cancel_cb_func)
{
   admin.confirm_cb_func = cb_func;
   admin.cancel_cb_func = cancel_cb_func;
   w3_innerHTML('id-confirm-msg', msg);
	w3_show_block('id-confirm');
	admin_resize();
}

function admin_restart_cb()
{
	admin.pending_restart = true;
	admin_confirm_show('Really restart?');
}

function admin_reboot_cb()
{
	admin.pending_reboot = true;
	admin_confirm_show('Really reboot?');
}

function admin_power_off_cb()
{
	admin.pending_power_off = true;
	admin_confirm_show('Really power off?');
}

function admin_confirm_cb()
{
	if (admin.pending_restart) {
		admin_restart_now_cb();
	} else
	if (admin.pending_reboot) {
		admin_reboot_now_cb();
	} else
	if (admin.pending_power_off) {
		ext_send('SET power_off');
		wait_then_reload_page(0, 'Powering off Beagle');
	} else {
	   w3_call(admin.confirm_cb_func);
	}
}

function admin_confirm_cancel_cb()
{
   w3_call(admin.cancel_cb_func);
	w3_hide('id-confirm');
	admin_resize();
   admin.confirm_cb_func = null;
   admin.cancel_cb_func = null;
}


function admin_int_cb(path, val, first)
{
	//console.log('admin_int_cb '+ path +'='+ val +' first='+ first);
	val = parseInt(val);
	if (isNaN(val)) {
	   // put old value back
	   val = ext_get_cfg_param(path);
	} else {
      // if first time don't save, otherwise always save
      var save = isArg(first)? (first? false : true) : true;
      //if (path.includes('rssi_azel_iq')) { console.log('admin_int_cb '+ path +' save='+ save); kiwi_trace(); }
	   ext_set_cfg_param(path, val, save);
	}
   w3_set_value(path, val);   // remove any fractional part from field
}

// limit precision using callback spec: 'admin_float_cb|prec'
function admin_float_cb(path, val, first, cb_a)
{
   var prec = -1;    // default to no precision limiting applied
	//console.log('admin_float_cb '+ path +'='+ val +' cb_a.len='+ cb_a.length);
	if (cb_a && cb_a.length >= 2) {
	   prec = +(cb_a[1]);
	   if (isNaN(prec)) prec = -1;
	   //console.log('admin_float_cb prec='+ prec);
	}
	val = parseFloat(val);
	if (isNaN(val)) {
	   // put old value back
	   val = ext_get_cfg_param(path);
	} else {
	   if (prec != -1) {
         var s = val.toFixed(prec);    // NB: .toFixed() does rounding
         //console.log('admin_float_cb val('+ prec +')='+ s);
         val = +s;
      }
      // if first time don't save, otherwise always save
      var save = isArg(first)? (first? false : true) : true;
	   ext_set_cfg_param(path, val, save);
	}
   w3_set_value(path, val);   // remove any non-numeric part from field
}

function admin_bool_cb(path, val, first)
{
	// if first time don't save, otherwise always save
	var save = isArg(first)? (first? false : true) : true;
	//console.log('admin_bool_cb path='+ path +' val='+ val +' first='+ first +' save='+ save);
	ext_set_cfg_param(path, val? true:false, save);
}

function admin_set_decoded_value(path)
{
	var val = ext_get_cfg_param(path);
	//console.log('admin_set_decoded_value: path='+ path +' val='+ val);
	w3_set_decoded_value(path, val);
	return val;
}

// translate radio button yes/no index to bool value
function admin_radio_YN_cb(path, idx, first)
{
	var enabled = (+idx == w3_SWITCH_YES_IDX);
   // first is used by direct callers to prevent a save
	var save = isArg(first)? (first? false : true) : true;
	//console.log('admin_radio_YN_cb path='+ path +' enabled='+ enabled +' first='+ first +' save='+ save);
   //if (path.includes('kiwisdr_com_register')) { console.log('admin_radio_YN_cb '+ path +' save='+ save); kiwi_trace(); }
	ext_set_cfg_param(path, enabled, save);
}

function admin_select_cb(path, idx, first)
{
	//console.log('admin_select_cb idx='+ idx +' path='+ path +' first='+ first);
	idx = +idx;
	if (idx != -1) {
      // if first time don't save, otherwise always save
      var save = isArg(first)? (first? false : true) : true;
		ext_set_cfg_param(path, idx, save);
	}
}

function admin_slider_cb(path, val, done, first)
{
   if (!done || first) return;
	//console.log('admin_slider_cb path='+ path +' val='+ val);
	val = +val;
   ext_set_cfg_param(path, val, EXT_SAVE);
}

function admin_preview_status_box(id, val)
{
	var s = w3_json_to_html('admin_preview_status_box:'+ id, val);
	if (!s || s == '') s = '&nbsp;';
	return s;
}
