#pragma once
#include <Arduino.h>

#define HOSTNAME "GlucoMonit-"

// Sensor types
enum SensorType {
    SENSOR_LIBRE = 0,
    SENSOR_DEXCOM = 1,
    SENSOR_NIGHTSCOUT = 2
};

enum GlucoseUnit {
    GLUCOSE_UNIT_MGDL = 0,
    GLUCOSE_UNIT_MMOLL = 1
};
//Couleur affichage glycemeie
enum GlucoseColor {
    GLUCOSE_BLANC = 0,
    GLUCOSE_COULEUR = 1
};

#define RecurrenceGlycemie 120000 // 2 minutes

//========= MACRO =========
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

//============ Version et Build ==========
extern const char* Version ;
extern const char* BuildDate ;

extern String ssid, password, hostname;
extern String MyIP;

extern String libreEmail;
extern String librePass;
extern String libreZone;
extern bool ServerConnu;

// Dexcom configuration
extern String dexcomUsername;
extern String dexcomPassword;
extern String dexcomRegion;

// NightScout configuration
extern String nightscoutUrl;
extern String nightscoutToken;
extern int8_t nightscoutIntervalMin; // 1, 2, or 5

// MQTT configuration
extern String mqttBroker;
extern uint16_t mqttPort;
extern String mqttUser;
extern String mqttPassword;
extern bool mqttEnabled;
extern bool mqttScreenOff;

// Sensor selection
extern SensorType sensorType;

extern const char *regions[12];
extern const char *regionsCode[12];

extern unsigned long lastDemandeGlycMillis, recurGlycMillis, lastReceptionGlycMillis, lastGlycOkMillis;
extern int8_t idxFuseau; // Fuseau Horaire
extern int8_t Jour;      //-1=inconnu,0=dimanche,1=lundi...
extern bool HeureValide;
extern int16_t Int_Heure, Int_Minute;
extern String DATE, HEURE, DateAMJ, Hmn;
extern long AgeGlycemie;
extern uint64_t T_On_seconde;

#define MAX_POINTS 300
extern int16_t glucoseValues[];
extern unsigned long glucoseHeure[];
extern int16_t pointCountGly;
extern String Glycemie;
extern int8_t TrendArrow;
extern unsigned long lastGlyUnixTime;
extern int16_t GlycemieVal,glucoseRangeMin,targetLow,targetHigh,glucoseWarn,glucoseRangeMax;
extern GlucoseUnit glucoseUnit;
extern GlucoseColor glucoseColor;

extern String ES, FS, GS, RS, US;

extern int16_t LuminositeNuit;
extern int16_t LuminositeJour;
extern int16_t LuminositeCourante;
extern int8_t nightStartHour;
extern int8_t nightStartMin;
extern int8_t nightEndHour;
extern int8_t nightEndMin;
extern bool   nightScheduleDisabled;
extern int16_t currentBrightness;
// Home screen layout: 0=Default (gauge+bar chart), 1=Gauge only (altView_01), 2=Value only (altView_02)
extern int8_t viewMode;

extern bool SetupEnCours;

//======= Page HTML Brute ============
extern bool AutorisationPageBrute;
extern unsigned long TimerAutorisationBruteMillis;

// PSRAM
extern EXT_RAM_BSS_ATTR char MessageEcran[];
extern EXT_RAM_BSS_ATTR String LoginJSON, GraphJSON, ConnectionJSON;

// Set to true to trigger a redraw of the on-screen config menu (e.g. after language change)
extern bool needsConfigRedraw;
// Set to true to publish MQTT state on the next loop() iteration
extern bool needsMqttStatePublish;

//======= Restart / WiFi diagnostics ============
// Survives resets (not power-on) — tags AlertePasdeGlycemie()'s restart so it can be
// told apart from other ESP_RST_SW restarts (manual /Restart, OTA, erase config...).
extern RTC_NOINIT_ATTR uint32_t restartCauseTag;
#define RESTART_TAG_NO_GLUCOSE_DATA 0x474D4E44u // "GMND"

extern String LastResetReasonStr;  // Raw esp_reset_reason(), decoded to text
extern String LastRestartCauseStr; // Interpreted cause, computed once at boot

extern uint32_t wifiDisconnectCount;
extern int16_t lastWifiDisconnectReason; // Raw wifi_err_reason_t code (0-255)
extern unsigned long lastWifiDisconnectMillis;

// Clear all data (glucose, Dexcom cache, LibreView cache) when switching accounts
void clearData();

String formatGlucoseValue(int16_t mgdl);
String getGlucoseUnitLabel();
