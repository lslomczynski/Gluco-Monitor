#include "Server.h"
#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <esp_timer.h>

#include "HTML/pageMain.h"
#include "HTML/pageBrute.h"
#include "HTML/pageSettings.h"
#include "HTML/pageAutorisationBrute.h"
#include "HTML/pageOTA.h"
#include "HTML/pageSetupAP.h"
#include "Internet.h"
#include "HTML/JS_Commun.js.h"
#include "HTML/JS_Main.js.h"
#include "Ecran/pageAutBrute.h"
#include "Langues/Langue.h"
#include "Heure.h"
#include "Langues/en.h"
#include "Langues/fr.h"
#include "Langues/de.h"
#include "Langues/it.h"
#include "Langues/es.h"
#include "Langues/pl.h"

// Serveur Web
static AsyncWebServer server(80);

uint8_t MonBuffer[4 + MAX_POINTS * 6]; // Pour les tableaux de glycemie
// Prototypes
void notFound(AsyncWebServerRequest *request);
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);

// Schedule a non-blocking ESP.restart() so the HTTP response is fully transmitted first.
// Using esp_timer (one-shot) avoids calling delay() inside an AsyncWebServer callback,
// which would block the TCP/IP stack before the response reaches the client.
static void restartTimerCallback(void *) { ESP.restart(); }
static void scheduleRestart(uint32_t delayMs)
{
    const esp_timer_create_args_t timerArgs = {
        .callback = restartTimerCallback,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "restart_timer",
        .skip_unhandled_events = false
    };
    esp_timer_handle_t timer;
    esp_timer_create(&timerArgs, &timer);
    esp_timer_start_once(timer, (uint64_t)delayMs * 1000ULL); // microseconds
}

void Init_Server()
{

  // Main Page
  //*********
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", MainHtml); });
  server.on("/Brute", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                  if (AutorisationPageBrute)
                  {                      
                      request->send(200, "text/html", BruteHtml);
                  }
                  else
                  {
                      PageActu = pageAutBrute;
                      request->send(200, "text/html", AutBruteHtml);
                  }
                  TimerAutorisationBruteMillis = millis(); });
  // Settings page — gated the same way as /Brute and /OTA
  server.on("/Settings", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                TimerAutorisationBruteMillis = millis();
                if (!AutorisationPageBrute) {
                    PageActu = pageAutBrute;
                    request->send(200, "text/html", AutBruteHtml);
                    return;
                }
                request->send(200, "text/html", SettingsHtml); });

  // Return current settings as JSON (no passwords)
  server.on("/ajaxSettings", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                if (!AutorisationPageBrute) {
                    request->send(403, "text/plain", "Unauthorized");
                    return;
                }
                JsonDocument doc;
                doc["sensorType"]      = (int)sensorType;
                doc["libreEmail"]      = libreEmail;
                doc["libreZone"]       = libreZone;
                doc["dexcomUsername"]  = dexcomUsername;
                doc["dexcomRegion"]    = dexcomRegion;
                doc["nightscoutUrl"]   = nightscoutUrl;
                doc["glucoseUnit"]     = (int)glucoseUnit;
                doc["glucoseColor"]    = (int)glucoseColor;
                doc["LuminositeNuit"]  = LuminositeNuit;
                doc["LaLangue"]        = LaLangue;
                doc["idxFuseau"]       = idxFuseau;
                doc["rotation"]        = rotation;
                doc["glucoseRangeMin"] = glucoseRangeMin;
                doc["targetLow"]       = targetLow;
                doc["targetHigh"]      = targetHigh;
                doc["glucoseWarn"]     = glucoseWarn;
                doc["glucoseRangeMax"] = glucoseRangeMax;
                doc["viewMode"]        = viewMode;
                String json;
                serializeJson(doc, json);
                request->send(200, "application/json", json); });

  // Save settings from /Settings page
  server.on("/saveSettings", HTTP_POST, [](AsyncWebServerRequest *request)
            {
                if (!AutorisationPageBrute) {
                    request->send(403, "application/json", "{\"ok\":false}");
                    return;
                }
                bool restartNeeded = false;
                // Sensor type
                if (request->hasParam("sensorType", true)) {
                    SensorType newType = (SensorType)request->getParam("sensorType", true)->value().toInt();
                    if (newType != sensorType) {
                        sensorType = newType;
                        clearData();
                    }
                }
                // LibreLinkUp credentials
                if (request->hasParam("libreEmail",  true)) { libreEmail = request->getParam("libreEmail",  true)->value(); libreEmail.trim(); }
                if (request->hasParam("librePass",   true)) librePass  = request->getParam("librePass",   true)->value();
                if (request->hasParam("libreZone",   true)) libreZone  = request->getParam("libreZone",   true)->value();
                // Dexcom credentials
                if (request->hasParam("dexcomUsername", true)) { dexcomUsername = request->getParam("dexcomUsername", true)->value(); dexcomUsername.trim(); }
                if (request->hasParam("dexcomPass",     true)) dexcomPassword = request->getParam("dexcomPass",     true)->value();
                if (request->hasParam("dexcomRegion",   true)) dexcomRegion   = request->getParam("dexcomRegion",   true)->value();
                // NightScout credentials
                if (request->hasParam("nightscoutUrl",   true)) { nightscoutUrl = request->getParam("nightscoutUrl", true)->value(); nightscoutUrl.trim(); }
                if (request->hasParam("nightscoutToken", true)) nightscoutToken = request->getParam("nightscoutToken", true)->value();
                // Display settings
                if (request->hasParam("glucoseUnit",  true)) glucoseUnit  = (GlucoseUnit)request->getParam("glucoseUnit",  true)->value().toInt();
                if (request->hasParam("glucoseColor", true)) glucoseColor = (GlucoseColor)request->getParam("glucoseColor", true)->value().toInt();
                if (request->hasParam("LuminositeNuit", true)) {
                    LuminositeNuit = request->getParam("LuminositeNuit", true)->value().toInt();
                    ledcWrite(GFX_BL, LuminositeNuit);
                }
                if (request->hasParam("LaLangue", true)) {
                    int8_t newLang = (int8_t)request->getParam("LaLangue", true)->value().toInt();
                    if (newLang != LaLangue) { LaLangue = newLang; needsConfigRedraw = true; }
                }
                if (request->hasParam("rotation",  true)) {
                    int8_t newRot = (int8_t)request->getParam("rotation", true)->value().toInt();
                    if (newRot != rotation) { rotation = newRot; restartNeeded = true; }
                }
                if (request->hasParam("viewMode", true))
                    viewMode = (int8_t)request->getParam("viewMode", true)->value().toInt();
                // Timezone — reconfigure NTP immediately
                if (request->hasParam("idxFuseau", true)) {
                    int8_t newTZ = (int8_t)request->getParam("idxFuseau", true)->value().toInt();
                    if (newTZ != idxFuseau) {
                        idxFuseau = newTZ;
                        DefFuseauHoraire();
                    }
                }
                // Glucose thresholds — take effect on next display refresh
                if (request->hasParam("glucoseRangeMin", true)) glucoseRangeMin = request->getParam("glucoseRangeMin", true)->value().toInt();
                if (request->hasParam("targetLow",       true)) targetLow       = request->getParam("targetLow",       true)->value().toInt();
                if (request->hasParam("targetHigh",      true)) targetHigh      = request->getParam("targetHigh",      true)->value().toInt();
                if (request->hasParam("glucoseWarn",     true)) glucoseWarn     = request->getParam("glucoseWarn",     true)->value().toInt();
                if (request->hasParam("glucoseRangeMax", true)) glucoseRangeMax = request->getParam("glucoseRangeMax", true)->value().toInt();

                RecordFichierParametres();
                String resp = restartNeeded
                    ? "{\"ok\":true,\"restart\":true}"
                    : "{\"ok\":true,\"restart\":false}";
                request->send(200, "application/json", resp); });

  server.on("/OTA", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
                if (AutorisationPageBrute)
                  {
                      request->send(200, "text/html", OTAupdateHtml);
                  }
                  else
                  {
                      PageActu = pageAutBrute;
                      request->send(200, "text/html", AutBruteHtml);
                  }
                  TimerAutorisationBruteMillis = millis(); });
  server.on(
      "/update", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
         size_t len, bool final)
      {
        handleDoUpdate(request, filename, index, data, len, final);
      });

  server.on("/JS_Commun", HTTP_GET, [](AsyncWebServerRequest *request)
            { // Pass all threshold variables needed by the web gauge and chart
              String complement =
                "\nconst Version = '" + String(Version) + "';"
                "\nconst glucoseUnit = " + String(glucoseUnit) + ";"
                "\nlet glucoseRangeMax = " + String(glucoseRangeMax) + ";"
                "\nlet glucoseRangeMin = " + String(glucoseRangeMin) + ";"
                "\nlet glucoseWarn = " + String(glucoseWarn) + ";"
                "\nlet targetLow = " + String(targetLow) + ";"
                "\nlet targetHigh = " + String(targetHigh) + ";";
              request->send(200, "text/javascript", String(JS_Commun) + complement); });
  server.on("/JS_Traduction", HTTP_GET, [](AsyncWebServerRequest *request)
            {  String file;
              switch(LaLangue)
                  {
                      case LANG_EN:
                          file=String(LangEN);
                          break;
                      case LANG_FR:
                          file=String(LangFR);
                          break;
                      case LANG_DE:
                          file=String(LangDE);
                          break;
                      case LANG_ES:
                          file=String(LangES);
                          break;
                      case LANG_IT:
                          file=String(LangIT);
                          break;
                      case LANG_PL:
                          file=String(LangPL);
                          break;
                  }
              
              request->send(200, "text/javascript",  "Traduction =" + file +";"); });
  server.on("/JS_Main", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/javascript", JS_Main); });
  server.on("/LoginJSON", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", LoginJSON); });
  server.on("/ConnectionJSON", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", ConnectionJSON); });
  server.on("/GraphJSON", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", GraphJSON); });
  server.on("/ajaxGlycemie", HTTP_GET, [](AsyncWebServerRequest *request)
            {JsonDocument doc;
                doc["GlycemieVal"] = GlycemieVal;
                doc["GlucoseUnitLabel"] = getGlucoseUnitLabel();
                doc["TrendArrow"] = TrendArrow;
                doc["lastGlyUnixTime"] = lastGlyUnixTime;
                doc["targetLow"] = targetLow;
                doc["targetHigh"] = targetHigh;
                doc["glucoseWarn"] = glucoseWarn;
                doc["glucoseRangeMin"] = glucoseRangeMin;
                doc["glucoseRangeMax"] = glucoseRangeMax;
                doc["sensorType"] = (int)sensorType;
                String Json;
                serializeJson(doc, Json);
                request->send(200, "application/json", Json); });
  server.on("/dataGly", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                int16_t tailles[2]; //Pour Javascript derrier, il faut un multiple de 4 octets
                tailles[0]=pointCountGly;
                memcpy(&MonBuffer[0], tailles,  2*sizeof(int16_t)); //En premier la taille des tableaux
                memcpy(&MonBuffer[4], glucoseHeure, pointCountGly * sizeof(uint32_t));
                memcpy(&MonBuffer[4+pointCountGly * sizeof(uint32_t)], glucoseValues, pointCountGly * sizeof(int16_t));

                
                size_t size =
                        2*sizeof(int16_t) +
                        pointCountGly * sizeof(uint32_t) +
                        pointCountGly * sizeof(int16_t);

                    AsyncWebServerResponse *response =
                        request->beginResponse(
                            200,
                            "application/octet-stream",
                            (uint8_t*)&MonBuffer,
                            size
                        );

                    request->send(response); });
  server.on("/Restart", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                request->send(200, "text/html", RestartHtml);
                scheduleRestart(2000); // Restart 2 s after response is sent
            });
  // GET /eraseConfig — show confirmation page (never erases directly)
  server.on("/eraseConfig", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                TimerAutorisationBruteMillis = millis();
                if (!AutorisationPageBrute) {
                    PageActu = pageAutBrute;
                    request->send(200, "text/html", AutBruteHtml);
                    return;
                }
                request->send(200, "text/html",
                    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                    "<title>Erase Configuration</title>"
                    "<style>body{background:#111;color:#eee;font-family:Arial,sans-serif;"
                    "display:flex;flex-direction:column;align-items:center;justify-content:center;"
                    "min-height:100vh;margin:0;padding:24px;box-sizing:border-box}"
                    "h2{color:#f55;margin-bottom:16px}p{color:#aaa;text-align:center;margin-bottom:28px}"
                    ".btn-erase{padding:14px 32px;background:#c00;color:#fff;border:none;"
                    "border-radius:8px;font-size:1.1em;font-weight:bold;cursor:pointer;margin-bottom:12px}"
                    ".btn-erase:active{background:#900}"
                    ".btn-cancel{padding:10px 28px;background:#333;color:#ccc;border:1px solid #555;"
                    "border-radius:8px;font-size:1em;cursor:pointer;text-decoration:none;display:inline-block}"
                    "</style></head><body>"
                    "<h2>&#x26A0; Erase Configuration</h2>"
                    "<p>This will permanently delete all saved settings<br>"
                    "(Wi-Fi, sensor credentials, thresholds).<br>"
                    "The device will restart and enter first-boot setup.</p>"
                    "<form method='POST' action='/eraseConfig'>"
                    "<button class='btn-erase' type='submit'>&#x1F5D1; Erase Configuration</button>"
                    "</form>"
                    "<a class='btn-cancel' href='/'>Cancel</a>"
                    "</body></html>"); });

  // POST /eraseConfig — actual erase after user confirmed on the page above
  server.on("/eraseConfig", HTTP_POST, [](AsyncWebServerRequest *request)
            {
                TimerAutorisationBruteMillis = millis();
                if (!AutorisationPageBrute) {
                    PageActu = pageAutBrute;
                    request->send(200, "text/html", AutBruteHtml);
                    return;
                }
                RemoveParametres();
                // Respond immediately with a polling page, then restart non-blockingly.
                // delay() inside an AsyncWebServer callback blocks the TCP/IP stack and
                // prevents the response from being transmitted — hence the esp_timer approach.
                request->send(200, "text/html",
                    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                    "<title>Restarting…</title>"
                    "<style>body{background:#111;color:#eee;font-family:Arial,sans-serif;"
                    "display:flex;flex-direction:column;align-items:center;justify-content:center;"
                    "min-height:100vh;margin:0;text-align:center}"
                    "h2{color:#8f8}p{color:#aaa}</style></head>"
                    "<body><h2>&#x2713; Configuration erased</h2>"
                    "<p>Device is restarting&hellip;</p>"
                    "<script>"
                    // Poll the root URL every 2 s; redirect as soon as the device responds.
                    "function poll(){"
                    "  fetch('/').then(r=>{ if(r.ok) location.href='/'; else setTimeout(poll,2000); })"
                    "           .catch(()=>setTimeout(poll,2000));}"
                    "setTimeout(poll,4000);" // Wait 4 s before first poll (restart takes ~3 s)
                    "</script></body></html>");
                scheduleRestart(2000); // Restart 2 s after response is sent
            });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "image/svg+xml", Favicon); });
  server.on("/favicon192.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "image/svg+xml", Favicon192); });
  server.on("/manifest.json", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", Manifest); });
  // AP first-boot configuration page
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", SetupAPHtml); });

  // Wi-Fi scan for AP setup page (works in WIFI_AP_STA mode)
  server.on("/scanWifi", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                String json = "[";
                int n = WiFi.scanNetworks();
                for (int i = 0; i < n; i++) {
                    if (i) json += ",";
                    // Escape quotes inside SSID name
                    String s = WiFi.SSID(i);
                    s.replace("\"", "\\\"");
                    json += "\"" + s + "\"";
                }
                json += "]";
                WiFi.scanDelete();
                request->send(200, "application/json", json); });

  // Save configuration from AP setup form and restart
  server.on("/saveConfig", HTTP_POST, [](AsyncWebServerRequest *request)
            {
                String ssidVal = request->hasParam("ssid", true)
                                 ? request->getParam("ssid", true)->value() : "";
                ssidVal.trim();
                if (ssidVal.length() == 0) {
                    request->send(400, "text/plain", "SSID required");
                    return;
                }
                ssid = ssidVal;
                password = request->getParam("password", true)->value();
                String sensor = request->hasParam("sensor", true)
                                ? request->getParam("sensor", true)->value() : "libre";
                sensorType = (sensor == "dexcom")      ? SENSOR_DEXCOM
                           : (sensor == "nightscout") ? SENSOR_NIGHTSCOUT
                           : SENSOR_LIBRE;
                if (request->hasParam("email",     true)) libreEmail     = request->getParam("email",     true)->value();
                if (request->hasParam("librepass", true)) librePass      = request->getParam("librepass", true)->value();
                if (request->hasParam("librezone", true)) libreZone      = request->getParam("librezone", true)->value();
                if (request->hasParam("dexuser",   true)) dexcomUsername = request->getParam("dexuser",   true)->value();
                if (request->hasParam("dexpass",   true)) dexcomPassword = request->getParam("dexpass",   true)->value();
                if (request->hasParam("dexregion", true)) dexcomRegion   = request->getParam("dexregion", true)->value();
                if (request->hasParam("nsurl",     true)) nightscoutUrl   = request->getParam("nsurl",     true)->value();
                if (request->hasParam("nstoken",   true)) nightscoutToken = request->getParam("nstoken",   true)->value();
                if (request->hasParam("lang",            true)) LaLangue      = (int8_t)   request->getParam("lang",            true)->value().toInt();
                if (request->hasParam("timezone",        true)) idxFuseau     =            request->getParam("timezone",        true)->value().toInt();
                if (request->hasParam("glucoseUnit",     true)) glucoseUnit   = (GlucoseUnit)request->getParam("glucoseUnit",     true)->value().toInt();
                if (request->hasParam("glucoseRangeMin", true)) glucoseRangeMin =            request->getParam("glucoseRangeMin", true)->value().toInt();
                if (request->hasParam("targetLow",       true)) targetLow     =            request->getParam("targetLow",       true)->value().toInt();
                if (request->hasParam("targetHigh",      true)) targetHigh    =            request->getParam("targetHigh",      true)->value().toInt();
                if (request->hasParam("glucoseWarn",     true)) glucoseWarn   =            request->getParam("glucoseWarn",     true)->value().toInt();
                if (request->hasParam("glucoseRangeMax", true)) glucoseRangeMax =           request->getParam("glucoseRangeMax", true)->value().toInt();
                RecordFichierParametres();
                request->send(200, "text/plain", "OK");
                scheduleRestart(500); // Non-blocking restart after response is sent
            });

  server.onNotFound(notFound);

  server.begin();
  EcranPrintln(T("Serveur80"));
}

void notFound(AsyncWebServerRequest *request)
{
  // In AP mode redirect every unknown URL to the setup page (captive portal behaviour)
  if (apModeActive) {
    request->redirect("http://192.168.4.1/config");
    return;
  }
  request->send(404, "text/plain", "Not found");
}

void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{ // Mise à jour par OTA
  char progress[30];
  if (!index)
  {
    EcranPrintln(T("Update"));
    // content_len = request->contentLength();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
    {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len)
  {
    Update.printError(Serial);
    sprintf(progress, "Progress: %d%%\n", (Update.progress() * 100) / Update.size());
    EcranPrintln(String(progress));
  }

  if (final)
  {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true))
    {
      Update.printError(Serial);
    }
    else
    {
      EcranPrintln(T("UpdateComplete"));
      Serial.flush();
      ESP.restart();
    }
  }
}