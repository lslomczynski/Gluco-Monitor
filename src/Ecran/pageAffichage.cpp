#include "Ecran/pageAffichage.h"
#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include "Ecran/Gestion.h"
#include "Stock.h"
#include "Langues/Langue.h"

// Bottom-row radio buttons for Color (0-1) and Rotation (2-3).
// X0/Y0 positions computed dynamically in DrawBoutons() based on EcranW.
static RadioBouton Rboutons[4] = {
    {0, 0, 12, "Blanc"},
    {0, 0, 12, "Couleur"},
    {0, 0, 12, "0°"},
    {0, 0, 12, "180°"},
};

// ── Brightness helpers ───────────────────────────────────────────────────────

static const int16_t BRIGHT_LEVELS[10] = {26, 51, 77, 102, 128, 153, 179, 204, 230, 255};

static int8_t brightToStep(int16_t lev) {
    int pct = (lev * 100 + 127) / 255;
    return (int8_t)constrain((pct + 5) / 10, 1, 10);
}
static int16_t stepToBright(int8_t step) { return BRIGHT_LEVELS[step - 1]; }

// ── Time helpers ─────────────────────────────────────────────────────────────

static void addMinutes(int8_t &h, int8_t &m, int delta) {
    int total = ((h * 60 + m + delta) % 1440 + 1440) % 1440;
    h = (int8_t)(total / 60);
    m = (int8_t)(total % 60);
}
static String fmtTime(int8_t h, int8_t m) {
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    return String(buf);
}

// ── Layout constants ─────────────────────────────────────────────────────────
// halfW, leftX, rightX are computed at runtime from EcranW.
// Row Y positions (top of each section box):
static const int16_t ROW_DAY   = 50;
static const int16_t ROW_NIGHT = 140;
static const int16_t ROW_BOT   = 230;
static const int16_t ROW_H     = 80;

void DrawBoutons();

// ── Draw a +/− slider row ────────────────────────────────────────────────────
// cx = circle centre Y, minusX/plusX = circle centre X, value = display string.
// When 'enabled' is false, only the value is drawn (read-only).
static void drawSlider(int16_t cy, int16_t minusX, int16_t plusX,
                       const String &value, bool enabled)
{
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    if (enabled) {
        CanvaBase->fillCircle(minusX, cy, 18, C_grisFonce);
        CanvaBase->drawCircle(minusX, cy, 18, RGB565_WHITE);
        PrintCentre(CanvaBase, "-", minusX, cy + 7, 1);
        CanvaBase->fillCircle(plusX, cy, 18, C_grisFonce);
        CanvaBase->drawCircle(plusX, cy, 18, RGB565_WHITE);
        PrintCentre(CanvaBase, "+", plusX, cy + 7, 1);
    }
    PrintCentre(CanvaBase, value.c_str(), (minusX + plusX) / 2, cy + 7, 1);
}

// ── Setup ────────────────────────────────────────────────────────────────────

void pageAffichageSetup()
{
    PageActu = pageAffichage;
    CanvaBase->fillScreen(C_grisFonce);
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    PrintCentre(CanvaBase, T("Display"), EcranW / 2, 30, 1);

    int16_t halfW  = (EcranW - 20) / 2;
    int16_t leftX  = 7;
    int16_t rightX = leftX + halfW + 6;
    uint16_t schedBg = nightScheduleDisabled ? (uint16_t)0x4210 : (uint16_t)RGB565_NAVY;

    // Row 1: Day brightness (left) | Day starts at (right)
    CanvaBase->fillRoundRect(leftX,  ROW_DAY, halfW, ROW_H, 8, schedBg);
    CanvaBase->drawRoundRect(leftX,  ROW_DAY, halfW, ROW_H, 8, RGB565_WHITE);
    CanvaBase->fillRoundRect(rightX, ROW_DAY, halfW, ROW_H, 8, schedBg);
    CanvaBase->drawRoundRect(rightX, ROW_DAY, halfW, ROW_H, 8, RGB565_WHITE);
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    PrintCentre(CanvaBase, T("LuminositeJour"), leftX  + halfW / 2, ROW_DAY + 18, 1);
    PrintCentre(CanvaBase, T("DayStartsAt"),   rightX + halfW / 2, ROW_DAY + 18, 1);

    // Row 2: Night brightness (left) | Night starts at (right)
    CanvaBase->fillRoundRect(leftX,  ROW_NIGHT, halfW, ROW_H, 8, schedBg);
    CanvaBase->drawRoundRect(leftX,  ROW_NIGHT, halfW, ROW_H, 8, RGB565_WHITE);
    CanvaBase->fillRoundRect(rightX, ROW_NIGHT, halfW, ROW_H, 8, schedBg);
    CanvaBase->drawRoundRect(rightX, ROW_NIGHT, halfW, ROW_H, 8, RGB565_WHITE);
    PrintCentre(CanvaBase, T("Luminosite"),    leftX  + halfW / 2, ROW_NIGHT + 18, 1);
    PrintCentre(CanvaBase, T("NightStartsAt"), rightX + halfW / 2, ROW_NIGHT + 18, 1);

    // Row 3: Glucose Color (left) | Screen Rotation (right)
    CanvaBase->fillRoundRect(leftX,  ROW_BOT, halfW, ROW_H, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(leftX,  ROW_BOT, halfW, ROW_H, 8, RGB565_WHITE);
    CanvaBase->fillRoundRect(rightX, ROW_BOT, halfW, ROW_H, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(rightX, ROW_BOT, halfW, ROW_H, 8, RGB565_WHITE);
    PrintCentre(CanvaBase, T("CouleurGlycemie"), leftX  + halfW / 2, ROW_BOT + 18, 1);
    PrintCentre(CanvaBase, T("Rotation"),        rightX + halfW / 2, ROW_BOT + 18, 1);

    Rboutons[0].Texte = T("Blanc");
    Rboutons[1].Texte = T("Couleur");

    DrawBoutons();
    CanvaBase->flush();
}

// ── Touch handler ─────────────────────────────────────────────────────────────

void handleTouch_Affichage(uint16_t touchX, uint16_t touchY)
{
    int16_t halfW  = (EcranW - 20) / 2;
    int16_t leftX  = 7;
    int16_t rightX = leftX + halfW + 6;

    // Minus/plus hit zones within each half panel (relative to panel left edge)
    auto hitMinus = [&](int16_t panelX) {
        return touchX >= panelX + 5 && touchX <= panelX + 70;
    };
    auto hitPlus = [&](int16_t panelX) {
        return touchX >= panelX + halfW - 70 && touchX <= panelX + halfW - 5;
    };

    if (!nightScheduleDisabled) {
        // Row 1 — Day (y=50..130)
        if (touchY >= ROW_DAY + 10 && touchY <= ROW_DAY + ROW_H - 5) {
            bool changed = false;
            if (touchX < rightX) {
                // Left panel: Day brightness
                int16_t oldJour = LuminositeJour;
                int8_t step = brightToStep(LuminositeJour);
                if (hitMinus(leftX))      LuminositeJour = stepToBright((int8_t)max((int)step - 1, 1));
                else if (hitPlus(leftX))  LuminositeJour = stepToBright((int8_t)min((int)step + 1, 10));
                if (LuminositeJour != oldJour) {
                    if (currentBrightness == oldJour) {
                        ledcWrite(GFX_BL, LuminositeJour);
                        currentBrightness = LuminositeJour;
                    }
                    changed = true;
                }
            } else {
                // Right panel: Day starts at (nightEnd)
                int8_t h = nightEndHour, m = nightEndMin;
                if (hitMinus(rightX))     addMinutes(nightEndHour, nightEndMin, -30);
                else if (hitPlus(rightX)) addMinutes(nightEndHour, nightEndMin, +30);
                changed = (nightEndHour != h || nightEndMin != m);
            }
            if (changed) { needsMqttStatePublish = true; RecordFichierParametres(); pageAffichageSetup(); }
            return;
        }
        // Row 2 — Night (y=140..220)
        if (touchY >= ROW_NIGHT + 10 && touchY <= ROW_NIGHT + ROW_H - 5) {
            bool changed = false;
            if (touchX < rightX) {
                // Left panel: Night brightness
                int16_t oldNuit = LuminositeNuit;
                int8_t step = brightToStep(LuminositeNuit);
                if (hitMinus(leftX))      LuminositeNuit = stepToBright((int8_t)max((int)step - 1, 1));
                else if (hitPlus(leftX))  LuminositeNuit = stepToBright((int8_t)min((int)step + 1, 10));
                if (LuminositeNuit != oldNuit) {
                    if (currentBrightness == oldNuit) {
                        ledcWrite(GFX_BL, LuminositeNuit);
                        currentBrightness = LuminositeNuit;
                    }
                    changed = true;
                }
            } else {
                // Right panel: Night starts at (nightStart)
                int8_t h = nightStartHour, m = nightStartMin;
                if (hitMinus(rightX))     addMinutes(nightStartHour, nightStartMin, -30);
                else if (hitPlus(rightX)) addMinutes(nightStartHour, nightStartMin, +30);
                changed = (nightStartHour != h || nightStartMin != m);
            }
            if (changed) { needsMqttStatePublish = true; RecordFichierParametres(); pageAffichageSetup(); }
            return;
        }
    }

    // Row 3 — Color (0-1) and Rotation (2-3) radio buttons
    int8_t oldRotation = rotation;
    for (int i = 0; i < 4; i++) {
        if (RadioBouton_Appui(Rboutons[i], touchX, touchY)) {
            switch (i) {
            case 0: glucoseColor = GLUCOSE_BLANC;   break;
            case 1: glucoseColor = GLUCOSE_COULEUR; break;
            case 2: rotation = 1;                   break;
            case 3: rotation = 3;                   break;
            }
            DrawBoutons();
            if (oldRotation != rotation) {
                CanvaBase->setRotation(rotation);
                pageAffichageSetup();
            }
            RecordFichierParametres();
            CanvaBase->flush();
            return;
        }
    }
}

// ── Draw dynamic content (sliders + radio buttons) ────────────────────────────

void DrawBoutons()
{
    int16_t halfW  = (EcranW - 20) / 2;
    int16_t leftX  = 7;
    int16_t rightX = leftX + halfW + 6;
    bool    en     = !nightScheduleDisabled;

    int16_t lMinusX = leftX  + 35;
    int16_t lPlusX  = leftX  + halfW - 35;
    int16_t rMinusX = rightX + 35;
    int16_t rPlusX  = rightX + halfW - 35;

    // Row 1 — Day: brightness (left) | time (right)
    drawSlider(ROW_DAY   + 52, lMinusX, lPlusX,
               String(brightToStep(LuminositeJour) * 10) + "%", en);
    drawSlider(ROW_DAY   + 52, rMinusX, rPlusX,
               fmtTime(nightEndHour, nightEndMin), en);

    // Row 2 — Night: brightness (left) | time (right)
    drawSlider(ROW_NIGHT + 52, lMinusX, lPlusX,
               String(brightToStep(LuminositeNuit) * 10) + "%", en);
    drawSlider(ROW_NIGHT + 52, rMinusX, rPlusX,
               fmtTime(nightStartHour, nightStartMin), en);

    // Row 3 — Color and Rotation radio buttons (dynamic X0 based on EcranW)
    const int16_t R      = 12;
    const int16_t btnGap = 18;
    const int16_t btnY   = 270;

    auto placeButtons = [&](int16_t boxX, int idx) {
        int16_t u1 = (int16_t)Rboutons[idx].Texte.length() * 10;
        int16_t u2 = (int16_t)Rboutons[idx+1].Texte.length() * 10;
        int16_t w1 = 2*R + 2 + u1;
        int16_t w2 = 2*R + 2 + u2;
        int16_t margin = max((int16_t)4, (int16_t)((halfW - w1 - btnGap - w2) / 2));
        Rboutons[idx].X0   = boxX + margin;
        Rboutons[idx].Y0   = btnY;
        Rboutons[idx+1].X0 = Rboutons[idx].X0 + w1 + btnGap;
        Rboutons[idx+1].Y0 = btnY;
    };
    placeButtons(leftX,  0); // Color
    placeButtons(rightX, 2); // Rotation

    int16_t coul[2] = {0, 1};
    for (int i = 0; i < 2; i++) {
        RadioBouton_Trace(Rboutons[i],   glucoseColor == coul[i] ? (uint16_t)RGB565_BLUE : (uint16_t)C_grisFonce);
        RadioBouton_Trace(Rboutons[i+2], rotation == (i==0?1:3)  ? (uint16_t)RGB565_BLUE : (uint16_t)C_grisFonce);
    }
}
