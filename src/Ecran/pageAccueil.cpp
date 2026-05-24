#include "Ecran/pageAccueil.h"
#include "Config.h"
#include <Arduino.h>
#include "Ecran/Gestion.h"
#include "time.h"
#include "Langues/Langue.h"

static bool flipCouleurs = false;
static float dtReponse = 0.0;
static bool altView = false; // false = normal view with bar chart, true = large gauge view

void Trace_Gauge(Arduino_Canvas *canva, int cx, int cy, int r_inner, int r_outer);

void AccueilInit()
{
}

void AccueiLoop()
{
    CanvaAccueil->fillScreen(RGB565_BLACK);
    CanvaAccueil->setTextColor(RGB565_WHITE);
    int16_t W2 = EcranW / 2;

    // Layout parameters — altView: larger gauge, no bar chart
    // Normal:  C=160, R0≈91,  R1=140, valY=185
    // Alt:     C=265, R0=137, R1=210  (×1.5 both radii)
    //   valY = EcranH-30 = 290  → glucose baseline 30 px from screen bottom
    //   Arc top = C−R1 = 265−210 = 55 → 20 px below clock at y=35  ✓
    //   All y > C=265 is below the semicircle → zero arc overlap with text/unit
    //   Glucose: setTextSize(2) on inb63_mn → ~126 px effective height
    //   Unit: helvB14 centred at (240, 300) → between glucose bottom and progress bar
    int16_t C  = altView ? 245               : EcranH / 2;
    int16_t R0 = altView ? 137               : (int)(EcranH / 3.5f);
    int16_t R1 = altView ? 210               : EcranH / 2 - 20;
    int16_t valY  = altView ? EcranH - 30    : C + 25;   // glucose text baseline
    int16_t unitX = W2 + R0;                              // unit label left edge (normal view)

    int16_t Teta0 = -180;
    uint16_t Couleurs[]     = {RGB565_RED, RGB565_GREEN, RGB565_ORANGE, RGB565_PURPLE};
    uint16_t CouleursFond[] = {C_fondBas, C_fondCible, C_fondHaut, C_fondTresHaut};
    int16_t glucoseInfoColor = RGB565_WHITE;
    int seuilCoul[] = {glucoseRangeMin, targetLow, targetHigh, glucoseWarn, glucoseRangeMax};
    float rangeSpan = float(glucoseRangeMax - glucoseRangeMin);
    int idxCoul = 0;

    Trace_Gauge(CanvaAccueil, W2, C, R0, R1);

    // HEURE
    CanvaAccueil->setFont(u8g2_font_fub35_tf);
    if (HeureValide)
        PrintDroite(CanvaAccueil, Hmn, -1, EcranH / 9, 1);

    // Affiche Glycemie
    if (Glycemie == "")
    {
        CanvaAccueil->setFont(u8g2_font_helvB18_tf);
        bool hasCredentials = (sensorType == SENSOR_LIBRE      && libreEmail.length()    >= 4) ||
                              (sensorType == SENSOR_DEXCOM      && dexcomUsername.length() >= 4) ||
                              (sensorType == SENSOR_NIGHTSCOUT  && nightscoutUrl.length()  >= 8);
        if (ssid.length() == 0 || !hasCredentials)
            PrintCentre(CanvaAccueil, T("ConfNul"), W2, valY, 1);
        else
            PrintCentre(CanvaAccueil, T("WaitGluco"), W2, valY, 1);
    }
    else
    {
        bool tooOld = AgeGlycemie / 60 > 20;
        if (glucoseColor == GLUCOSE_COULEUR)
        {
            for (int c = 0; c < 4; c++)
                if (GlycemieVal > seuilCoul[c])
                    idxCoul = c;
            glucoseInfoColor = Couleurs[idxCoul];
        }
        glucoseInfoColor = tooOld ? RGB565(50, 50, 50) : glucoseInfoColor;

        if (tooOld)
        {
            CanvaAccueil->setFont(u8g2_font_helvB18_tf);
            int16_t x1, y1;
            uint16_t w, h;
            String text = T("WaitGluco");
            CanvaAccueil->getTextBounds(utf8ToLatin15(text), 0, 0, &x1, &y1, &w, &h);
            int16_t rectX = W2 - w / 2 - 2;
            int16_t rectY = EcranH / 9 - h - 2;
            CanvaAccueil->fillRect(rectX, rectY, w + 4, h + 8, RGB565_BLACK);
            CanvaAccueil->setTextColor(RGB565_RED);
            PrintCentre(CanvaAccueil, text, W2, EcranH / 9, 1);
        }

        CanvaAccueil->setTextColor(glucoseInfoColor);
        // Glucose value
        // Normal:  inb63_mn, PrintCentre Sz=1 → 63 px
        // Alt:     inb49_mn, PrintCentre Sz=2 → 49*2=98 px (fits in 210 radius with room for unit label below)
        // NOTE: PrintCentre internally calls setTextSize(Sz) — must pass Sz here, not before.
        if (altView) {
            CanvaAccueil->setFont(u8g2_font_inb49_mn);
            PrintCentre(CanvaAccueil, formatGlucoseValue(GlycemieVal), W2, valY, 2);
        } else {
            CanvaAccueil->setFont(u8g2_font_inb63_mn);
            PrintCentre(CanvaAccueil, formatGlucoseValue(GlycemieVal), W2, valY, 1);
        }
        CanvaAccueil->setTextSize(1);  // always restore after glucose print

        // Unit label
        // Normal: left-aligned at W2+R0, y = valY-5  (10x20 font)
        // Alt:    centred at x = W2 + (R0+R1)/2 = mid of right arc ring width
        //         y = valY → bottom edge of unit = bottom edge of glucose value
        //         font helvB14 (14 px),  size 1
        if (altView) {
            CanvaAccueil->setFont(u8g2_font_helvB14_tf);
            int16_t unitCX = W2 + (R0 + R1) / 2;   // = 240 + (137+210)/2 = 413
            PrintCentre(CanvaAccueil, getGlucoseUnitLabel(), unitCX, valY - 20, 1);
        } else {
            CanvaAccueil->setFont(u8g2_font_10x20_tf);
            PrintGauche(CanvaAccueil, getGlucoseUnitLabel(), unitX, valY - 5, 1);
        }

        glucoseInfoColor = tooOld ? RGB565(50, 50, 50) : RGB565_WHITE;

        // Needle angle — maps GlycemieVal onto the arc (same scale as Trace_Gauge)
        Teta0 = -180 + (int)(180.0f * (constrain((int)GlycemieVal, glucoseRangeMin, glucoseRangeMax) - glucoseRangeMin) / rangeSpan);
        if (Teta0 > 0)   Teta0 = 0;
        if (Teta0 < -180) Teta0 = -180;
        float Trad = float(Teta0) * 3.14f / 180.0f;
        int16_t R0n = (int16_t)(0.8f * R0); // needle base radius — separate from gauge ring radius
        CanvaAccueil->fillTriangle(
            W2 + R1  * cos(Trad), C + R1  * sin(Trad),
            W2 + R0n * cos(Trad - 0.2f), C + R0n * sin(Trad - 0.2f),
            W2 + R0n * cos(Trad + 0.2f), C + R0n * sin(Trad + 0.2f),
            glucoseInfoColor);

        // Flèche tendance — fixed screen position (unchanged in both views)
        int16_t X0 = EcranW / 6;
        int16_t Y0 = EcranH / 6;
        int16_t x0, y0, x1, y1, x2, y2, x3, y3, x4, y4;
        int16_t offset = 40;
        switch (TrendArrow)
        {
        case -1: // DoubleDown
            x0 = -20; y0 = 0; x1 = 0; y1 = 20; x2 = 20; y2 = 0;
            x3 = -10; y3 = -50; x4 = +10; y4 = -50;
            CanvaAccueil->fillTriangle(X0+x0-offset, Y0+y0, X0+x1-offset, Y0+y1, X0+x2-offset, Y0+y2, glucoseInfoColor);
            CanvaAccueil->fillTriangle(X0+x3-offset, Y0+y3, X0+x1-offset, Y0+y1, X0+x4-offset, Y0+y4, glucoseInfoColor);
            break;
        case 1:
            x0 = -20; y0 = 0; x1 = 0; y1 = 20; x2 = 20; y2 = 0;
            x3 = -10; y3 = -50; x4 = +10; y4 = -50;
            break;
        case 2:
            x0 = 0; y0 = 20; x1 = 20; y1 = 20; x2 = 20; y2 = 0;
            x3 = -30; y3 = -40; x4 = -40; y4 = -30;
            break;
        case 3:
            x0 = 0; y0 = 20; x1 = 20; y1 = 0; x2 = 0; y2 = -20;
            x3 = -50; y3 = -10; x4 = -50; y4 = +10;
            break;
        case 4:
            x0 = 20; y0 = 0; x1 = 20; y1 = -20; x2 = 0; y2 = -20;
            x3 = -30; y3 = +40; x4 = -40; y4 = +30;
            break;
        case 5:
            x0 = 20; y0 = 0; x1 = 0; y1 = -20; x2 = -20; y2 = 0;
            x3 = -10; y3 = 50; x4 = +10; y4 = 50;
            break;
        case 6: // DoubleUp
            x0 = 20; y0 = 0; x1 = 0; y1 = -20; x2 = -20; y2 = 0;
            x3 = -10; y3 = 50; x4 = +10; y4 = 50;
            CanvaAccueil->fillTriangle(X0+x0-offset, Y0+y0, X0+x1-offset, Y0+y1, X0+x2-offset, Y0+y2, glucoseInfoColor);
            CanvaAccueil->fillTriangle(X0+x3-offset, Y0+y3, X0+x1-offset, Y0+y1, X0+x4-offset, Y0+y4, glucoseInfoColor);
            break;
        default:
            x0 = 0; y0 = 0; x1 = 0; y1 = 0; x2 = 0; y2 = 0;
            x3 = 0; y3 = 0; x4 = 0; y4 = 0;
            break;
        }
        CanvaAccueil->fillTriangle(X0+x0, Y0+y0, X0+x1, Y0+y1, X0+x2, Y0+y2, glucoseInfoColor);
        CanvaAccueil->fillTriangle(X0+x3, Y0+y3, X0+x1, Y0+y1, X0+x4, Y0+y4, glucoseInfoColor);
    }

    // Durée depuis la dernière glycémie
    CanvaAccueil->setFont(u8g2_font_helvB18_tf);
    CanvaAccueil->setTextColor(RGB565_WHITE);
    if (HeureValide && lastGlyUnixTime > 0)
    {
        time_t now;
        time(&now);
        AgeGlycemie = (long)now - lastGlyUnixTime;
        int minutes = AgeGlycemie / 60;
        int secondes = AgeGlycemie % 60;
        char buffer[10];
        sprintf(buffer, "%d:%02d", minutes, secondes);
        if (minutes >= 10) CanvaAccueil->setTextColor(RGB565_ORANGE);
        if (minutes >= 15) CanvaAccueil->setTextColor(RGB565_RED);
        PrintDroite(CanvaAccueil, String(buffer), EcranW, EcranH / 3, 1);
    }
    else
    {
        CanvaAccueil->setTextColor(RGB565_GREY);
        PrintDroite(CanvaAccueil, T("Age"), EcranW, EcranH / 3, 1);
    }
    CanvaAccueil->setTextColor(RGB565_WHITE);

    // Barre de progression (demande glycémie)
    float dT = 0.0f;
    if (lastReceptionGlycMillis <= lastDemandeGlycMillis)
    {
        dtReponse = float((millis() - lastDemandeGlycMillis)) * float(EcranW) / float(recurGlycMillis);
    }
    else
    {
        dtReponse = float((lastReceptionGlycMillis - lastDemandeGlycMillis)) * float(EcranW) / float(recurGlycMillis);
        dT = float((millis() - lastReceptionGlycMillis)) * (float(EcranW) - dtReponse) / float(recurGlycMillis);
        if (dT > (float(EcranW + 10) - dtReponse))
            CanvaAccueil->fillRect(0, EcranH - 10, EcranW, 10, RGB565_RED);
        else
            CanvaAccueil->fillRect(dtReponse, EcranH - 10, dT, 10, C_grisMoyen);
    }
    if (dtReponse > float(EcranW + EcranW2))
        dtReponse = float(EcranW + 20);
    CanvaAccueil->fillRect(0, EcranH - 10, int(dtReponse), 10, RGB565_MAGENTA);

    // Graphe en barres — affiché uniquement en vue normale
    if (!altView && pointCountGly > 1)
    {
        int16_t X0 = 30;
        int16_t Y0 = EcranH / 1.9;
        int16_t W  = EcranW - X0;
        int16_t H  = EcranH * 0.37;
        int16_t EcranH10 = EcranH - 10;
        int16_t x, y, last_x;
        int lastHeure = -1;
        unsigned long Tmin = 0, Tmax = 0;
        Tmax = glucoseHeure[pointCountGly - 1];
        // Standardised 8-hour display window for all sensor sources.
        // If less than 8h of data is available, show whatever is stored.
        const unsigned long CHART_WINDOW_S = 8UL * 3600UL; // 28 800 s
        Tmin = (Tmax > CHART_WINDOW_S) ? Tmax - CHART_WINDOW_S : glucoseHeure[0];
        // Skip points older than the display window to avoid out-of-bounds X positions.
        int iStart = 0;
        while (iStart < pointCountGly - 1 && glucoseHeure[iStart] < Tmin) iStart++;
        last_x = X0;
        float DT = float(W) / float(Tmax - Tmin);
        CanvaAccueil->setFont(u8g2_font_6x10_tf);

        for (int c = 0; c < 4; c++)
        {
            int16_t y2 = EcranH10 - (int)(H * (seuilCoul[c]     - glucoseRangeMin) / rangeSpan);
            y           = EcranH10 - (int)(H * (seuilCoul[c + 1] - glucoseRangeMin) / rangeSpan);
            String Seuil = String(seuilCoul[c + 1]);
            if (glucoseUnit == 1)
                Seuil = String(float(seuilCoul[c + 1]) / 18.0f, 1);
            PrintDroite(CanvaAccueil, Seuil, X0, y, 1);
            CanvaAccueil->fillRect(X0, y, W, y2 - y, CouleursFond[c]);
        }
        for (int i = iStart; i < pointCountGly; i++)
        {
            x = X0 + int(DT * float(glucoseHeure[i] - Tmin));
            y = (int)(H * (constrain(glucoseValues[i], glucoseRangeMin, glucoseRangeMax) - glucoseRangeMin) / rangeSpan);
            for (int c = 0; c < 4; c++)
                if (glucoseValues[i] > seuilCoul[c])
                    idxCoul = c;
            CanvaAccueil->fillRect(last_x, EcranH10 - y, x - last_x, y, Couleurs[idxCoul]);
            last_x = x;
            int heure = unixToHeure(glucoseHeure[i]);
            if (heure != lastHeure)
            {
                if (heure >= 0 && lastHeure >= 0)
                {
                    CanvaAccueil->drawFastVLine(x, EcranH10, 10, RGB565_WHITE);
                    if (x <= W + X0)
                        PrintGauche(CanvaAccueil, String(heure), x, EcranH - 5, 1);
                }
                lastHeure = heure;
            }
        }
        // Ligne horizontale à targetLow — sur le dessus des barres (2 px, rouge, tirets 6+4)
        int16_t yTL = EcranH10 - (int)(H * (targetLow - glucoseRangeMin) / rangeSpan);
        for (int16_t xd = X0; xd < X0 + W; xd += 10)
            CanvaAccueil->fillRect(xd, yTL, min((int16_t)6, (int16_t)(X0 + W - xd)), 2, RGB565_RED);
        CanvaAccueil->drawFastVLine(X0, EcranH10 - H, H, RGB565_WHITE); // Axe vertical
    }
}

// ==========================
// TOUCH — bascule vue alt
// ==========================
void AccueilHandleTouch(uint16_t x, uint16_t y)
{
    if (GlycemieVal <= 0)
        return; // pas de valeur, rien à basculer

    // Zone de touché sur la valeur glycémique
    // Normal:  inb63 Sz=1 → 63 px,  baseline y=185,  top ≈ 122
    // Alt:     inb49 Sz=3 → ~147 px, baseline y=290, top ≈ valY-147-5 = 138
    uint16_t hitX1 = 100, hitX2 = 380;
    uint16_t hitY1 = altView ? 187 : 110;
    uint16_t hitY2 = altView ? 295 : 200;

    if (x >= hitX1 && x <= hitX2 && y >= hitY1 && y <= hitY2)
        altView = !altView;
}

void Trace_Gauge(Arduino_Canvas *canva, int cx, int cy, int r_inner, int r_outer)
{
    float rangeSpan = glucoseRangeMax - glucoseRangeMin;

    // glucoseRangeMin → -180°, glucoseRangeMax → 0°
    auto toAngle = [&](int val) -> int {
        float clamped = constrain(val, glucoseRangeMin, glucoseRangeMax);
        return -180 + (int)(180.0f * (clamped - glucoseRangeMin) / rangeSpan);
    };

    int Teta0, Teta1;

    // Red: glucoseRangeMin → targetLow (hypoglycaemia)
    Teta0 = -180;
    Teta1 = toAngle(targetLow);
    canva->fillArc(cx, cy, r_inner, r_outer, Teta0, Teta1, RGB565_RED);

    // Green: targetLow → targetHigh (target range)
    Teta0 = Teta1;
    Teta1 = toAngle(targetHigh);
    canva->fillArc(cx, cy, r_inner, r_outer, Teta0, Teta1, RGB565_GREEN);

    // Orange: targetHigh → glucoseWarn (above target)
    Teta0 = Teta1;
    Teta1 = toAngle(glucoseWarn);
    canva->fillArc(cx, cy, r_inner, r_outer, Teta0, Teta1, RGB565_ORANGE);

    // Purple: glucoseWarn → glucoseRangeMax (very high)
    Teta0 = Teta1;
    canva->fillArc(cx, cy, r_inner, r_outer, Teta0, 0, RGB565_PURPLE);
}