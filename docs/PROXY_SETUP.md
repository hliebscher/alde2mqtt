# WomoLIN als Proxy - Setup Anleitung

Diese Anleitung beschreibt, wie du den WomoLIN mit 3 LIN-Bus-Eingängen als Proxy zwischen dem Truma-Modul und der Alde-Heizung verwendest, um den gesamten Datenverkehr zu analysieren.

## Hardware Setup

### Verbindungen

```
Truma-Modul (Bluetooth/Original) 
    ↓
LIN1 (GPIO 14/15) → ESP32 → LIN2 (GPIO 12/13)
    ↓
TIN-Bus / Alde-Heizung
```

**Wichtig:** Da die Standard-UART 1-Pins IO16/17 vom Ethernet PHY verwendet werden, müssen die LIN-Interfaces auf andere GPIO-Pins umgelegt werden.

**WomoLIN Konfiguration:**
- **LIN1 (Truma-Eingang):**
  - TX: GPIO 15 (TXD1 LIN1)
  - RX: GPIO 14 (RXD1 LIN1)
  - Baudrate: 9600, 8N2
  - Modus: Slave (für Inetbox-Emulation)

- **LIN2 (Alde-Ausgang):**
  - TX: GPIO 12 (TXD2 LIN2)
  - RX: GPIO 13 (RXD2 LIN2)
  - Baudrate: 9600, 8N2
  - Modus: Master

- **CI Bus (Optional, Dometic Klima):**
  - TX: GPIO 19
  - RX: GPIO 18
  - Baudrate: 19200, 8N2
  - Modus: Master

### Physische Verbindungen

1. **Truma-Modul an LIN1:**
   - Truma-Modul TX → ESP32 GPIO 14 (RXD1 LIN1)
   - Truma-Modul RX → ESP32 GPIO 15 (TXD1 LIN1)
   - GND verbinden

2. **Alde-Heizung an LIN2:**
   - ESP32 GPIO 12 (TXD2 LIN2) → Alde-Heizung RX
   - ESP32 GPIO 13 (RXD2 LIN2) → Alde-Heizung TX
   - GND verbinden

3. **Ethernet PHY (bereits auf Platine):**
   - IO16: 25 MHz Taktquelle für PHY
   - IO17: PHY Data
   - PHY-Konfiguration: `eth->phy_reg_write(eth, ksz80xx->addr, 0x1f, 0x80)`

3. **Power:**
   - ESP32 mit 5V oder 3.3V versorgen
   - GND gemeinsam verbinden

## Software Setup

### 1. Proxy-Konfiguration

Verwende die Datei `alde2mqtt_proxy.yaml`:

```yaml
lin_bus_proxy:
  truma_uart: truma_uart
  alde_uart: alde_uart
  proxy_mode: true
  logging_enabled: true
  log_sensor:
    name: "LIN Bus Proxy Log"
```

### 2. Kompilieren und Flashen

```bash
esphome compile alde2mqtt_proxy.yaml
esphome upload alde2mqtt_proxy.yaml
esphome logs alde2mqtt_proxy.yaml
```

## Funktionsweise

### Proxy-Modus

Der ESP32 fungiert als transparente Brücke:

1. **Daten von Truma empfangen:**
   - ESP32 liest Daten von UART 1 (Truma)
   - Analysiert Frames
   - Protokolliert Daten

2. **Daten an Alde weiterleiten:**
   - ESP32 sendet Daten an UART 2 (Alde)
   - Daten werden 1:1 durchgeleitet
   - Keine Verzögerung (minimal)

3. **Daten von Alde empfangen:**
   - ESP32 liest Antworten von UART 2 (Alde)
   - Analysiert Frames
   - Protokolliert Daten

4. **Daten an Truma weiterleiten:**
   - ESP32 sendet Antworten an UART 1 (Truma)
   - Vollständige bidirektionale Kommunikation

### Frame-Analyse

Der Proxy analysiert automatisch:

- **Frame-IDs** (0x03, 0x04, 0x05, 0x06, 0x07, 0x16)
- **Temperaturwerte** (Raum, Wasser)
- **Status-Informationen**
- **Befehle** (Kraftstoffmodus, Elektroleistung, Lüfter)

### Logging

Alle Frames werden geloggt:

- **Serial-Output:** Hex-Dump mit Timestamp
- **Text-Sensor:** Letzte Nachricht für Home Assistant
- **Statistik:** Anzahl der weitergeleiteten Frames

## Verwendung

### 1. Proxy aktivieren

Der Proxy ist standardmäßig aktiviert. Du kannst ihn über Home Assistant steuern:

```yaml
switch:
  - platform: template
    name: "Proxy Modus"
    id: proxy_mode_switch
```

### 2. Logging aktivieren/deaktivieren

```yaml
switch:
  - platform: template
    name: "Proxy Logging"
    id: proxy_logging_switch
```

### 3. Daten analysieren

**Serial Monitor:**
```
[1234] Truma->Alde: 00 55 43 1A 0B 00 00
  Frame 0x03 (Air Heater Command), 7 Bytes
  -> Temperatur: 22.0°C (0x0B1A)

[1235] Alde->Truma: 00 55 56 00 1A 0B 2C 0D 03 01 02 00
  Frame 0x16 (Info), 12 Bytes
  -> Raumtemperatur: 22.0°C
  -> Wassertemperatur: 45.0°C
  -> Status: 0x03
```

**Home Assistant:**
- Text-Sensor "LIN Bus Proxy Log" zeigt letzte Nachricht
- Sensoren für Frame-Statistik

## Vorteile des Proxy-Modus

### 1. Vollständige Protokoll-Analyse

- **Alle Frames** werden erfasst
- **Bidirektionale Kommunikation** sichtbar
- **Timing-Informationen** verfügbar

### 2. Keine Bus-Störung

- **Keine parallelen Verbindungen** nötig
- **Keine Pull-Up-Widerstände** erforderlich
- **Saubere Signal-Integrität**

### 3. Flexible Analyse

- **Frame-Filterung** möglich
- **Datenmodifikation** möglich (für Tests)
- **Statistik** in Echtzeit

### 4. Transparente Weiterleitung

- **Minimale Latenz** (< 1ms)
- **Keine Datenverluste**
- **Original-Kommunikation** bleibt erhalten

## Erweiterte Funktionen

### Frame-Filterung

Du kannst die Proxy-Komponente erweitern, um bestimmte Frames zu filtern:

```cpp
// In lin_bus_proxy.cpp
if (frame_id == 0x16) {
  // Status-Frames nicht weiterleiten (nur loggen)
  // return; // Nicht weiterleiten
}
```

### Datenmodifikation

Für Tests kannst du Daten modifizieren:

```cpp
// Temperatur-Wert ändern (nur für Tests!)
if (frame_id == 0x03) {
  data[3] = 0x1E; // Neue Temperatur
  data[4] = 0x0B;
}
```

### Statistik

Die Komponente zählt automatisch:
- Anzahl Truma→Alde Frames
- Anzahl Alde→Truma Frames
- Verfügbar als Sensoren in Home Assistant

## Troubleshooting

### Keine Daten werden weitergeleitet

1. **UART-Verbindungen prüfen**
   - TX/RX-Pins korrekt?
   - GND verbunden?
   - Baudrate korrekt (9600, 8N2)?

2. **Proxy-Modus aktiviert?**
   ```yaml
   proxy_mode: true
   ```

3. **Logs prüfen**
   ```bash
   esphome logs alde2mqtt_proxy.yaml
   ```

### Frames werden nicht erkannt

1. **Frame-Detection anpassen**
   - Break-Sequenz prüfen
   - Sync-Byte (0x55) prüfen
   - Timeout-Werte anpassen

2. **Buffer-Größe erhöhen**
   ```cpp
   uint8_t truma_rx_buffer_[512]; // Statt 256
   ```

### Hohe Latenz

1. **Delays reduzieren**
   ```cpp
   delayMicroseconds(100); // Statt 500
   ```

2. **Flush optimieren**
   ```cpp
   to->flush(); // Nur wenn nötig
   ```

## Beispiel-Konfiguration

Vollständige Konfiguration siehe `alde2mqtt_proxy.yaml`.

## Nächste Schritte

1. **Protokoll analysieren**
   - Alle Frames dokumentieren
   - Datenstrukturen verstehen
   - Checksummen validieren

2. **Komponente anpassen**
   - Frame-Strukturen in `alde_tin_bus.cpp` anpassen
   - Basierend auf gesnifften Daten

3. **Steuerung implementieren**
   - Von Proxy-Modus zu Steuerungs-Modus wechseln
   - Eigene Befehle senden

## Zusammenfassung

Der WomoLIN als Proxy bietet:
- ✅ Vollständige Protokoll-Analyse
- ✅ Transparente Datenweiterleitung
- ✅ Echtzeit-Logging
- ✅ Keine Bus-Störung
- ✅ Flexible Erweiterung

Perfekt für Reverse Engineering und Protokoll-Analyse!

