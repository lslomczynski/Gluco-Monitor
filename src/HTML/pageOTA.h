// OTA firmware update page — dark theme
const char* OTAupdateHtml = R"====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Update — Gluco-Monitor</title>
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
.ver-line{color:#bbb;font-size:.9em;margin-bottom:8px}
.ota-steps{list-style:none;padding:0;color:#ccc;font-size:.9em;line-height:1.9;margin-bottom:8px}
iframe.ver-list{width:100%;height:150px;border:1px solid #333;border-radius:6px;
  background:#fff;display:block;margin-bottom:12px}
input[type=file]{display:block;margin:12px 0;color:#ccc;font-size:.9em}
.btn-upload{padding:10px 24px;background:#0a5;color:#fff;border:none;border-radius:8px;
  font-size:1em;font-weight:bold;cursor:pointer}
.btn-upload:active{background:#083}
progress{width:100%;margin-top:10px;display:none;accent-color:#4af}
#status{margin-top:10px;font-size:.9em;color:#8af;min-height:1.2em}
.ext-link{color:#8af;font-size:.85em}
.ext-link a{color:#8af}
</style>
</head>
<body onload="init();">
<div class="top">
  <img src="/favicon.ico" alt="">
  <h1>Gluco-Monitor</h1>
  <nav class="MiniMenu">
    <a href="/" data-i18n="Glucose">Glucose</a>
    <a href="/Settings">Settings</a>
    <a href="/Brute" id="menuBrute">Data</a>
    <a href="/OTA" class="active" data-i18n="Update">Update</a>
    <a href="/Restart" class="warn" data-i18n="Restart">Restart</a>
    <a href="/eraseConfig" class="danger">Erase</a>
  </nav>
</div>

<main>
  <div class="section">
    <h2 data-i18n="UpdateOTA">OTA Update</h2>
    <p class="ver-line"><span data-i18n="VersionNow">Current version:</span> <span id="version"></span></p>
    <p class="ver-line" data-i18n="VersionDispo">Available versions:</p>
    <iframe class="ver-list" src="https://f1atb.fr/web_tool_GlucoMonit/scan_dir_bin.php"></iframe>
    <ul class="ota-steps">
      <li data-i18n="Telecharge">1 - Download the desired version to your computer</li>
      <li data-i18n="SelectFile">2 - Select the file</li>
      <li><span data-i18n="SendBin">3 - Click </span>'<span data-i18n="SendBinBut">Send binary</span>'</li>
    </ul>
  </div>

  <div class="section">
    <h2>Upload Firmware</h2>
    <form id="form">
      <input type="file" id="bin" accept=".bin" required>
      <button class="btn-upload" type="submit" id="boutonSubmit" data-i18n="SendBinBut">Upload</button>
    </form>
    <progress id="prog" max="100" value="0"></progress>
    <p id="status"></p>
  </div>

  <div class="ver">Version: <span id="version2"></span></div>
</main>

<script>
document.getElementById('form').onsubmit = function(e) {
  e.preventDefault();
  var file = document.getElementById('bin').files[0];
  if (!file) return;
  var prog = document.getElementById('prog');
  var status = document.getElementById('status');
  prog.style.display = 'block';
  status.textContent = 'Upload in progress...';
  var xhr = new XMLHttpRequest();
  xhr.open('POST', '/update');
  xhr.upload.onprogress = function(ev) {
    if (ev.lengthComputable) {
      prog.value = Math.round(ev.loaded / ev.total * 100);
    }
  };
  xhr.onload = function() {
    if (xhr.status === 200) {
      status.textContent = 'Success! The ESP32 is restarting...';
    } else {
      status.textContent = 'Error: ' + xhr.responseText;
    }
  };
  var fd = new FormData();
  fd.append('firmware', file, file.name);
  xhr.send(fd);
};

function init() {
  GH('version', Version);
  GH('version2', Version);
  SetTraduction();
}
</script>
</body>
</html>
)====";
