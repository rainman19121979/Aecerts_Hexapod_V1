#include <Arduino.h>
#include "Page.h"
#include "Screen.h"
#include "Inputs.h"
#include "Globals.h"
#include "Popup.h"
#include "NRF.h"

// HINWEIS: Advanced Settings werden in rc_advanced_settings (NRF.h) gespeichert
// Alle Werte sind bytes, passend zur Hexapod-Seite

struct AdvSetting
{
    const char *name;
    byte *value;
    byte minVal, maxVal;
};

AdvSetting advSettings[] = {
    {"Lift Height", &rc_advanced_settings.lift_height, 50, 250},
    {"Land Height", &rc_advanced_settings.land_height, 30, 120},
    {"Stride Overshoot", &rc_advanced_settings.stride_overshoot, 0, 50},
    {"Distance Center", &rc_advanced_settings.distance_from_center, 150, 200},
    {"Leg Angle", &rc_advanced_settings.leg_placement_angle, 40, 70},
    {"Standing Adj", &rc_advanced_settings.standing_distance_adj, 0, 100}  // 0-100 -> -50 to +50
};

const int numAdvSettings = sizeof(advSettings) / sizeof(advSettings[0]);

void AdvancedPage::init()
{
    rotaryEncoderButtonReady = false;
}

void AdvancedPage::loop()
{
    if (getRotaryEncoderSwitchValue() == UNPRESSED) rotaryEncoderButtonReady = true;

    /*Back*/
    if (getButtonValue(A) == PRESSED){
        currentPage = mainMenuPage;
    }

    /*Scrolling*/
    int increment = 0;
    int spins = getRotaryEncoderSpins();

    if (spins > 0) increment = 1;
    if (spins < 0) increment = -1;

    hovered += increment;
    if (hovered >= numAdvSettings) hovered = numAdvSettings - 1;
    else if (hovered < 0) hovered = 0;

    /*Display Header*/
    drawPageHeader("< Home < Menu < ", "Advanced");

    /*Display the list*/
    int rowSpacing = 11;
    int listYStart = 22;
    int listLeftSpacing = 7;

    if (hovered >= 4)
        listYStart = listYStart - rowSpacing * (hovered - 3);

    char buffer[32];

    u8g2.setFont(FONT_TEXT);
    for (int i = 0; i < numAdvSettings; i++)
    {
        const char *name = advSettings[i].name;
        byte value = *advSettings[i].value;

        // Format display - Standing Adj ist speziell (0-100 -> -50 bis +50)
        if(i == 5) {  // Standing Adj
            sprintf(buffer, "%s: %d", name, (int)value - 50);
        } else {
            sprintf(buffer, "%s: %d", name, value);
        }

        if (hovered < i + 5)
        {
            u8g2.drawStr(listLeftSpacing, listYStart, buffer);
            if (hovered == i)
                u8g2.drawRFrame(listLeftSpacing - 4, listYStart - 10,
                               u8g2.getStrWidth(buffer) + 8, 13, 5);
        }

        listYStart += rowSpacing;
    }

    /*Change Setting Value*/
    if (getRotaryEncoderSwitchValue() == PRESSED && rotaryEncoderButtonReady)
    {
        byte min = advSettings[hovered].minVal;
        byte max = advSettings[hovered].maxVal;
        byte val = *advSettings[hovered].value;

        // Standing Adj: Zeige -50 bis +50, speichere aber 0-100
        long int displayMin = min;
        long int displayMax = max;
        long int displayVal = val;
        if(hovered == 5) {  // Standing Adj
            displayMin = (int)min - 50;
            displayMax = (int)max - 50;
            displayVal = (int)val - 50;
        }

        long int newValue = openPopupNumber(advSettings[hovered].name,
                                            constrain(displayVal, displayMin, displayMax),
                                            displayMin, displayMax);

        // Standing Adj: Konvertiere zurück zu 0-100
        if(hovered == 5) {
            *advSettings[hovered].value = (byte)(newValue + 50);
        } else {
            *advSettings[hovered].value = (byte)newValue;
        }

        // Paket wird automatisch in main.cpp gesendet (wenn currentPage == advancedPage)
        rotaryEncoderButtonReady = false;
    }
}
