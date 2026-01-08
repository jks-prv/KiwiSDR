
// Copyright (c) 2022-2026 John Seamons, ZL4VO/KF6VO

var kmap = {
   ADD_TO_MAP: true,
   NO_ADD_TO_MAP: false,
   DIR_LEFT: true,
   DIR_RIGHT: false,
   VISIBLE: true,
   NOT_VISIBLE: false,
   
   NO_CONTAINER: -1,

   _last_last: 0
};

function kiwi_map_init(ext_name, init_latlon, init_zoom, mapZoom_nom, move_cb, zoom_cb)
{
   var map_tiles;
   var maxZoom = 19;
   var server_e = { MapTiler_Vector:0, MapTiler_Raster_512:1, MapTiler_Raster_256:2, OSM_Raster:3 };
   var server = server_e.OSM_Raster;

	
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

   // MapTiler vector tiles using LeafletGL/MapBoxGL
   if (server == server_e.MapTiler_Vector) {
      map_tiles = function(map_style) {
         return L.mapboxGL({
            attribution: '<a href="https://www.maptiler.com/license/maps/" target="_blank">&copy; MapTiler</a> <a href="https://www.openstreetmap.org/copyright" target="_blank">&copy; OpenStreetMap contributors</a>',
            accessToken: 'not-needed',
            style: 'https://api.maptiler.com/maps/'+ map_style +'/style.json'+ 'key'
         });
      };
   }

   // MapTiler 512/256 px raster tiles
   if (server == server_e.MapTiler_Raster_512 || server == server_e.MapTiler_Raster_256) {
      var slash_256 = (server == server_e.MapTiler_Raster_256)? '/256':'';
      map_tiles = function(map_style) {
         return L.tileLayer(
            (map_style == 'hybrid')?
               'https://api.maptiler.com/maps/'+ map_style + slash_256 +'/{z}/{x}/{y}{r}.jpg'+ 'key'
            :
               'https://api.maptiler.com/maps/'+ map_style + slash_256 +'/{z}/{x}/{y}.png'+ 'key', {
            tileSize: (server == server_e.MapTiler_Raster_256)? 256 : 512,
            zoomOffset: (server == server_e.MapTiler_Raster_256)? 0 : -1,
            attribution: '<a href="https://www.maptiler.com/copyright/" target="_blank">&copy; MapTiler</a> <a href="https://www.openstreetmap.org/copyright" target="_blank">&copy; OpenStreetMap contributors</a>',
            crossOrigin: true
         });
      };
   }

   var tiles = map_tiles('hybrid');
   var map = L.map('id-'+ ext_name +'-map',
      {
         maxZoom: maxZoom,
         minZoom: 1,
         doubleClickZoom: false,    // don't interfere with double-click of host/ref markers
         zoomControl: false
      }
   ).setView(init_latlon, init_zoom);
   
   // NB: hack! jog map slightly to prevent grey, unfilled tiles after basemap change
   map.on('baselayerchange', function(e) {
      var tmap = e.target;
      console.log(tmap);
      var center = tmap.getCenter();
      kiwi_map_int_pan_zoom(tmap, [center.lat + 0.1, center.lng], -1);
      kiwi_map_int_pan_zoom(tmap, [center.lat, center.lng], -1);
   });
   
   map.attributionControl.setPosition('bottomleft');
   
   var kmap = { ext_name: ext_name, map: map };

   map.on('click', function(e, kmap) { w3_call(ext_name +'_map_click_cb', kmap, e); });
   map.on('move', function(e, kmap) { w3_call(ext_name +'_map_move_cb', kmap, e); });
   map.on('zoom', function(e, kmap) { w3_call(ext_name +'_map_zoom_cb', kmap, e); });
   map.on('mousemove', function(e, kmap) { w3_call(ext_name +'_map_mouse_move_cb', kmap, e); });
   map.on('moveend', function(e, kmap) { w3_call(ext_name +'_map_move_end_cb', kmap, e); });
   map.on('zoomend', function(e, kmap) { w3_call(ext_name +'_map_zoom_end_cb', kmap, e); });
   
   L.control.zoom_Kiwi( { position: 'topleft', zoomNomText: '2', zoomNomLatLon: init_latlon } ).addTo(map);
   tiles.addTo(map);

   // MapTiler map choices
   if (server != server_e.OSM_Raster) {
      L.control.layers(
         {
            'Satellite': tiles,
            'Basic': map_tiles('basic'),
            'Bright': map_tiles('bright'),
            'Positron': map_tiles('positron'),
            'Street': map_tiles('streets'),
            'Topo': map_tiles('topo')
         },
         null
      ).addTo(map);
   }

   var scale = L.control.scale();
   scale.addTo(map);
   scale.setPosition('bottomleft');

   L.control.ruler({ position: 'bottomleft' }).addTo(map);

   var day_night = new Terminator();
   day_night.setStyle({ fillOpacity: 0.35 });
   day_night.addTo(map);
   kmap.day_night_interval = setInterval(function() {
      var t2 = new Terminator();
      day_night.setLatLngs(t2.getLatLngs());
      day_night.redraw();
   }, 60000);
   day_night._path.style.cursor = 'grab';

   var graticule = L.latlngGraticule();
   graticule.addTo(map);

   kmap.map_layers = [ graticule ];
   kmap.graticule = graticule;
   kmap.day_night = day_night;
   kmap.init_latlon = init_latlon;
   kmap.init_zoom = init_zoom;
   kmap.mapZoom_nom = mapZoom_nom;
   kmap.all_markers = [];
   kmap.submit_status = kiwi_map_id(kmap, 'submit-status');
   
   return kmap;
}

function kiwi_map_blur(kmap)
{
   kiwi_clearInterval(kmap.day_night_interval);
}

function kiwi_map_id(kmap, s) { return w3_sbc('-', 'id', kmap.ext_name, s); }


////////////////////////////////
// map
////////////////////////////////

function kiwi_map_div_icon(opts)
{
   var props = {};
   w3_obj_mix(props, opts);
   return new L.DivIcon(props);
}

function kiwi_map_add_marker_url(kmap, addToMap, url, latlon, anchor, opacity, func)
{
   var icon =
      L.icon({
         iconUrl: url,
         iconAnchor: anchor
      });

   var marker = L.marker(latlon, { 'icon':icon, 'opacity':opacity });
   //console.log(marker);
   if (addToMap) {
      marker.addTo(kmap.map);
      if (func) func(marker._icon);
   }
}

function kiwi_map_add_marker_div(kmap, addToMap, latlon, icon_opts, marker_opts)
{
   var iprops = { className:'', html:'', iconAnchor:[12, 12], tooltipAnchor:[0, 0] };
   w3_obj_mix(iprops, icon_opts);
   var icon = L.divIcon(iprops);

   var mprops = { icon:icon, opacity:1.0 };
   w3_obj_mix(mprops, marker_opts);
   var marker = L.marker(latlon, mprops);

   //console.log('kiwi_map_add_marker_div: addToMap='+ addToMap +' iprops,mprops,marker=(next)');
   //console.log(iprops); console.log(mprops); console.log(marker);
   if (addToMap) marker.addTo(kmap.map);
   return marker;
}

function kiwi_style_marker(kmap, addToMap, marker, mkr_name, useTooltip, className, left, func)
{
   if (useTooltip) {
      marker.bindTooltip(mkr_name,
         {
            className:  className,
            permanent:  true, 
            direction:  left? 'left' : 'right'
         }
      );
   }
   
   // Can only access element to add title via an indexed id.
   // But element only exists as marker emerges from cluster.
   // Fortunately we can use 'add' event to trigger insertion of title.
   //console.log('style');
   //console.log(marker);
   if (func) marker.on('add', function(ev) {
      func(ev);
   });
   
   // only add to map in cases where L.markerClusterGroup() is not used
   if (addToMap) {
      //console.log(marker);
      marker.addTo(kmap.map);
      kmap.map_layers.push(marker);
      kmap.all_markers.push(marker);
      //console.log('marker '+ mkr_name +' x,y='+ map.latLngToLayerPoint(marker.getLatLng()));
   }
}

function kiwi_map_remove_all_markers(kmap)
{
   if (kmap.all_markers) {
      kmap.all_markers.forEach(function(ml) {
         if (ml) ml.remove();
      });
      kmap.all_markers = [];
   }
}

function kiwi_map_day_night_visible(kmap, vis)
{
   var day_night = kmap.day_night;
   if (vis) {
      day_night.addTo(kmap.map);
      kmap.map_layers.push(day_night);
      day_night._path.style.cursor = 'grab';
   } else {
      day_night.remove();
      kmap.map_layers = kmap.map_layers.filter(function(ae) { return ae != day_night; });
   }
}

function kiwi_map_graticule_visible(kmap, vis)
{
   var graticule = kmap.graticule;
   if (vis) {
      graticule.addTo(kmap.map);
      kmap.map_layers.push(graticule);
   } else {
      graticule.remove();
      kmap.map_layers = kmap.map_layers.filter(function(ae) { return ae != graticule; });
   }
}

function kiwi_map_markers_visible(id, vis)
{
	w3_iterate_children('leaflet-marker-pane',
	   function(el, i) {
	      if (el.className.includes(id)) {
	         w3_hide2(el, !vis);
	      }
	   }
	);

	w3_iterate_children('leaflet-tooltip-pane',
	   function(el, i) {
	      if (el.className.includes(id)) {
	         w3_hide2(el, !vis);
	      }
	   }
	);
}

function kiwi_map_int_pan_zoom(map, latlon, zoom)
{
   //console.log('kiwi_map_int_pan_zoom z='+ zoom);
   //console.log(map);
   //console.log(latlon);
   //console.log(zoom);
   if (latlon == null) latlon = map.getCenter();
   if (zoom == -1) zoom = map.getZoom();
   map.setView(latlon, zoom, { duration: 0, animate: false });
}

function kiwi_map_pan_zoom(kmap, latlon, zoom)
{
   kiwi_map_int_pan_zoom(kmap.map, latlon, zoom);
}

////////////////////////////////
// UI helpers
////////////////////////////////

// field_idx = kmap.NO_CONTAINER if the icon isn't in a container
function kiwi_map_set_icon(km, name, field_idx, icon, size, color, title, cb, cb_param)
{
   var isContainer = (field_idx > kmap.NO_CONTAINER);
   var el_tag = kiwi_map_id(km, name +'-icon');
   var el_cont = el_tag +'-c'+ (isContainer? field_idx : '');
   //console.log('kiwi_map_set_icon field_idx='+ field_idx +' el_tag='+ el_tag +' el_cont='+ el_cont);
   
   w3_innerHTML(el_cont,   // icon container
      (icon == '')? '' :
      w3_icon(el_tag, icon, size, color, cb, cb_param)
   );
   
   // all icon elements in the container have "id-{ext_name}-{name}-icon" so use w3_els() to find them
   if (isString(title))
      w3_els(el_cont, function(el) { w3_title(el, title); });
}

// field_idx = kmap.NO_CONTAINER if the icon isn't in a container
function kiwi_map_set_icon_color(km, name, field_idx, color, background)
{
   var isContainer = (+field_idx > kmap.NO_CONTAINER);
   var el_tag = kiwi_map_id(km, name +'-icon');
   var el_cont = el_tag +'-c'+ (isContainer? field_idx : '');
   //console.log('kiwi_map_set_icon_color field_idx='+ field_idx +' el_tag='+ el_tag +' el_cont='+ el_cont);

   if (isContainer) {
      // container:
      // apply color to all child elements of container
      w3_iterate_children(el_cont, function(el) {
         //console.log('container');
         //console.log(el);
         w3_color(el, color, background);
      });
   } else {
      // no container:
      // all icon elements should have "id-{ext_name}-{name}-icon" tag, so use w3_els(el_tag) to find them
      w3_els(el_tag, function(el) {
         //console.log('not container');
         //console.log(el);
         w3_color(el, color, background);
      });
   }
}


////////////////////////////////
// waterfall preview
////////////////////////////////

function kiwi_map_preview_click(kmap, host, ev, opt)
{
   kmap.wf_opt = opt;
   if (kmap.wf_ws != null) {
      // there is an existing preview shown (wf_ws web socket active)
      kmap.wf_host = host;
      // must delay call to kiwi_map_wf_preview() until kiwi_map_waterfall_close() has been called
      kmap.wf_ws.close();
   } else {
      var auto = (wf.aper == kiwi.APER_AUTO);
      kmap.aper_save = wf.aper;

      // must also save/restore man maxdb/mindb_un, source depends on aperture mode
      kmap.maxdb_save = auto? wf.save_maxdb : maxdb;
      kmap.mindb_un_save = auto? wf.save_mindb_un : mindb_un;
      //console.log('$kiwi_map_preview_click aper SAVE '+ (auto? 'AUTO' : 'MAN') +' maxdb='+ kmap.maxdb_save +' mindb='+ kmap.mindb_un_save);
      kiwi_map_wf_preview(kmap, host);
   }
}

function kiwi_map_all_msg_cb(msg_a, ws)
{
   var kmap = ws.gen_cb_param;
   var cmd = msg_a[0];
   var param = isString(msg_a[1])? msg_a[1] : '';
   console.log('kiwi_map_all_msg_cb: cmd='+ cmd +' param='+ param);
   var reason = 'Connection failed';
   
   switch (cmd) {
      case 'badp':
         if (+param != 0) {
            //console.log('kmap_wf BADP='+ param);
            kmap.wf_conn_bad = true;
         }
         break;
      
      case 'rx_chan':
         kmap.rx_chan = +param;
         break;

      case 'wf_chans':
         kmap.wf_chans = +param;
         break;

      case 'monitor':
         //console.log('kmap_wf MONITOR');
         reason = 'Preview: all channels full';
         kmap.wf_conn_bad = true;
         break;

      case 'wf_setup':
         //console.log('kmap_wf '+ cmd);
         if (kmap.wf_up) break;
         if (kmap.rx_chan >= kmap.wf_chans) {
            console.log(sprintf('no wf chan: rx_chan(%d) >= wf_chans(%d)', kmap.rx_chan, kmap.wf_chans));
            if (kmap.wf_ws) kmap.wf_ws.close();
            reason = 'Preview: no WF channel';
            kmap.wf_conn_bad = true;
         } else {
            kmap.wf_up = true;
            waterfall_add_line(wf_canvas_actual_line+1);
            var c = wf_cur_canvas;
            var x = freq_to_pixel(freq_passband_center());
            waterfall_add_text(wf_canvas_actual_line+4, x, 12, kmap.wf_id, 'Arial', 14, 'white');
         }
         break;

      case 'maxdb':
      case 'mindb':
         //console.log('kmap_wf ACCEPT: '+ cmd +' '+ param);
         kiwi_msg(msg_a, ws);
         break;

      default:
         //console.log('kmap_wf IGNORE: '+ cmd +' '+ param);
         break;
   }
   
   if (kmap.wf_conn_bad) {
      if (kmap.wf_ws) kmap.wf_ws.close();
      w3_innerHTML(kmap.submit_status, reason +' '+ kmap.wf_id2);
   }
   return true;
}

function kiwi_map_waterfall_add_queue(what, ws, firstChars)
{
   //console.log('kiwi_map_waterfall_add_queue');
   var kmap = ws.gen_cb_param;
   if (kiwi.wf_preview_mode) {
      waterfall_add_queue2(what, ws, firstChars);
      kmap.preview_lines++;
      if (kmap.preview_lines == 2) {
         wf_aper_cb('wf.aper', kiwi.APER_MAN);
         setTimeout(wf_autoscale_cb, 1);
      }
   } else {
	   if (kiwi_gc_wf) what = null;  // gc
	}
}

function kiwi_map_waterfall_close(kmap_or_ws)
{
   var kmap;
   if (kmap_or_ws.gen_cb_param)
      kmap = kmap_or_ws.gen_cb_param;
   else
      kmap = kmap_or_ws;
   
   //console.log('kiwi_map_waterfall_close');
   kiwi_clearTimeout(kmap.preview_timeo);

   if (kmap.wf_ws != null && kmap.wf_host != null) {
      // there is another preview request queued up
      var host = kmap.wf_host;
      kmap.wf_host = null;
      kiwi_map_wf_preview(kmap, host);
   } else {
      kmap.wf_ws = kmap.wf_host = null;
      kiwi.wf_preview_mode = false;
      var auto = (wf.aper == kiwi.APER_AUTO);
      //console.log('kiwi_map aper RESTORE '+ (auto? 'AUTO' : 'MAN') +' maxdb='+ kmap.maxdb_save +' mindb='+ kmap.mindb_un_save);
      
      // restore for both cases: returning to auto or man aperture mode
      if (isArg(kmap.maxdb_save)) wf.save_maxdb = maxdb = kmap.maxdb_save;
      if (isArg(kmap.mindb_un_save)) wf.save_mindb_un = mindb_un = kmap.mindb_un_save;
      if (isArg(kmap.aper_save)) wf_aper_cb('wf.aper', kmap.aper_save);

      if (!kmap.wf_conn_bad) {
         w3_clearInnerHTML(kmap.submit_status);
      }

      if (kmap.wf_up) {
      
         // need to set correct start bin on this Kiwi so wf fixup logic doesn't get stuck
         wf_send('SET zoom='+ zoom_level +' start='+ x_bin);
         waterfall_add_line(wf_canvas_actual_line+1);
         var c = wf_cur_canvas;
         var id = 'Back to this Kiwi';
         var x = freq_to_pixel(freq_passband_center());
         waterfall_add_text(wf_canvas_actual_line+4, x, 12, id, 'Arial', 14, 'white');
      }
   }

   kmap.wf_up = false;
}

function kiwi_map_wf_preview(kmap, h)
{
   //console.log(h);
   // NB: always include port in xxx.proxy.kiwisdr.com:8073 because otherwise
   // redirection fails with open_websocket()
   kmap.wf_url = h.hp;
   kmap.wf_url_full = h.h +':'+ h.p;
   if (kmap.wf_opt && kmap.wf_opt.show_url) {
      kmap.wf_id = h.hp +' ('+ h.snr +')';
      kmap.wf_id2 = h.hp;
   } else {
      kmap.wf_id = h.id_snr;
      kmap.wf_id2 = h.hp +' ('+ h.snr +')';
   }
   w3_innerHTML(kmap.submit_status, 'Waterfall preview: '+ kmap.wf_id2);

   kmap.wf_ws = open_websocket('W/F',
      function() {   // open_cb
         //setTimeout(function() {
         kmap.preview_lines = 0;
         kmap.wf_ws.send("SET auth t=kiwi");
         kmap.wf_ws.send("SERVER DE CLIENT openwebrx.js W/F");
         kmap.wf_ws.send("SET ident_user=WF_preview");
         kmap.wf_ws.send("SET send_dB=1");
         
         // Need to send the start bin, not cf, otherwise the wf preview autoscale
         // doesn't work due to interaction with wf pan/zoom fixup
         //kmap.wf_ws.send("SET zoom=10 cf="+ freq_displayed_kHz_str_with_freq_offset);
         kmap.wf_ws.send('SET zoom='+ zoom_level +' start='+ x_bin);

         kmap.wf_ws.send("SET maxdb=0 mindb=-100");
	      kmap.wf_ws.send("SET wf_speed=3");

         kmap.preview_timeo = setTimeout(function() { if (kmap.wf_ws) kmap.wf_ws.close(); }, 10000);
         //}, 5000);
      },
      kmap,                         // open_cb_param
      null,                         // msg_cb, instead msgs sent to all_msg_cb below
      kiwi_map_waterfall_add_queue, // recv_cb
      kiwi_map_wf_preview_error_cb, // error_cb
      kiwi_map_waterfall_close,     // close_cb
      // opt
      { url: kmap.wf_url_full, new_ts: true, qs: '', all_msg_cb: kiwi_map_all_msg_cb, gen_cb_param: kmap, trace:1 }
   );

   kmap.wf_conn_bad = false;
   kiwi.wf_preview_mode = true;
}

function kiwi_map_wf_preview_error_cb(ev, ws)
{
   console.log('kiwi_map_wf_preview_error_cb..');
   console.log(ev);
   console.log(ws);
   console.log('..kiwi_map_wf_preview_error_cb');
}


////////////////////////////////
// TBD FIXME
////////////////////////////////

/* jksx not referenced?
function hfdl_ref_marker_offset(doOffset)
{
   if (!hfdl.known_location_idx) return;
   var r = hfdl.refs[hfdl.known_location_idx];

   //if (doOffset) {
   if (false) {   // don't like the way this makes the selected ref marker inaccurate until zoom gets close-in
      // offset selected reference so it doesn't cover solo (unclustered) nearby reference
      var m = hfdl.cur_map;
      var pt = m.latLngToLayerPoint(L.latLng(r.lat, r.lon));
      pt.x += 20;    // offset in x & y to better avoid overlapping
      pt.y -= 20;
      r.mkr.setLatLng(m.layerPointToLatLng(pt));
   } else {
      // reset the offset
      r.mkr.setLatLng([r.lat, r.lon]);
   }
}

function hfdl_marker_click(mkr)
{
   //console.log('hfdl_marker_click');

   var r = mkr.kiwi_mkr_2_ref_or_host;
   var t = r.title.replace('\n', ' ');
   var loc = r.lat.toFixed(2) +' '+ r.lon.toFixed(2) +' '+ r.id +' '+ t + (r.f? (' '+ r.f +' kHz'):'');
   w3_set_value('id-hfdl-known-location', loc);
   if (hfdl.known_location_idx) {
      var rp = hfdl.refs[hfdl.known_location_idx];
      rp.selected = false;
      console.log('ref_click: deselect ref '+ rp.id);
      hfdl_ref_marker_offset(false);
   }
   hfdl.known_location_idx = r.idx;
   r.selected = true;
   console.log('ref_click: select ref '+ r.id);
   if (r.f)
      ext_tune(r.f, 'iq', ext_zoom.ABS, r.z, -r.p/2, r.p/2);
   hfdl_update_link();
   hfdl_ref_marker_offset(true);
}
*/

////////////////////////////////
// end map
////////////////////////////////
