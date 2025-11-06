#include <Arduino.h>
#include "Page.h"
#include "Screen.h"
#include "Inputs.h"
#include "Globals.h"
#include "Popup.h"
#include "NRF.h"

// HINWEIS: Sensor-Daten werden aus Globals.h verwendet (extern deklariert)
// HINWEIS: Feature-Toggles sind in rc_sensor_control_data (NRF.h)

void SensorsPage::init()
{
    rotaryEncoderButtonReady = false;
}

void SensorsPage::loop()
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
    if (hovered >= 6) hovered = 5;
    else if (hovered < 0) hovered = 0;

    /*Display Header*/
    drawPageHeader("< Home < Menu < ", "Sensors");

    /*Display Live Sensor Data*/
    u8g2.setFont(FONT_TINY_TEXT);

    // Fuß-Kontakte visualisieren (6 kleine Kreise)
    for(int i = 0; i < 6; i++) {
        int x = 10 + i * 18;
        int y = 18;
        bool hasContact = (foot_contact >> i) & 0x01;
        if(hasContact) {
            u8g2.drawDisc(x, y, 3);  // Gefüllter Kreis
        } else {
            u8g2.drawCircle(x, y, 3);  // Leerer Kreis
        }
    }

    // Kontakt-Anzahl
    u8g2.setFont(FONT_TEXT);
    char buffer[32];
    sprintf(buffer, "Contact: %d/6", contact_count);
    u8g2.drawStr(100, 18, buffer);

    // Terrain Rauheit (Balken)
    u8g2.drawFrame(10, 22, 50, 6);
    int barWidth = (terrain_roughness * 48) / 255;
    u8g2.drawBox(11, 23, barWidth, 4);
    u8g2.setFont(FONT_TINY_TEXT);
    u8g2.drawStr(62, 27, "Rough");

    // Speed Multiplier (Balken)
    u8g2.drawFrame(10, 30, 50, 6);
    barWidth = (adaptive_speed_multiplier * 48) / 255;
    u8g2.drawBox(11, 31, barWidth, 4);
    u8g2.drawStr(62, 35, "Speed");

    /*Display Toggle List*/
    int rowSpacing = 11;
    int listYStart = 45;
    int listLeftSpacing = 7;

    const char* featureNames[] = {
        "Terrain Adapt",
        "Stumble Detect",
        "Adaptive Speed",
        "Balance Ctrl",
        "Gait Optimize",
        "Sensor Debug"
    };

    // Verwende die Byte-Felder aus rc_sensor_control_data
    byte* featureToggles[] = {
        &rc_sensor_control_data.enableTerrainAdaptation,
        &rc_sensor_control_data.enableStumbleDetection,
        &rc_sensor_control_data.enableAdaptiveSpeed,
        &rc_sensor_control_data.enableBalanceControl,
        &rc_sensor_control_data.enableGaitOptimization,
        &rc_sensor_control_data.enableSensorDebug
    };

    u8g2.setFont(FONT_TEXT);
    for (int i = 0; i < 6; i++)
    {
        sprintf(buffer, "%s: %s", featureNames[i], (*featureToggles[i]) ? "On" : "Off");

        u8g2.drawStr(listLeftSpacing, listYStart + i * rowSpacing, buffer);
        if (hovered == i)
            u8g2.drawRFrame(listLeftSpacing - 4, listYStart + i * rowSpacing - 10,
                           u8g2.getStrWidth(buffer) + 8, 13, 5);
    }

    /*Toggle Feature On/Off*/
    if (getRotaryEncoderSwitchValue() == PRESSED && rotaryEncoderButtonReady)
    {
        // Toggle selected feature (direkt im Paket)
        *featureToggles[hovered] = !(*featureToggles[hovered]);

        // Paket wird automatisch in main.cpp gesendet (wenn currentPage == sensorsPage)
        rotaryEncoderButtonReady = false;
    }
}
