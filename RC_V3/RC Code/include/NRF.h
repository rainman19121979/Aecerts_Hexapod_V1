#pragma once

#include <Arduino.h>
#include "Helpers.h"
#include <SPI.h>
#include <RF24.h>

// Define the radio and addresses
extern RF24 radio;

extern unsigned long rc_send_interval;

// Define the data packages
struct RC_Control_Data_Package {
    byte type; // 1 byte
    
    byte joy1_X; // 1 byte
    byte joy1_Y; // 1 byte
    
    byte joy2_X; // 1 byte
    byte joy2_Y; // 1 byte  
    byte slider1; // 1 byte
    byte slider2; // 1 byte

    byte joy1_Button:1; // 1 bit
    byte joy2_Button:1; // 1 bit
    byte pushButton1:1; // 1 bit
    byte pushButton2:1; // 1 bit
    byte idle:1;        // 1 bit
    byte sleep:1;        // 1 bit
    byte dynamic_stride_length:1; // 1 bit
    byte reserved : 1;  // 1 bits padding, 1 byte total

    byte gait;  // 1 byte
};

struct RC_Settings_Data_Package {
    byte type; // 1 byte
    
    byte calibrating:1; //1 bit
    byte reserved:7;              //7 bits padding, 1 byte total

    int8_t offsets[18];             //18 bytes
};

struct Hexapod_Settings_Data_Package {
    byte type; // 1 byte
    int8_t offsets[18]; // 18 bytes
};

struct Hexapod_Sensor_Data_Package {
    byte type; // 1 byte
    float current_sensor_value; // 4 bytes
    Vector2int foot_positions[6]; // 6 * 2 * 2 bytes = 24 bytes

    // NEU: Fuß-Kontakt-Sensor Daten
    byte foot_contact;              // 1 byte - 6 bits für Fuß-Kontakte
    byte contact_count;             // 1 byte - Anzahl Beine mit Bodenkontakt
    byte terrain_roughness;         // 1 byte - Terrain-Rauheit 0-255
    byte adaptive_speed_multiplier; // 1 byte - Geschwindigkeitsfaktor 0-255
    // Total: 32 bytes
};

// NEU: Sensor-Feature Steuerung (Type 5) - MUSS MIT HEXAPOD RC.H ÜBEREINSTIMMEN!
struct RC_Sensor_Control_Data_Package {
    byte type; // 1 byte

    // Feature Toggles (1 byte = 8 bits)
    byte enableTerrainAdaptation:1;   // Bit 0
    byte enableStumbleDetection:1;    // Bit 1
    byte enableAdaptiveSpeed:1;       // Bit 2
    byte enableBalanceControl:1;      // Bit 3
    byte enableGaitOptimization:1;    // Bit 4
    byte enableSensorDebug:1;         // Bit 5
    byte reserved:2;                  // Bits 6-7

    // Erweiterte Parameter
    byte stumble_threshold_ms;        // 1 byte: 0-255 * 2 = 0-510ms
    byte debounce_time_ms;            // 1 byte: 0-255ms
    byte min_contact_count;           // 1 byte: Minimum Kontakte (2-4)

    // Padding auf 32 bytes
    byte padding[28];
};

// NEU: Advanced Settings (Type 6) - MUSS MIT HEXAPOD RC.H ÜBEREINSTIMMEN!
struct RC_Advanced_Settings_Package {
    byte type; // 1 byte

    // Bewegungsparameter (6 bytes)
    byte lift_height;              // 1 byte: 50-250mm
    byte land_height;              // 1 byte: 30-120mm
    byte stride_overshoot;         // 1 byte: 0-50mm
    byte distance_from_center;     // 1 byte: 150-200mm
    byte leg_placement_angle;      // 1 byte: 40-70°
    byte standing_distance_adj;    // 1 byte: -50 bis +50mm (als offset)

    // Gait-Overrides (4 bytes)
    byte override_push_fraction;   // 0-100% → 0.0-1.0
    byte override_speed_mult;      // 0-200% → 0.0-2.0
    byte override_lift_mult;       // 0-300% → 0.0-3.0
    byte override_stride_mult;     // 0-200% → 0.0-2.0

    // Padding auf 32 bytes
    byte padding[21];
};

// Declare the data package variables
extern RC_Control_Data_Package rc_control_data;
extern RC_Settings_Data_Package rc_settings_data;
extern Hexapod_Settings_Data_Package hex_settings_data;
extern Hexapod_Sensor_Data_Package hex_sensor_data;
extern RC_Sensor_Control_Data_Package rc_sensor_control_data;      // NEU
extern RC_Advanced_Settings_Package rc_advanced_settings;           // NEU (Name wie Hexapod)

// Function declarations
void setupNRF();
void sendNRFData(PackageType type);