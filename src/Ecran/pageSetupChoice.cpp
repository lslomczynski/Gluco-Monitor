#include "Ecran/pageSetupChoice.h"
#include "Ecran/pageAPInfo.h"
#include "Ecran/pageWifiList.h"
#include "Ecran/Gestion.h"
#include "Config.h"
#include "Internet.h"
#include "Langues/Langue.h"
#include <Arduino.h>

static Bouton BoutonOnScreen;
static Bouton BoutonAP;

void pageSetupChoiceSetup()
{
    PageActu = pageSetupChoice;

    CanvaBase->fillScreen(RGB565_BLACK);
    CanvaBase->setTextColor(RGB565_WHITE);

    // Title
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    PrintCentre(CanvaBase, "Gluco-Monitor", EcranW / 2, 50, 1);

    // Two large buttons — centred on the landscape screen (EcranW≈480, EcranH≈320)
    int16_t btnX = 40;
    int16_t btnW = EcranW - 80;
    int16_t btnH = 90;

    BoutonOnScreen = {btnX, 90,  btnW, btnH, T("SetupOnScreen")};
    BoutonAP       = {btnX, 210, btnW, btnH, T("SetupAP")};

    Bouton_Trace(BoutonOnScreen);
    Bouton_Trace(BoutonAP);

    CanvaBase->flush();
}

void handleTouch_SetupChoice(uint16_t touchX, uint16_t touchY)
{
    if (Bouton_Appui(BoutonOnScreen, touchX, touchY))
    {
        WifiListSetup();
    }
    else if (Bouton_Appui(BoutonAP, touchX, touchY))
    {
        StartAPMode();
        pageAPInfoSetup();
    }
}
