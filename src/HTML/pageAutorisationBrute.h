// Authorization gate for the raw data page — dark theme
const char *AutBruteHtml = R"====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Authorization Required — Gluco-Monitor</title>
<script src="/JS_Commun"></script>
<script src="/JS_Traduction"></script>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#111;color:#eee;font-family:Arial,sans-serif;max-width:700px;margin:auto}
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
.auth-body{display:flex;flex-direction:column;align-items:center;text-align:center;padding:40px 14px}
.auth-body h2{font-size:1.3em;color:#8af;margin-bottom:16px}
.auth-body p{color:#aaa;font-size:.95em;margin-bottom:32px}
.btn-cancel{padding:14px 36px;background:#333;color:#ccc;border:1px solid #555;
  border-radius:8px;font-size:1.1em;cursor:pointer;text-decoration:none;display:inline-block}
.btn-cancel:hover{background:#444;color:#eee}
</style>
</head>
<body onload="Init();">
<div class="top">
  <img src="/favicon.ico" alt="">
  <h1>Gluco-Monitor</h1>
  <nav class="MiniMenu">
    <a href="/" data-i18n="Glucose">Glucose</a>
    <a href="/Settings">Settings</a>
    <a href="/Brute" id="menuBrute">Data</a>
    <a href="/OTA" data-i18n="Update">Update</a>
    <a href="/Restart" class="warn" data-i18n="Restart">Restart</a>
    <a href="/eraseConfig" class="danger">Erase</a>
  </nav>
</div>

<main>
  <div class="auth-body">
    <h2>Authorization Required</h2>
    <p data-i18n="AutoOnMonitor">Authorize access on the monitor</p>
    <a class="btn-cancel" href="/" data-i18n="Cancel">Cancel</a>
  </div>
  <div class="ver">Version: <span id="version"></span></div>
</main>

<script>
// Auto-reload to detect when the user has authorized on the physical display
function ReLoad() {
  location.reload();
}
setInterval(ReLoad, 2500);

function Init() {
  GH('version', Version);
  // Fetch sensor type to set correct label on the Data nav link
  fetch('/ajaxGlycemie')
    .then(function(r) { return r.json(); })
    .then(function(data) {
      if (data.sensorType === 1) {
        document.getElementById('menuBrute').setAttribute('data-i18n', 'dataDexcom');
      } else if (data.sensorType === 2) {
        document.getElementById('menuBrute').setAttribute('data-i18n', 'dataNightScout');
      } else {
        document.getElementById('menuBrute').setAttribute('data-i18n', 'dataLibreview');
      }
      SetTraduction();
    })
    .catch(function() {
      SetTraduction();
    });
}
</script>
</body>
</html>
)====";
