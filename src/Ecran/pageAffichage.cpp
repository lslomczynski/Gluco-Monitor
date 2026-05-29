#include "Ecran/pageAffichage.h"
#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include "Ecran/Gestion.h"
#include "Stock.h"
#include "Langues/Langue.h"

static RadioBouton Rboutons[8] = {
    {10, 90, 15, "10%"},
    {10, 90, 15, "25%"},
    {10, 90, 15, "50%"},
    {10, 90, 15, "100%"},
    {10, 180, 15, "0°"},
    {10, 180, 15, "180°"},
    {10, 270, 15, "Blanc"},
    {10, 270, 15, "Couleur"}};

void DrawBoutons();

void pageAffichageSetup()
{
    PageActu = pageAffichage;
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    CanvaBase->fillScreen(C_grisFonce);
    PrintCentre(CanvaBase, T("Display"), EcranW / 2, 30, 1);
    //============= Luminosité ==========================
    uint16_t lumBg = nightScheduleDisabled ? (uint16_t)0x4210 : (uint16_t)RGB565_NAVY;
    CanvaBase->fillRoundRect(7, 50, EcranW - 14, 80, 8, lumBg);
    CanvaBase->drawRoundRect(7, 50, EcranW - 14, 80, 8, RGB565_WHITE);
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    PrintCentre(CanvaBase, T("Luminosite"), EcranW / 2, 70, 1);
    if (nightScheduleDisabled) {
        CanvaBase->setFont(u8g2_font_helvB14_tf);
        CanvaBase->setTextColor(RGB565_WHITE);
        PrintCentre(CanvaBase, "Disabled. Visit Web UI to enable.", EcranW / 2, 105, 1);
    }

    //============= Rotation ==========================
    CanvaBase->fillRoundRect(7, 140, EcranW - 14, 80, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(7, 140, EcranW - 14, 80, 8, RGB565_WHITE);
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    PrintCentre(CanvaBase, T("Rotation"), EcranW / 2, 160, 1);

//============= Couleur Glycémie ======================
    CanvaBase->fillRoundRect(7, 230, EcranW - 14, 80, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(7, 230, EcranW - 14, 80, 8, RGB565_WHITE);
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    PrintCentre(CanvaBase, T("CouleurGlycemie"), EcranW / 2, 250, 1);

    // Update translatable labels for the colour radio buttons — must be set at runtime,
    // not in the static initializer (LaLangue is not yet set when static vars are initialized)
    Rboutons[6].Texte = T("Blanc");
    Rboutons[7].Texte = T("Couleur");
    DrawBoutons();

    CanvaBase->flush();
}

void handleTouch_Affichage(uint16_t touchX, uint16_t touchY)
{
    int8_t oldRotation=rotation;
    for (int i = 0; i < 8; i++)
    {
        if (i < 4 && nightScheduleDisabled) continue;
        if (RadioBouton_Appui(Rboutons[i], touchX, touchY))
        {
            switch (i)
            {
            case 0:
                LuminositeNuit = 26;   // 10%
                break;
            case 1:
                LuminositeNuit = 64;   // 25%
                break;
            case 2:
                LuminositeNuit = 128;  // 50%
                break;
            case 3:
                LuminositeNuit = 255;  // 100%
                break;
            case 4:
                rotation = 1;
                break;
            case 5:
                rotation = 3;
                break;
            case 6:
                glucoseColor = GLUCOSE_BLANC;
                break;
            case 7:
                glucoseColor = GLUCOSE_COULEUR;
                break;
            }
            
            DrawBoutons();
            if (i < 4)
            {
                ledcWrite(GFX_BL, LuminositeNuit);
                currentBrightness = LuminositeNuit;
                needsMqttStatePublish = true;
                delay(1000);
            }
            if (oldRotation!=rotation){
                CanvaBase->setRotation(rotation);
                pageAffichageSetup();
            }
            RecordFichierParametres();
        }
    }
}
void DrawBoutons()
{
    int16_t Seuil[4] = {26, 64, 128, 255};
    if (!nightScheduleDisabled) {
        for (int i = 0; i < 4; i++)
        {
            Rboutons[i].X0 = EcranW * (i * 3 + 2) / 13-20;
            if (LuminositeNuit == Seuil[i])
            {
                RadioBouton_Trace(Rboutons[i], RGB565_BLUE);
            }
            else
            {
                RadioBouton_Trace(Rboutons[i]);
            }
        }
    }
    int16_t rot[2] = {1, 3};
    for (int i = 4; i < 6; i++)
    {
        Rboutons[i].X0 = EcranW * (i * 2 -7) / 4-20;
        if (rotation == rot[i - 4])
        {
            RadioBouton_Trace(Rboutons[i], RGB565_BLUE);
        }
        else
        {
            RadioBouton_Trace(Rboutons[i]);
        }
    }
    int16_t coul[2] = {0, 1};
    for (int i = 6; i < 8; i++)
    {
        Rboutons[i].X0 = EcranW * (i * 2 -11) / 4-20;
        if (glucoseColor == coul[i - 6])
        {
            RadioBouton_Trace(Rboutons[i], RGB565_BLUE);
        }
        else
        {
            RadioBouton_Trace(Rboutons[i]);
        }
    }
}