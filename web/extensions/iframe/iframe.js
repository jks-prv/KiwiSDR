// Copyright (c) 2020-2025 Kari Karvonen, OH1KK
// Copyright (c) 2020-2025 John Seamons, ZL4VO/KF6VO

var iframe = {
   ext_name: 'iframe',  // NB: must match iframe.cpp:iframe_ext.name
   first_time: true,
   inst: 0,             // NB: not a cfg param, just the instance menu selection
   n_menu: 16,
   menu: null,
   allow_tune: false,
   msg_handler: null,
   
   std: [
      // must be first in list
      { src:0, url:'//spots.kiwisdr.com', menu:'DX spots', w:400, h:500, tune:1,
         title:'<span style="color:cyan">Spots by <a href="//www.sk6aw.net/cluster" target="_blank">SK6AW.NET</a></span>',
         help:'Clicking on a spot frequency will tune the Kiwi.'
      },
         
      { src:1, menu:'solar cond', w:175, h:555, tune:0, title:'Solar cond', help:'',
         html:'<center><a href="//www.hamqsl.com/solar.html" title="Click to add Solar-Terrestrial Data to your website!" target="_blank"><img src="//www.hamqsl.com/solarn0nbh.php"></a></center>'
      },
         
      { src:0, url:'//api.meteoagent.com/widgets/v1/kindex', menu:'K-index', w:400, h:600, tune:0, title:'Solar K-index', help:''
      },
         
      { src:0, url:'//www.sws.bom.gov.au/Images/HF Systems/Global HF/Ionospheric Map/WorldIMap.gif', menu:'HF-prop', w:500, h:500,
         tune:0, title:'HF propagation', help:''
      },
         
      { src:0, url:'//map.blitzortung.org/#2.0/-27.0/146', menu:'lightning', w:400, h:300, tune:0, title:'Live Lightning Map',
         help:'Click and hold on Map for repositioning, use +/- for Zoom.<br>Admin: change default zoom and lat/lon in URL.'
      },
         
      { src:0, url:'//services.swpc.noaa.gov/images/aurora-forecast-northern-hemisphere.jpg', menu:'aurora-N', w:0, h:0,
         tune:0, title:'Aurora northern hemisphere', help:''
      },
         
      { src:0, url:'//services.swpc.noaa.gov/images/aurora-forecast-southern-hemisphere.jpg', menu:'aurora-S', w:0, h:0,
         tune:0, title:'Aurora southern hemisphere', help:''
      }
   ],
   
   SRC_URL: 0,
   SRC_HTML: 1,
   
   CMD1: 0,
};

function iframe_main()
{
   ext_switch_to_client(iframe.ext_name, iframe.first_time, iframe_recv);     // tell server to use us (again)
   if (!iframe.first_time) iframe_controls_setup();
   iframe.first_time = false;
}

function iframe_str(el, inst)
{
   // iframeN.el
   var _inst = isNumber(inst)? inst : iframe.inst;
   return ('iframe'+ (_inst? _inst : '') +'.'+ el);
}

function iframe_get(el, inst, init_val, save)
{
   var el_s = iframe_str(el, inst);
   if (isDefined(init_val)) save = (save == EXT_SAVE)? EXT_SAVE : EXT_NO_SAVE;
   
   if (isDefined(init_val)) {
      return ext_get_cfg_param(el_s, init_val, save);
   } else {
      return ext_get_cfg_param(el_s);
   }
}

function iframe_get_string(el, inst, init_val, save)
{
   var el_s = iframe_str(el, inst);
   if (isDefined(init_val)) save = (save == EXT_SAVE)? EXT_SAVE : EXT_NO_SAVE;
   
   if (isDefined(init_val)) {
      return ext_get_cfg_param_string(el_s, init_val, save);
   } else {
      return ext_get_cfg_param_string(el_s);
   }
}

function iframe_set(inst, el, val)
{
   var el_s = iframe_str(el, inst);
   return ext_set_cfg_param(el_s, val);
}

function iframe_recv(data)
{
   var firstChars = arrayBufferToStringLen(data, 3);
   
   // process data sent from server/C by ext_send_data_msg()
   if (firstChars == "DAT") {
      var ba = new Uint8Array(data, 4);
      var cmd = ba[0];

      if (cmd == iframe.CMD1) {
         // do something ...
      } else {
         console.log('iframe_recv: DATA UNKNOWN cmd='+ cmd +' len='+ (ba.length-1));
      }
      return;
   }
   
   // process command sent from server/C by ext_send_msg() or ext_send_msg_encoded()
   var stringData = arrayBufferToString(data);
   var params = stringData.substring(4).split(" ");

   for (var i=0; i < params.length; i++) {
      var param = params[i].split("=");

      switch (param[0]) {

         case "ready":
            iframe_controls_setup();
            break;
         default:
            console.log('iframe_recv: UNKNOWN CMD '+ param[0]);
            break;
      }
   }
}

function iframe_controls_setup()
{
   // determine actual name in extension menu
   var ext_names = [];
   extint_enum_names(
      function(i, value, id, id_en) {
         //console.log('iframe LIST '+ i +'|'+ value +' '+ id);
         ext_names.push(id);
      }
   );
   
   // set instance number based on extension menu entry match (defaults to instance 0)
   var menu_idx = w3_el('id-select-ext').value;
   var menu_name = ext_names[menu_idx];
   //console.log('iframe NAME '+ menu_idx +' '+ menu_name);
   iframe.inst = 0;

   if (menu_name != 'iframe') {
      for (var i = 0; i < iframe.n_menu; i++) {
         var s = 'iframe' + (i? i : '');
         if (isDefined(cfg[s])) {
            var o = cfg[s];
            //console.log(o);
            if (isNonEmptyString(o.menu) && o.menu == menu_name) {
               //console.log('iframe MATCH inst='+ i);
               iframe.inst = i;
               break;
            }
         }
      }
   }
   
   iframe.src        = iframe_get('src');
   iframe.title      = iframe_get_string('title');
   iframe.helptext   = iframe_get_string('help');
   iframe.url        = iframe_get_string('url');
   iframe.html       = iframe_get_string('html');
   iframe.width      = iframe_get('width');
   iframe.height     = iframe_get('height');
   iframe.allow_tune = iframe_get('allow_tune');

   /* sanity checks */
   iframe.width = parseInt(iframe.width);
   iframe.height = parseInt(iframe.height);
   var margin = 20, top_line = 25; 
   if (iframe.width <= 0) iframe.width = 450;
   if (iframe.height <= 0) iframe.height = 450;
   if (iframe.helptext == '') iframe.helptext = '';
   if (iframe.url == '') iframe.url = '/gfx/kiwi-with-headphones.51x67.png';
   if (iframe.title == '') iframe.title = 'iframe extension';
   //console_nv('iframe', 'iframe.url', 'iframe.html', 'iframe.title', 'iframe.width', 'iframe.height', 'iframe.helptext');
   
   var controls_html =
      w3_div('w3-text-white',
         w3_div('w3-medium', '<b>'+ iframe.title +'</b>'),
         w3_div('id-iframe-container w3-margin-T-8 w3-relative',
            '<iframe id="id-iframe-src"' +
               ' style="width:'+ px(iframe.width) +'; height:'+ px(iframe.height) +'; border:0;">' +
            '</iframe>'
         )
      );

   ext_panel_show(controls_html, null, null);
   ext_set_controls_width_height(iframe.width + margin, iframe.height + margin + top_line);

   if (iframe.src == iframe.SRC_URL) {
      w3_attribute('id-iframe-src', 'src', iframe.url);
   } else {
      w3_attribute('id-iframe-src', 'srcdoc', iframe.html);
   }
      
   iframe.msg_handler = function(ev) {
      if (iframe.allow_tune) {
         console.log('IFRAME tune:');
         var p = parse_freq_pb_mode_zoom(ev.data);
         console.log(p);
         var fdsp, mode, zoom;
         if (p[1]) fdsp = +p[1];
         if (p[3]) mode = p[3].toLowerCase();
         if (p[4]) zoom = +p[4];
         ext_tune(fdsp, mode, ext_zoom.ABS, zoom);
      } else {
         console.log('IFRAME tune not allowed');
      }
   };

   window.addEventListener("message", iframe.msg_handler);

   /*
      // example of sending message to iframe
      setTimeout(function() {
         var el = w3_el('id-iframe-src');
         el.contentWindow.postMessage('msg to iframe', '*');
      }, 1000);
   */
   
   /*
      // example of iframe source HTML section:
      
      <!DOCTYPE html>
      <html>
      <body>
            <style>
               a.freq {
                  color: yellow;
                  cursor: pointer;
               }
            </style>
            
            <script type="text/javascript">
               //window.addEventListener("message",
               //   function(ev) {
               //      console.log('from parent msg: '+ ev.data);
               //      console.log('from parent origin: '+ ev.origin);
               //   }
               //);
               
               function tune(msg) { parent.postMessage(msg, '*'); }
            </script>
            
            <a class="freq" onclick="tune('7020 cw')">7020 cw</a><br>
            <a class="freq" onclick="tune('10136 usb')">10136 usb</a><br>
            <a class="freq" onclick="tune('14106 usb')">14106 usb</a><br>
      </body>
      </html>
   */
}

function iframe_blur()
{
   // remove iframe content so e.g. it closes web sockets etc.
   w3_innerHTML('id-iframe-container', '');
   
   if (iframe.msg_handler) {
      window.removeEventListener("message", iframe.msg_handler);
      iframe.msg_handler = null;
   }
}

// called to display HTML for configuration parameters in admin interface
function iframe_admin_html()
{
   var inst_menu = [];
   for (var i = 0; i < iframe.n_menu; i++) {
      var menu = iframe_get_string('menu', i);
      if (isNonEmptyString(menu)) menu = ': '+ menu;
      inst_menu.push(i + menu);
   }
   //console.log(inst_menu);
   
   var s =
      w3_text('w3-text-black',
         'The iframe extension can display content from two sources: <br>' +
         '<ul>' +
            '<li>An arbitrary URL</li>' +
            '<li>The specified HTML/Javascript</li>' +
         '</ul>' +
         'Both sources are wrapped in a browser iframe for better isolation from the Kiwi user interface. <br>' +
         'If enabled by the checkbox below it\'s possible for the iframe content to set the Kiwi frequency, mode and zoom. <br><br>' +
         'There can be up to '+ iframe.n_menu +' iframe instances, each with a separate named entry in the extension menu.'
      ) +
      '<hr>' +

      w3_inline('w3-margin-bottom/',
         w3_select('id-iframe-inst w3-label-inline', 'Instance', '', 'iframe.inst', iframe.inst, inst_menu, 'iframe_inst_cb'),
         w3_select_get_param('w3-margin-L-32/w3-label-inline/', 'Source', '', iframe_str('src'), ['URL', 'HTML'], 'iframe_src_cb'),
         w3_inline('w3-margin-L-32/',
            w3_button('w3-yellow', 'Populate with standard entries', 'iframe_populate_cb'),
            w3_text('w3-margin-L-8 w3-text-black', '(won\'t disturb existing entries)')
         ),
         w3_button('w3-btn-right w3-aqua', 'Clear instance', 'iframe_clr_inst_cb')
      ) +

      w3_inline_percent('w3-valign-start/',
         w3_divs('/w3-margin-bottom',
            w3_input_get('id-iframe-url//', 'URL', iframe_str('url'), 'w3_string_set_cfg_cb', ''),
            w3_textarea_get_param('id-iframe-html//w3-input-any-change|width:100%',
               w3_inline('',
                  w3_text('w3-bold w3-text-teal w3-show-block', 'HTML/Javascript'),
                  w3_button('w3-margin-left w3-aqua', 'Save', 'iframe_html_save_cb')
               ),
               iframe_str('html'), 30, 50, 'w3_string_set_cfg_cb', ''
            )
         ), 65,
         '', 5,
         w3_divs('w3-margin-T-10/w3-margin-bottom',
            '&nbsp;',
            w3_input_get('', 'Extension menu entry (keep short)', iframe_str('menu'), 'iframe_menu_cb', ''),
            w3_input_get('', 'Title text/HTML', iframe_str('title'), 'w3_string_set_cfg_cb', ''),
            w3_input_get('', 'Window width', iframe_str('width'), 'w3_num_set_cfg_cb', 0),
            w3_input_get('', 'Window height', iframe_str('height'), 'w3_num_set_cfg_cb', 0),
            w3_input_get('', 'Help text/HTML', iframe_str('help'), 'w3_string_set_cfg_cb', ''),
            w3_checkbox_get_param('/w3-label-inline', 'Allow iframe to tune Kiwi',
               iframe_str('allow_tune'), 'w3_bool_set_cfg_cb', /* init_val */ false),
            w3_div('w3-right w3-text-black', 'iframe by Kari Karvonen, OH1KK')
         )
      );
   return s;
}

function iframe_config_html()
{
   console.log('iframe_config_html inst='+ iframe.inst +' src='+ iframe_str('src'));
   ext_config_html(iframe, 'iframe', 'iframe', 'iframe extension configuration', iframe_admin_html());
}

function iframe_inst_cb(path, idx, first)
{
   console.log('iframe_inst_cb idx='+ idx +' first='+ first);
   if (first) return;
   iframe.inst = +idx;
   w3_innerHTML('id-iframe', iframe_admin_html());
}

function iframe_inst_empty(inst)
{
   return (
      iframe_get_string('menu', inst, '') == '' && 
      iframe_get_string('title', inst, '') == '' && 
      iframe_get_string('help', inst, '') == '' && 
      iframe_get_string('url', inst, '') == '' && 
      iframe_get_string('html', inst, '') == '' && 
      iframe_get_string('help', inst, '') == ''
   );
}

function iframe_populate_cb(path)
{
   var i;
   console.log('iframe_populate_cb');
   
   // construct list of existing url and html entry fields
   var inst_urls = [];
   for (i = 0; i < iframe.n_menu; i++) {
      inst_urls.push(iframe_get_string('url', i));
   }
   console.log(inst_urls);
   var inst_html = [];
   for (i = 0; i < iframe.n_menu; i++) {
      inst_html.push(iframe_get_string('html', i));
   }
   console.log(inst_html);

   var j = 0;
   iframe.std.forEach(
      function(o,i) {
         console.log(i +': '+ o.menu);
         
         // prevent duplicate entries
         if (o.src == iframe.SRC_URL) {
            if (kiwi_array_iter_rv(inst_urls,
               function(s) { 
                  //console.log(s +' '+ o.url +' '+ TF(s.includes(o.url)));
                  return s.includes(o.url);
               }
            )) {
               console.log('an entry already has matching url');
               return;
            }
         } else {
            if (kiwi_array_iter_rv(inst_html,
               function(s) {
                  //console.log(s +' '+ o.html +' '+ TF(s.includes(o.html)));
                  return s.includes(o.html);
               }
            )) {
               console.log('an entry already has matching html');
               return;
            }
         }

         while (!iframe_inst_empty(j)) {
            j++;     // skip existing non-empty entries
         }
         if (j == iframe.n_menu) return;
         iframe_set(j, 'src', o.src);
         iframe_set(j, 'menu', o.menu);
         iframe_set(j, 'width', o.w);
         iframe_set(j, 'height', o.h);
         iframe_set(j, 'allow_tune', o.tune? true:false);
         iframe_set(j, 'title', o.title);
         iframe_set(j, 'help', o.help);
         iframe_set(j, 'url', o.url);
         iframe_set(j, 'html', o.html);
      }
   );
   ext_cfg_save();
   w3_innerHTML('id-iframe', iframe_admin_html());
}

function iframe_clr_inst_cb(path)
{
   var i = iframe.inst;
   iframe_set(i, 'src', 0);
   iframe_set(i, 'menu', '');
   iframe_set(i, 'width', 0);
   iframe_set(i, 'height', 0);
   iframe_set(i, 'allow_tune', false);
   iframe_set(i, 'title', '');
   iframe_set(i, 'help', '');
   iframe_set(i, 'url', '');
   iframe_set(i, 'html', '');
   ext_cfg_save();
   w3_innerHTML('id-iframe', iframe_admin_html());
}

function iframe_src_cb(path, idx, first)
{
   console.log('iframe_src_cb path='+ path +' idx='+ idx +' iframe.src='+ iframe.src);
   iframe.src = +idx;
   admin_select_cb(path, iframe.src, first);
   //console.log('iframe_src_cb: src='+ iframe.src +' '+ (iframe.src != iframe.SRC_URL) +' '+ (iframe.src != iframe.SRC_HTML));
   w3_disable('id-iframe-url', iframe.src != iframe.SRC_URL);
   //w3_set_props('id-iframe-url', 'w3-disabled w3-pointer-events-none', iframe.src != iframe.SRC_URL);
   w3_disable('id-iframe-html', iframe.src != iframe.SRC_HTML);
   //w3_set_props('id-iframe-html', 'w3-disabled w3-pointer-events-none', iframe.src != iframe.SRC_HTML);
}

function iframe_menu_cb(path, val, first)
{
   // rename current instance menu selection
   val = val.trim();
   console.log('iframe_menu_cb: path='+ path +' val='+ val);
   w3_select_enum('id-iframe-inst',
      function(el, i) {
         //console.log(el);
         if (el.value == iframe.inst) {
            w3_innerHTML(el, (val == '')? el.value : (el.value +': '+ val));
         }
      }
   );
   w3_string_set_cfg_cb(path, val);
}

function iframe_html_save_cb(path)
{
   console.log('iframe_src_save_cb '+ 'id-'+ iframe_str('html'));
   var el = w3_el('id-'+ iframe_str('html'));
   //console.log('val='+ el.value);
   w3_string_set_cfg_cb(iframe_str('html'), el.value);
   w3_schedule_highlight(el);
}

function iframe_help(show)
{
   if (show) {
      var s =  w3_text('w3-medium w3-bold w3-text-aqua', iframe.title +' help') + '<br><br>'+ iframe.helptext+ '';
      confirmation_show_content(s, 610, 125);
   }
   return true;
}
