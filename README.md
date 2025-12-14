# Alde2MQTT - ESPHome TIN-Bus Komponente

ESPHome-Komponente zur Steuerung von Alde-Heizungen über TIN-Bus (Truma Information Network) mit WomoLIN LIN-Controller.

## Features

- ✅ Raumtemperatur-Steuerung und -Monitoring
- ✅ Wassertemperatur-Steuerung und -Monitoring
- ✅ Kraftstoffmodus (Gas/Elektro)
- ✅ Elektroleistung
- ✅ Lüftergeschwindigkeit
- ✅ Status-Monitoring
- ✅ MQTT-Integration
- ✅ Home Assistant Discovery

## Hardware

### WomoLIN LIN-Controller

- **ESP32 WROOM** mit 16 MB Flash-Speicher
- **Externe WLAN-Antenne** (UFL-Stecker, im Lieferumfang enthalten)
- **Ethernet PHY:** KSZ8081RNA-Chip
  - Verwendet IO16/17 für PHY (25 MHz Taktquelle über IO16)
  - PHY-Konfiguration: `eth->phy_reg_write(eth, ksz80xx->addr, 0x1f, 0x80)`

### GPIO-Pin-Mapping

**Wichtig:** Da die Standard-UART 1-Pins IO16/17 vom Ethernet PHY verwendet werden, müssen die LIN-Interfaces auf andere GPIO-Pins umgelegt werden.

| GPIO | Bezeichnung | Verwendung |
|------|-------------|------------|
| 14   | RXD1 LIN1   | Truma TIN Bus (Eingang) |
| 15   | TXD1 LIN1   | Truma TIN Bus (Eingang) |
| 13   | RXD2 LIN2   | Alde TIN Bus (Ausgang) |
| 12   | TXD2 LIN2   | Alde TIN Bus (Ausgang) |
| 16   | PHY Clock   | Ethernet PHY (25 MHz) |
| 17   | PHY Data    | Ethernet PHY |

### LIN-Bus Konfiguration

**Truma TIN Bus:**
- Baudrate: **9600**
- Datenbits: 8
- Parität: NONE
- Stoppbits: 2
- Modus: **Slave** (für Inetbox-Emulation, z.B. mit Inetbox2MQTT)

**CI Bus (Dometic Klima):**
- Baudrate: **19200**
- Datenbits: 8
- Parität: NONE
- Stoppbits: 2
- Modus: **Master**

**Verwendung:**
- Truma-Heizung über TIN Bus steuern
- Dometic-Klima über CI Bus steuern
- Beide Buses können gleichzeitig verwendet werden

## Hardware-Details

### WomoLIN Spezifikationen

- **Mikrocontroller:** ESP32 WROOM mit 16 MB Flash
- **WLAN:** Externe Antenne mit UFL-Stecker (im Lieferumfang)
- **Ethernet:** KSZ8081RNA PHY-Chip
- **LIN-Bus:** 2x LIN-Interface (LIN1, LIN2)
- **CI-Bus:** Optional für Dometic-Klima

### Pin-Belegung

| GPIO | Funktion | Beschreibung |
|------|----------|--------------|
| 12   | TXD2 LIN2 | Alde TIN Bus (Ausgang) |
| 13   | RXD2 LIN2 | Alde TIN Bus (Ausgang) |
| 14   | RXD1 LIN1 | Truma TIN Bus (Eingang) |
| 15   | TXD1 LIN1 | Truma TIN Bus (Eingang) |
| 16   | PHY Clock | Ethernet PHY (25 MHz) |
| 17   | PHY Data  | Ethernet PHY |
| 18   | CI RX     | CI Bus (optional) |
| 19   | CI TX     | CI Bus (optional) |

### Beispiel: Ethernet-Konfiguration

Falls du Ethernet verwenden möchtest:

```cpp
// PHY-Konfiguration für KSZ8081RNA
eth->phy_reg_write(eth, ksz80xx->addr, 0x1f, 0x80);
```

**Weitere Details:** Siehe [Hardware-Referenz](docs/HARDWARE.md) für vollständige Pin-Belegung und Konfiguration.

## Installation

### 1. Komponenten einbinden

Die Komponenten werden automatisch über GitHub eingebunden:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/hliebscher/alde2mqtt
      ref: main
    components: [alde_tin_bus, lin_bus_proxy]
```

**Verfügbare Komponenten:**
- `alde_tin_bus` - TIN-Bus Steuerung für Alde-Heizung
- `lin_bus_proxy` - Proxy-Komponente für Protokoll-Analyse

**Alternative: Lokale Installation**

Falls du die Komponenten lokal verwenden möchtest:

```bash
# Im ESPHome-Verzeichnis
mkdir -p ~/.esphome/custom_components
cp -r components/alde_tin_bus ~/.esphome/custom_components/
cp -r components/lin_bus_proxy ~/.esphome/custom_components/
```

Dann in der YAML-Datei:

```yaml
external_components:
  - source:
      type: local
      path: components
```

### 2. Secrets-Datei erstellen

1. **Beispiel-Datei kopieren:**
   ```bash
   cp secrets.yaml.example secrets.yaml
   ```

2. **Secrets-Datei bearbeiten** (`secrets.yaml`):
   ```yaml
   # WiFi-Konfiguration
   wifi_ssid: "DEIN_WLAN_SSID"
   wifi_password: "DEIN_WLAN_PASSWORT"
   
   # MQTT-Broker Konfiguration
   mqtt_broker: "192.168.1.100"  # IP-Adresse oder Hostname
   mqtt_username: "mqtt_user"   # Optional
   mqtt_password: "mqtt_pass"   # Optional
   ```

**Wichtig:** Die `secrets.yaml` ist in `.gitignore` und wird nicht ins Repository committet. Die `secrets.yaml.example` dient als Vorlage.

### 3. Kompilieren und Flashen

```bash
# Kompilieren
esphome compile alde2mqtt.yaml

# Auf ESP32 flashen
esphome upload alde2mqtt.yaml

# Logs anzeigen
esphome logs alde2mqtt.yaml
```

## Konfiguration

### Basis-Konfiguration

```yaml
alde_tin_bus:
  uart_id: lin_uart_bus
  update_interval: 10s
  
  air_temperature:
    name: "Alde Raumtemperatur"
  water_temperature:
    name: "Alde Wassertemperatur"
  power:
    name: "Alde Heizung"
```

### Vollständige Konfiguration

Siehe `alde2mqtt.yaml` für alle verfügbaren Optionen.

## Protokoll-Analyse

Falls die Komponente nicht direkt funktioniert, musst du möglicherweise das Protokoll analysieren:

### WomoLIN als Proxy (Empfohlen)

**Beste Methode für WomoLIN mit 3 LIN-Bus-Eingängen!**

Siehe [Proxy Setup Anleitung](docs/PROXY_SETUP.md) für Details.

**Schnellstart:**
1. Truma-Modul an UART 1 anschließen (GPIO 14/15)
2. Alde-Heizung an UART 2 anschließen (GPIO 16/17)
3. `alde2mqtt_proxy.yaml` verwenden
4. ESP32 leitet Daten durch und protokolliert alles
5. Vollständige bidirektionale Kommunikation analysieren

**Vorteile:**
- ✅ Keine Bus-Störung
- ✅ Alle Frames werden erfasst
- ✅ Transparente Weiterleitung
- ✅ Echtzeit-Logging

### Bus-Sniffing

Siehe [Bus-Sniffing Anleitung](docs/BUS_SNIFFING.md) für Details.

**Schnellstart:**
1. ESP32 als Bus-Sniffer konfigurieren
2. Truma-Modul und Alde-Heizung verbinden
3. Datenverkehr mitschneiden
4. Frame-Strukturen analysieren

### Bluetooth-Analyse

Falls du ein Truma Bluetooth-Modul hast:

Siehe [Bluetooth-Analyse Anleitung](docs/BLUETOOTH_ANALYSIS.md) für Details.

**Schnellstart:**
1. nRF Connect App installieren
2. Truma-Modul scannen und verbinden
3. Services und Characteristics identifizieren
4. Datenverkehr loggen

## Anpassungen

Die Komponente muss möglicherweise an dein spezifisches Alde-Modell angepasst werden:

### Frame-Struktur anpassen

In `components/alde_tin_bus/alde_tin_bus.cpp`:

```cpp
void AldeTinBusComponent::parse_status_frame(uint8_t *data, uint8_t len) {
  // Hier die tatsächliche Datenstruktur anpassen
  // basierend auf gesnifften Daten
}
```

### Device-ID prüfen

Falls deine Alde-Heizung eine andere Device-ID verwendet, muss diese möglicherweise in den Frames angepasst werden.

### Checksummen-Algorithmus

Der aktuelle Checksummen-Algorithmus (Classic Checksum) muss möglicherweise angepasst werden, falls deine Heizung einen anderen verwendet.

## Troubleshooting

### Keine Daten empfangen

1. **UART-Verbindung prüfen**
   - TX/RX-Pins korrekt?
   - GND verbunden?
   - Baudrate korrekt (9600, 8N2)?

2. **LIN-Bus-Verbindung prüfen**
   - LIN-Controller korrekt angeschlossen?
   - Bus-Terminierung vorhanden?

3. **Logs aktivieren**
   ```yaml
   logger:
     level: DEBUG
   ```

### Falsche Temperaturwerte

1. **Frame-Struktur prüfen**
   - Byte-Reihenfolge (Little/Big Endian)?
   - Temperatur-Kodierung (Kelvin × 10)?

2. **Status-Frame analysieren**
   - Gesnifften Datenverkehr mit Code vergleichen

### Befehle werden nicht ausgeführt

1. **Frame-ID prüfen**
   - Korrekte Protected ID?
   - Checksumme korrekt?

2. **Timing prüfen**
   - Break-Sequenz vorhanden?
   - Delays zwischen Frames?

## Projektstruktur

```
alde2mqtt/
├── components/
│   └── alde_tin_bus/
│       ├── alde_tin_bus.h          # Header-Datei
│       ├── alde_tin_bus.cpp        # Implementierung
│       └── __init__.py             # Python-Bindings
├── docs/
│   ├── BUS_SNIFFING.md             # Bus-Sniffing Anleitung
│   └── BLUETOOTH_ANALYSIS.md       # Bluetooth-Analyse Anleitung
├── alde2mqtt.yaml                  # ESPHome Konfiguration
└── README.md                       # Diese Datei
```

## Entwicklung

### Komponente erweitern

1. **Neue Funktionen hinzufügen**
   - In `alde_tin_bus.h`: Methoden deklarieren
   - In `alde_tin_bus.cpp`: Implementieren
   - In `__init__.py`: Python-Bindings hinzufügen

2. **Tests**
   - Mit Bus-Sniffer validieren
   - Logs analysieren

## Referenzen

- [ESPHome Dokumentation](https://esphome.io/)
- [WomoLIN](https://womolin.de/)
- [TIN-Bus Protokoll (WomoNET Wiki)](https://wiki.womonet.io/de/protocols/tin/)
- [Legacy TIN Protokoll](https://wiki.womonet.io/de/protocols/tin/legacy_tin/)

## Lizenz

MIT License

## Beitragen

Beiträge sind willkommen! Bitte:
1. Fork das Repository
2. Erstelle einen Feature-Branch
3. Committe deine Änderungen
4. Erstelle einen Pull Request

## Support

Bei Problemen:
1. Logs prüfen (DEBUG-Level)
2. Bus-Sniffing durchführen
3. Issue im Repository erstellen

## Changelog

### v1.0.0
- Initiale Implementierung
- Basis-Funktionalität für TIN-Bus
- Raum- und Wassertemperatur
- Kraftstoffmodus
- Elektroleistung
- Lüftergeschwindigkeit

