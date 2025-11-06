#include "NRF.h"
#include "Globals.h"

// Initialize the radio and addresses
RF24 radio(49, 4); // CE, CSN
bool radioNumber = 0;

unsigned long rc_send_interval = 50;

// Initialize the data packages
RC_Control_Data_Package rc_control_data;
RC_Settings_Data_Package rc_settings_data;
Hexapod_Settings_Data_Package hex_settings_data;
Hexapod_Sensor_Data_Package hex_sensor_data;
RC_Sensor_Control_Data_Package rc_sensor_control_data;          // NEU
RC_Advanced_Settings_Package rc_advanced_settings;               // NEU

void setupNRF() {
    radio.begin();
    radio.setPALevel(RF24_PA_LOW);
    radio.setPayloadSize(32);
    radio.setChannel(124);
    radio.enableAckPayload();
    radio.setRetries(5, 5);
    radio.openWritingPipe(nrfAddress);

    rc_control_data.type = RC_CONTROL_DATA;

    rc_control_data.joy1_X = 127;
    rc_control_data.joy1_Y = 127;
    rc_control_data.joy1_Button = UNPRESSED;

    rc_control_data.joy2_X = 127;
    rc_control_data.joy2_Y = 127;
    rc_control_data.joy2_Button = UNPRESSED;

    rc_control_data.slider1 = 50;
    rc_control_data.slider2 = 50;

    rc_control_data.pushButton1 = UNPRESSED;
    rc_control_data.pushButton2 = UNPRESSED;

    rc_control_data.dynamic_stride_length = dynamicStrideLength;


    rc_settings_data.type = RC_SETTINGS_DATA;


    for (int i = 0; i < 18; i++) {
        rc_settings_data.offsets[i] = 0;
    }

    // NEU: Sensor-Control Paket initialisieren (wie Hexapod)
    rc_sensor_control_data.type = RC_SENSOR_CONTROL_DATA;
    rc_sensor_control_data.enableTerrainAdaptation = 1;  // Standard: AN
    rc_sensor_control_data.enableStumbleDetection = 1;   // Standard: AN
    rc_sensor_control_data.enableAdaptiveSpeed = 1;      // Standard: AN
    rc_sensor_control_data.enableBalanceControl = 1;     // Standard: AN
    rc_sensor_control_data.enableGaitOptimization = 1;   // Standard: AN
    rc_sensor_control_data.enableSensorDebug = 0;        // Standard: AUS
    rc_sensor_control_data.stumble_threshold_ms = 125;   // 250ms (125*2)
    rc_sensor_control_data.debounce_time_ms = 10;        // 10ms
    rc_sensor_control_data.min_contact_count = 3;        // Minimum 3 Kontakte
    memset(rc_sensor_control_data.padding, 0, 28);       // Padding löschen

    // NEU: Advanced-Settings Paket initialisieren (wie Hexapod)
    rc_advanced_settings.type = RC_ADVANCED_SETTINGS_DATA;
    rc_advanced_settings.lift_height = 130;              // 130mm
    rc_advanced_settings.land_height = 70;               // 70mm
    rc_advanced_settings.stride_overshoot = 10;          // 10mm
    rc_advanced_settings.distance_from_center = 173;     // 173mm
    rc_advanced_settings.leg_placement_angle = 56;       // 56°
    rc_advanced_settings.standing_distance_adj = 0;      // 0mm (keine Anpassung)
    rc_advanced_settings.override_push_fraction = 0;     // 0 = nicht überschreiben
    rc_advanced_settings.override_speed_mult = 0;        // 0 = nicht überschreiben
    rc_advanced_settings.override_lift_mult = 0;         // 0 = nicht überschreiben
    rc_advanced_settings.override_stride_mult = 0;       // 0 = nicht überschreiben
    memset(rc_advanced_settings.padding, 0, 21);         // Padding löschen
}

void sendNRFData(PackageType type) {
    every(rc_send_interval) {
        bool report = false;

        if (type == RC_CONTROL_DATA) {
            report = radio.write(&rc_control_data, sizeof(rc_control_data)); // Send control data
        } else if (type == RC_SETTINGS_DATA) {
            report = radio.write(&rc_settings_data, sizeof(rc_settings_data)); // Send settings data
        } else if (type == RC_SENSOR_CONTROL_DATA) {
            report = radio.write(&rc_sensor_control_data, sizeof(rc_sensor_control_data)); // NEU: Send sensor control data
        } else if (type == RC_ADVANCED_SETTINGS_DATA) {
            report = radio.write(&rc_advanced_settings, sizeof(rc_advanced_settings)); // NEU: Send advanced settings
        }

        if (report) {
            if (radio.isAckPayloadAvailable()) {
                byte ackType;
                radio.read(&ackType, sizeof(ackType));

                if (ackType == HEXAPOD_SETTINGS_DATA) {
                    radio.read(&hex_settings_data, sizeof(hex_settings_data));
                    
                    for (int i = 0; i < 18; i++) {
                        hexSavedOffsets[i] = hex_settings_data.offsets[i];
                    }
                    
                } else if (ackType == HEXAPOD_SENSOR_DATA) {
                    radio.read(&hex_sensor_data, sizeof(hex_sensor_data));

                    current_sensor_value = hex_sensor_data.current_sensor_value;
                    for (int i = 0; i < 6; i++) {
                        foot_positions[i] = hex_sensor_data.foot_positions[i];
                    }

                    // NEU: Sensor-Daten extrahieren
                    foot_contact = hex_sensor_data.foot_contact;
                    contact_count = hex_sensor_data.contact_count;
                    terrain_roughness = hex_sensor_data.terrain_roughness;
                    adaptive_speed_multiplier = hex_sensor_data.adaptive_speed_multiplier;
                }

                //no data is being received
                else{
                    current_sensor_value = 0;
                    for (int i = 0; i < 6; i++) {
                        foot_positions[i] = Vector2int(0,0);
                    }
                }
            }
        }
    }
}