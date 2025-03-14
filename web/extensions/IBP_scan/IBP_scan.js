// Copyright (c) 2017 Peter Jennings, VE3SUN

var ibp = {
   scan_ext_name: 'IBP_scan',    // NB: must match IBP_scan.c:ibp_scan_ext.name
   first_time: true,

   SLOTS: 18,     // number of slots/stations
   ALL: 20,
   BANDS: 30,

   run: false,
   annotate: true,
   autosave: false,
   mindb_band: [],
   canvasSaved: false,
   oldSlot: -1,
   monitorBeacon: -1,
   sound: false,
   band: 0,
   bands_s: [ "IBP 20m", "IBP 17m", "IBP 15m", "IBP 12m", "IBP 10m" ],
   freqs: [ '14.100', '18.110', '21.150', '24.930', '28.200' ]
};

function IBP_scan_main()
{
   //console.log('IBP_scan_main');
   ext_switch_to_client(ibp.scan_ext_name, ibp.first_time, ibp_recv_msg);  // tell server to use us (again)
   if (!ibp.first_time)
      ibp_controls_setup();
   ibp.first_time = false;
}

function ibp_recv_msg(data)
{
   // process command sent from server/C by ext_send_msg() or ext_send_msg_encoded()
   var stringData = arrayBufferToString(data);
   var params = stringData.substring(4).split(" ");

   for (var i=0; i < params.length; i++) {
      var param = params[i].split("=");

      switch (param[0]) {

         case "ready":
            ibp_controls_setup();
            break;

         default:
            console.log('ibp_recv: UNKNOWN CMD '+ param[0]);
            break;
      }
   }
}

function ibp_controls_setup()
{
   var i;

   var data_html =
      time_display_html('IBP_scan') +
      w3_div('id-IBP-report|width:1024px; height:200px; overflow:hidden; position:relative; background-color:white;',
         '<canvas id="id-IBP-canvas" width="1024" height="200" style="position:absolute"></canvas>'
      );
   
   var select = { 'IBP': { value: -2, disabled: 1, selected: 1 }, 'OFF': { value: -1 } };
   w3_obj_enum(dx_ibp_stations, function(key, i, o) {
      select[key] = { value: o.value, text: o.text };
   });
   select['By band:'] = { value: -1, disabled: 1, text: '<b>By band:</b>' };
   select['All Bands'] = { value: ibp.ALL, text: 'All Bands' };
   ibp.bands_s.forEach(function(s, i) {
      select[s] = { value: ibp.BANDS+i, text: s };
   });

   var controls_html =
      w3_div('id-tc-controls w3-text-white',
         w3_div('w3-medium w3-text-aqua',
            '<b><a href="http://www.ncdxf.org/beacon/index.html">International Beacon Project</a> (IBP) Scanner</b>'
         ),

         w3_col_percent('w3-margin-T-4',
            w3_div('', 'by VE3SUN'), 25,
            w3_div('', 'Info: <b><a href="http://ve3sun.com/KiwiSDR/IBP.html" target="_blank">ve3sun.com/KiwiSDR/IBP</a></b>'), 55,
            '', 10
         ),
         
         w3_inline('w3-halign-space-between w3-margin-T-8|width:90%;/',
            w3_select('id-IBP-menu w3-left w3-margin-right w3-show-inline', '', '', '', 0, select, 'IBP_menu_cb'),
            w3_checkbox('id-IBP-annotate w3-label-inline w3-label-not-bold', 'Annotate Waterfall', 'ibp.annotate', true, 'w3_bool_cb'),
            w3_checkbox('id-IBP-autosave w3-label-inline w3-label-not-bold', 'Autosave PNG', 'ibp.autosave', false, 'IBP_Autosave')
         )
      );
   
   //console.log('ibp_controls_setup');
   ext_panel_show(controls_html, data_html, null);
   ext_set_controls_width_height(475, 90);
   time_display_setup('IBP_scan');
	IBP_environment_changed( {resize:1} );
   
   ibp.autosave = kiwi_storeRead('IBP_PNG_Autosave');
   if (ibp.autosave != 'true') ibp.autosave = false;

   // Use extension parameter as beacon station call.
   // Or 'cycle' or 'all' for cycle mode.
   // Or band name/freq, e.g. '15m', '21'
   // e.g. kiwisdr.local:8073/?ext=ibp,4u1un (upper or lowercase)
   var p = ext_param();
	if (p) {
      p = p.split(',');
      p.forEach(function(a, i) {
         //console.log('IBP param1 <'+ a +'>');
         if (i == 0) {
            var call = a.toLowerCase();
            var idx = -1;
            w3_obj_enum(dx_ibp_stations, function(key, i, o) {
               if (key.toLowerCase() == call)
                  idx = i;
            });
            if (idx == -1) {
               if (call == 'cycle' || call == 'all') {
                  idx = ibp.ALL;
               } else {
                  ibp.bands_s.forEach(function(s, i) {
                     if (s.includes(call))
                        idx = ibp.BANDS + i;
                  });
                  if (idx == -1) ibp.freqs.forEach(function(s, i) {
                     if (s.includes(call))
                        idx = ibp.BANDS + i;
                  });
               }
            }
            if (idx != -1) {
               console.log('IBP: URL set '+ call);
               console.log('IBP URL_param='+ idx);
               IBP_menu_cb('', idx);
            }
         } else {
            var r;
            if ((r = w3_ext_param('annotate', a)).match) {
               ibp.annotate = (r.num == 0)? false : true;
            } else
            if ((r = w3_ext_param('autosave', a)).match) {
               ibp.autosave = (r.num == 0)? false : true;
            } else
            if (w3_ext_param('help', a).match) {
               ext_help_click();
            }
         }
      });
   }

   w3_checkbox_set('id-IBP-annotate', ibp.annotate);
   w3_checkbox_set('id-IBP-autosave', ibp.autosave);
      
   var canv = w3_el('id-IBP-canvas');
   var label = '';
   if (canv) {
      var ctx = canv.getContext("2d");
      ctx.fillStyle="#ffffff";
      ctx.fillRect(0,0,1024,200);
      
      ctx.font = "12px Arial";
      ctx.fillStyle = "red";
      ctx.textAlign = "center";
      
      w3_obj_enum(dx_ibp_stations, function(key, i, o) {
         ctx.fillText(key, 102 + o.value*51, 16); 
      });

      for (i=0; i < 5; i++) {
         label = ibp.freqs[i];
         ctx.fillText(label, 45, 36*i+40); 
      }
   }

   var cookie = kiwi_storeRead('mindb_band');
   if (cookie) {
      var obj = kiwi_JSON_parse('ibp_controls_setup', cookie);
      if (obj) ibp.mindb_band = obj;
   }

   ibp.run = true;
}

function IBP_environment_changed(changed)
{
   if (!changed.resize) return;
   var el = w3_el('id-IBP-report');
   var left = (window.innerWidth - 1024 - time_display_width()) / 2;
   el.style.left = px(left);
}

function IBP_scan_blur()
{
   //console.log('IBP_scan_blur');
   IBP_menu_cb('', -1);
   ibp.run = false;
}

function IBP_Autosave(path, checked)
{
   ibp.autosave = checked? true:false;
   kiwi_storeWrite('IBP_PNG_Autosave', ibp.autosave);
}

// If menu has ever been selected then we restore band to 20m on blur,
// else leave alone so e.g. zoom won't change.

function IBP_menu_cb(path, v, first)    // called by IBP selector with beacon value
{
   if (first) return;
   v = +v;
   //console.log('IBP_menu_cb v='+ v);
   w3_el('id-IBP-menu').value = v;     // for benefit of direct callers
   var selected = (v >= 0);
   ibp.band = 0;
   ibp.monitorBeacon = v;

   if (v >= ibp.BANDS) {
      ibp.band = v-ibp.BANDS;
      //console.log('IBP_menu_cb MENU band='+ ibp.band);
   } else

   if (v < 0) {    // menu = "off"
      w3_el('id-IBP-menu').selectedIndex = 0;
      if (selected) {
         ibp.band = 0;
         //console.log('IBP_menu_cb MENU=off ibp.band=0');
      }
   }

   select_band(ibp.bands_s[ibp.band]);
   //console.log('IBP_menu_cb ibp.band='+ ibp.band +' ibp.monitorBeacon='+ ibp.monitorBeacon);
}

   
function IBP_bandchange(d, BeaconN)
{
   if (ibp.monitorBeacon == ibp.ALL) {
      var band = Math.floor(d.getTime() / 180000) % 5; // 3 min per band each 15 min

      if (band != ibp.band) {
         ibp.band = band;
         select_band(ibp.bands_s[ibp.band]);
         return ibp.band;
      }
      return false;
   }

   if (ibp.monitorBeacon != (BeaconN + ibp.SLOTS-1) % ibp.SLOTS) return false;
   
   ibp.band++;
   ibp.band %= 5;
   select_band(ibp.bands_s[ibp.band]);
   return ibp.band;
}

function ibp_save_Canvas(d)
{
   var canv = w3_el('id-IBP-canvas');
   if (canv) {
      var ctx = canv.getContext("2d");
      ctx.fillStyle = "black";
      ctx.fillText(d.getUTCHours().leadingZeros() +':'+ d.getUTCMinutes().leadingZeros() +' UTC', 40, 16); 

      var imgURL = canv.toDataURL("image/png");
      var dlLink = document.createElement('a');
      dlLink.download = 'IBP '+ d.getUTCFullYear() + (d.getUTCMonth()+1).leadingZeros() + d.getUTCDate().leadingZeros() +' '+
         d.getUTCHours().leadingZeros() +'h'+ d.getUTCMinutes().leadingZeros() +'Z.png';
      dlLink.href = imgURL;
      dlLink.dataset.downloadurl = ["image/png", dlLink.download, dlLink.href].join(':');
   
      document.body.appendChild(dlLink);
      dlLink.click();
      document.body.removeChild(dlLink);
      ctx.fillStyle = "#ffffff";
      ctx.fillRect(0,0,75,19);
   }
}

function ibp_annotate_waterfall(beaconN)
{
   var c = wf_cur_canvas;
   waterfall_add_line(wf_canvas_actual_line+1);
   
   var call, location;
   w3_obj_enum(dx_ibp_stations, function(key, i, o) {
      if (i == beaconN) { call = key; location = o.text; }
   });

   var sL = call +' '+ location;
   waterfall_add_text(wf_canvas_actual_line+4, c.width/2, 12, sL, 'Arial', 14, 'lime');
}

// called every waterfall update
function IBP_scan_plot(oneline_image)
{
   if (!ibp.run) return;
   var canv = w3_el('id-IBP-canvas');
   if (!canv) return;

   var ctx = canv.getContext("2d");
   var subset = new ImageData(1,1);
   
   var d = new Date(dx_ibp_server_time_ms + (Date.now() - dx_ibp_local_time_epoch_ms));
   var msec = d.getTime();
   var bsec = Math.floor((msec % 10000) / 200);    // for 50 pixel slot image
   var slot = Math.floor(msec/10000) % ibp.SLOTS;
      
   var f = get_visible_freq_range();
   var fb = Math.floor((f.center - 14e6) / 3e6);
   var plot_y = 20 + 36*fb;
   
   var beaconN = (slot - fb + ibp.SLOTS) % ibp.SLOTS;  // actual beacon transmitting
   var plot_x = 76 + 51 * beaconN;
   //console.log('IBP slot='+ slot +' x='+ plot_x +' y='+ plot_y);
   
   if (ibp.autosave) {
      if (slot) {
         ibp.canvasSaved = false;
      } else {
         if (!ibp.canvasSaved) {
            ctx.fillStyle="red";
            ctx.fillRect(plot_x-1,plot_y,1,35);  // mark save time on canvas

            ibp_save_Canvas(d);
            ibp.canvasSaved = true;

            ctx.fillStyle="#ffffff";
            ctx.fillRect(plot_x-1,plot_y,1,35);  // unmark it
         }
      }     
   }

   if ((ibp.oldSlot > -1) && (ibp.oldSlot != slot)) {
      if (ibp.annotate)
         ibp_annotate_waterfall((beaconN + ibp.SLOTS-1) % ibp.SLOTS); 
      
      var new_band = IBP_bandchange(d, beaconN);
      if (new_band !== false) {  // returns new band if band changed, else false
         if (ibp.mindb_band[new_band] && (mindb != ibp.mindb_band[new_band]))
            setmindb(true,ibp.mindb_band[new_band]);
         ibp.oldSlot = -2;
         return;
      }
   }

   if (ibp.oldSlot != slot) {
      if (kiwi.muted && (ibp.monitorBeacon == beaconN)) {
         toggle_or_set_mute();
         setTimeout(function() { toggle_or_set_mute(); }, 50000);
      }
      ctx.fillStyle = "#000055";
      ctx.fillRect(plot_x,plot_y,50,35);
   } else {
      if (ibp.mindb_band[fb] != mindb) { 
         ibp.mindb_band[fb] = mindb;
         kiwi_storeWrite('mindb_band', JSON.stringify(ibp.mindb_band));
      }
   }

   for (var i = 495; i < 530; i++) {
      for (var j = 0; j < 4; j++) {
        subset.data[j] = oneline_image.data[4*i+j];
        ctx.putImageData(subset, plot_x+bsec, plot_y+i-495);
     }
   }

   ibp.oldSlot = slot;
}

function IBP_scan_help(show)
{
   if (show) {
      var s = 
         w3_text('w3-medium w3-bold w3-text-aqua', 'IBP scanner help') +
         w3_div('w3-margin-T-8 w3-scroll-y|height:90%',
            w3_div('w3-margin-R-8',
               '<br>URL parameters: <br>' +
               'First parameter can be an entry from the IBP menu: One of the station callsigns. <br>' +
               'Or a band or frequency (MHz) entry, e.g. "20m", "28". Or "all" or "cycle" to scan all bands. <br>' +
               'The two checkbox values can also be set. Use a num value of "1" to set the checkbox. <br>' +
               w3_text('|color:orange', 'annotate:<i>num</i> &nbsp; autosave:<i>num</i>') +
               '<br>Keywords are case-insensitive and can be abbreviated (except for callsigns). <br>' +
               'So for example these are valid: <i>ext=ibp,all</i> &nbsp; <i>ext=ibp,zl6b,auto:1</i> &nbsp; <i>ext=ibp,15m</i>  &nbsp; <i>ext=ibp,28</i> <br>' +
               ''
            )
         );
      confirmation_show_content(s, 630, 200);
      w3_el('id-confirmation-container').style.height = '100%';   // to get the w3-scroll-y above to work
   }
   return true;
}
