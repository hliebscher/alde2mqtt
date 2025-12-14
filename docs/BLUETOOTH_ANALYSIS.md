# Bluetooth-Protokoll Analyse für Truma TIN Bus Modul

Diese Anleitung beschreibt, wie du die Bluetooth-Kommunikation zwischen der Truma-App und dem TIN-Bus-Modul analysieren kannst.

## Hardware Setup

### Option 1: ESP32 mit BLE-Sniffer

Der ESP32 kann als BLE-Sniffer fungieren, benötigt aber spezielle Firmware.

### Option 2: Dedizierter BLE-Sniffer

- **nRF52840 dongle** (Nordic Semiconductor)
- **CC2540 USB dongle** (Texas Instruments)
- **Ubertooth One** (Open Source BLE Sniffer)

### Option 3: Smartphone-App

- **nRF Connect** (Nordic Semiconductor) - kostenlos
- **BLE Scanner** - verschiedene Apps verfügbar

## Software-Tools

### Option 1: ESP32 BLE-Sniffer (Arduino)

Erstelle eine Datei `ble_sniffer/ble_sniffer.ino`:

```cpp
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BLEScan* pBLEScan;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.printf("Advertised Device: %s\n", advertisedDevice.toString().c_str());
      
      // MAC-Adresse ausgeben
      Serial.printf("  Address: %s\n", advertisedDevice.getAddress().toString().c_str());
      
      // Name prüfen
      if (advertisedDevice.haveName()) {
        Serial.printf("  Name: %s\n", advertisedDevice.getName().c_str());
      }
      
      // Services
      if (advertisedDevice.haveServiceUUID()) {
        Serial.printf("  Service UUID: %s\n", advertisedDevice.getServiceUUID().toString().c_str());
      }
      
      // RSSI
      Serial.printf("  RSSI: %d dBm\n", advertisedDevice.getRSSI());
      Serial.println();
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("BLE Sniffer gestartet...");
  
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // Aktives Scannen für mehr Informationen
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  Serial.println("Scanne nach BLE-Geräten...");
  BLEScanResults foundDevices = pBLEScan->start(5, false);
  Serial.printf("Gefundene Geräte: %d\n", foundDevices.getCount());
  pBLEScan->clearResults();
  delay(2000);
}
```

### Option 2: ESP32 BLE-Client (zum Verbinden)

Erstelle eine Datei `ble_client/ble_client.ino`:

```cpp
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEUtils.h>

// MAC-Adresse des Truma-Moduls (aus Sniffer ermitteln)
static BLEAddress serverAddress("XX:XX:XX:XX:XX:XX");

BLEClient* pClient;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Verbunden mit Truma-Modul");
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("Verbindung getrennt");
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("BLE Client gestartet...");
  
  BLEDevice::init("");
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  
  Serial.printf("Verbinde mit %s...\n", serverAddress.toString().c_str());
  
  if (pClient->connect(serverAddress)) {
    Serial.println("Verbunden!");
    
    // Services auflisten
    std::map<std::string, BLERemoteService*>* services = pClient->getServices();
    Serial.printf("Gefundene Services: %d\n", services->size());
    
    for (auto it = services->begin(); it != services->end(); ++it) {
      Serial.printf("  Service UUID: %s\n", it->first.c_str());
      
      BLERemoteService* service = it->second;
      std::map<std::string, BLERemoteCharacteristic*>* characteristics = service->getCharacteristics();
      
      for (auto charIt = characteristics->begin(); charIt != characteristics->end(); ++charIt) {
        Serial.printf("    Characteristic UUID: %s\n", charIt->first.c_str());
        BLERemoteCharacteristic* characteristic = charIt->second;
        
        // Properties
        if (characteristic->canRead()) Serial.println("      -> Read");
        if (characteristic->canWrite()) Serial.println("      -> Write");
        if (characteristic->canNotify()) Serial.println("      -> Notify");
      }
    }
  } else {
    Serial.println("Verbindung fehlgeschlagen!");
  }
}

void loop() {
  delay(1000);
}
```

### Option 3: nRF Connect (Smartphone App)

1. **App installieren**
   - iOS: App Store
   - Android: Google Play Store

2. **Truma-Modul scannen**
   - App öffnen
   - "Scan" drücken
   - Nach Truma-Modul suchen (Name oder MAC-Adresse)

3. **Services und Characteristics identifizieren**
   - Gerät auswählen
   - "Connect" drücken
   - Services durchsuchen
   - Characteristics für Read/Write/Notify finden

4. **Datenverkehr loggen**
   - "Enable notifications" für Notify-Characteristics
   - Befehle von App senden und Daten beobachten
   - Log exportieren

### Option 4: Wireshark mit BLE-Adapter

1. **BLE-Adapter anschließen**
   - nRF52840 dongle oder ähnlich
   - USB an Computer

2. **Wireshark öffnen**
   - Download: https://www.wireshark.org/

3. **Bluetooth-Interface wählen**
   - Capture → Options
   - Bluetooth-Interface auswählen

4. **Filter setzen**
   - Filter: `btle`
   - Oder spezifischer: `btle.addr == XX:XX:XX:XX:XX:XX`

5. **Datenverkehr aufzeichnen**
   - Capture starten
   - App-Befehle ausführen
   - Pakete analysieren

## Analyse-Schritte

### 1. Device Discovery

```
Ziele:
- MAC-Adresse des Truma-Moduls notieren
- Advertising-Daten analysieren
- Service-UUIDs identifizieren
- Device-Name erkennen
```

**Beispiel-Output:**
```
Advertised Device: Name: Truma-TIN-Modul
  Address: AA:BB:CC:DD:EE:FF
  Service UUID: 0000xxxx-0000-1000-8000-00805f9b34fb
  RSSI: -45 dBm
```

### 2. Service-Charakteristika

```
Ziele:
- GATT Services scannen
- Characteristics für Read/Write identifizieren
- Notify-Characteristics für Status-Updates finden
- UUIDs dokumentieren
```

**Typische GATT-Struktur:**
```
Service: 0000xxxx-0000-1000-8000-00805f9b34fb
  Characteristic: 0000xxxx-0000-1000-8000-00805f9b34fb
    Properties: Read, Write, Notify
    Value: [Daten]
```

### 3. Protokoll-Reverse-Engineering

#### Schritt 1: Befehle von App mitschneiden

1. **App öffnen und mit Modul verbinden**
2. **Aktionen ausführen:**
   - Heizung einschalten
   - Temperatur ändern
   - Status abfragen
3. **Datenverkehr aufzeichnen:**
   - nRF Connect: Log exportieren
   - Wireshark: Capture speichern
   - ESP32: Serial-Output loggen

#### Schritt 2: Datenstrukturen identifizieren

**Mögliche Formate:**
- **Raw Bytes**: Direkte TIN-Bus-Befehle
- **JSON**: Strukturierte Daten
- **Protobuf**: Binäres Format
- **Proprietär**: Herstellerspezifisch

**Beispiel-Analyse:**
```
Befehl: Heizung einschalten
  App → Modul: [0x05, 0x03, 0x00]
  Modul → App: [0x05, 0x03, 0x01] (Bestätigung)

Befehl: Temperatur setzen (22°C)
  App → Modul: [0x03, 0x1A, 0x0B, 0x00]
  (Frame-ID, Low-Byte, High-Byte, Checksum?)
```

#### Schritt 3: Checksummen/Validierung

- **CRC**: Cyclic Redundancy Check
- **XOR**: Einfache XOR-Checksumme
- **Summe**: Byte-Summe
- **Keine**: Keine Validierung

### 4. ESP32 BLE-Client Implementierung

Basierend auf den Erkenntnissen:

```cpp
#include <BLEDevice.h>
#include <BLEClient.h>

static BLEAddress serverAddress("XX:XX:XX:XX:XX:XX");
static BLEUUID serviceUUID("0000xxxx-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID("0000xxxx-0000-1000-8000-00805f9b34fb");

BLEClient* pClient;
BLERemoteCharacteristic* pRemoteCharacteristic;

void sendCommand(uint8_t* data, size_t length) {
  pRemoteCharacteristic->writeValue(data, length);
}

void setup() {
  BLEDevice::init("");
  pClient = BLEDevice::createClient();
  pClient->connect(serverAddress);
  
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  
  // Notifications aktivieren
  pRemoteCharacteristic->registerForNotify(notifyCallback);
}

void loop() {
  // Befehle senden
  uint8_t cmd[] = {0x05, 0x03};
  sendCommand(cmd, 2);
  delay(1000);
}
```

## Erwartete Datenstruktur

Die Bluetooth-Kommunikation könnte:

1. **TIN-Bus-Befehle direkt übertragen**
   - Gleiche Struktur wie LIN-Bus
   - Frame-IDs identisch

2. **Proprietäres Protokoll verwenden**
   - Herstellerspezifische Kodierung
   - Verschlüsselung möglich

3. **JSON/Protobuf für Datenstruktur nutzen**
   - Strukturierte Daten
   - Einfacher zu parsen

## Nächste Schritte

1. **Protokoll dokumentieren**
   - Alle Befehle auflisten
   - Datenstrukturen beschreiben
   - Checksummen-Algorithmus identifizieren

2. **ESP32 BLE-Client implementieren**
   - Verbindung aufbauen
   - Befehle senden
   - Antworten empfangen

3. **In ESPHome-Komponente integrieren**
   - BLE-Komponente erstellen
   - Oder: TIN-Bus-Befehle über BLE senden

4. **Testen und Validieren**
   - Alle Funktionen testen
   - Fehlerbehandlung implementieren

## Tipps

- **Logging**: Alle Datenverkehre protokollieren
- **Vergleich**: Bluetooth vs. LIN-Bus vergleichen
- **Dokumentation**: Alle Erkenntnisse sofort dokumentieren
- **Community**: Erfahrungen in Foren teilen

## Beispiel-Protokoll

```
[BLE] App → Modul: Write Characteristic
  UUID: 0000xxxx-0000-1000-8000-00805f9b34fb
  Data: 00 55 43 1A 0B 00 00
  -> TIN-Bus Frame 0x03 (Air Heater Command)
  -> Temperatur: 22.0°C

[BLE] Modul → App: Notification
  UUID: 0000xxxx-0000-1000-8000-00805f9b34fb
  Data: 00 55 56 00 1A 0B 2C 0D 03 01 02 00
  -> TIN-Bus Frame 0x16 (Info/Status)
  -> Raumtemperatur: 22.0°C
  -> Wassertemperatur: 45.0°C
```

