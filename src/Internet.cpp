#include "Internet.h"
#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <Serie.h>
#include "Heure.h"
#include "Stock.h"
#include "Server.h"
#include "Ecran/pageWifiList.h"
#include "Ecran/pageSetupChoice.h"
#include "Ecran/Gestion.h"
#include "Ecran/pageMessages.h"
#include "Langues/Langue.h"

// WIFI

int16_t ComSurv = 6; // Timeout sans Wifi par pas de 30s

// Backoff for loopWifiReconnect() — doubles from base up to a cap on each failed
// attempt so we stop hammering an AP that just rejected reassociation (e.g. reason
// 208 ASSOC_COMEBACK_TIME_TOO_LONG), and resets once a connection succeeds.
#define WIFI_RECONNECT_BACKOFF_BASE_MS 3000UL
#define WIFI_RECONNECT_BACKOFF_MAX_MS 60000UL
static unsigned long wifiReconnectBackoffMs = WIFI_RECONNECT_BACKOFF_BASE_MS;
static unsigned long lastWifiReconnectAttempt = 0;

// Access Point mode
bool apModeActive = false;
static DNSServer dnsServer;

String Liste_AP = "";
static uint8_t bestBSSID[6]; // Meilleur en dBm adresse MAC

String Format_WiFi(int num, const String &nom, int niveau, const String &MAC, int channel);
bool Liste_WIFI();

// Guard to ensure Init_Server() is called only once (AP → Cancel → AP path)
static bool serverStarted = false;

// Logs the raw 802.11 disconnect reason code and keeps a running count, so that
// after an unattended outage we can tell a real disconnect happened (and why),
// without needing a serial cable attached when it occurs.
static void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
    {
        wifiDisconnectCount++;
        lastWifiDisconnectReason = (int16_t)info.wifi_sta_disconnected.reason;
        lastWifiDisconnectMillis = millis();
        Serial.printf("WiFi disconnected, reason=%d (count=%u)\n",
                      lastWifiDisconnectReason, wifiDisconnectCount);
    }
    else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP)
    {
        // Successful (re)connect — start the next disconnect's backoff from scratch.
        wifiReconnectBackoffMs = WIFI_RECONNECT_BACKOFF_BASE_MS;
        // (Re)configure TZ/SNTP here rather than only at initial boot: if the boot-time
        // connect attempt below doesn't land within its wait window, this is the only
        // place TZ ever gets set for the rest of the run — otherwise localtime() silently
        // falls back to UTC (no DST offset) even once WiFi later reconnects.
        DefFuseauHoraire();
    }
}

// Reconnects with a growing backoff instead of the ESP32 core's default instant
// auto-reconnect (disabled via WiFi.setAutoReconnect(false) in Init_Internet()),
// so repeated AP rejections (e.g. reason 208 ASSOC_COMEBACK_TIME_TOO_LONG) don't
// turn into a disconnect storm that starves the task watchdog.
void loopWifiReconnect()
{
    if (WiFi.getMode() != WIFI_STA) return; // skip during AP/config-portal mode
    if (WiFi.status() == WL_CONNECTED) return;
    if (millis() - lastWifiReconnectAttempt < wifiReconnectBackoffMs) return;

    lastWifiReconnectAttempt = millis();
    Serial.printf("WiFi reconnect attempt (backoff was %lums)\n", wifiReconnectBackoffMs);
    WiFi.reconnect();
    wifiReconnectBackoffMs = min(wifiReconnectBackoffMs * 2, WIFI_RECONNECT_BACKOFF_MAX_MS);
}

// Start Wi-Fi Access Point for first-boot configuration
void StartAPMode()
{
    WiFi.mode(WIFI_AP_STA);
    delay(100); // Wait for mode switch before configuring softAP
    WiFi.softAP(hostname.c_str(), "monitor1"); // WPA2 minimum is 8 characters
    dnsServer.start(53, "*", WiFi.softAPIP()); // Redirect all DNS queries (captive portal)
    apModeActive = true;
    // Start web server after AP is up — TCP/IP stack is ready at this point
    if (!serverStarted) {
        Init_Server();
        serverStarted = true;
    }
}
 
// Stop Access Point mode and return to station-only mode
void StopAPMode()
{
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    apModeActive = false;
}

// ***********************************
// INIT INTERNET
// Configure adresses WIFI
// ***************************
void Init_Internet()
{
    String PointsMessage = "", PointsMessage2 = "", PointsMessage3 = "";
    hostname = String(HOSTNAME);
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    hostname += String(chipId); // Add chip ID to hostname
    EcranPrintln("Hostname : " + hostname);
    if (ssid.length() == 0)
    {
        // Show two-button choice: on-screen WiFi list vs Access Point setup
        // Init_Server() is called from StartAPMode() after the AP is up (TCP/IP stack ready)
        pageSetupChoiceSetup();
        // Blocking loop — exits only via ESP.restart() (from either setup path)
        while (true)
        {
            loopEcran();
            if (apModeActive) dnsServer.processNextRequest();
            delay(10);
        }
    }
    CanvaBase->fillRect(0, EcranH2, EcranW, EcranH2, RGB565_BLACK);

    CanvaBase->setFont(u8g2_font_helvB18_tf);
    PrintCentre(CanvaBase, T("InitWifi"), EcranW2, EcranH - 100, 1);
    CanvaBase->flush();

    WiFi.hostname(hostname);
    bool bestWifi = false;

    EcranPrintln(T("InitWifi"));

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.onEvent(onWifiEvent);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    bestWifi = Liste_WIFI();
    EcranPrint(T("Version"));
    EcranPrintln(Version);
    LireSerial();

    // Check WiFi connection
    // ... check mode
    if (WiFi.getMode() != WIFI_STA)
    {
        WiFi.mode(WIFI_STA);
        delay(10);
    }

    // WIFI

    EcranPrintln(T("Nom_WiFi") + ssid);
    EcranPrintln(T("MotDePasse") + password);
    if (ssid.length() > 0)
    {

        CanvaBase->fillRect(0, EcranH2, EcranW, EcranH2, RGB565_BLACK);
        CanvaBase->setFont(u8g2_font_helvB18_tf);
        PrintCentre(CanvaBase, "WiFi", EcranW2, EcranH - 100, 1);
        PrintCentre(CanvaBase, ssid, EcranW2, EcranH - 50, 1);
        CanvaBase->flush();
        EcranPrintln(T("RechercheWiFi") + ssid);
        if (bestWifi)
        {
            WiFi.begin(ssid.c_str(), password.c_str(), 0, bestBSSID); // Connexion forcée au BSSID choisi
        }
        else
        {
            WiFi.begin(ssid.c_str(), password.c_str());
        }
        WiFi.setSleep(false); // Disable WiFi modem power-save (experiment for intermittent-disconnect diagnosis)
        WiFi.setAutoReconnect(false); // We drive reconnects ourselves (loopWifiReconnect()) with backoff

        while (WiFi.status() != WL_CONNECTED && (millis() < 40000))
        { // Attente connexion au Wifi
            PointsMessage += ".";
            PrintCentre(CanvaBase, PointsMessage, EcranW2, EcranH - 10, 1);
            EcranPrint(".");
            EcranPrint(String(WiFi.status()));
            CanvaBase->flush();
            for (int i = 0; i < 10; i++)
            {
                delay(30);
                LireSerial();
            }
        }
        EcranPrintln("");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        MyIP = WiFi.localIP().toString();
        EcranPrintln(T("ConnectedWiFi") + MyIP + T("ou") + hostname + ".local");
        PointsMessage = T("ConnectedWiFi");
        PointsMessage2 = MyIP;
        PointsMessage3 = T("ou") + hostname + ".local";
        // TZ/SNTP configuration now happens in onWifiEvent()'s GOT_IP handler —
        // that fires here too, and also covers reconnects after the boot window.
    }
    else
    {
        EcranPrintln(T("EchecWiFi"));

        PointsMessage = T("EchecWiFi");
    }
    CanvaBase->setFont(u8g2_font_10x20_tf);
    CanvaBase->fillRect(0, EcranH2, EcranW, EcranH2, RGB565_BLACK);
    PrintCentre(CanvaBase, PointsMessage, EcranW2, EcranH - 120, 1);
    PrintCentre(CanvaBase, PointsMessage2, EcranW2, EcranH - 80, 1);
    PrintCentre(CanvaBase, PointsMessage3, EcranW2, EcranH - 40, 1);
    WiFi.scanDelete(); //
    CanvaBase->flush();
    delay(3000);
}

bool Liste_WIFI()
{
    int bestNetworkDb = -1000;
    bool bestFound = false;
    int n = 0;
    // WiFi.disconnect();
    delay(100);
    EcranPrintln(T("ScanStart"));
    // WiFi.scanNetworks will return the number of networks found.
    n = WiFi.scanNetworks();
    EcranPrintln(T("ScanTermine"));
    Liste_AP = "";
    if (n <= 0)
    {
        EcranPrintln(T("PasReseau"));
    }
    else
    {
        EcranPrint(String(n));
        EcranPrintln(T("reseauxTrouves"));
        EcranPrintln("|Nr|          SSID              |   RSSI  |       MAC       | Channel |");
        for (int i = 0; i < n; ++i)
        {
            // Print SSID and RSSI for each network found
            EcranPrintln(Format_WiFi(i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.BSSIDstr(i), WiFi.channel(i)));
            Liste_AP += WiFi.SSID(i).c_str() + RS + String(WiFi.RSSI(i)) + RS + WiFi.BSSIDstr(i).c_str() + RS + String(WiFi.channel(i)) + GS;
            if (WiFi.SSID(i) == ssid)
            {
                if (WiFi.RSSI(i) > bestNetworkDb)
                {
                    bestNetworkDb = WiFi.RSSI(i);
                    memcpy(bestBSSID, WiFi.BSSID(i), 6);
                    bestFound = true;
                }
            }
        }
    }
    WiFi.scanDelete();
    return bestFound;
}
String Format_WiFi(int num, const String &nom, int niveau, const String &MAC, int channel)
{
    char value[100];
    sprintf(value, "|%2d|%-28s|%4d dBm |%-16s|%9d|", num, nom.c_str(), niveau, MAC.c_str(), channel);
    return String(value);
}
