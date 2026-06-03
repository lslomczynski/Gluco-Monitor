#include <Arduino.h>
#include <Heure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "NightScout.h"
#include "Langues/Langue.h"

// Map NightScout direction string to the project's TrendArrow int8_t.
// NightScout uses the same direction names as Dexcom Share.
static int8_t mapNightScoutDirection(const char *dir)
{
    if (dir == nullptr) return 0;
    String d = String(dir);
    if (d == "DoubleUp")       return 6;
    if (d == "SingleUp")       return 5;
    if (d == "FortyFiveUp")    return 4;
    if (d == "Flat")           return 3;
    if (d == "FortyFiveDown")  return 2;
    if (d == "SingleDown")     return 1;
    if (d == "DoubleDown")     return -1;
    return 0; // NONE, NOT COMPUTABLE, RATE OUT OF RANGE, etc.
}

// Perform a lightweight connectivity and authentication check.
// Calls /api/v1/status.json and verifies HTTP 200 + {"status":"ok"}.
bool testNightScoutConnection()
{
    ServerConnu = false;

    if (nightscoutUrl.length() == 0) {
        Serial.println("NightScout URL not set");
        return false;
    }

    String url = nightscoutUrl + "/api/v1/status.json";
    if (nightscoutToken.length() > 0) {
        url += "?token=" + nightscoutToken;
    }

    Serial.println("NightScout connection test: " + url);

    HTTPClient https;
    https.begin(url);
    https.setTimeout(15000);
    https.addHeader("Accept", "application/json");

    int httpCode = https.GET();
    String response = https.getString();
    https.end();

    Serial.println("NightScout status HTTP code: " + String(httpCode));

    if (httpCode != HTTP_CODE_OK) {
        Serial.println("NightScout server unreachable: " + String(httpCode));
        EcranPrintln(HEURE + T("ServerNoAccess") + " (" + String(httpCode) + ")", RGB565_ORANGE);
        return false;
    }

    ServerConnu = true;

    // Parse and verify {"status":"ok"}
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Serial.println("NightScout status JSON parse error: " + String(error.c_str()));
        return false;
    }

    const char *status = doc["status"];
    if (status == nullptr || String(status) != "ok") {
        Serial.println("NightScout status not OK: " + response);
        return false;
    }

    LoginJSON = response;
    Serial.println("NightScout connection OK");
    return true;
}

// Fetch up to 288 entries (24h at 5-min intervals) from /api/v1/entries.json.
// Uses getString() (same pattern as Dexcom) for reliable buffered reads over HTTPS.
void getNightScoutReadings()
{
    if (nightscoutUrl.length() == 0) return;

    // 96 entries = 8 hours at 5-min CGM intervals — matches the ~5h LibreView window
    // and keeps bar chart bars wide enough to be readable (~3 px each at 290 px width).
    String url = nightscoutUrl + "/api/v1/entries.json?count=96";
    if (nightscoutToken.length() > 0) {
        url += "&token=" + nightscoutToken;
    }

    Serial.println("getNightScoutReadings URL: " + url);

    HTTPClient https;
    https.begin(url);
    https.setTimeout(15000);
    https.addHeader("Accept", "application/json");

    int httpCode = https.GET();
    Serial.println("NightScout entries HTTP code: " + String(httpCode));

    if (httpCode != HTTP_CODE_OK) {
        EcranPrintln(HEURE + T("GlucoFailed") + String(httpCode), RGB565_ORANGE);
        https.end();
        return;
    }

    // Buffer the full response before parsing — getStream() is unreliable for large
    // HTTPS responses with chunked transfer encoding on ESP32.
    String response = https.getString();
    https.end();

    Serial.println("NightScout response length: " + String(response.length()));
    GraphJSON = response;

    // Filter: retain only the three fields we need to minimise ArduinoJson heap usage.
    JsonDocument filter;
    filter[0]["sgv"]       = true;
    filter[0]["date"]      = true;
    filter[0]["direction"] = true;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response,
                                                 DeserializationOption::Filter(filter));

    if (error) {
        Serial.println("NightScout JSON parse error: " + String(error.c_str()));
        EcranPrintln(HEURE + T("GlucoFailed") + " (JSON error)", RGB565_ORANGE);
        return;
    }

    JsonArray readings = doc.as<JsonArray>();
    if (readings.size() == 0) {
        Serial.println("NightScout: no readings in response");
        EcranPrintln(HEURE + T("GlucoFailed") + " (no data)", RGB565_ORANGE);
        return;
    }

    // Most-recent entry is first in the NightScout array.
    JsonObject latest = readings[0];
    GlycemieVal = latest["sgv"].as<int16_t>();
    Glycemie    = String(GlycemieVal);

    TrendArrow = mapNightScoutDirection(latest["direction"]);

    // NightScout timestamps are Unix milliseconds.
    long long dateMs = latest["date"].as<long long>();
    lastGlyUnixTime  = (unsigned long)(dateMs / 1000LL);

    String DateGly = unixToTimestamp(lastGlyUnixTime);
    EcranPrintln(HEURE + T("LastGlyco") + formatGlucoseValue(GlycemieVal) + " " +
                 getGlucoseUnitLabel() + " " + T("le") + DateGly);

    Serial.println("NightScout: " + formatGlucoseValue(GlycemieVal) + " " +
                   getGlucoseUnitLabel() + "  trend=" + String(TrendArrow) +
                   "  ts=" + String(lastGlyUnixTime));

    // Fill the glucose history arrays (oldest → newest).
    pointCountGly = 0;
    for (int i = (int)readings.size() - 1; i >= 0; i--) {
        if (pointCountGly >= MAX_POINTS) break;
        int16_t sgv = readings[i]["sgv"].as<int16_t>();
        long long ts = readings[i]["date"].as<long long>();
        glucoseValues[pointCountGly] = sgv;
        glucoseHeure[pointCountGly]  = (unsigned long)(ts / 1000LL);
        pointCountGly++;
    }

    Serial.println("NightScout: " + String(pointCountGly) + " points stored");
    lastReceptionGlycMillis = millis();
    lastGlycOkMillis        = millis();
}

// Main polling function, called every loop() iteration when NightScout is active.
// Uses the same adaptive timing logic as LectureDexcom().
void LectureNightScout()
{
    if (nightscoutUrl.length() == 0) {
        // Throttle the warning to once per polling cycle rather than every 2 ms.
        if (millis() - lastReceptionGlycMillis > 30000 || lastDemandeGlycMillis == 0) {
            lastDemandeGlycMillis   = millis();
            lastReceptionGlycMillis = millis();
            EcranPrintln(T("NightScoutIndefini"));
        }
        return;
    }

    long intervalSec = (long)nightscoutIntervalMin * 60L + 15L; // +15 s margin
    recurGlycMillis  = (unsigned long)intervalSec * 1000UL;

    // Shorten only when data is genuinely stale: no new reading for 2+ full intervals
    if (AgeGlycemie > intervalSec * 2L) {
        recurGlycMillis = 90000;
    }

    // Timer-based guard (same pattern as LibreLinkUp): wait full recurGlycMillis
    // since last reception regardless of sensor-side data age — this ensures the
    // progress bar always fills to 100% before the next poll.
    if (millis() - lastReceptionGlycMillis < recurGlycMillis && lastDemandeGlycMillis != 0) {
        return;
    }

    lastDemandeGlycMillis = millis();
    Serial.println("Requesting NightScout glucose...");
    getNightScoutReadings();
    lastReceptionGlycMillis = millis();
}

// NightScout is stateless (JWT token in URL, no server-side session).
// Nothing to reset — function exists so Config.cpp clearData() can call it
// uniformly alongside clearDexcomCache() and clearLibreViewCache().
void clearNightScoutCache()
{
    Serial.println("NightScout cache cleared (stateless — nothing to reset)");
}
