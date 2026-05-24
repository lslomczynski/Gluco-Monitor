const char* SetupAPHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Gluco-Monitor Setup</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#111;color:#eee;font-family:Arial,sans-serif;padding:16px;max-width:500px;margin:auto}
h1{text-align:center;color:#4af;font-size:1.4em;margin-bottom:18px}
h2{color:#aaa;font-size:.95em;margin:14px 0 8px;border-bottom:1px solid #333;padding-bottom:4px}
label{display:block;margin-bottom:3px;color:#bbb;font-size:.88em}
input[type=text],select{width:100%;padding:9px;background:#222;color:#eee;border:1px solid #444;border-radius:6px;font-size:1em;margin-bottom:10px}
.row{display:flex;gap:8px;align-items:flex-start}
.row input{flex:1;margin-bottom:0}
.scan-btn{padding:9px 12px;background:#224;color:#8af;border:1px solid #446;border-radius:6px;font-size:.9em;cursor:pointer;white-space:nowrap;flex-shrink:0}
.radio-row{display:flex;gap:16px;margin-bottom:12px}
.radio-row label{display:flex;align-items:center;gap:6px;color:#eee;font-size:1em;cursor:pointer;margin-bottom:0}
.section{background:#1a1a2a;border-radius:8px;padding:14px;margin-bottom:12px}
.save-btn{display:block;width:100%;padding:14px;background:#0a5;color:#fff;border:none;border-radius:8px;font-size:1.1em;font-weight:bold;cursor:pointer;margin-top:18px}
.save-btn:active{background:#083}
#scanStatus{color:#777;font-size:.82em;min-height:16px;margin:6px 0}
#ssidResults{display:none;border:1px solid #444;border-radius:6px;max-height:200px;overflow-y:auto;margin-bottom:10px}
#ssidResults div{padding:10px 12px;cursor:pointer;border-bottom:1px solid #222;font-size:.95em}
#ssidResults div:last-child{border-bottom:none}
#ssidResults div:hover,#ssidResults div:active{background:#1a2a4a;color:#8af}
/* Glucose sliders */
input[type=range]{-webkit-appearance:none;width:100%;height:6px;border-radius:3px;background:#333;outline:none;cursor:pointer;margin:4px 0}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;border-radius:50%;background:#4af;cursor:pointer;border:2px solid #fff}
.gRow{display:flex;align-items:center;gap:8px;margin-bottom:10px}
.gLbl{width:90px;font-size:.84em;font-weight:bold;flex-shrink:0;line-height:1.2}
.gVal{width:80px;text-align:right;font-size:.84em;color:#ddd;flex-shrink:0;font-variant-numeric:tabular-nums}
#gBar{height:18px;border-radius:6px;margin-bottom:14px;border:1px solid #333}
#gErr{color:#f66;font-size:.82em;min-height:14px;margin-top:6px;text-align:center}
#msg{display:none;text-align:center;padding:12px;border-radius:6px;margin-top:14px;font-weight:bold}
</style>
</head>
<body>
<!-- Hidden focus trap: prevents iOS Safari from auto-scrolling to first real input on load -->
<input type="text" tabindex="-1" aria-hidden="true" style="opacity:0;position:fixed;top:0;left:0;width:0;height:0;font-size:16px">

<h1>&#x1F4F6; Gluco-Monitor Setup</h1>

<div class="section">
  <h2>Language &amp; Timezone</h2>
  <label>Language</label>
  <select id="lang">
    <option value="0">English</option>
    <option value="1">Fran&#231;ais</option>
    <option value="2">Deutsch</option>
    <option value="3">Espa&#241;ol</option>
    <option value="4">Italiano</option>
    <option value="5">Polski</option>
  </select>
  <label>Timezone</label>
  <select id="timezone">
    <option value="0">UTC (Coordinated Universal Time)</option>
    <option value="1">United Kingdom</option>
    <option value="2" selected>Central Europe (France, Germany, Italy)</option>
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
</div>

<div class="section">
  <h2>Wi-Fi</h2>
  <label>Network (SSID)</label>
  <div class="row">
    <input type="text" id="ssid" placeholder="Type or scan for networks..." autocomplete="off">
    <button class="scan-btn" onclick="doScan()" type="button">&#x1F50D; Scan</button>
  </div>
  <div id="scanStatus"></div>
  <div id="ssidResults"></div>
  <label>Password</label>
  <input type="text" id="password" placeholder="Wi-Fi password" autocomplete="off">
</div>

<div class="section">
  <h2>Sensor type</h2>
  <div class="radio-row">
    <label><input type="radio" name="sensor" value="libre" checked onchange="showSensor()"> FreeStyle Libre</label>
    <label><input type="radio" name="sensor" value="dexcom" onchange="showSensor()"> Dexcom</label>
    <label><input type="radio" name="sensor" value="nightscout" onchange="showSensor()"> NightScout</label>
  </div>
</div>

<div id="libreSection" class="section">
  <h2>FreeStyle Libre &#8212; LibreLinkUp</h2>
  <label>Email</label>
  <input type="text" id="email" placeholder="account@email.com" autocomplete="off">
  <label>Password</label>
  <input type="text" id="librepass" placeholder="LibreLinkUp password" autocomplete="off">
  <label>Server region</label>
  <select id="librezone">
    <option value="">Auto / Global</option>
    <option value="eu">Europe (eu)</option>
    <option value="eu2">Europe 2 (eu2)</option>
    <option value="fr">France (fr)</option>
    <option value="de">Germany (de)</option>
    <option value="us">USA (us)</option>
    <option value="ca">Canada (ca)</option>
    <option value="au">Australia (au)</option>
    <option value="jp">Japan (jp)</option>
    <option value="ap">Asia-Pacific (ap)</option>
    <option value="ae">UAE (ae)</option>
  </select>
</div>

<div id="dexcomSection" class="section" style="display:none">
  <h2>Dexcom Share</h2>
  <label>Username</label>
  <input type="text" id="dexuser" placeholder="Dexcom username" autocomplete="off">
  <label>Password</label>
  <input type="text" id="dexpass" placeholder="Dexcom password" autocomplete="off">
  <label>Region</label>
  <select id="dexregion">
    <option value="Non-US">Non-US (International)</option>
    <option value="US">US</option>
    <option value="JP">Japan</option>
  </select>
</div>

<div id="nightscoutSection" class="section" style="display:none">
  <h2>NightScout</h2>
  <label>NightScout URL</label>
  <input type="text" id="nsurl" placeholder="https://yoursite.nightscout.net" autocomplete="off">
  <label>Access Token</label>
  <input type="text" id="nstoken" placeholder="NightScout access token" autocomplete="off">
</div>

<div class="section">
  <h2>Glucose Thresholds</h2>
  <!-- Unit selector — also sets glucoseUnit saved to device -->
  <div class="radio-row" style="margin-bottom:14px">
    <label><input type="radio" name="gunit" value="0" checked onchange="gUnitChange()"> mg/dL</label>
    <label><input type="radio" name="gunit" value="1" onchange="gUnitChange()"> mmol/L</label>
  </div>
  <!-- Live color-zone bar -->
  <div id="gBar"></div>
  <!-- Threshold rows: slider always in mg/dL, label converts on the fly -->
  <div class="gRow">
    <span class="gLbl" style="color:#f55">Graph<br>Min</span>
    <input type="range" id="gMin" min="0" max="400" step="5" value="0" oninput="gUpdate()">
    <span id="gMinV" class="gVal">0 mg/dL</span>
  </div>
  <div class="gRow">
    <span class="gLbl" style="color:#5d5">Target<br>Low</span>
    <input type="range" id="gLow" min="0" max="400" step="5" value="70" oninput="gUpdate()">
    <span id="gLowV" class="gVal">70 mg/dL</span>
  </div>
  <div class="gRow">
    <span class="gLbl" style="color:#5d5">Target<br>High</span>
    <input type="range" id="gHigh" min="0" max="400" step="5" value="180" oninput="gUpdate()">
    <span id="gHighV" class="gVal">180 mg/dL</span>
  </div>
  <div class="gRow">
    <span class="gLbl" style="color:#f90">Warning<br>Value</span>
    <input type="range" id="gWarn" min="0" max="400" step="5" value="300" oninput="gUpdate()">
    <span id="gWarnV" class="gVal">300 mg/dL</span>
  </div>
  <div class="gRow">
    <span class="gLbl" style="color:#b6f">Graph<br>Max</span>
    <input type="range" id="gMax" min="0" max="400" step="5" value="400" oninput="gUpdate()">
    <span id="gMaxV" class="gVal">400 mg/dL</span>
  </div>
  <div id="gErr"></div>
</div>

<button class="save-btn" onclick="doSave()">&#x1F4BE; Save &amp; Restart</button>
<div id="msg"></div>

<script>
// Prevent iOS auto-scroll to first input
window.scrollTo(0,0);

var MG = 18.02; // mg/dL per mmol/L

function gFmt(mgdl) {
  var mmol = document.querySelector('input[name=gunit]:checked').value === '1';
  return mmol ? (mgdl / MG).toFixed(1) + ' mmol/L' : mgdl + ' mg/dL';
}

function gUpdate() {
  var min  = +document.getElementById('gMin').value;
  var low  = +document.getElementById('gLow').value;
  var high = +document.getElementById('gHigh').value;
  var warn = +document.getElementById('gWarn').value;
  var max  = +document.getElementById('gMax').value;

  document.getElementById('gMinV').textContent  = gFmt(min);
  document.getElementById('gLowV').textContent  = gFmt(low);
  document.getElementById('gHighV').textContent = gFmt(high);
  document.getElementById('gWarnV').textContent = gFmt(warn);
  document.getElementById('gMaxV').textContent  = gFmt(max);

  // Color bar: zones relative to [min, max] so gMin slider also affects proportions
  var range = max - min > 0 ? max - min : 1;
  var p1 = ((low  - min) / range * 100).toFixed(1);
  var p2 = ((high - min) / range * 100).toFixed(1);
  var p3 = ((warn - min) / range * 100).toFixed(1);
  document.getElementById('gBar').style.background =
    'linear-gradient(to right,' +
    '#c33 0% ' + p1 + '%,' +
    '#3a3 ' + p1 + '% ' + p2 + '%,' +
    '#c70 ' + p2 + '% ' + p3 + '%,' +
    '#84c ' + p3 + '% 100%)';

  // Inline validation hint
  var err = '';
  if (min >= low)  err = 'Graph Min must be less than Target Low';
  else if (low >= high) err = 'Target Low must be less than Target High';
  else if (high >= warn) err = 'Target High must be less than Warning Value';
  else if (warn >= max)  err = 'Warning Value must be less than Graph Max';
  document.getElementById('gErr').textContent = err;
}

function gUnitChange() { gUpdate(); }

function showSensor() {
  var s = document.querySelector('input[name=sensor]:checked').value;
  document.getElementById('libreSection').style.display      = s === 'libre'       ? '' : 'none';
  document.getElementById('dexcomSection').style.display     = s === 'dexcom'      ? '' : 'none';
  document.getElementById('nightscoutSection').style.display = s === 'nightscout'  ? '' : 'none';
}

function doScan() {
  var st  = document.getElementById('scanStatus');
  var res = document.getElementById('ssidResults');
  st.textContent = 'Scanning networks...';
  res.style.display = 'none';
  res.innerHTML = '';
  fetch('/scanWifi').then(function(r) { return r.json(); }).then(function(list) {
    if (list.length === 0) { st.textContent = 'No networks found.'; return; }
    st.textContent = list.length + ' network(s) found — tap to select:';
    list.forEach(function(n) {
      var d = document.createElement('div');
      d.textContent = '📶 ' + n;
      d.onclick = function() {
        document.getElementById('ssid').value = n;
        res.style.display = 'none';
        st.textContent = 'Selected: ' + n;
      };
      res.appendChild(d);
    });
    res.style.display = 'block';
  }).catch(function() { st.textContent = 'Scan failed — try again.'; });
}

function doSave() {
  var ssid = document.getElementById('ssid').value.trim();
  if (!ssid) { alert('Please enter a Wi-Fi network name.'); return; }
  if (document.getElementById('gErr').textContent) {
    alert('Please fix the glucose threshold values before saving.');
    return;
  }
  var sensor = document.querySelector('input[name=sensor]:checked').value;
  var body = 'ssid='            + encodeURIComponent(ssid)
    + '&password='              + encodeURIComponent(document.getElementById('password').value)
    + '&sensor='                + encodeURIComponent(sensor)
    + '&email='                 + encodeURIComponent(document.getElementById('email').value.trim())
    + '&librepass='             + encodeURIComponent(document.getElementById('librepass').value)
    + '&librezone='             + encodeURIComponent(document.getElementById('librezone').value)
    + '&dexuser='               + encodeURIComponent(document.getElementById('dexuser').value.trim())
    + '&dexpass='               + encodeURIComponent(document.getElementById('dexpass').value)
    + '&dexregion='             + encodeURIComponent(document.getElementById('dexregion').value)
    + '&nsurl='                 + encodeURIComponent(document.getElementById('nsurl').value.trim())
    + '&nstoken='               + encodeURIComponent(document.getElementById('nstoken').value)
    + '&lang='                  + encodeURIComponent(document.getElementById('lang').value)
    + '&timezone='              + encodeURIComponent(document.getElementById('timezone').value)
    + '&glucoseUnit='           + encodeURIComponent(document.querySelector('input[name=gunit]:checked').value)
    + '&glucoseRangeMin='       + encodeURIComponent(document.getElementById('gMin').value)
    + '&targetLow='             + encodeURIComponent(document.getElementById('gLow').value)
    + '&targetHigh='            + encodeURIComponent(document.getElementById('gHigh').value)
    + '&glucoseWarn='           + encodeURIComponent(document.getElementById('gWarn').value)
    + '&glucoseRangeMax='       + encodeURIComponent(document.getElementById('gMax').value);

  var m = document.getElementById('msg');
  m.style.display = 'block'; m.style.background = '#224'; m.style.color = '#8af';
  m.textContent = 'Saving... device will restart.';
  fetch('/saveConfig', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: body })
    .then(function()  { m.style.background = '#050'; m.style.color = '#8f8'; m.textContent = 'Saved! Device is restarting...'; })
    .catch(function() { m.style.background = '#500'; m.style.color = '#f88'; m.textContent = 'Error — please try again.'; });
}

// Initialize bar and labels on load
gUpdate();
</script>
</body>
</html>
)rawliteral";
