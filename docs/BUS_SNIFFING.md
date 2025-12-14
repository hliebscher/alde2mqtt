# TIN-Bus Sniffing Anleitung

Diese Anleitung beschreibt, wie du den TIN-Bus-Verkehr zwischen dem Truma-Modul und der Alde-Heizung mitschneiden kannst, um das Protokoll zu verstehen.

## Hardware Setup

### Option 1: ESP32 als Bus-Monitor (Parallel)

1. **T-Adapter oder Bus-Monitor verwenden**
   - LIN-Bus zwischen Truma-Modul und Alde-Heizung aufteilen
   - ESP32 parallel an den Bus anschließen (hochohmig, um Bus nicht zu stören)

2. **Verbindungen**
   - ESP32 RX (GPIO 14) an LIN-Bus (über Pull-Up Widerstand)
   - GND verbinden
   - Vorsicht: TX nicht anschließen, nur RX zum Mitschneiden

### Option 2: ESP32 zwischen Modul und Heizung

1. **ESP32 als Gateway**
   - Truma-Modul → ESP32 → Alde-Heizung
   - ESP32 leitet Daten durch und protokolliert gleichzeitig

## Software für Sniffing

### Option 1: ESP32 als Bus-Sniffer (Arduino)

Erstelle eine Datei `bus_sniffer/bus_sniffer.ino`:

```cpp
#include <HardwareSerial.h>

HardwareSerial LinBus(2); // Serial2

void setup() {
  Serial.begin(115200);
  // WomoLIN Konfiguration: 9600, 8N2
  LinBus.begin(9600, SERIAL_8N2, 14, 15);
  
  Serial.println("TIN-Bus Sniffer gestartet");
  Serial.println("Format: [Timestamp] RX: [Hex Bytes]");
}

void loop() {
  if (LinBus.available()) {
    Serial.print("[");
    Serial.print(millis());
    Serial.print("] RX: ");
    
    uint8_t buffer[32];
    int len = 0;
    
    while (LinBus.available() && len < 32) {
      buffer[len++] = LinBus.read();
    }
    
    // Hex-Dump ausgeben
    for (int i = 0; i < len; i++) {
      if (buffer[i] < 0x10) Serial.print("0");
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    // Frame-Analyse
    if (len >= 3 && buffer[0] == 0x55) {
      uint8_t pid = buffer[1];
      uint8_t frame_id = pid & 0x3F;
      Serial.print("  -> Frame ID: 0x");
      if (frame_id < 0x10) Serial.print("0");
      Serial.print(frame_id, HEX);
      Serial.print(" (Protected ID: 0x");
      if (pid < 0x10) Serial.print("0");
      Serial.print(pid, HEX);
      Serial.println(")");
    }
  }
}
```

### Option 2: Python-Script für Analyse

Erstelle eine Datei `bus_sniffer/analyze_bus.py`:

```python
#!/usr/bin/env python3
import serial
import time
import sys

# Serial-Port anpassen (z.B. /dev/ttyUSB0, COM3, etc.)
SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 9600

# WomoLIN Konfiguration: 8N2
ser = serial.Serial(
    SERIAL_PORT,
    BAUD_RATE,
    bytesize=8,
    parity='N',
    stopbits=2,
    timeout=1
)

print("TIN-Bus Sniffer gestartet...")
print("Drücke Ctrl+C zum Beenden")
print("-" * 60)

try:
    buffer = []
    while True:
        if ser.in_waiting:
            byte = ser.read(1)[0]
            buffer.append(byte)
            
            # Frame erkannt (Sync Byte)
            if len(buffer) >= 3 and buffer[0] == 0x55:
                pid = buffer[1]
                frame_id = pid & 0x3F
                
                # Frame komplett? (mindestens 3 Bytes + Daten + Checksumme)
                # Annahme: Frame ist komplett wenn 100ms keine Daten kommen
                time.sleep(0.1)
                while ser.in_waiting:
                    buffer.append(ser.read(1)[0])
                
                # Hex-Dump
                timestamp = time.time()
                hex_data = ' '.join(f'{b:02X}' for b in buffer)
                print(f"[{timestamp:.3f}] {hex_data}")
                
                # Frame-Analyse
                print(f"  Frame ID: 0x{frame_id:02X} (PID: 0x{pid:02X})")
                
                # Bekannte Frames
                frame_names = {
                    0x03: "Air Heater Command",
                    0x04: "Water Heater Command",
                    0x05: "Fuel Command",
                    0x06: "Electro Command",
                    0x07: "Vent Command",
                    0x16: "Info (Status)"
                }
                
                if frame_id in frame_names:
                    print(f"  -> {frame_names[frame_id]}")
                
                # Temperatur-Daten extrahieren (für Frame 0x03, 0x04, 0x16)
                if frame_id in [0x03, 0x04] and len(buffer) >= 6:
                    temp_kelvin10 = buffer[3] | (buffer[4] << 8)
                    temp_celsius = (temp_kelvin10 / 10.0) - 273.15
                    print(f"  -> Temperatur: {temp_celsius:.1f}°C (0x{temp_kelvin10:04X})")
                
                if frame_id == 0x16 and len(buffer) >= 10:
                    air_temp = buffer[2] | (buffer[3] << 8)
                    water_temp = buffer[4] | (buffer[5] << 8)
                    air_celsius = (air_temp / 10.0) - 273.15
                    water_celsius = (water_temp / 10.0) - 273.15
                    print(f"  -> Raumtemperatur: {air_celsius:.1f}°C")
                    print(f"  -> Wassertemperatur: {water_celsius:.1f}°C")
                    print(f"  -> Status: 0x{buffer[6]:02X}")
                
                buffer = []
        else:
            # Timeout: Buffer zurücksetzen
            if len(buffer) > 0:
                buffer = []
            time.sleep(0.01)

except KeyboardInterrupt:
    print("\nSniffing beendet")
    ser.close()
    sys.exit(0)
```

## Protokoll-Analyse

### Erwartete Frame-Struktur

```
[Break: 0x00] [Sync: 0x55] [Protected ID] [Data Bytes...] [Checksum]
```

### Frame-IDs (Legacy TIN)

- **0x03**: Air Heater Command (Raumtemperatur-Sollwert)
  - Daten: 2 Bytes (Kelvin × 10, Little Endian)
  
- **0x04**: Water Heater Command (Wassertemperatur-Sollwert)
  - Daten: 2 Bytes (Kelvin × 10, Little Endian)
  
- **0x05**: Fuel Command (Kraftstoff/Elektrofreigabe)
  - Daten: 1 Byte (0=aus, 1=Gas, 2=Elektro, 3=Gas+Elektro)
  
- **0x06**: Electro Command (Elektroheizungs-Leistungsstufe)
  - Daten: 1 Byte (0=aus, 1-3=Leistungsstufen)
  
- **0x07**: Vent Command (Lüfter/Lüftung)
  - Daten: 1 Byte (0=aus, 1-7=Lüfterstufen)
  
- **0x16**: Info (Statusinformationen vom Gerät)
  - Daten: 8+ Bytes (Temperaturwerte, Status, etc.)

## Analyse-Schritte

1. **Heizung einschalten**
   - Beobachte Frame 0x05 mit Daten
   - Notiere die Byte-Werte für verschiedene Modi

2. **Temperatur ändern**
   - Beobachte Frame 0x03 (Raumtemperatur) oder 0x04 (Wassertemperatur)
   - Verifiziere die Temperatur-Kodierung (Kelvin × 10)

3. **Status abfragen**
   - Beobachte Frame 0x16 (Info)
   - Analysiere die Datenstruktur der Antwort

4. **Muster erkennen**
   - Dokumentiere wiederkehrende Sequenzen
   - Identifiziere Checksummen-Algorithmus

## Tipps

- **Logging**: Speichere alle gesnifften Daten in einer Datei für spätere Analyse
- **Filterung**: Filtere nach bekannten Frame-IDs für bessere Übersicht
- **Timing**: Beachte die Timing-Charakteristika (Break-Sequenz, Delays)
- **Validierung**: Vergleiche gesendete und empfangene Frames

## Beispiel-Output

```
[1234.567] 00 55 43 1A 0B 00 00
  Frame ID: 0x03 (PID: 0x43)
  -> Air Heater Command
  -> Temperatur: 22.0°C (0x0B1A)

[1235.123] 00 55 56 00 1A 0B 2C 0D 03 01 02 00
  Frame ID: 0x16 (PID: 0x56)
  -> Info (Status)
  -> Raumtemperatur: 22.0°C
  -> Wassertemperatur: 45.0°C
  -> Status: 0x03
```

