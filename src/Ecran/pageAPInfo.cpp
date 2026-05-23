#include "Ecran/pageAPInfo.h"
#include "Ecran/pageSetupChoice.h"
#include "Ecran/Gestion.h"
#include "Config.h"
#include "Internet.h"
#include "Langues/Langue.h"
#include <Arduino.h>
#include <WiFi.h>

static Bouton BoutonCancel;

void pageAPInfoSetup()
{
    PageActu = pageAPInfo;

    CanvaBase->fillScreen(RGB565_BLACK);
    CanvaBase->setTextColor(RGB565_WHITE);

    // Title
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    PrintCentre(CanvaBase, T("APConnect"), EcranW / 2, 35, 1);

    // Network name
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    PrintCentre(CanvaBase, hostname, EcranW / 2, 100, 1);

    // Password
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    PrintCentre(CanvaBase, T("APPassword"), EcranW / 2, 150, 1);

    // Browser instructions
    PrintCentre(CanvaBase, T("APOpenBrowser"), EcranW / 2, 195, 1);

    // AP IP address (always 192.168.4.1 for softAP default)
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    PrintCentre(CanvaBase, WiFi.softAPIP().toString(), EcranW / 2, 240, 1);

    // Cancel button
    int16_t btnW = 200;
    BoutonCancel = {(int16_t)((EcranW - btnW) / 2), 268, btnW, 45, T("Cancel")};
    Bouton_Trace(BoutonCancel);

    CanvaBase->flush();
}

void handleTouch_APInfo(uint16_t touchX, uint16_t touchY)
{
    if (Bouton_Appui(BoutonCancel, touchX, touchY))
    {
        StopAPMode();
        pageSetupChoiceSetup();
    }
}
