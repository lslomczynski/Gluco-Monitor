#include "Ecran/pageLangue.h"
#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include "Ecran/Gestion.h"
#include "Ecran/pageConfiguration.h"
#include "Stock.h"
#include "Langues/Langue.h"

// 6 languages: en, fr, de, es, it, pl
// Layout: 5 on row 1, pl alone on row 2
#define NB_LANG 6
#define NB_LANG_ROW1 5

static RadioBouton Rboutons[NB_LANG] = {
    {10, 100, 15, ""},
    {10, 100, 15, ""},
    {10, 100, 15, ""},
    {10, 100, 15, ""},
    {10, 100, 15, ""},
    {10, 100, 15, ""}};

static RadioBouton UnitBoutons[2] = {
    {10, 250, 15, "mg/dL"},
    {10, 250, 15, "mmol/L"}};

void DrawBoutons_();

void pageLangueSetup()
{
    PageActu = pageLangue;
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    CanvaBase->fillScreen(C_grisFonce);
    PrintCentre(CanvaBase, T("Lang"), EcranW / 2, 30, 1);

    CanvaBase->fillRoundRect(7, 50, EcranW - 14, 120, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(7, 50, EcranW - 14, 120, 8, RGB565_WHITE);

    CanvaBase->fillRoundRect(7, 220, EcranW - 14, 70, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(7, 220, EcranW - 14, 70, 8, RGB565_WHITE);
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    PrintCentre(CanvaBase, T("GlucoseUnit"), EcranW / 2, 210, 1);

    DrawBoutons_();
    CanvaBase->flush();
}

void handleTouch_Langue(uint16_t touchX, uint16_t touchY)
{
    for (int i = 0; i < NB_LANG; i++)
    {
        if (RadioBouton_Appui(Rboutons[i], touchX, touchY))
        {
            LaLangue = i;
            RecordFichierParametres();
            pageLangueSetup();
            ParaInit();
            return;
        }
    }

    for (int i = 0; i < 2; i++)
    {
        if (RadioBouton_Appui(UnitBoutons[i], touchX, touchY))
        {
            glucoseUnit = (i == 0) ? GLUCOSE_UNIT_MGDL : GLUCOSE_UNIT_MMOLL;
            RecordFichierParametres();
            pageLangueSetup();
            return;
        }
    }
}

void DrawBoutons_()
{
    // Row 1: 5 languages evenly distributed
    for (int i = 0; i < NB_LANG_ROW1; i++)
    {
        Rboutons[i].X0 = EcranW * (i * 2 + 1) / 12;
        Rboutons[i].Y0 = 70;
        Rboutons[i].Texte = LangueSymbole[i];
        if (i == LaLangue)
            RadioBouton_Trace(Rboutons[i], RGB565_BLUE);
        else
            RadioBouton_Trace(Rboutons[i]);
    }

    // Row 2: pl first in second row
    Rboutons[5].X0 = EcranW * (0 * 2 + 1) / 12;
    Rboutons[5].Y0 = 120;
    Rboutons[5].Texte = LangueSymbole[5];
    if (5 == LaLangue)
        RadioBouton_Trace(Rboutons[5], RGB565_BLUE);
    else
        RadioBouton_Trace(Rboutons[5]);

    // Glucose unit buttons
    for (int i = 0; i < 2; i++)
    {
        UnitBoutons[i].X0 = EcranW * (i + 1) / 4;
        UnitBoutons[i].Y0 = 240;
        if ((i == 0 && glucoseUnit == GLUCOSE_UNIT_MGDL) || (i == 1 && glucoseUnit == GLUCOSE_UNIT_MMOLL))
            RadioBouton_Trace(UnitBoutons[i], RGB565_BLUE);
        else
            RadioBouton_Trace(UnitBoutons[i]);
    }
}