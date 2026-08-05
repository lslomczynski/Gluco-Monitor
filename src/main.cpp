/*
 * FreeStyle, Libre, and related brand marks are trademarks of Abbott Diabetes Care Inc. in various jurisdictions.
 Other trademarks are the property of their respective owners.
 * This software is not affiliated with Abbott Diabetes Care, Inc. or any of its subsidiaries

 * Dexcom and related brand marks are trademarks of Dexcom, Inc. in various jurisdictions. Other trademarks are the property of their respective owners.
 * This software is not affiliated with Dexcom, Inc. or any of its subsidiaries
 */

 // ============ Gluco-Monitor versions  ===========

 /*
v1.0 : 1er version de base / First Version Freestyle Only in mg/dL
v2.0 : Ajout du support du Dexcom  / Added Dexcom  support    
v3.0 : Ajout du support des unités mmol/L  / Added support for mmol/L units 
v3.1 : Choix couleurs ou blanc de la valeur de glycémie
       Choix à 10% de luminosité la nuit
v3.2 : Correction du mapping des flèches de tendance Dexcom
       Ajout de la flèche DoubleUp et DoubleDown
       Correction bug luminosité 10% la nuit
*/

//Support available on : https://F1ATB.fr  Documentation and Forum in French and English

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "mbedtls/md.h"
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include <esp_system.h>

#include "Heure.h"
#include "Libreview.h"
#include "Dexcom.h"
#include "NightScout.h"
#include "Config.h"
#include "Stock.h"
#include "Serie.h"
#include "Internet.h"
#include "Server.h"
#include "Ecran/Gestion.h"
#include "Ecran/pageMessages.h"
#include "Ecran/pageAccueil.h"
#include "Ecran/pageConfiguration.h"
#include "Ecran/pageLibreServeur.h"
#include "Ecran/pageCompte.h"
#include "Ecran/pageInfos.h"
#include "Ecran/pageFuseauH.h"
#include "Langues/Langue.h"
#include "MQTT.h"

static unsigned long testWatchdog = 0;

#define WDT_TIMEOUT_SECONDS 600 // Watchdog 10 minutes = 600 secondes

// Decode esp_reset_reason() to a short human-readable string
static String resetReasonToString(esp_reset_reason_t reason)
{
  switch (reason)
  {
    case ESP_RST_POWERON:   return "Power-on";
    case ESP_RST_EXT:       return "External pin";
    case ESP_RST_SW:        return "Software (ESP.restart)";
    case ESP_RST_PANIC:     return "Panic/exception";
    case ESP_RST_INT_WDT:   return "Interrupt watchdog";
    case ESP_RST_TASK_WDT:  return "Task watchdog";
    case ESP_RST_WDT:       return "Other watchdog";
    case ESP_RST_DEEPSLEEP: return "Deep sleep wake";
    case ESP_RST_BROWNOUT:  return "Brownout";
    case ESP_RST_SDIO:      return "SDIO";
    case ESP_RST_USB:       return "USB peripheral (flash/serial monitor)";
    case ESP_RST_JTAG:      return "JTAG";
    case ESP_RST_EFUSE:     return "Efuse error";
    case ESP_RST_PWR_GLITCH:return "Power glitch";
    case ESP_RST_CPU_LOCKUP:return "CPU lockup (double exception)";
    default:                return "Unknown (" + String((int)reason) + ")";
  }
}

// Decode the last restart's cause, combining esp_reset_reason() with the RTC-persisted
// tag set by AlertePasdeGlycemie() — both can produce ESP_RST_SW, so the tag is what
// tells the two apart. Clears the tag after reading so it doesn't leak into a future,
// unrelated restart.
static void LogRestartCause()
{
  esp_reset_reason_t reason = esp_reset_reason();
  LastResetReasonStr = resetReasonToString(reason);

  if (restartCauseTag == RESTART_TAG_NO_GLUCOSE_DATA)
  {
    LastRestartCauseStr = "No glucose data timeout";
    restartCauseTag = 0;
  }
  else if (reason == ESP_RST_TASK_WDT || reason == ESP_RST_INT_WDT ||
           reason == ESP_RST_WDT || reason == ESP_RST_PANIC ||
           reason == ESP_RST_CPU_LOCKUP)
  {
    LastRestartCauseStr = "Watchdog/crash (" + LastResetReasonStr + ")";
  }
  else if (reason == ESP_RST_BROWNOUT)
  {
    LastRestartCauseStr = "Brownout";
  }
  else
  {
    LastRestartCauseStr = "Normal/manual restart (" + LastResetReasonStr + ")";
  }
  Serial.println("Last restart cause: " + LastRestartCauseStr);
}

void setup()
{
  Serial.begin(115200);
  LogRestartCause();
  SetupEnCours=true;
  LaLangue = LANG_NONDEF;
  //=========== Watchdog initialisation ==========
  esp_task_wdt_deinit();
  esp_task_wdt_config_t wdt_cfg = {
      .timeout_ms = WDT_TIMEOUT_SECONDS * 1000UL,
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // Bitmask of all cores
      .trigger_panic = true,
  };
  esp_task_wdt_init(&wdt_cfg);

  // Abonner la tâche Arduino loop() au watchdog
  esp_task_wdt_add(NULL);
  esp_task_wdt_reset();
  delay(1);
  //======= Stockage =============
  for (int i = 0; i < MAX_POINTS; i++) // Tableau des glycémies sur 24h
  {
    glucoseValues[i] = 0;
    glucoseHeure[i] = 0;
  }

  LireSerial();
  InitStock(); // Init LittleFS
  if (psramInit())
  {
    Serial.println("PSRAM  correctement initialisée");
  }
  else
  {
    Serial.println("La PSRAM ne fonctionne pas");
  }
  LireSerial();

  //========== Anciens paramètres ==============
  ReadFichierParametres();
  LireSerial();
  // =========== Ecran =========================
  bool LangueNonDefini=false;
  if(LaLangue == LANG_NONDEF){
    LaLangue = LANG_EN; //Par defaut
    LangueNonDefini=true;
  }
  InitEcran();
  if (nightScheduleDisabled) {          // MQTT/HA owns brightness — restore last known level
      ledcWrite(GFX_BL, LuminositeCourante);
      currentBrightness = LuminositeCourante;
  }
  LireSerial();
  // ===== Definition de la langue si non encore definie ====
  // Skip if ssid is also empty — WiFi setup takes priority on first boot;
  // language/timezone can be changed later via the Configuration menu.
  if (LangueNonDefini && ssid.length() > 0)
  {
    QuestionConfiguration(T("Lang"), pageLangueSetup);
    QuestionConfiguration(T("F_Hor"), pageFuseauSetup);
  }
  // ============ Internet / Wifi et Heure ==============

  Init_Internet();
  CanvaBase->flush();
  esp_task_wdt_reset();
  delay(1);
  // Normal path (ssid set): Init_Server() is called here after WiFi connects.
  // First-boot path (ssid empty): Init_Internet() enters a blocking loop;
  // Init_Server() is called from StartAPMode() after the AP is configured.
  Init_Server();
  if (mqttEnabled) initMqtt();
  LireSerial();

  //  ========Modification du programme par le Wifi  - OTA(On The Air) ================

  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin(); // Mandatory

  LireSerial();

  //======== Demande compte LibreLinkUp ou Dexcom si non défini =====================
  if (sensorType == SENSOR_LIBRE && libreEmail.length() < 4)
  {
    QuestionConfiguration(T("SetLibreLinkUp"), CompteSetup);
  }
  else if (sensorType == SENSOR_DEXCOM && dexcomUsername.length() < 4)
  {
    QuestionConfiguration(T("SetDexcom"), CompteSetup);
  }

  esp_task_wdt_reset();
  delay(1);
  Serial.printf("PSRAM: %d\n", psramFound());
  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
  SetupEnCours=false;

}

void loop()
{

  LireSerial();
  
  // Periodically poll for NTP sync if not yet synchronized
  static unsigned long lastNTPCheck = 0;
  if (!HeureValide && millis() - lastNTPCheck > 5000)
  {
    lastNTPCheck = millis();
    CheckNTPSync();
  }
  
  if (HeureValide)
  {
    // Call appropriate sensor reading function based on sensor type
    if (sensorType == SENSOR_LIBRE)
    {
      LectureGlycemie();
    }
    else if (sensorType == SENSOR_DEXCOM)
    {
      LectureDexcom();
    }
    else if (sensorType == SENSOR_NIGHTSCOUT)
    {
      LectureNightScout();
    }
    FormatteHeureDate();
  }
  loopEcran();
  loopMqtt();

  if (needsConfigRedraw) {
    needsConfigRedraw = false;
    ParaInit();
    publishMqttState();
  }
  if (needsMqttStatePublish) {
    needsMqttStatePublish = false;
    publishMqttState();
  }

  //== Tests si fonctionnement nominal ============
  if (millis() - lastGlycOkMillis > 1210000) // Si on n'a pas réussi à récupérer une glycémie depuis plus de 20 minutes, on redémarre le module pour tenter de résoudre les problèmes de communication
    AlertePasdeGlycemie();

  if (HeureValide && lastGlyUnixTime > 0)
  {

    time_t now;
    time(&now);
    AgeGlycemie = (long)now - lastGlyUnixTime;
    if (AgeGlycemie > 1800 && millis() > 300000)
      AlertePasdeGlycemie(); // Pas de nouvelle mesure depuis 30mn. Exemple changement de capteur
  }
  if (millis() - testWatchdog > 10000)
  {
    testWatchdog = millis();
    if (WiFi.status() == WL_CONNECTED)
    {
      esp_task_wdt_reset(); // Reset du watchdog
      delay(1);
    }
  }

  //======= Page HTML Brute ============

  if (millis() - TimerAutorisationBruteMillis > 180000)
    AutorisationPageBrute = false; // Autorisation pour 3mn

  delay(2);
}
