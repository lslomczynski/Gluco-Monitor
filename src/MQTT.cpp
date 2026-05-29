#include "MQTT.h"
#include "Config.h"
#include "Heure.h"
#include "Ecran/Gestion.h"
#include "Stock.h"
#include <WiFi.h>
#include <PubSubClient.h>

static WiFiClient       mqttWifiClient;
static PubSubClient     mqttClient(mqttWifiClient);
static unsigned long    lastReconnectAttempt = 0;

// ---- Helpers ----------------------------------------------------------------

static String macSuffix() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toUpperCase();
    return mac.substring(6);
}

static String baseTopic() {
    return "gluco_monitor/" + macSuffix();
}

static int16_t pctToLevel(int pct) {
    return (int16_t)map(constrain(pct, 0, 100), 0, 100, 0, 255);
}
static int levelToPct(int16_t level) {
    return (int)map((long)level, 0, 255, 0, 100);
}

static String layoutName() {
    if (viewMode == 1) return "Gauge only";
    if (viewMode == 2) return "Value only";
    return "Default";
}
static int layoutFromName(const String& s) {
    if (s == "Gauge only") return 1;
    if (s == "Value only") return 2;
    return 0;
}

// ---- State ------------------------------------------------------------------

void publishMqttState() {
    if (!mqttClient.connected()) return;
    String state = String("{\"screen\":\"") + (mqttScreenOff ? "OFF" : "ON") + "\""
        + ",\"brightness\":"       + levelToPct(currentBrightness)
        + ",\"brightness_night\":" + levelToPct(LuminositeNuit)
        + ",\"layout\":\"" + layoutName() + "\"}";
    mqttClient.publish((baseTopic() + "/state").c_str(), state.c_str(), true);
}

// ---- Discovery --------------------------------------------------------------

static void publishDiscovery() {
    String mac6  = macSuffix();
    String devId = "gluco_monitor_" + mac6;
    String base  = baseTopic();

    String dev = "\"device\":{\"identifiers\":[\"" + devId + "\"],"
                 "\"name\":\"Gluco-Monitor\",\"model\":\"ESP32-S3\",\"manufacturer\":\"DIY\"}";

    auto pub = [&](const String& type, const String& entity, const String& payload) {
        String topic = "homeassistant/" + type + "/" + devId + "/" + entity + "/config";
        mqttClient.publish(topic.c_str(), payload.c_str(), true);
    };

    pub("switch", "screen",
        "{\"name\":\"Screen\",\"unique_id\":\"" + devId + "_screen\","
        + dev + ","
        "\"cmd_t\":\"" + base + "/cmd/screen\","
        "\"stat_t\":\"" + base + "/state\","
        "\"val_tpl\":\"{{ value_json.screen }}\","
        "\"pl_on\":\"ON\",\"pl_off\":\"OFF\"}");

    pub("number", "brightness",
        "{\"name\":\"Brightness\",\"unique_id\":\"" + devId + "_brightness\","
        + dev + ","
        "\"cmd_t\":\"" + base + "/cmd/brightness\","
        "\"stat_t\":\"" + base + "/state\","
        "\"val_tpl\":\"{{ value_json.brightness }}\","
        "\"min\":0,\"max\":100,\"unit_of_measurement\":\"%\"}");

    pub("number", "brightness_night",
        "{\"name\":\"Night Brightness\",\"unique_id\":\"" + devId + "_brightness_night\","
        + dev + ","
        "\"cmd_t\":\"" + base + "/cmd/brightness_night\","
        "\"stat_t\":\"" + base + "/state\","
        "\"val_tpl\":\"{{ value_json.brightness_night }}\","
        "\"min\":0,\"max\":100,\"unit_of_measurement\":\"%\"}");

    pub("select", "layout",
        "{\"name\":\"Layout\",\"unique_id\":\"" + devId + "_layout\","
        + dev + ","
        "\"cmd_t\":\"" + base + "/cmd/layout\","
        "\"stat_t\":\"" + base + "/state\","
        "\"val_tpl\":\"{{ value_json.layout }}\","
        "\"options\":[\"Default\",\"Gauge only\",\"Value only\"]}");
}

// ---- Message callback -------------------------------------------------------

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String t(topic);
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

    if (t.endsWith("/cmd/brightness") || t.endsWith("/cmd/brightness_night")) {
        LuminositeNuit = pctToLevel(msg.toInt());
        if (!mqttScreenOff) {
            ledcWrite(GFX_BL, LuminositeNuit);
            currentBrightness = LuminositeNuit;
        }
        RecordFichierParametres();
        publishMqttState();
    } else if (t.endsWith("/cmd/screen")) {
        if (msg == "ON") {
            mqttScreenOff = false;
            int16_t val = (Int_Heure >= 7 && Int_Heure < 21) ? 255 : LuminositeNuit;
            ledcWrite(GFX_BL, val);
            currentBrightness = val;
        } else {
            mqttScreenOff = true;
            ledcWrite(GFX_BL, 0);
            currentBrightness = 0;
        }
        publishMqttState();
    } else if (t.endsWith("/cmd/layout")) {
        viewMode = layoutFromName(msg);
        needsConfigRedraw = true;
        RecordFichierParametres();
        publishMqttState();
    }
}

// ---- Connect / reconnect ----------------------------------------------------

static bool mqttConnect() {
    String clientId = "GlucoMonitor_" + macSuffix();
    bool ok = (mqttUser.length() > 0)
        ? mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPassword.c_str())
        : mqttClient.connect(clientId.c_str());
    if (ok) {
        String base = baseTopic();
        mqttClient.subscribe((base + "/cmd/brightness").c_str());
        mqttClient.subscribe((base + "/cmd/brightness_night").c_str());
        mqttClient.subscribe((base + "/cmd/screen").c_str());
        mqttClient.subscribe((base + "/cmd/layout").c_str());
        publishDiscovery();
        publishMqttState();
        Serial.println("MQTT connected: " + base);
    } else {
        Serial.printf("MQTT connect failed, rc=%d\n", mqttClient.state());
    }
    return ok;
}

// ---- Public API -------------------------------------------------------------

bool testMqttConnection(const String& broker, uint16_t port,
                        const String& user, const String& pass) {
    if (broker.length() == 0) return false;
    WiFiClient wc;
    PubSubClient client(wc);
    client.setServer(broker.c_str(), port);
    bool ok = (user.length() > 0)
        ? client.connect("GlucoMonitor_test", user.c_str(), pass.c_str())
        : client.connect("GlucoMonitor_test");
    if (ok) client.disconnect();
    return ok;
}

void initMqtt() {
    if (mqttBroker.length() == 0) return;
    mqttClient.setServer(mqttBroker.c_str(), mqttPort);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(1024);
    mqttConnect();
}

void loopMqtt() {
    if (!mqttEnabled || WiFi.status() != WL_CONNECTED) return;
    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            mqttConnect();
        }
    } else {
        mqttClient.loop();
    }
}
