#pragma once
#include "Config.h"
#include <Arduino.h>
#include "Langues/Langue.h"

void WifiListSetup();
void handleTouch_WifiList(uint16_t touchX, uint16_t touchY);
void wifiRestorePreviousSsid(); // restores ssid to pre-selection value (called by Cancel in pageClavier)