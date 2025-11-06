#include <SPI.h>
#include <RF24.h>
#define Current_Sensor_Pin 0
#define UNPRESSED 0x1
#define PRESSED  0x0

RF24 radio(49, 48); // CE, CSN
uint8_t address[6] = "HEX01";
bool radioNumber = 1;

unsigned long rc_last_received_time = 0;
unsigned long rc_timeout = 1000;

enum PackageType {
    RC_CONTROL_DATA = 1,
    RC_SETTINGS_DATA = 2,
    HEXAPOD_SETTINGS_DATA = 3,
    HEXAPOD_SENSOR_DATA = 4,
    RC_SENSOR_CONTROL_DATA = 5,     // Sensor-Feature Steuerung
    RC_ADVANCED_SETTINGS_DATA = 6   // Advanced Settings (Bewegungsparameter)
};

// Define the data packages
struct RC_Control_Data_Package {
    byte type; // 1 byte

    byte joy1_X; // 1 byte
    byte joy1_Y; // 1 byte

    byte joy2_X; // 1 byte
    byte joy2_Y; // 1 byte
    byte slider1; // 1 byte
    byte slider2; // 1 byte

    byte joy1_Button:1; // 1 bit - Slam Attack
    byte joy2_Button:1; // 1 bit
    byte pushButton1:1; // 1 bit - Bumper A (Front Left)
    byte pushButton2:1; // 1 bit - Bumper B (Back Left)
    byte idle:1;        // 1 bit
    byte sleep:1;       // 1 bit
    byte dynamic_stride_length:1; // 1 bit
    byte reserved : 1;  // 1 bit padding, 1 byte total

    byte gait;  // 1 byte

    // NEU: Bumpers C & D + Toggles (1 byte = 8 bits)
    byte bumperC:1;         // Bit 0 - Bumper C (Front Right)
    byte bumperD:1;         // Bit 1 - Bumper D (Back Right)
    byte toggleGyro:1;      // Bit 2 - Gyro-Steuerung an/aus
    byte toggleHighStep:1;  // Bit 3 - Hohe Schritte an/aus
    byte toggleEasyMode:1;  // Bit 4 - Easy Mode an/aus
    byte toggleSpeedBoost:1;// Bit 5 - Speed Boost
    byte reserved2:2;       // Bits 6-7 padding
    // Total: 12 bytes
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

struct Vector2int{
    int x;
    int y;

    Vector2int(int xVal, int yVal) : x(xVal), y(yVal) {}
    Vector2int() : x(0), y(0) {}
};

struct Hexapod_Sensor_Data_Package {
    byte type; // 1 byte
    float current_sensor_value; // 4 bytes
    Vector2int foot_positions[6]; // 6 * 2 * 2 bytes = 24 bytes

    // Neu: Fuß-Kontaktsensor Daten (3 bytes)
    byte foot_contact:6;           // 6 bits: Fuß-Kontakt (1 bit pro Bein)
    byte contact_count:4;          // 4 bits: Anzahl Kontakte (0-6)
    byte terrain_roughness;        // 1 byte: Terrain-Rauheit (0-255, mapped von 0.0-1.0)
    byte adaptive_speed_multiplier; // 1 byte: Geschwindigkeitsfaktor (0-255, mapped von 0.0-1.0)
    // Total: 29 + 3 = 32 bytes (Perfekt!)
};

// Neu: Sensor-Feature Steuerung von RC zu Hexapod
struct RC_Sensor_Control_Data_Package {
    byte type; // 1 byte: RC_SENSOR_CONTROL_DATA

    // Feature Toggles (1 byte = 8 bits)
    byte enableTerrainAdaptation:1;   // Bit 0
    byte enableStumbleDetection:1;    // Bit 1
    byte enableAdaptiveSpeed:1;       // Bit 2
    byte enableBalanceControl:1;      // Bit 3
    byte enableGaitOptimization:1;    // Bit 4
    byte enableSensorDebug:1;         // Bit 5: Debug-Ausgabe aktivieren
    byte reserved:2;                  // Bits 6-7: Reserviert für zukünftige Features

    // Erweiterte Parameter (optional, falls mehr Kontrolle gewünscht)
    byte stumble_threshold_ms;        // 1 byte: 0-255 * 2 = 0-510ms
    byte debounce_time_ms;            // 1 byte: 0-255ms
    byte min_contact_count;           // 1 byte: Minimum Kontakte für sichere Balance (2-4)

    // Padding auf 32 bytes
    byte padding[28];                 // Reserviert für zukünftige Erweiterungen
};

// NEU: Advanced Settings für Bewegungsparameter
struct RC_Advanced_Settings_Package {
    byte type; // 1 byte: RC_ADVANCED_SETTINGS_DATA (type 6)

    // Bewegungsparameter (10 bytes)
    byte lift_height;              // 1 byte: 50-250mm (mapped)
    byte land_height;              // 1 byte: 30-120mm
    byte stride_overshoot;         // 1 byte: 0-50mm
    byte distance_from_center;     // 1 byte: 150-200mm
    byte leg_placement_angle;      // 1 byte: 40-70°
    byte standing_distance_adj;    // 1 byte: -50 bis +50mm

    // Gait-Overrides (4 bytes) - 0 = nicht überschreiben, sonst Wert
    byte override_push_fraction;   // 0-100% → 0.0-1.0
    byte override_speed_mult;      // 0-200% → 0.0-2.0
    byte override_lift_mult;       // 0-300% → 0.0-3.0
    byte override_stride_mult;     // 0-200% → 0.0-2.0

    // Padding auf 32 bytes
    byte padding[21];
};

// Declare the data package variables
RC_Control_Data_Package rc_control_data;
RC_Settings_Data_Package rc_settings_data;
RC_Sensor_Control_Data_Package rc_sensor_control_data;
RC_Advanced_Settings_Package rc_advanced_settings;
Hexapod_Settings_Data_Package hex_settings_data;
Hexapod_Sensor_Data_Package hex_sensor_data;

void RC_Setup();
void RC_DisplayData();
bool GetSendNRFData(PackageType sendType);
void initializeHexPayload();
void initializeControllerPayload();
void processControlData(const RC_Control_Data_Package& data);
void processSettingsData(const RC_Settings_Data_Package& data);

void RC_Setup(){
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {} // hold in infinite loop
  } else {
    Serial.println(F("radio hardware is ready!"));
  }

  radio.setAddressWidth(5);
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(32); // Set the payload size to the maximum of 32 bytes
  radio.setChannel(124);
  radio.openReadingPipe(1, address);
  radio.enableAckPayload();
  radio.startListening();

  initializeHexPayload();  
  initializeControllerPayload();

  radio.writeAckPayload(1, &hex_sensor_data, sizeof(hex_sensor_data)); 
}

void initializeHexPayload(){
  hex_sensor_data.type = HEXAPOD_SENSOR_DATA;
  hex_sensor_data.current_sensor_value = 0;
  // Neu: Initialisiere Sensor-Felder
  hex_sensor_data.foot_contact = 0;
  hex_sensor_data.contact_count = 0;
  hex_sensor_data.terrain_roughness = 0;
  hex_sensor_data.adaptive_speed_multiplier = 255; // 1.0 als default

  hex_settings_data.type = HEXAPOD_SETTINGS_DATA;
  Serial.println("Filling hex_settings_data.offsets with 0's.");
  for (int i = 0; i < 18; i++) {
      hex_settings_data.offsets[i] = 0;
  }
}

void initializeControllerPayload(){

  //control package
  rc_control_data.type = RC_CONTROL_DATA;

  rc_control_data.joy1_X = 127;
  rc_control_data.joy1_Y = 180;
  rc_control_data.joy1_Button = UNPRESSED;

  rc_control_data.joy2_X = 127;
  rc_control_data.joy2_Y = 127;
  rc_control_data.joy2_Button = UNPRESSED;

  rc_control_data.slider1 = 40;
  rc_control_data.slider2 = 0;

  rc_control_data.pushButton1 = UNPRESSED;
  rc_control_data.pushButton2 = UNPRESSED;

  rc_control_data.idle = 1;
  rc_control_data.sleep = 1;

  rc_control_data.gait = 0;

  rc_control_data.dynamic_stride_length = 1;

  // NEU: Initialisiere Bumpers und Toggles
  rc_control_data.bumperC = UNPRESSED;
  rc_control_data.bumperD = UNPRESSED;
  rc_control_data.toggleGyro = 0;        // Default: aus
  rc_control_data.toggleHighStep = 0;    // Default: aus
  rc_control_data.toggleEasyMode = 0;    // Default: aus
  rc_control_data.toggleSpeedBoost = 0;  // Default: aus

  //settings package
  rc_settings_data.type = RC_SETTINGS_DATA;
  rc_settings_data.calibrating = 0;
  for (int i = 0; i < 18; i++) {
      rc_settings_data.offsets[i] = -128;
  }

  //sensor control package (neu)
  rc_sensor_control_data.type = RC_SENSOR_CONTROL_DATA;
  rc_sensor_control_data.enableTerrainAdaptation = 1;  // Default: an
  rc_sensor_control_data.enableStumbleDetection = 1;   // Default: an
  rc_sensor_control_data.enableAdaptiveSpeed = 1;      // Default: an
  rc_sensor_control_data.enableBalanceControl = 1;     // Default: an
  rc_sensor_control_data.enableGaitOptimization = 1;   // Default: an
  rc_sensor_control_data.enableSensorDebug = 0;        // Default: aus
  rc_sensor_control_data.stumble_threshold_ms = 250;   // 500ms (250*2)
  rc_sensor_control_data.debounce_time_ms = 10;        // 10ms
  rc_sensor_control_data.min_contact_count = 3;        // Minimum 3 Kontakte
  memset(rc_sensor_control_data.padding, 0, 28);       // Padding löschen

  //advanced settings package (neu)
  rc_advanced_settings.type = RC_ADVANCED_SETTINGS_DATA;
  rc_advanced_settings.lift_height = 130;               // 130mm default
  rc_advanced_settings.land_height = 70;                // 70mm default
  rc_advanced_settings.stride_overshoot = 10;           // 10mm default
  rc_advanced_settings.distance_from_center = 173;      // 173mm default
  rc_advanced_settings.leg_placement_angle = 56;        // 56° default
  rc_advanced_settings.standing_distance_adj = 0;       // 0mm (keine Anpassung)
  rc_advanced_settings.override_push_fraction = 0;      // 0 = nicht überschreiben
  rc_advanced_settings.override_speed_mult = 0;         // 0 = nicht überschreiben
  rc_advanced_settings.override_lift_mult = 0;          // 0 = nicht überschreiben
  rc_advanced_settings.override_stride_mult = 0;        // 0 = nicht überschreiben
  memset(rc_advanced_settings.padding, 0, 21);          // Padding löschen
}

byte currentType = RC_CONTROL_DATA;
byte sendType = HEXAPOD_SENSOR_DATA;

bool GetSendNRFData(){  
  // This device is a RX node
  uint8_t pipe;
  if (radio.available(&pipe)) {
    uint8_t bytes = radio.getPayloadSize(); // get the size of the payload    
    byte incomingType;
    radio.read(&incomingType, sizeof(incomingType));
    if(currentType != incomingType && incomingType != NULL)currentType = incomingType;

    if (incomingType == RC_CONTROL_DATA) {
        radio.read(&rc_control_data, sizeof(rc_control_data));
        Serial.println("Receiving CONTROL");
    } else if (incomingType == RC_SETTINGS_DATA) {
        radio.read(&rc_settings_data, sizeof(rc_settings_data));
        Serial.println("Receiving SETTINGS");
    } else if (incomingType == RC_SENSOR_CONTROL_DATA) {
        radio.read(&rc_sensor_control_data, sizeof(rc_sensor_control_data));
        Serial.println("Receiving SENSOR_CONTROL");
    } else if (incomingType == RC_ADVANCED_SETTINGS_DATA) {
        radio.read(&rc_advanced_settings, sizeof(rc_advanced_settings));
        Serial.println("Receiving ADVANCED_SETTINGS");
    }   

    hex_sensor_data.current_sensor_value = mapFloat(analogRead(Current_Sensor_Pin), 0, 1024, 0, 50);

    if(sendType == HEXAPOD_SETTINGS_DATA){
      radio.writeAckPayload(1, &hex_settings_data, sizeof(hex_settings_data)); // load the payload for the next time
      Serial.println("Sending SETTINGS");
    }     
    
    if(sendType == HEXAPOD_SENSOR_DATA){
      radio.writeAckPayload(1, &hex_sensor_data, sizeof(hex_sensor_data)); // load the payload for the next time
      Serial.println("Sending SENSOR");
    }
    

    rc_last_received_time = millis();    
  }

  if (millis() - rc_last_received_time > rc_timeout) return false;

  return true;
}



void RC_DisplayData(){
  Serial.print("Joy1 X: ");
  Serial.print(rc_control_data.joy1_X);

  Serial.print(" | Joy1 Y: ");
  Serial.print(rc_control_data.joy1_Y);

  Serial.print(" | Joy1 Button: ");
  Serial.print(rc_control_data.joy1_Button);

  Serial.print(" | Joy2 X: ");
  Serial.print(rc_control_data.joy2_X);

  Serial.print(" | Joy2 Y: ");
  Serial.print(rc_control_data.joy2_Y);

  Serial.print(" | Joy2 Button: ");
  Serial.print(rc_control_data.joy2_Button);

  Serial.print(" | Pot 1: ");
  Serial.print(rc_control_data.slider1);

  Serial.print(" | Pot 2: ");
  Serial.print(rc_control_data.slider2);

  Serial.print(" | Button 1: ");
  Serial.print(rc_control_data.pushButton1);

  Serial.print(" | Button 2: ");
  Serial.println(rc_control_data.pushButton2);
}