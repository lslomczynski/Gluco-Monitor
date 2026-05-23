#include "Ecran/pageGlucoseSettings.h"
#include "Config.h"
#include <Arduino.h>
#include "Ecran/Gestion.h"
#include "Stock.h"
#include "Langues/Langue.h"

// Page number - added in Gestion.h as #define pageGlucoseSettings 90
#define pageGlucoseSettings 90

// Layout constants
#define GS_TOP_Y    44      // Y where rows start
#define GS_ROW_H    44      // height of each parameter row
#define GS_BTN_W    44      // width of +/- buttons
#define GS_BTN_H    34      // height of +/- buttons
#define GS_STEP     10       // increment/decrement step (mg/dL)

// ── Local copies edited on screen; committed on Save ──────────────────────────
static int16_t edit_glucoseRangeMin;
static int16_t edit_targetLow;
static int16_t edit_targetHigh;
static int16_t edit_glucoseWarn;
static int16_t edit_glucoseRangeMax;

// ── Buttons: 5 rows × 3 buttons (minus, value label area, plus) + 2 bottom ──
// Indices:  0-4 = minus buttons, 5-9 = plus buttons, 10 = Save, 11 = Cancel
static Bouton Btns[12];

// ── Error message (empty = no error) ──────────────────────────────────────────
static String errMsg = "";

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────────────────────
static void drawRows();
static void drawRow(uint8_t idx, int16_t value, uint16_t color, const String &label);
static void drawBottomButtons();
static void showError(const String &msg);
static bool validate();

// ─────────────────────────────────────────────────────────────────────────────
// Row labels (short, fits on screen)
// ─────────────────────────────────────────────────────────────────────────────
static const char* rowLabels[5] = {
    "Graph Min",
    "Target Low",
    "Target High",
    "Warning Value",
    "Graph Max"
};

// ─────────────────────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────────────────────
void pageGlucoseSettingsSetup()
{
    PageActu = pageGlucoseSettings;

    // Load current values into local edit copies
    edit_glucoseRangeMin = glucoseRangeMin;
    edit_targetLow     = targetLow;
    edit_targetHigh    = targetHigh;
    edit_glucoseWarn   = glucoseWarn;
    edit_glucoseRangeMax = glucoseRangeMax;
    errMsg = "";

    // ── Background ────────────────────────────────────────────────────────────
    CanvaBase->fillScreen(C_grisFonce);
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    PrintCentre(CanvaBase, T("ChartLimits"), EcranW / 2, 28, 1);

    // ── Panel border ─────────────────────────────────────────────────────────
    int16_t panelH = 5 * GS_ROW_H + 6;
    CanvaBase->fillRoundRect(7, GS_TOP_Y - 4, EcranW - 14, panelH, 6, RGB565_NAVY);
    CanvaBase->drawRoundRect(7, GS_TOP_Y - 4, EcranW - 14, panelH, 6, RGB565_WHITE);

    drawRows();
    drawBottomButtons();
    CanvaBase->flush();
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw all five parameter rows
// ─────────────────────────────────────────────────────────────────────────────
static void drawRows()
{
    int16_t values[5] = {
        edit_glucoseRangeMin,
        edit_targetLow,
        edit_targetHigh,
        edit_glucoseWarn,
        edit_glucoseRangeMax
    };

    // Color coding matches zone convention: red=low, green=target, orange=high, purple=very high
    uint16_t colors[5] = {
        RGB565_RED,    // glucoseRangeMin — chart floor (low/hypo zone)
        RGB565_GREEN,  // targetLow       — lower target boundary
        RGB565_GREEN,  // targetHigh      — upper target boundary
        RGB565_ORANGE, // glucoseWarn     — high zone threshold
        RGB565_PURPLE  // glucoseRangeMax — chart ceiling (very high zone)
    };

    for (uint8_t i = 0; i < 5; i++) {
        drawRow(i, values[i], colors[i], rowLabels[i]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw a single parameter row
//   idx   : 0-4 (top to bottom)
//   value : current value
//   color : accent colour for the label
//   label : short name
// ─────────────────────────────────────────────────────────────────────────────
static void drawRow(uint8_t idx, int16_t value, uint16_t color, const String &label)
{
    int16_t rowY  = GS_TOP_Y + idx * GS_ROW_H;          // top of this row
    int16_t midY  = rowY + GS_ROW_H / 2;                 // vertical centre

    // ── Erase row background ─────────────────────────────────────────────────
    CanvaBase->fillRect(9, rowY, EcranW - 18, GS_ROW_H - 2, RGB565_NAVY);

    // ── Label (left side, coloured) ──────────────────────────────────────────
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    CanvaBase->setTextColor(color);
    PrintGauche(CanvaBase, label, 18, midY + 5, 1);

    // ── Value (centre) — show in the user's chosen unit ──────────────────────
    String valStr;
    if (glucoseUnit == GLUCOSE_UNIT_MMOLL)
        valStr = String(value / 18.02f, 1) + " mmol/L";
    else
        valStr = String(value) + " mg/dL";
    CanvaBase->setTextColor(RGB565_WHITE);
    PrintCentre(CanvaBase, valStr, EcranW / 2, midY + 5, 1);

    // ── Minus button ─────────────────────────────────────────────────────────
    int16_t btnX_minus = EcranW - 10 - GS_BTN_W * 2 - 6;
    int16_t btnY       = midY - GS_BTN_H / 2;

    Btns[idx].X0 = btnX_minus;
    Btns[idx].Y0 = btnY;
    Btns[idx].W  = GS_BTN_W;
    Btns[idx].H  = GS_BTN_H;
    Btns[idx].Texte = "-";
    Bouton_Trace(Btns[idx], RGB565_WHITE, CanvaBase);
    CanvaBase->setTextColor(RGB565_WHITE);

    // ── Plus button ───────────────────────────────────────────────────────────
    int16_t btnX_plus = EcranW - 10 - GS_BTN_W;

    Btns[5 + idx].X0 = btnX_plus;
    Btns[5 + idx].Y0 = btnY;
    Btns[5 + idx].W  = GS_BTN_W;
    Btns[5 + idx].H  = GS_BTN_H;
    Btns[5 + idx].Texte = "+";
    Bouton_Trace(Btns[5 + idx], RGB565_WHITE, CanvaBase);
    CanvaBase->setTextColor(RGB565_WHITE);
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw Save / Cancel buttons at the bottom
// ─────────────────────────────────────────────────────────────────────────────
static void drawBottomButtons()
{
    int16_t bottomY = GS_TOP_Y + 5 * GS_ROW_H + 14;

    // Cancel
    Btns[10].X0 = 20;
    Btns[10].Y0 = bottomY - 3;
    Btns[10].W  = 120;
    Btns[10].H  = 40;
    Btns[10].Texte = "Cancel";
    Bouton_Trace(Btns[10], C_grisMoyen, CanvaBase);
    CanvaBase->setTextColor(RGB565_WHITE);
    //PrintCentre(CanvaBase, "Cancel", 20 + 60, bottomY + 22, 1);

    // Save
    Btns[11].X0 = EcranW - 140;
    Btns[11].Y0 = bottomY - 3;
    Btns[11].W  = 120;
    Btns[11].H  = 40;
    Btns[11].Texte = "Save";
    Bouton_Trace(Btns[11], RGB565_GREEN, CanvaBase);
    CanvaBase->setTextColor(RGB565_WHITE);
    //PrintCentre(CanvaBase, "Save", EcranW - 140 + 60, bottomY + 22, 1);

    // Error message area (cleared first)
    CanvaBase->fillRect(0, bottomY + 46, EcranW, 20, C_grisFonce);
    if (errMsg.length() > 0) {
        CanvaBase->setFont(u8g2_font_helvB12_tf);
        CanvaBase->setTextColor(RGB565_RED);
        PrintCentre(CanvaBase, errMsg, EcranW / 2, bottomY + 60, 1);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Validation: enforces glucoseRangeMin < targetLow < targetHigh < glucoseWarn < glucoseRangeMax
//             and sensible absolute bounds (40–600 mg/dL)
// ─────────────────────────────────────────────────────────────────────────────
static bool validate()
{
    if (edit_targetLow < 40 || edit_glucoseRangeMax > 600) {
        errMsg = (glucoseUnit == GLUCOSE_UNIT_MMOLL)
                 ? "Values: 2.2 - 33.3 mmol/L"
                 : "Values must be 40-600 mg/dL";
        return false;
    }
    if (!(edit_glucoseRangeMin < edit_targetLow)) {
        errMsg = "Graph Min must be < Target Low";
        return false;
    }
    if (!(edit_targetLow < edit_targetHigh)) {
        errMsg = "Target Low must be < Target High";
        return false;
    }
    if (!(edit_targetHigh < edit_glucoseWarn)) {
        errMsg = "Target High must be < Warning Value";
        return false;
    }
    if (!(edit_glucoseWarn < edit_glucoseRangeMax)) {
        errMsg = "Warning Value must be < Graph Max";
        return false;
    }
    errMsg = "";
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Touch handler
// ─────────────────────────────────────────────────────────────────────────────
void handleTouch_GlucoseSettings(uint16_t touchX, uint16_t touchY)
{
    int16_t *editVals[5] = {
        &edit_glucoseRangeMin,
        &edit_targetLow,
        &edit_targetHigh,
        &edit_glucoseWarn,
        &edit_glucoseRangeMax
    };

    uint16_t rowColors[5] = {
        RGB565_RED,    // glucoseRangeMin — chart floor (low/hypo zone)
        RGB565_GREEN,  // targetLow       — lower target boundary
        RGB565_GREEN,  // targetHigh      — upper target boundary
        RGB565_ORANGE, // glucoseWarn     — high zone threshold
        RGB565_PURPLE  // glucoseRangeMax — chart ceiling (very high zone)
    };

    // ── Minus buttons (0-4) ───────────────────────────────────────────────────
    for (uint8_t i = 0; i < 5; i++) {
        if (Bouton_Appui(Btns[i], touchX, touchY, CanvaBase)) {
            *editVals[i] -= GS_STEP;
            if (*editVals[i] < 0) *editVals[i] = 0;
            errMsg = "";
            drawRow(i, *editVals[i], rowColors[i], rowLabels[i]);
            drawBottomButtons();
            CanvaBase->flush();
            return;
        }
    }

    // ── Plus buttons (5-9) ────────────────────────────────────────────────────
    for (uint8_t i = 0; i < 5; i++) {
        if (Bouton_Appui(Btns[5 + i], touchX, touchY, CanvaBase)) {
            *editVals[i] += GS_STEP;
            if (*editVals[i] > 600) *editVals[i] = 600;
            errMsg = "";
            drawRow(i, *editVals[i], rowColors[i], rowLabels[i]);
            drawBottomButtons();
            CanvaBase->flush();
            return;
        }
    }

    // ── Cancel ────────────────────────────────────────────────────────────────
    if (Bouton_Appui(Btns[10], touchX, touchY, CanvaBase)) {
        // Restore original values and go back to Configuration
        extern void ParaInit();
        PageActu = pageConfiguration;
        ParaInit();
        CanvaConfig->flush();
        return;
    }

    // ── Save ──────────────────────────────────────────────────────────────────
    if (Bouton_Appui(Btns[11], touchX, touchY, CanvaBase)) {
        if (validate()) {
            // Commit to global variables
            glucoseRangeMin = edit_glucoseRangeMin;
            targetLow      = edit_targetLow;
            targetHigh     = edit_targetHigh;
            glucoseWarn    = edit_glucoseWarn;
            glucoseRangeMax = edit_glucoseRangeMax;

            // Persist to LittleFS
            RecordFichierParametres();

            // Return to Configuration page
            extern void ParaInit();
            PageActu = pageConfiguration;
            ParaInit();
            CanvaConfig->flush();
        } else {
            // Show error, stay on page
            drawBottomButtons();
            CanvaBase->flush();
        }
        return;
    }
}
