#include "pageCompte.h"
#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Ecran/Gestion.h>
#include <Ecran/pageClavier.h>
#include <Ecran/pageLibreServeur.h>
#include <Stock.h>
#include "Langues/Langue.h"
#include "Libreview.h"
#include "Dexcom.h"
#include "NightScout.h"

static Bouton Boutons[5] = {
    {15, 40, 150, 30, "Sensor Type"},
    {15, 288, 110, 30, "Modifier"},
    {15, 288, 110, 30, "Modifier"},
    {15, 288, 110, 30, "Modifier"},
    {165, 288, 200, 30, "Tester"}};
void drawPara(String Titre, String Valeur, int H0, int index);

void CompteSetup()
{
    PageActu = pageCompte;
    Boutons[1].Texte = T("Modifier");
    Boutons[2].Texte = T("Modifier");
    Boutons[3].Texte = T("Modifier");
    Boutons[4].Texte = T("Tester");
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    CanvaBase->fillScreen(C_grisFonce);

    // --- Line 1: "Sensor Type" label ---
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    PrintCentre(CanvaBase, T("SensorType"), EcranW / 2, 25, 1);

    // --- Line 2: three sensor-type buttons (FreeStyle / Dexcom / NightScout) ---
    // 4 gaps × 7 px = 28 px total; remaining width / 3 per button.
    // All X positions computed from bw so layout scales with EcranW.
    int bw    = (EcranW - 28) / 3;
    int btn1X = 7;
    int btn2X = btn1X + bw + 7;
    int btn3X = btn2X + bw + 7;
    // All buttons: Y=35, height=35, text baseline Y≈55

    uint16_t libreColor      = (sensorType == SENSOR_LIBRE)      ? RGB565_GREEN : RGB565_NAVY;
    uint16_t dexcomColor     = (sensorType == SENSOR_DEXCOM)     ? RGB565_GREEN : RGB565_NAVY;
    uint16_t nightscoutColor = (sensorType == SENSOR_NIGHTSCOUT) ? RGB565_GREEN : RGB565_NAVY;

    CanvaBase->fillRoundRect(btn1X, 35, bw, 35, 8, libreColor);
    CanvaBase->drawRoundRect(btn1X, 35, bw, 35, 8, RGB565_WHITE);

    CanvaBase->fillRoundRect(btn2X, 35, bw, 35, 8, dexcomColor);
    CanvaBase->drawRoundRect(btn2X, 35, bw, 35, 8, RGB565_WHITE);

    CanvaBase->fillRoundRect(btn3X, 35, bw, 35, 8, nightscoutColor);
    CanvaBase->drawRoundRect(btn3X, 35, bw, 35, 8, RGB565_WHITE);

    CanvaBase->setFont(u8g2_font_helvB14_tf);
    // Compute baseline Y so the label is vertically centred inside the button.
    // Button spans Y=35 to Y=70 (height=35). Use the Bouton_Trace formula:
    //   baseline = buttonY0 + (buttonH + textHeight) / 2 - 2
    {
        int16_t tx1, ty1; uint16_t tw, th;
        CanvaBase->getTextBounds("FreeStyle", 0, 0, &tx1, &ty1, &tw, &th);
        int textY = 35 + (35 + (int)th) / 2 - 2;
        PrintCentre(CanvaBase, "FreeStyle",  btn1X + bw / 2, textY, 1);
        PrintCentre(CanvaBase, "Dexcom",     btn2X + bw / 2, textY, 1);
        PrintCentre(CanvaBase, "NightScout", btn3X + bw / 2, textY, 1);
    }

    // --- Line 3 + credentials, depend on selected sensor ---
    if (sensorType == SENSOR_LIBRE)
    {
        PrintCentre(CanvaBase, T("Compte") + " LibreLinkUp", EcranW / 2, 95, 1);
        drawPara("Email", libreEmail, 110, 1);
        drawPara(T("Password"), librePass, 170, 2);
        String zoneAffichee = T("Undefined");
        for (int i = 0; i < 12; i++)
        {
            if (strlen(regions[i]) > 0 && String(regionsCode[i]) == libreZone)
            {
                zoneAffichee = regions[i];
                break;
            }
        }
        drawPara(T("ServerZone"), zoneAffichee, 230, 3);
    }
    else if (sensorType == SENSOR_DEXCOM)
    {
        PrintCentre(CanvaBase, T("Compte") + " Dexcom Share", EcranW / 2, 95, 1);
        drawPara(T("Username"), dexcomUsername, 110, 1);
        drawPara(T("Password"), dexcomPassword, 170, 2);

        // Region selection with radio buttons (centred)
        CanvaBase->setFont(u8g2_font_helvB14_tf);
        CanvaBase->fillRoundRect(7, 230, EcranW - 14, 50, 8, RGB565_NAVY);
        CanvaBase->drawRoundRect(7, 230, EcranW - 14, 50, 8, RGB565_WHITE);
        PrintCentre(CanvaBase, T("Region"), EcranW2, 250, 1);

        int radioY = 265;
        int radioR = 8;
        int textOffset = 15;

        int totalWidth = 83 + 20 + 48 + 20 + 48;
        int startX = EcranW2 - (totalWidth / 2);

        int radioX1 = startX + 8;
        int radioX2 = radioX1 + 83 + 20 + 8;
        int radioX3 = radioX2 + 48 + 20 + 8;

        CanvaBase->fillCircle(radioX1, radioY, radioR, RGB565_WHITE);
        CanvaBase->fillCircle(radioX1, radioY, radioR - 3,
            (dexcomRegion != "US" && dexcomRegion != "JP") ? RGB565_GREEN : RGB565_NAVY);
        CanvaBase->setFont(u8g2_font_helvB14_tf);
        CanvaBase->setCursor(radioX1 + textOffset, radioY + 5);
        CanvaBase->print("Non-US");

        CanvaBase->fillCircle(radioX2, radioY, radioR, RGB565_WHITE);
        CanvaBase->fillCircle(radioX2, radioY, radioR - 3,
            (dexcomRegion == "US") ? RGB565_GREEN : RGB565_NAVY);
        CanvaBase->setCursor(radioX2 + textOffset, radioY + 5);
        CanvaBase->print("US");

        CanvaBase->fillCircle(radioX3, radioY, radioR, RGB565_WHITE);
        CanvaBase->fillCircle(radioX3, radioY, radioR - 3,
            (dexcomRegion == "JP") ? RGB565_GREEN : RGB565_NAVY);
        CanvaBase->setCursor(radioX3 + textOffset, radioY + 5);
        CanvaBase->print("JP");
    }
    else // SENSOR_NIGHTSCOUT
    {
        PrintCentre(CanvaBase, T("Compte") + " NightScout", EcranW / 2, 95, 1);
        drawPara(T("NightScoutURL"),   nightscoutUrl,   110, 1);
        drawPara(T("NightScoutToken"), nightscoutToken, 170, 2);
        // No third field (no region selector needed for NightScout)
    }

    Bouton_Trace(Boutons[4]); // Test button
    CanvaBase->flush();
}
void drawPara(String Titre, String Valeur, int H0, int index)
{
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    CanvaBase->fillRoundRect(7, H0, EcranW - 14, 50, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(7, H0, EcranW - 14, 50, 8, RGB565_WHITE);

    PrintCentre(CanvaBase,  Titre , EcranW2, H0 + 20, 1);
    CanvaBase->setFont(u8g2_font_10x20_mf );
    PrintGauche(CanvaBase, Valeur, 10, H0 + 40, 1);
    Boutons[index].X0 = EcranW - 122;
    Boutons[index].Y0 = H0 + 10;
    Bouton_Trace(Boutons[index]);
}

void handleTouch_Compte(uint16_t touchX, uint16_t touchY)
{
    // --- Sensor type selection: three buttons (Y=35..70) ---
    if (touchY >= 35 && touchY <= 70)
    {
        // Mirror the geometry from CompteSetup() exactly
        int bw    = (EcranW - 28) / 3;
        int btn1X = 7;
        int btn2X = btn1X + bw + 7;
        int btn3X = btn2X + bw + 7;

        SensorType newType = sensorType;
        if      (touchX >= btn1X && touchX < btn1X + bw) newType = SENSOR_LIBRE;
        else if (touchX >= btn2X && touchX < btn2X + bw) newType = SENSOR_DEXCOM;
        else if (touchX >= btn3X && touchX < btn3X + bw) newType = SENSOR_NIGHTSCOUT;

        if (newType != sensorType) {
            sensorType = newType;
            clearData(); // reset cache when switching sensor type
            RecordFichierParametres();
            CompteSetup();
        }
        return;
    }

    // --- Edit button 1: Email / Username / NightScout URL ---
    if (Bouton_Appui(Boutons[1], touchX, touchY))
    {
        if      (sensorType == SENSOR_LIBRE)       PageActu = pageClavier_CompteEmail;
        else if (sensorType == SENSOR_DEXCOM)      PageActu = pageClavier_DexcomUsername;
        else                                        PageActu = pageClavier_NightScoutUrl;
        setup_clavier();
    }
    // --- Edit button 2: Password / NightScout Token ---
    else if (Bouton_Appui(Boutons[2], touchX, touchY))
    {
        if      (sensorType == SENSOR_LIBRE)       PageActu = pageClavier_ComptePwd;
        else if (sensorType == SENSOR_DEXCOM)      PageActu = pageClavier_DexcomPwd;
        else                                        PageActu = pageClavier_NightScoutToken;
        setup_clavier();
    }
    // --- Dexcom region radio buttons (Y=230..280) ---
    else if (sensorType == SENSOR_DEXCOM && touchY >= 230 && touchY <= 280)
    {
        int totalWidth = 83 + 20 + 48 + 20 + 48;
        int startX = EcranW2 - (totalWidth / 2);
        int radioX1 = startX + 8;
        int radioX2 = radioX1 + 83 + 20 + 8;
        int radioX3 = radioX2 + 48 + 20 + 8;
        int touchRadius = 50;

        if (touchX >= radioX1 - touchRadius && touchX <= radioX1 + touchRadius) {
            if (dexcomRegion == "US" || dexcomRegion == "JP") {
                dexcomRegion = "Non-US";
                RecordFichierParametres();
                CompteSetup();
            }
            return;
        } else if (touchX >= radioX2 - touchRadius && touchX <= radioX2 + touchRadius) {
            if (dexcomRegion != "US") {
                dexcomRegion = "US";
                RecordFichierParametres();
                CompteSetup();
            }
            return;
        } else if (touchX >= radioX3 - touchRadius && touchX <= radioX3 + touchRadius) {
            if (dexcomRegion != "JP") {
                dexcomRegion = "JP";
                RecordFichierParametres();
                CompteSetup();
            }
            return;
        }
    }
    // --- Edit button 3: Server zone (LibreView only) ---
    else if (Bouton_Appui(Boutons[3], touchX, touchY))
    {
        if (sensorType == SENSOR_LIBRE)
            pageLibreServeurSetup();
    }
    // --- Test connection button ---
    else if (Bouton_Appui(Boutons[4], touchX, touchY))
    {
        CanvaBase->fillRect(0, 225, EcranW, 62, C_grisFonce);
        bool loginSuccess = false;

        if      (sensorType == SENSOR_LIBRE)       loginSuccess = loginLibreLinkUp();
        else if (sensorType == SENSOR_DEXCOM)      loginSuccess = loginDexcomShare();
        else                                        loginSuccess = testNightScoutConnection();

        if (loginSuccess)
        {
            CanvaBase->setFont(u8g2_font_helvB18_tf);
            PrintGauche(CanvaBase, T("ConnectOK"), 10, 258, 1);
            CanvaBase->flush();
            for (int i = 3; i > 0; i--) {
                CanvaBase->fillRect(EcranW - 82, 240, 78, 26, C_grisFonce);
                PrintCentre(CanvaBase, String(i) + "..", EcranW - 43, 258, 1);
                CanvaBase->flush();
                delay(1000);
            }
            PageActu = pageAccueil;
        }
        else
        {
            CanvaBase->setFont(u8g2_font_10x20_tf);
            if (ServerConnu)
                PrintCentre(CanvaBase, T("UserUnknown"),    EcranW / 2, 255, 1);
            else
                PrintCentre(CanvaBase, T("ServerNoAccess"), EcranW / 2, 255, 1);
            CanvaBase->flush();
        }
    }
}
