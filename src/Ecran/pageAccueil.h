#pragma once
#include <Arduino.h>
#include "Config.h"
#include <Arduino_GFX_Library.h>
#include "Ecran/Gestion.h"
#include "Heure.h"

void AccueilInit();
void AccueiLoop();
void Trace_Gauge(Arduino_Canvas *canva, int cx, int cy, int r_inner, int r_outer);
void AccueilHandleTouch(uint16_t x, uint16_t y);