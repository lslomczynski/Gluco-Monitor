#pragma once
#include "Config.h"
#include <Arduino.h>

void Init_Internet();
bool Liste_WIFI();
void loopWifiReconnect();

// Access Point mode for first-boot configuration
void StartAPMode();
void StopAPMode();
extern bool apModeActive;