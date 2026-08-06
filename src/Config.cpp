#include "Config.h"
#include <Arduino.h>
#include "Dexcom.h"
#include "Libreview.h"
#include "NightScout.h"

//============ Version et Build ==========
const char* Version = PROG_VERSION;
const char* BuildDate = BUILD_DATE;

//======= VARIABLES ========
String ssid = "", password = "", hostname = "";
String MyIP = "0.0.0.0";

String  libreEmail = "";
String librePass = "";
String libreZone = "";
bool ServerConnu = false;

// Dexcom configuration
String dexcomUsername = "";
String dexcomPassword = "";
String dexcomRegion = "Non-US"; // Default to "Non-US" region

// NightScout configuration
String nightscoutUrl      = "";
String nightscoutToken    = "";
int8_t nightscoutIntervalMin = 5;

// MQTT configuration
String mqttBroker   = "";
uint16_t mqttPort   = 1883;
String mqttUser     = "";
String mqttPassword = "";
bool mqttEnabled    = false;
bool mqttScreenOff  = false;

// Sensor selection
SensorType sensorType = SENSOR_LIBRE; // Default to FreeStyle Libre

//Regions possibles pour LibreLinkUp
const char *regions[12] = {"General", "", "Europe", " Europe 2", "France", "Germany", "USA", "Canada", "Australia", "Japan ", "Asia Pacific", "UAE"};
const char *regionsCode[12] = {"", "", "eu", "eu2", "fr", "de", "us", "ca", "au", "jp", "ap", "ae"};

// Timers
unsigned long lastDemandeGlycMillis = 0;
unsigned long recurGlycMillis = 120000; // 2 minutes
unsigned long lastReceptionGlycMillis = 0;
unsigned long lastGlycOkMillis = 0;
long AgeGlycemie = 0;


// Heure
int8_t idxFuseau = 2; // Fuseau Horaire
int8_t Jour;        //-1=inconnu,0=dimanche,1=lundi...
bool HeureValide=false;
int16_t Int_Heure, Int_Minute;
String DATE, HEURE, DateAMJ, Hmn;
uint64_t T_On_seconde = 0;




// Glycémie
int16_t glucoseValues[MAX_POINTS]; // Tableau glycemie sur environ 24h
unsigned long glucoseHeure[MAX_POINTS]; // Heure glycemie sur environ  24h
int16_t pointCountGly = 0; // Nombre de points de glycémie actuellement stockés
String Glycemie = "";
int16_t GlycemieVal=0;
int8_t TrendArrow = 0; // 0=non défini, -1=Double flèche vers le bas, 1=Flèche vers le bas, 2=Flèche vers le bas à droite, 3=Flèche vers la droite, 4=Flèche vers le haut à droite, 5=Flèche vers le haut, 6=Double flèche vers le haut
unsigned long lastGlyUnixTime = 0; // Heure de la dernière glycémie reçue en format Unix Time
int16_t glucoseRangeMin=0,targetLow=70,targetHigh=180,glucoseWarn=300,glucoseRangeMax=400; //Seuils zone verte
GlucoseUnit glucoseUnit = GLUCOSE_UNIT_MGDL;
GlucoseColor glucoseColor = GLUCOSE_BLANC;


//Generaux
String ES = String((char)27); // ESC Separator
String FS = String((char)28); // File Separator
String GS = String((char)29); // Group Separator
String RS = String((char)30); // Record Separator
String US = String((char)31); // Unit Separator

int16_t LuminositeNuit=255;
int16_t LuminositeJour=255;
int16_t LuminositeCourante=255;
int8_t nightStartHour = 21;
int8_t nightStartMin  = 0;
int8_t nightEndHour   = 7;
int8_t nightEndMin    = 0;
bool   nightScheduleDisabled = false;
int16_t currentBrightness=255;
int8_t viewMode = 0;

bool SetupEnCours=true;
bool needsMqttStatePublish=false;

//======= Restart / WiFi diagnostics ============
RTC_NOINIT_ATTR uint32_t restartCauseTag;
String LastResetReasonStr = "";
String LastRestartCauseStr = "";
RTC_NOINIT_ATTR uint32_t wifiDisconnectCount;
RTC_NOINIT_ATTR int16_t lastWifiDisconnectReason;
RTC_NOINIT_ATTR unsigned long lastWifiDisconnectMillis;

//======= Page HTML Brute ============
bool AutorisationPageBrute=false;
bool needsConfigRedraw=false;
unsigned long TimerAutorisationBruteMillis=0;

// PSRAM
EXT_RAM_BSS_ATTR char MessageEcran[8192];
EXT_RAM_BSS_ATTR String LoginJSON = "", GraphJSON = "",ConnectionJSON = "";

String formatGlucoseValue(int16_t mgdl)
{
    if (glucoseUnit == GLUCOSE_UNIT_MMOLL)
    {
        return String(float(mgdl) / 18.0f, 1);
    }
    return String(mgdl);
}

String getGlucoseUnitLabel()
{
    if (glucoseUnit == GLUCOSE_UNIT_MMOLL)
    {
        return "mmol/L";
    }
    return "mg/dL";
}

void clearData()
{
    Serial.println("Clearing all data (glucose, Dexcom cache, LibreView cache)...");
    
    // Clear glucose arrays
    for (int i = 0; i < MAX_POINTS; i++) {
        glucoseValues[i] = 0;
        glucoseHeure[i] = 0;
    }
    
    // Reset glucose variables
    pointCountGly = 0;
    Glycemie = "";
    GlycemieVal = 0;
    TrendArrow = 0;
    lastGlyUnixTime = 0;
    
    // Clear JSON data
    LoginJSON = "";
    GraphJSON = "";
    ConnectionJSON = "";
    
    // Reset timers to force immediate data fetch
    lastDemandeGlycMillis = 0;
    lastReceptionGlycMillis = 0;
    lastGlycOkMillis = 0;
    AgeGlycemie = 0;
    
    // Clear Dexcom cache
    clearDexcomCache();

    // Clear LibreView cache
    clearLibreViewCache();

    // Clear NightScout cache
    clearNightScoutCache();
}
    