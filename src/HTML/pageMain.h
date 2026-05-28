//************************************************
// Page principale HTML et Javascript
//************************************************
const char *MainHtml = R"====(
<!DOCTYPE html>
<html>

<head>
    <meta charset="UTF-8">
    <style>
        body {
            background: #111;
            color: white;
            font-family: Arial;
            text-align: center;
        }

        a {
            color: white;
            text-decoration: none;
        }

        svgGauge {
            width: 320px;
        }

        .zone {
            fill: none;
            stroke-width: 50;
        }

        #aiguille {
            fill: white;
        }

        #SvgGauge {

            width: 80%;
        }

        #SvgGraphe {
            width: 50%;
        }

        #SvgFlecheTendance {
            width: 50%;
        }

        .graduationD {
            font-size: 14px;
            fill: white;
            text-anchor: end;
        }

        .graduationM {
            font-size: 14px;
            fill: white;
            text-anchor: middle;
        }

        .graduationG {
            font-size: 14px;
            fill: white;
            text-anchor: start;
        }

        .flex3c {
            display: flex;
        }

        .w50p {
            width: 50%;
        }

        .w25p {
            width: 25%;
        }

        #lAge {
            position: relative;
            top: 50%;
            font-size: 30px;
        }

        .MiniMenu {
            text-align: right;
            font-size: 30px;
        }
        .LeBas{
            display:flex;
            justify-content: space-between;
            color:white;
            margin-top:10px;
        }
    </style>
    <title>Gluco-Monitor</title>
    <script src="/JS_Commun"></script>
    <script src="/JS_Main"></script>
    <script src="/JS_Traduction"></script>
</head>

<body onload="init();">
    <div class="flex3c">
        <div id="svgTendance" class="w25p">

        </div>
        <div class="w50p">
            <svg viewBox="0 0 300 180" id="SvgGauge">

                <!-- zones couleur -->

                <path id="z1" class="zone" stroke="red" />
                <path id="z2" class="zone" stroke="green" />
                <path id="z3" class="zone" stroke="orange" />
                <path id="z4" class="zone" stroke="purple" />

                <!-- aiguille -->

                <polygon id="aiguille" points="150,150 150,150 150,150" />

                <!-- centre -->



                <!-- valeur -->

                <text id="valeur" x="150" y="150" font-size="60" text-anchor="middle" fill="white">0</text>

                <text id="unit" x="270" y="165" font-size="15" text-anchor="middle" fill="white">mg/dl</text>

            </svg>

        </div>
        <div class="w25p">
            <div class="MiniMenu"><a href="/Settings">...</a></div>
            <h1>Gluco-Monitor</h1>
            <div id="lAge"><span style="color:grey" data-i18n="AgeGlyc">-Age-</span></div>
        </div>
    </div>
    <div id="svgGraphe"></div>

    <div class="LeBas">
    <div>Version : <span id="version"></span></div>
    <div><a href="https://f1atb.fr">https://F1ATB.fr</a></div>
  </div>


</body>

</html>


)====";

// Confirmation page shown by GET /Restart (orange theme, no auto-restart)
const char *RestartHtml = R"====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Restart — Gluco-Monitor</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#111;color:#eee;font-family:Arial,sans-serif;
  display:flex;flex-direction:column;align-items:center;justify-content:center;
  min-height:100vh;text-align:center;padding:24px;box-sizing:border-box}
h2{font-size:1.5em;color:#f90;margin-bottom:16px}
p{color:#aaa;font-size:.95em;margin-bottom:16px;max-width:320px}
.note{color:#8a8;font-size:.88em;margin-bottom:28px;max-width:320px}
.btn-restart{padding:14px 32px;background:#f90;color:#111;border:none;
  border-radius:8px;font-size:1.1em;font-weight:bold;cursor:pointer;
  margin-bottom:12px;display:block;width:240px}
.btn-restart:active{background:#c70}
.btn-cancel{padding:10px 28px;background:#333;color:#ccc;border:1px solid #555;
  border-radius:8px;font-size:1em;cursor:pointer;text-decoration:none;display:inline-block}
.btn-cancel:hover{background:#444;color:#eee}
</style>
</head>
<body>
<h2>&#x26A0; Restart Device</h2>
<p>The device will reboot and reconnect to Wi&#8209;Fi.</p>
<p class="note">&#x2714; All settings, credentials, and thresholds will be preserved.</p>
<form method="POST" action="/Restart">
  <button class="btn-restart" type="submit">&#x23FB; Restart</button>
</form>
<a class="btn-cancel" href="/">Cancel</a>
</body>
</html>
)====";

// Polling page sent after POST /Restart confirms the reboot
const char *RestartingHtml = R"====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Restarting — Gluco-Monitor</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#111;color:#eee;font-family:Arial,sans-serif;
  display:flex;flex-direction:column;align-items:center;justify-content:center;
  min-height:100vh;text-align:center;padding:24px}
h1{font-size:1.6em;color:#8af;margin-bottom:12px}
p{color:#aaa;font-size:1em}
</style>
</head>
<body>
<h1>Restarting...</h1>
<p>The device will be back in a few seconds.</p>
<script>
function poll(){
  fetch('/').then(function(r){
    if(r.ok){location.href='/';}else{setTimeout(poll,2000);}
  }).catch(function(){setTimeout(poll,2000);});
}
setTimeout(poll,4000);
</script>
</body>
</html>
)====";

// icône 64pixels
const char * Favicon = R"====(
<svg width="64" height="64" viewBox="0 20 180 90" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <style>
      .gp { fill: none; stroke-width: 25; stroke-linecap: butt; }
    </style>
  </defs>
  <path class="gp" stroke="blue" d="M 22.5,80 A 67.5,67.5 0 0,1 42.27,32.27" />
  <path class="gp" stroke="green" d="M 42.27,32.27 A 67.5,67.5 0 0,1 90,12.5" />
  <path class="gp" stroke="orange" d="M 90,12.5 A 67.5,67.5 0 0,1 137.73,32.27" />
  <path class="gp" stroke="red" d="M 137.73,32.27 A 67.5,67.5 0 0,1 157.5,80" />
</svg>
)====";
// icône 192pixels
const char * Favicon192 = R"====(
<svg width="192" height="192" viewBox="0 20 180 90" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <style>
      .gp { fill: none; stroke-width: 25; stroke-linecap: butt; }
    </style>
  </defs>
  <path class="gp" stroke="blue" d="M 22.5,80 A 67.5,67.5 0 0,1 42.27,32.27" />
  <path class="gp" stroke="green" d="M 42.27,32.27 A 67.5,67.5 0 0,1 90,12.5" />
  <path class="gp" stroke="orange" d="M 90,12.5 A 67.5,67.5 0 0,1 137.73,32.27" />
  <path class="gp" stroke="red" d="M 137.73,32.27 A 67.5,67.5 0 0,1 157.5,80" />
</svg>
)====";
// Manifest pour Android
const char * Manifest = R"====(
{
  "name": "Routeur F1ATB",
  "short_name": "Routeur",
  "start_url": "/",
  "display": "standalone",
  "icons": [
    {
      "src": "/favicon192.ico",
      "sizes": "192x192",
      "type": "image/svg+xml"
    },
    {
      "src": "/favicon.ico",
      "sizes": "64x64",
      "type": "image/svg+xml"
    }
  ]
}
)====";
