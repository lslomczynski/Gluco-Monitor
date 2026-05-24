#pragma once
#include "Config.h"
#include <Arduino.h>
#include "Heure.h"
#include "Ecran/Gestion.h"

bool testNightScoutConnection();
void getNightScoutReadings();
void LectureNightScout();
void clearNightScoutCache();
