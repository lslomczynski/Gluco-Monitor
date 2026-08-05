// Raw JSON data viewer — dark theme, sensor-aware sections
const char *BruteHtml = R"====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Data — Gluco-Monitor</title>
<script src="/JS_Commun"></script>
<script src="/JS_Traduction"></script>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#111;color:#eee;font-family:Arial,sans-serif;max-width:700px;margin:auto;font-size:112%}
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
.section h2{color:#8af;font-size:.95em;margin-bottom:10px;border-bottom:1px solid #2a2a4a;padding-bottom:6px}
.ver{text-align:center;color:#444;font-size:.78em;margin-top:10px;padding-bottom:16px}
.json-key{color:#e88}
.json-string{color:#8e8}
.json-number{color:#88f}
.json-boolean{color:#f88}
.json-null{color:#888}
.json-toggle{cursor:pointer;font-weight:bold;color:#8af}
ul{list-style-type:none;padding-left:20px;margin:0}
</style>
</head>
<body onload="Init();">
<div class="top">
  <img src="/favicon.ico" alt="">
  <h1>Gluco-Monitor</h1>
  <nav class="MiniMenu">
    <a href="/" data-i18n="Glucose">Glucose</a>
    <a href="/Settings">Settings</a>
    <a href="/Brute" class="active" id="menuBrute">Data</a>
    <a href="/OTA" data-i18n="Update">Update</a>
    <a href="/Restart" class="warn" data-i18n="Restart">Restart</a>
    <a href="/eraseConfig" class="danger">Erase</a>
  </nav>
</div>

<main>
  <div class="section">
    <h2 id="pageTitle">JSON Data</h2>
    <h3 id="sec1Title" style="color:#bbb;font-size:.85em;margin-bottom:8px"></h3>
    <div id="LoginJSON"></div>
  </div>
  <div class="section" id="sec2Wrap">
    <h2 id="sec2Title" style="color:#8af;font-size:.95em;margin-bottom:10px;border-bottom:1px solid #2a2a4a;padding-bottom:6px"></h2>
    <div id="ConnectionJSON"></div>
  </div>
  <div class="section">
    <h2 id="sec3Title" style="color:#8af;font-size:.95em;margin-bottom:10px;border-bottom:1px solid #2a2a4a;padding-bottom:6px"></h2>
    <div id="GraphJSON"></div>
  </div>
  <div class="section">
    <h2 style="color:#8af;font-size:.95em;margin-bottom:10px;border-bottom:1px solid #2a2a4a;padding-bottom:6px">Diagnostics</h2>
    <div id="DiagJSON"></div>
  </div>
  <div class="ver">Version: <span id="version"></span></div>
</main>

<script>
// Build a collapsible DOM node for a JSON key/value pair
function createNode(key, value) {
  var li = document.createElement('li');
  if (typeof value === 'object' && value !== null) {
    var isArray = Array.isArray(value);
    var toggle = document.createElement('span');
    toggle.textContent = isArray ? '[ ]' : '{ }';
    toggle.className = 'json-toggle';
    var keySpan = document.createElement('span');
    keySpan.className = 'json-key';
    keySpan.textContent = key ? key + ': ' : '';
    var container = document.createElement('ul');
    container.style.display = 'none';
    toggle.onclick = function() {
      container.style.display = container.style.display === 'none' ? 'block' : 'none';
    };
    li.appendChild(keySpan);
    li.appendChild(toggle);
    li.appendChild(container);
    for (var k in value) {
      container.appendChild(createNode(k, value[k]));
    }
  } else {
    var keySpan = document.createElement('span');
    keySpan.className = 'json-key';
    keySpan.textContent = key ? key + ': ' : '';
    var valueSpan = document.createElement('span');
    if (typeof value === 'string') {
      valueSpan.className = 'json-string';
      valueSpan.textContent = '"' + value + '"';
    } else if (typeof value === 'number') {
      valueSpan.className = 'json-number';
      valueSpan.textContent = value;
    } else if (typeof value === 'boolean') {
      valueSpan.className = 'json-boolean';
      valueSpan.textContent = value;
    } else if (value === null) {
      valueSpan.className = 'json-null';
      valueSpan.textContent = 'null';
    }
    li.appendChild(keySpan);
    li.appendChild(valueSpan);
  }
  return li;
}

// Render a parsed JSON object as a collapsible tree into the given container element
function renderJSON(json, containerId) {
  var container = document.getElementById(containerId);
  container.innerHTML = '';
  var ul = document.createElement('ul');
  for (var key in json) {
    ul.appendChild(createNode(key, json[key]));
  }
  container.appendChild(ul);
}

// Fetch a JSON endpoint and render it; fall back to raw text on parse error
function loadSection(endpoint, containerId) {
  fetch(endpoint)
    .then(function(r) { return r.text(); })
    .then(function(text) {
      try {
        var obj = JSON.parse(text);
        renderJSON(obj, containerId);
      } catch(e) {
        GH(containerId, text || '(empty)');
      }
    })
    .catch(function() {
      GH(containerId, '(failed to load)');
    });
}

function Init() {
  GH('version', Version);
  fetch('/ajaxGlycemie')
    .then(function(r) { return r.json(); })
    .then(function(data) {
      var st = data.sensorType;
      // Set section headings and visibility based on sensor type
      if (st === 0) {
        // FreeStyle Libre / LibreLinkUp
        GH('pageTitle', 'LibreLinkUp JSON Data');
        GH('sec1Title', 'Login response');
        GH('sec2Title', 'Patient / Connection data');
        GH('sec3Title', 'Graph data');
        document.getElementById('sec2Wrap').style.display = '';
      } else if (st === 1) {
        // Dexcom Share
        GH('pageTitle', 'Dexcom Share JSON Data');
        GH('sec1Title', 'Login response');
        GH('sec2Title', 'Connection data');
        GH('sec3Title', 'Readings data');
        document.getElementById('sec2Wrap').style.display = '';
      } else {
        // NightScout — ConnectionJSON is always empty, hide section 2
        GH('pageTitle', 'NightScout JSON Data');
        GH('sec1Title', 'Connection test response');
        GH('sec3Title', 'Glucose readings');
        document.getElementById('sec2Wrap').style.display = 'none';
      }
      SetTraduction();
      loadSection('/LoginJSON', 'LoginJSON');
      if (st !== 2) {
        loadSection('/ConnectionJSON', 'ConnectionJSON');
      }
      loadSection('/GraphJSON', 'GraphJSON');
      loadSection('/ajaxDiag', 'DiagJSON');
    })
    .catch(function() {
      // Fallback: show Libre layout, load all sections
      GH('pageTitle', 'JSON Data');
      GH('sec1Title', 'Login response');
      GH('sec2Title', 'Connection data');
      GH('sec3Title', 'Graph data');
      SetTraduction();
      loadSection('/LoginJSON', 'LoginJSON');
      loadSection('/ConnectionJSON', 'ConnectionJSON');
      loadSection('/GraphJSON', 'GraphJSON');
      loadSection('/ajaxDiag', 'DiagJSON');
    });
}
</script>
</body>
</html>
)====";
