// Settings page — web UI for all device configuration parameters
const char *SettingsHtml = R"====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Settings — Gluco-Monitor</title>
<script src="/JS_Commun"></script>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#111;color:#eee;font-family:Arial,sans-serif;max-width:600px;margin:auto;font-size:112%}
.top{display:flex;align-items:center;padding:10px 12px;border-bottom:1px solid #333;gap:12px;flex-wrap:wrap}
.top img{height:32px;width:32px;flex-shrink:0}
.top h1{font-size:1.1em;color:#eee;flex:1;min-width:80px}
.MiniMenu{display:flex;gap:5px;flex-wrap:wrap}
.MiniMenu a{padding:5px 9px;border-radius:6px;color:#aaa;text-decoration:none;font-size:.82em;background:#1a1a1a;border:1px solid #333}
.MiniMenu a:hover{color:#fff;border-color:#555}
.MiniMenu a.active{background:#1a2a4a;color:#8af;border-color:#446;font-weight:bold}
.MiniMenu a.warn{color:#f90;border-color:#630}
.MiniMenu a.warn:hover{color:#fb4;border-color:#850}
.MiniMenu a.danger{color:#f88;border-color:#522}
.MiniMenu a.danger:hover{color:#fcc;border-color:#744}
main{padding:14px}
.section{background:#1a1a2a;border-radius:8px;padding:14px;margin-bottom:14px}
.section h2{color:#8af;font-size:.95em;margin-bottom:12px;border-bottom:1px solid #2a2a4a;padding-bottom:6px}
label{display:block;margin-top:10px;margin-bottom:3px;color:#bbb;font-size:.87em}
label:first-of-type{margin-top:0}
input[type=text],input[type=password],input[type=number],select{
  width:100%;padding:9px;background:#222;color:#eee;
  border:1px solid #444;border-radius:6px;font-size:.95em}
input[type=number]{width:auto;text-align:center;min-width:80px}
.radio-row{display:flex;gap:12px;flex-wrap:wrap;margin-top:6px}
.radio-row label{display:flex;align-items:center;gap:6px;color:#eee;font-size:.9em;
  cursor:pointer;margin:0;background:none;border:none}
.cred-section{display:none;margin-top:10px;padding-top:10px;border-top:1px solid #2a2a4a}

/* Threshold row: [label] gap [input] gap [+10] [-10] */
.trow{display:flex;align-items:center;margin-bottom:10px}
.trow .tlbl{flex:1;font-size:.85em;color:#bbb;font-weight:bold;padding-right:10px}
.trow input[type=number]{width:80px;padding:7px;flex-shrink:0;text-align:center;margin-right:10px}
.tbtns{display:flex;gap:4px;flex-shrink:0}
.tbtns button{padding:7px 12px;background:#2a2a3a;color:#eee;border:1px solid #444;
  border-radius:6px;font-size:.92em;cursor:pointer;line-height:1}
.tbtns button:active{background:#3a3a5a}

/* Glucose color bar */
#gBar{width:100%;height:18px;border-radius:6px;margin-bottom:4px;display:block}

.btn-test{padding:9px 20px;background:#1a2a4a;color:#8af;border:1px solid #446;
  border-radius:6px;font-size:.9em;cursor:pointer}
.btn-test:active{background:#2a3a5a}
.test-sep{border-top:1px solid #2a2a4a;padding-top:12px;margin-top:14px}
.hint{color:#666;font-size:.8em;margin-top:6px}
.btn-row{display:flex;gap:10px;margin-top:16px}
.btn-save{flex:2;padding:13px;background:#0a5;color:#fff;border:none;border-radius:8px;
  font-size:1em;font-weight:bold;cursor:pointer}
.btn-save:active{background:#083}
.btn-cancel{flex:1;padding:13px;background:#333;color:#aaa;border:1px solid #555;
  border-radius:8px;font-size:.9em;cursor:pointer;text-decoration:none;
  display:flex;align-items:center;justify-content:center}
.btn-cancel:hover{background:#444;color:#eee}
#status{display:none;text-align:center;padding:10px;border-radius:6px;margin-top:12px;font-weight:bold}
.ok{background:#1a3a1a;color:#6f6;border:1px solid #3a5a3a}
.err{background:#3a1a1a;color:#f66;border:1px solid #5a3a3a}
.mqtt-soon{color:#666;font-style:italic;text-align:center;padding:10px 0;font-size:.9em}
.ver{text-align:center;color:#444;font-size:.78em;margin-top:10px;padding-bottom:16px}
</style>
</head>
<body>
<div class="top">
  <img src="/favicon.ico" alt="">
  <h1>Gluco-Monitor</h1>
  <nav class="MiniMenu">
    <a href="/">Glucose</a>
    <a href="/Settings" class="active">Settings</a>
    <a href="/Brute" id="abrute">Data</a>
    <a href="/OTA">Update</a>
    <a href="/Restart" class="warn">Restart</a>
    <a href="/eraseConfig" class="danger">Erase</a>
  </nav>
</div>

<main>

<!-- Sensor Configuration -->
<div class="section">
  <h2>Sensor Configuration</h2>
  <div class="radio-row">
    <label><input type="radio" name="sensorType" value="0" onchange="updateSensor()"> FreeStyle Libre</label>
    <label><input type="radio" name="sensorType" value="1" onchange="updateSensor()"> Dexcom</label>
    <label><input type="radio" name="sensorType" value="2" onchange="updateSensor()"> NightScout</label>
  </div>

  <div id="sec-libre" class="cred-section">
    <label>LibreLinkUp email</label>
    <input type="text" id="libreEmail" autocomplete="off">
    <label>Password <span style="color:#555">(leave blank to keep current)</span></label>
    <input type="password" id="librePass" placeholder="(unchanged)" autocomplete="new-password">
    <label>Server region</label>
    <select id="libreZone">
      <option value="">Auto / Global</option>
      <option value="eu">Europe (eu)</option>
      <option value="eu2">Europe 2 (eu2)</option>
      <option value="fr">France (fr)</option>
      <option value="de">Germany (de)</option>
      <option value="us">USA (us)</option>
      <option value="ca">Canada (ca)</option>
      <option value="au">Australia (au)</option>
      <option value="jp">Japan (jp)</option>
      <option value="ap">Asia Pacific (ap)</option>
      <option value="ae">UAE (ae)</option>
    </select>
  </div>

  <div id="sec-dexcom" class="cred-section">
    <label>Dexcom Share username</label>
    <input type="text" id="dexcomUsername" autocomplete="off">
    <label>Password <span style="color:#555">(leave blank to keep current)</span></label>
    <input type="password" id="dexcomPass" placeholder="(unchanged)" autocomplete="new-password">
    <label>Region</label>
    <select id="dexcomRegion">
      <option value="Non-US">Non-US (International)</option>
      <option value="US">US</option>
    </select>
  </div>

  <div id="sec-ns" class="cred-section">
    <label>NightScout URL</label>
    <input type="text" id="nightscoutUrl" placeholder="https://your-ns.example.com" autocomplete="off">
    <label>Access token <span style="color:#555">(leave blank to keep current)</span></label>
    <input type="password" id="nightscoutToken" placeholder="(unchanged)" autocomplete="new-password">
  </div>
  <div class="test-sep">
    <button class="btn-test" onclick="testConn()">Test Connection</button>
    <div id="testResult" style="font-size:.85em;margin-top:8px;min-height:1.2em"></div>
  </div>
</div>

<!-- Display Settings -->
<div class="section">
  <h2>Display Settings</h2>
  <label>Language</label>
  <select id="LaLangue">
    <option value="0">English</option>
    <option value="1">Fran&#231;ais</option>
    <option value="2">Deutsch</option>
    <option value="3">Espa&#241;ol</option>
    <option value="4">Italiano</option>
    <option value="5">Polski</option>
  </select>
  <label>Timezone</label>
  <select id="idxFuseau">
    <option value="0">UTC (Coordinated Universal Time)</option>
    <option value="1">United Kingdom</option>
    <option value="2">Central Europe (France, Germany, Italy)</option>
    <option value="3">Eastern Europe (Greece, Finland, Romania)</option>
    <option value="4">Russia (Moscow)</option>
    <option value="5">Canada Atlantic</option>
    <option value="6">USA Eastern (New York, Washington)</option>
    <option value="7">USA Central (Chicago, Dallas)</option>
    <option value="8">USA Mountain (Denver)</option>
    <option value="9">USA Pacific (Los Angeles, Seattle)</option>
    <option value="10">Alaska</option>
    <option value="11">Hawaii</option>
    <option value="12">Brazil</option>
    <option value="13">Argentina</option>
    <option value="14">West Africa (Nigeria, Cameroon)</option>
    <option value="15">East Africa (Kenya, Ethiopia)</option>
    <option value="16">United Arab Emirates (Dubai)</option>
    <option value="17">India</option>
    <option value="18">Thailand / Vietnam</option>
    <option value="19">China</option>
    <option value="20">Japan</option>
    <option value="21">South Korea</option>
    <option value="22">Australia Eastern (Sydney, Melbourne)</option>
    <option value="23">Australia Central (Adelaide)</option>
    <option value="24">New Zealand</option>
  </select>
  <label>Glucose unit</label>
  <select id="glucoseUnit">
    <option value="0">mg/dL</option>
    <option value="1">mmol/L</option>
  </select>
  <label>Glucose value color</label>
  <select id="glucoseColor">
    <option value="0">White</option>
    <option value="1">Color (range-based)</option>
  </select>
  <label>Display layout</label>
  <select id="viewMode">
    <option value="0">Default (gauge + bar chart)</option>
    <option value="1">Gauge only (altView_01)</option>
    <option value="2">Value only (altView_02)</option>
  </select>
  <label>Screen rotation</label>
  <select id="rotation">
    <option value="1">Portrait (1)</option>
    <option value="3">Portrait flipped (3)</option>
  </select>
  <label>Night screen brightness</label>
  <div class="radio-row">
    <label><input type="radio" name="LuminositeNuit" value="15"> 10%</label>
    <label><input type="radio" name="LuminositeNuit" value="40"> 25%</label>
    <label><input type="radio" name="LuminositeNuit" value="100"> 50%</label>
    <label><input type="radio" name="LuminositeNuit" value="255"> 100%</label>
  </div>
</div>

<!-- Glucose Thresholds -->
<div class="section">
  <h2>Glucose Thresholds</h2>
  <p class="hint" style="margin-bottom:12px">Values in mg/dL. Buttons adjust by ±10.</p>

  <div class="trow">
    <div class="tlbl">Range Min</div>
    <input type="number" id="glucoseRangeMin" min="0" max="100" onchange="validate()">
    <div class="tbtns">
      <button onclick="adj('glucoseRangeMin',+10)">+10</button>
      <button onclick="adj('glucoseRangeMin',-10)">&#8722;10</button>
    </div>
  </div>
  <div class="trow">
    <div class="tlbl">Target Low</div>
    <input type="number" id="targetLow" min="40" max="200" onchange="validate()">
    <div class="tbtns">
      <button onclick="adj('targetLow',+10)">+10</button>
      <button onclick="adj('targetLow',-10)">&#8722;10</button>
    </div>
  </div>
  <div class="trow">
    <div class="tlbl">Target High</div>
    <input type="number" id="targetHigh" min="100" max="400" onchange="validate()">
    <div class="tbtns">
      <button onclick="adj('targetHigh',+10)">+10</button>
      <button onclick="adj('targetHigh',-10)">&#8722;10</button>
    </div>
  </div>
  <div class="trow">
    <div class="tlbl">Warn High</div>
    <input type="number" id="glucoseWarn" min="100" max="500" onchange="validate()">
    <div class="tbtns">
      <button onclick="adj('glucoseWarn',+10)">+10</button>
      <button onclick="adj('glucoseWarn',-10)">&#8722;10</button>
    </div>
  </div>
  <div class="trow">
    <div class="tlbl">Range Max</div>
    <input type="number" id="glucoseRangeMax" min="200" max="500" onchange="validate()">
    <div class="tbtns">
      <button onclick="adj('glucoseRangeMax',+10)">+10</button>
      <button onclick="adj('glucoseRangeMax',-10)">&#8722;10</button>
    </div>
  </div>

  <canvas id="gBar"></canvas>
  <p class="hint" style="margin-top:4px">
    <span style="color:red">&#9632;</span> Low &nbsp;
    <span style="color:green">&#9632;</span> Target &nbsp;
    <span style="color:orange">&#9632;</span> High &nbsp;
    <span style="color:purple">&#9632;</span> Very high
  </p>
</div>

<!-- Home Assistant / MQTT (placeholder) -->
<div class="section">
  <h2>Home Assistant / MQTT</h2>
  <p class="mqtt-soon">MQTT integration — coming soon</p>
</div>

<div class="btn-row">
  <button class="btn-save" onclick="doSave()">Save Settings</button>
  <a class="btn-cancel" href="/">Cancel</a>
</div>
<div id="status"></div>
<div class="ver">Version: <span id="version"></span></div>

</main>

<script>
function sel(id,v){var e=document.getElementById(id);if(e)e.value=String(v);}
function selRadio(name,v){document.querySelectorAll('input[name="'+name+'"]').forEach(function(r){r.checked=(r.value==String(v));});}

function updateSensor(){
  var v=document.querySelector('input[name="sensorType"]:checked');
  var t=v?v.value:'0';
  document.getElementById('sec-libre').style.display=t=='0'?'block':'none';
  document.getElementById('sec-dexcom').style.display=t=='1'?'block':'none';
  document.getElementById('sec-ns').style.display=t=='2'?'block':'none';
}

function adj(id,delta){
  var e=document.getElementById(id);
  var v=(parseInt(e.value)||0)+delta;
  var mn=parseInt(e.min)||0;
  var mx=parseInt(e.max)||9999;
  e.value=Math.max(mn,Math.min(mx,v));
  validate();
}

// Enforce rMin < tLow < tHigh < warn < rMax (min gap 10 mg/dL), then redraw bar
function validate(){
  var GAP=10;
  var ids=['glucoseRangeMin','targetLow','targetHigh','glucoseWarn','glucoseRangeMax'];
  var v=ids.map(function(id){return parseInt(document.getElementById(id).value)||0;});
  // Forward pass — push up
  for(var i=1;i<v.length;i++){if(v[i]<v[i-1]+GAP){v[i]=v[i-1]+GAP;}}
  // Backward pass — push down
  for(var i=v.length-2;i>=0;i--){if(v[i]>v[i+1]-GAP){v[i]=v[i+1]-GAP;}}
  ids.forEach(function(id,i){document.getElementById(id).value=v[i];});
  updateBar(v[0],v[1],v[2],v[3],v[4]);
}

function updateBar(rMin,tLow,tHigh,warn,rMax){
  var canvas=document.getElementById('gBar');
  canvas.width=500; canvas.height=18;
  var span=rMax-rMin||1;
  function p(v){return Math.max(0,Math.min(1,(v-rMin)/span));}
  var ctx=canvas.getContext('2d');
  var g=ctx.createLinearGradient(0,0,500,0);
  g.addColorStop(0,       'red');
  g.addColorStop(p(tLow), 'red');
  g.addColorStop(p(tLow), 'green');
  g.addColorStop(p(tHigh),'green');
  g.addColorStop(p(tHigh),'orange');
  g.addColorStop(p(warn), 'orange');
  g.addColorStop(p(warn), 'purple');
  g.addColorStop(1,       'purple');
  ctx.fillStyle=g;
  ctx.fillRect(0,0,500,18);
}

// Match LuminositeNuit value to nearest radio button (15/40/100/255)
function nearestLum(v){
  var opts=[15,40,100,255];
  var best=opts[0];
  var bestD=Math.abs(v-best);
  for(var i=1;i<opts.length;i++){
    var d=Math.abs(v-opts[i]);
    if(d<bestD){bestD=d;best=opts[i];}
  }
  return best;
}

function init(){
  document.getElementById('version').textContent=Version;
  fetch('/ajaxSettings')
    .then(function(r){return r.json();})
    .then(function(d){
      selRadio('sensorType',d.sensorType);
      updateSensor();
      document.getElementById('libreEmail').value=d.libreEmail||'';
      sel('libreZone',d.libreZone||'');
      document.getElementById('dexcomUsername').value=d.dexcomUsername||'';
      sel('dexcomRegion',d.dexcomRegion||'Non-US');
      document.getElementById('nightscoutUrl').value=d.nightscoutUrl||'';
      sel('LaLangue',d.LaLangue||0);
      sel('idxFuseau',d.idxFuseau||0);
      sel('glucoseUnit',d.glucoseUnit||0);
      sel('glucoseColor',d.glucoseColor||0);
      sel('viewMode',d.viewMode||0);
      sel('rotation',d.rotation||1);
      selRadio('LuminositeNuit', nearestLum(d.LuminositeNuit||255));
      document.getElementById('glucoseRangeMin').value=d.glucoseRangeMin||0;
      document.getElementById('targetLow').value=d.targetLow||70;
      document.getElementById('targetHigh').value=d.targetHigh||180;
      document.getElementById('glucoseWarn').value=d.glucoseWarn||300;
      document.getElementById('glucoseRangeMax').value=d.glucoseRangeMax||400;
      validate();
    })
    .catch(function(){showStatus('Could not load current settings.','err');});
}

function doSave(){
  var st=document.querySelector('input[name="sensorType"]:checked');
  var lum=document.querySelector('input[name="LuminositeNuit"]:checked');
  var p=new URLSearchParams();
  p.append('sensorType',st?st.value:'0');
  p.append('libreEmail',document.getElementById('libreEmail').value.trim());
  var lp=document.getElementById('librePass').value;
  if(lp) p.append('librePass',lp);
  p.append('libreZone',document.getElementById('libreZone').value);
  p.append('dexcomUsername',document.getElementById('dexcomUsername').value.trim());
  var dp=document.getElementById('dexcomPass').value;
  if(dp) p.append('dexcomPass',dp);
  p.append('dexcomRegion',document.getElementById('dexcomRegion').value);
  p.append('nightscoutUrl',document.getElementById('nightscoutUrl').value.trim());
  var nt=document.getElementById('nightscoutToken').value;
  if(nt) p.append('nightscoutToken',nt);
  p.append('LaLangue',document.getElementById('LaLangue').value);
  p.append('idxFuseau',document.getElementById('idxFuseau').value);
  p.append('glucoseUnit',document.getElementById('glucoseUnit').value);
  p.append('glucoseColor',document.getElementById('glucoseColor').value);
  p.append('viewMode',document.getElementById('viewMode').value);
  p.append('rotation',document.getElementById('rotation').value);
  p.append('LuminositeNuit',lum?lum.value:'255');
  p.append('glucoseRangeMin',document.getElementById('glucoseRangeMin').value);
  p.append('targetLow',document.getElementById('targetLow').value);
  p.append('targetHigh',document.getElementById('targetHigh').value);
  p.append('glucoseWarn',document.getElementById('glucoseWarn').value);
  p.append('glucoseRangeMax',document.getElementById('glucoseRangeMax').value);

  fetch('/saveSettings',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:p.toString()})
    .then(function(r){return r.json();})
    .then(function(d){
      if(d.ok){
        showStatus(d.restart
          ?'Saved. Restart the device to apply all changes.'
          :'Settings saved.','ok');
      } else {
        showStatus('Save failed.','err');
      }
    })
    .catch(function(){showStatus('Save failed — network error.','err');});
}

function showStatus(msg,cls){
  var s=document.getElementById('status');
  s.className=cls; s.textContent=msg; s.style.display='block';
  s.scrollIntoView({behavior:'smooth',block:'nearest'});
}

function testConn(){
  var el=document.getElementById('testResult');
  el.style.color='#aaa';
  el.textContent='Testing... (may take up to 15 s)';
  var st=document.querySelector('input[name="sensorType"]:checked');
  var p=new URLSearchParams();
  p.append('sensorType', st ? st.value : '0');
  p.append('libreEmail', document.getElementById('libreEmail').value.trim());
  var lp=document.getElementById('librePass').value;
  if(lp) p.append('librePass', lp);
  p.append('libreZone', document.getElementById('libreZone').value);
  p.append('dexcomUsername', document.getElementById('dexcomUsername').value.trim());
  var dp=document.getElementById('dexcomPass').value;
  if(dp) p.append('dexcomPass', dp);
  p.append('dexcomRegion', document.getElementById('dexcomRegion').value);
  p.append('nightscoutUrl', document.getElementById('nightscoutUrl').value.trim());
  var nt=document.getElementById('nightscoutToken').value;
  if(nt) p.append('nightscoutToken', nt);
  fetch('/testConnection', {method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:p.toString()})
    .then(function(r){return r.json();})
    .then(function(d){
      el.style.color = d.ok ? '#6f6' : '#f88';
      el.textContent = d.msg || (d.ok ? 'OK' : 'Failed');
    })
    .catch(function(){
      el.style.color='#f88';
      el.textContent='Connection test failed (network error).';
    });
}

window.onload=init;
</script>
</body>
</html>
)====";
