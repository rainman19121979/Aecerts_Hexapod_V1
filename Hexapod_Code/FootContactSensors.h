// =============================================================================
// Fuß-Kontaktsensor Integration für Hexapod
// =============================================================================
// Features:
// - Terrain-Anpassung (adaptive Schritthöhe)
// - Stolper-Detektion und Recovery
// - Adaptive Geschwindigkeit bei unsicherem Terrain
// - Balance-Verbesserung (min. 3 Beine am Boden)
// - Gait-Optimierung basierend auf Bodenkontakt
// =============================================================================

#ifndef FOOT_CONTACT_SENSORS_H
#define FOOT_CONTACT_SENSORS_H

#include <Arduino.h>

// =============================================================================
// Pin-Definitionen für Fuß-Kontaktsensoren (Anpassen an deine Hardware!)
// =============================================================================
const int footContactPins[6] = {
  40,  // Bein 0 (vorne rechts)
  41,  // Bein 1 (mitte rechts)
  42,  // Bein 2 (hinten rechts)
  43,  // Bein 3 (hinten links)
  44,  // Bein 4 (mitte links)
  45   // Bein 5 (vorne links)
};

// =============================================================================
// Sensor-Konfiguration
// =============================================================================
const bool FOOT_CONTACT_ACTIVE_LOW = true;  // true wenn Sensor bei Kontakt LOW ist
int DEBOUNCE_TIME_MS = 10;                  // Entprellzeit in Millisekunden (von RC steuerbar)
int STUCK_DETECTION_THRESHOLD_MS = 500;     // Fuß gilt als "feststeckend" nach 500ms (von RC steuerbar)

// =============================================================================
// Feature-Flags (Ein/Aus-Schalten einzelner Features)
// =============================================================================
bool enableTerrainAdaptation = true;        // Adaptive Schritthöhe
bool enableStumbleDetection = true;         // Stolper-Erkennung
bool enableAdaptiveSpeed = true;            // Geschwindigkeitsanpassung
bool enableBalanceControl = true;           // Balance-Kontrolle
bool enableGaitOptimization = true;         // Gait-Optimierung

// =============================================================================
// Globale Sensor-Variablen
// =============================================================================
bool footContact[6] = {false, false, false, false, false, false};
bool previousFootContact[6] = {false, false, false, false, false, false};
unsigned long footLiftTime[6] = {0, 0, 0, 0, 0, 0};
unsigned long footLandTime[6] = {0, 0, 0, 0, 0, 0};
unsigned long lastContactChange[6] = {0, 0, 0, 0, 0, 0};

// Statistiken
int contactCount = 0;  // Anzahl Beine mit Bodenkontakt
float terrainRoughness = 0.0f;  // 0.0 = glatt, 1.0 = sehr rau
float adaptiveSpeedMultiplier = 1.0f;  // Dynamischer Geschwindigkeitsfaktor

// =============================================================================
// Sensor-Initialisierung
// =============================================================================
void initFootContactSensors() {
  Serial.println("Initialisiere Fuß-Kontaktsensoren...");

  for(int i = 0; i < 6; i++) {
    pinMode(footContactPins[i], INPUT_PULLUP);
    footContact[i] = false;
    previousFootContact[i] = false;
    footLiftTime[i] = 0;
    footLandTime[i] = 0;
    lastContactChange[i] = millis();
  }

  Serial.println("Fuß-Kontaktsensoren bereit!");
}

// =============================================================================
// Sensor-Auslesen mit Entprellung
// =============================================================================
void readFootContactSensors() {
  unsigned long currentTime = millis();
  contactCount = 0;

  for(int i = 0; i < 6; i++) {
    // Sensor auslesen (mit ACTIVE_LOW Logik)
    bool currentReading = digitalRead(footContactPins[i]);
    if(FOOT_CONTACT_ACTIVE_LOW) {
      currentReading = !currentReading;
    }

    // Entprellung: Nur wenn genug Zeit vergangen ist
    if(currentTime - lastContactChange[i] > DEBOUNCE_TIME_MS) {
      previousFootContact[i] = footContact[i];
      footContact[i] = currentReading;

      // Ereignis-Erkennung
      if(footContact[i] && !previousFootContact[i]) {
        // Fuß hat gerade den Boden berührt
        footLandTime[i] = currentTime;
        lastContactChange[i] = currentTime;
      }
      else if(!footContact[i] && previousFootContact[i]) {
        // Fuß hat gerade den Boden verlassen
        footLiftTime[i] = currentTime;
        lastContactChange[i] = currentTime;
      }
    }

    // Zähle Beine mit Kontakt
    if(footContact[i]) contactCount++;
  }
}

// =============================================================================
// Feature 1: Terrain-Anpassung
// =============================================================================
// Berechnet wie rau das Terrain ist basierend auf Kontakt-Variationen
void updateTerrainRoughness() {
  if(!enableTerrainAdaptation) return;

  // Analysiere Kontakt-Muster über Zeit
  static unsigned long lastTerrainUpdate = 0;
  static int previousContactCount = 3;

  if(millis() - lastTerrainUpdate > 100) {  // Alle 100ms aktualisieren
    // Berechne Variation im Kontakt
    int contactVariation = abs(contactCount - previousContactCount);

    // Exponentieller gleitender Durchschnitt für Terrain-Rauheit
    terrainRoughness = terrainRoughness * 0.9f + (contactVariation / 6.0f) * 0.1f;
    terrainRoughness = constrain(terrainRoughness, 0.0f, 1.0f);

    previousContactCount = contactCount;
    lastTerrainUpdate = millis();
  }
}

// Gibt angepasste Lifthöhe zurück basierend auf Terrain
float getAdaptiveLiftHeight(float baseLiftHeight) {
  if(!enableTerrainAdaptation) return baseLiftHeight;

  // Bei rauem Terrain: Höhere Schritte
  // Bei glattem Terrain: Normale Schritte
  float adaptiveFactor = 1.0f + (terrainRoughness * 0.5f);  // Bis zu 50% höher
  return baseLiftHeight * adaptiveFactor;
}

// =============================================================================
// Feature 2: Stolper-Detektion
// =============================================================================
// Erkennt wenn ein Bein "stecken bleibt" (kein Kontakt obwohl es sollte)
bool detectStumble(int leg) {
  if(!enableStumbleDetection) return false;

  unsigned long currentTime = millis();

  // Wenn Bein in Lifting-Phase ist aber schon lange keinen Kontakt hatte
  if(!footContact[leg] && (currentTime - footLiftTime[leg]) > STUCK_DETECTION_THRESHOLD_MS) {
    // Prüfe ob Bein in Lifting-Phase sein sollte
    if(legStates[leg] == Lifting) {
      return true;  // Bein ist möglicherweise feststeckend!
    }
  }

  return false;
}

// Recovery-Aktion bei Stolpern
void handleStumbleRecovery(int leg) {
  Serial.print("STOLPER erkannt bei Bein ");
  Serial.println(leg);

  // Erhöhe Lift-Height temporär für dieses Bein
  extern float liftHeight;
  liftHeight = liftHeight * 1.5f;

  // Optional: Roboter stoppen und vorsichtig weitermachen
  extern float globalSpeedMultiplier;
  globalSpeedMultiplier = globalSpeedMultiplier * 0.5f;
}

// =============================================================================
// Feature 3: Adaptive Geschwindigkeit
// =============================================================================
void updateAdaptiveSpeed() {
  if(!enableAdaptiveSpeed) {
    adaptiveSpeedMultiplier = 1.0f;
    return;
  }

  // Berechne Sicherheitsfaktor basierend auf:
  // 1. Terrain-Rauheit (je rauer, desto langsamer)
  // 2. Anzahl Kontakte (weniger als 3 = gefährlich)
  // 3. Stolper-Ereignisse

  float safetyFactor = 1.0f;

  // Terrain-Einfluss: Raues Terrain = langsamer
  safetyFactor -= terrainRoughness * 0.3f;  // Bis zu 30% langsamer

  // Balance-Einfluss: Weniger als 3 Kontakte = deutlich langsamer
  if(contactCount < 3) {
    safetyFactor *= 0.5f;  // 50% der Geschwindigkeit
  }

  // Sanft überblenden
  adaptiveSpeedMultiplier = lerp(adaptiveSpeedMultiplier, safetyFactor, 0.05f);
  adaptiveSpeedMultiplier = constrain(adaptiveSpeedMultiplier, 0.3f, 1.0f);
}

// =============================================================================
// Feature 4: Balance-Kontrolle
// =============================================================================
// Stellt sicher dass mindestens 3 Beine immer Bodenkontakt haben
bool checkBalanceSafety() {
  if(!enableBalanceControl) return true;

  // Mindestens 3 Beine sollten Kontakt haben für stabilen Stand
  if(contactCount < 3) {
    Serial.println("WARNUNG: Weniger als 3 Beine haben Bodenkontakt!");
    return false;
  }

  return true;
}

// Berechnet welche Beine als nächstes gehoben werden können
bool canLiftLeg(int leg) {
  if(!enableBalanceControl) return true;

  // Zähle wie viele Beine aktuell Bodenkontakt haben (außer diesem)
  int otherContactCount = 0;
  for(int i = 0; i < 6; i++) {
    if(i != leg && footContact[i]) {
      otherContactCount++;
    }
  }

  // Erlaube Lift nur wenn danach mindestens 3 Beine am Boden bleiben
  return (otherContactCount >= 3);
}

// =============================================================================
// Feature 5: Gait-Optimierung
// =============================================================================
// Wählt besten Gait basierend auf Terrain und Kontakt-Muster
Gait suggestOptimalGait() {
  if(!enableGaitOptimization) {
    extern Gait currentGait;
    return currentGait;
  }

  // Logik für Gait-Auswahl basierend auf Terrain
  if(terrainRoughness > 0.7f) {
    // Sehr raues Terrain: WAVE (langsamster, stabilster)
    return WAVE;
  }
  else if(terrainRoughness > 0.4f) {
    // Mittleres Terrain: RIPPLE (guter Kompromiss)
    return RIPPLE;
  }
  else if(contactCount >= 4) {
    // Glatter Boden + guter Kontakt: TRI (schnell)
    return TRI;
  }
  else {
    // Unsicher: QUAD (4 Beine Support)
    return QUAD;
  }
}

// =============================================================================
// Sensor-Daten an RC senden (für hex_sensor_data Paket)
// =============================================================================
void updateSensorDataPackage() {
  // Fuß-Kontakt als 6-bit Bitfeld (extern from RC.h)
  extern Hexapod_Sensor_Data_Package hex_sensor_data;

  // Packe Fuß-Kontakte in 6 bits
  hex_sensor_data.foot_contact = 0;
  for(int i = 0; i < 6; i++) {
    if(footContact[i]) {
      hex_sensor_data.foot_contact |= (1 << i);
    }
  }

  // Anzahl Kontakte
  hex_sensor_data.contact_count = contactCount;

  // Terrain-Rauheit (0.0-1.0 → 0-255)
  hex_sensor_data.terrain_roughness = (byte)(terrainRoughness * 255.0f);

  // Geschwindigkeitsfaktor (0.0-1.0 → 0-255)
  hex_sensor_data.adaptive_speed_multiplier = (byte)(adaptiveSpeedMultiplier * 255.0f);
}

// =============================================================================
// Haupt-Update-Funktion (in loop() aufrufen!)
// =============================================================================
void updateFootContactSensors() {
  // 1. Sensoren auslesen
  readFootContactSensors();

  // 2. Terrain analysieren
  updateTerrainRoughness();

  // 3. Geschwindigkeit anpassen
  updateAdaptiveSpeed();

  // 4. Stolper-Detektion für jedes Bein
  for(int i = 0; i < 6; i++) {
    if(detectStumble(i)) {
      handleStumbleRecovery(i);
    }
  }

  // 5. Balance prüfen
  checkBalanceSafety();

  // 6. Sensor-Daten für RC-Übertragung aktualisieren
  updateSensorDataPackage();
}

// =============================================================================
// Debug-Funktionen
// =============================================================================
void printFootContactStatus() {
  Serial.print("Kontakte: [");
  for(int i = 0; i < 6; i++) {
    Serial.print(footContact[i] ? "●" : "○");
    if(i < 5) Serial.print(" ");
  }
  Serial.print("] Total: ");
  Serial.print(contactCount);
  Serial.print(" | Rauheit: ");
  Serial.print(terrainRoughness, 2);
  Serial.print(" | Speed: ");
  Serial.print(adaptiveSpeedMultiplier, 2);
  Serial.println();
}

#endif // FOOT_CONTACT_SENSORS_H
