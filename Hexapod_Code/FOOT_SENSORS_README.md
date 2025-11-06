# 🦾 Fuß-Kontaktsensoren - Bedienungsanleitung

## 📋 Übersicht

Die Fuß-Kontaktsensoren ermöglichen intelligente Features für deinen Hexapod-Roboter:

✅ **Terrain-Anpassung** - Passt Schritthöhe automatisch an unebenes Terrain an
✅ **Stolper-Detektion** - Erkennt wenn ein Bein feststeckt und korrigiert
✅ **Adaptive Geschwindigkeit** - Verlangsamt bei unsicherem Terrain
✅ **Balance-Kontrolle** - Stellt sicher, dass mindestens 3 Beine immer Bodenkontakt haben
✅ **Gait-Optimierung** - Wählt automatisch den besten Gait für das Terrain

---

## ⚙️ Hardware-Konfiguration

### 1. Sensor-Pins anpassen

Öffne `FootContactSensors.h` und passe die Pin-Nummern an deine Hardware an:

```cpp
const int footContactPins[6] = {
  40,  // Bein 0 (vorne rechts)
  41,  // Bein 1 (mitte rechts)
  42,  // Bein 2 (hinten rechts)
  43,  // Bein 3 (hinten links)
  44,  // Bein 4 (mitte links)
  45   // Bein 5 (vorne links)
};
```

### 2. Sensor-Typ konfigurieren

Je nach Sensor-Typ (Normal-Open oder Normal-Closed):

```cpp
const bool FOOT_CONTACT_ACTIVE_LOW = true;  // true = Kontakt bei LOW Signal
                                             // false = Kontakt bei HIGH Signal
```

**Beispiele:**
- Mikro-Schalter (Normal-Open): `true`
- Force-Sensitive Resistor (FSR): `false` (mit Analog-Anpassung)
- Infrarot-Sensor: hängt vom Modell ab

### 3. Entprellzeit anpassen

Bei mechanischen Schaltern wichtig:

```cpp
const int DEBOUNCE_TIME_MS = 10;  // 5-20ms empfohlen
```

---

## 🎮 Features Ein/Ausschalten

In `FootContactSensors.h` kannst du einzelne Features aktivieren/deaktivieren:

```cpp
bool enableTerrainAdaptation = true;   // Adaptive Schritthöhe
bool enableStumbleDetection = true;    // Stolper-Erkennung
bool enableAdaptiveSpeed = true;       // Geschwindigkeitsanpassung
bool enableBalanceControl = true;      // Balance-Kontrolle
bool enableGaitOptimization = true;    // Gait-Optimierung
```

**Empfohlene Einstellungen:**

| Terrain | Terrain-Adapt | Stolper | Speed | Balance | Gait |
|---------|---------------|---------|-------|---------|------|
| **Glatt (Indoor)** | ❌ | ✅ | ❌ | ✅ | ❌ |
| **Uneben (Outdoor)** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Sehr rau** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Performance-Test** | ❌ | ❌ | ❌ | ❌ | ❌ |

---

## 🔧 Erweiterte Konfiguration

### Stolper-Detektion Schwellwert

Wie lange ohne Kontakt bis "feststeckend" erkannt wird:

```cpp
const int STUCK_DETECTION_THRESHOLD_MS = 500;  // 300-800ms empfohlen
```

- **Niedrig (300ms)**: Reaktionsschnell, aber mehr Fehlalarme
- **Mittel (500ms)**: Guter Kompromiss
- **Hoch (800ms)**: Konservativ, weniger Fehlalarme

---

## 📊 Debug und Monitoring

### Sensor-Status ausgeben

In `loop()` in `Hexapod_Code.ino` hinzufügen:

```cpp
void loop() {
  // ... bestehender Code ...

  // Alle 500ms Sensor-Status ausgeben
  static unsigned long lastDebug = 0;
  if(millis() - lastDebug > 500) {
    printFootContactStatus();
    lastDebug = millis();
  }
}
```

**Beispiel-Ausgabe:**
```
Kontakte: [● ○ ● ○ ● ○] Total: 3 | Rauheit: 0.23 | Speed: 0.87
```

- `●` = Bein hat Bodenkontakt
- `○` = Bein in der Luft
- `Total` = Anzahl Kontakte
- `Rauheit` = 0.0 (glatt) bis 1.0 (sehr rau)
- `Speed` = Aktueller Geschwindigkeitsfaktor

---

## 🎯 Typische Sensor-Verdrahtung

### Mikro-Schalter (Normal-Open)

```
                    Arduino Pin
                         │
                        ┌┴┐
                       │ R │ Pull-Up Resistor (10kΩ intern)
                        └┬┘
                         │
    Fuß-Schalter         │
        ───              │
        ● ●──────────────┤
        ───              │
                        GND
```

**Code:**
```cpp
pinMode(footContactPins[i], INPUT_PULLUP);
const bool FOOT_CONTACT_ACTIVE_LOW = true;
```

### Force-Sensitive Resistor (FSR)

```
                    Arduino Pin (Analog)
                         │
                        ┌┴┐
                       │ R │ 10kΩ Resistor
                        └┬┘
                         │
                        FSR
                         │
                        GND
```

**Code:**
```cpp
pinMode(footContactPins[i], INPUT);
const bool FOOT_CONTACT_ACTIVE_LOW = false;

// In readFootContactSensors():
int reading = analogRead(footContactPins[i]);
bool currentReading = (reading > 512);  // Schwellwert anpassen
```

---

## 🚀 Beispiele

### Beispiel 1: Nur Balance-Kontrolle

```cpp
// In FootContactSensors.h:
bool enableTerrainAdaptation = false;
bool enableStumbleDetection = false;
bool enableAdaptiveSpeed = false;
bool enableBalanceControl = true;   // Nur dieses aktiv
bool enableGaitOptimization = false;
```

**Ergebnis:** Roboter stellt sicher, dass immer mindestens 3 Beine Bodenkontakt haben.

---

### Beispiel 2: Vollständige Outdoor-Konfiguration

```cpp
// In FootContactSensors.h:
bool enableTerrainAdaptation = true;   // Alle aktiv
bool enableStumbleDetection = true;
bool enableAdaptiveSpeed = true;
bool enableBalanceControl = true;
bool enableGaitOptimization = true;

const int STUCK_DETECTION_THRESHOLD_MS = 400;  // Schnellere Reaktion
```

**Ergebnis:** Roboter passt sich automatisch an Terrain an, vermeidet Stolpern, hält Balance.

---

### Beispiel 3: Eigene Funktion basierend auf Sensordaten

```cpp
// In Hexapod_Code.ino:

void checkForObstacles() {
  // Wenn vorne rechts UND vorne links gleichzeitig keinen Kontakt haben
  // während andere Beine Kontakt haben -> möglicherweise Kante/Hindernis

  if(!footContact[0] && !footContact[5] &&  // Vorne ohne Kontakt
     (footContact[1] || footContact[4])) {   // Mitte hat Kontakt

    Serial.println("WARNUNG: Kante oder Hindernis erkannt!");
    // Stop oder Rückwärts-Bewegung auslösen
  }
}

void loop() {
  updateFootContactSensors();
  checkForObstacles();
  // ... rest ...
}
```

---

## 🔬 Erweiterte Features

### Eigene Terrain-Analyse

Du kannst die `terrainRoughness` Variable nutzen:

```cpp
void loop() {
  updateFootContactSensors();

  if(terrainRoughness > 0.8f) {
    Serial.println("SEHR RAUES TERRAIN - Vorsichtig!");
    // LED einschalten, Summer aktivieren, etc.
  }
}
```

### Eigene Gait-Wahl basierend auf Sensoren

```cpp
void loop() {
  updateFootContactSensors();

  // Vorschlag holen
  Gait suggested = suggestOptimalGait();

  // Optional: Eigene Logik hinzufügen
  if(contactCount < 4) {
    currentGait = WAVE;  // Sicherster Gait bei wenig Kontakt
  } else {
    currentGait = suggested;
  }
}
```

---

## 🐛 Troubleshooting

### Problem: Sensoren werden nicht erkannt

**Lösung:**
1. Prüfe Pin-Nummern in `footContactPins[]`
2. Prüfe `FOOT_CONTACT_ACTIVE_LOW` Einstellung
3. Teste einzelne Sensoren mit Serial Monitor:
   ```cpp
   for(int i = 0; i < 6; i++) {
     Serial.print("Pin ");
     Serial.print(footContactPins[i]);
     Serial.print(": ");
     Serial.println(digitalRead(footContactPins[i]));
   }
   ```

### Problem: Roboter bewegt sich zu langsam

**Lösung:**
- `enableAdaptiveSpeed = false` setzen
- Oder `terrainRoughness` manuell reduzieren:
  ```cpp
  terrainRoughness = constrain(terrainRoughness, 0.0f, 0.3f);
  ```

### Problem: Zu viele Stolper-Warnungen

**Lösung:**
- `STUCK_DETECTION_THRESHOLD_MS` erhöhen (z.B. auf 800)
- Oder `enableStumbleDetection = false` setzen

### Problem: Roboter hebt Beine zu hoch

**Lösung:**
- `enableTerrainAdaptation = false` setzen
- Oder in `FootContactSensors.h` anpassen:
  ```cpp
  float adaptiveFactor = 1.0f + (terrainRoughness * 0.2f);  // Statt 0.5f
  ```

---

## 📈 Performance-Tipps

1. **Loop-Zeit optimieren**: Sensor-Update ist optimiert und dauert <1ms
2. **Serial Print reduzieren**: `printFootContactStatus()` nur bei Bedarf nutzen
3. **Features deaktivieren**: Nicht benötigte Features ausschalten für bessere Performance

---

## 🎓 Weitere Ideen

Mit den Fuß-Kontaktsensoren kannst du noch mehr machen:

- 🗺️ **Terrain-Mapping**: Speichere welche Bereiche rau/glatt sind
- 🎯 **Präzise Positionierung**: Nutze Kontakt für genaues Anhalten
- 🏃 **Sprint-Modus**: Wenn alle Sensoren OK sind, maximale Speed
- 🛡️ **Kollisions-Erkennung**: Unerwarteter Kontakt = Hindernis
- 📊 **Datenanalyse**: Logge Kontakt-Muster über Zeit

---

## 📝 Lizenz

Dieses Feature ist Teil des Aecerts_Hexapod_V1 Projekts.

**Viel Erfolg mit deinem intelligenten Hexapod! 🦾**
