# WomoLIN Hardware-Referenz

## Übersicht

Der WomoLIN ist ein ESP32-basierter LIN-Bus-Controller für die Steuerung von Wohnmobil-Heizungen (Truma, Alde) und Klimaanlagen (Dometic) über TIN- und CI-Bus.

## Hardware-Spezifikationen

### Mikrocontroller
- **Typ:** ESP32 WROOM
- **Flash:** 16 MB
- **CPU:** 240 MHz (konfigurierbar)

### Kommunikation
- **WLAN:** Externe Antenne mit UFL-Stecker (im Lieferumfang)
- **Ethernet:** KSZ8081RNA PHY-Chip
  - 25 MHz Taktquelle über IO16
  - PHY-Konfiguration: `eth->phy_reg_write(eth, ksz80xx->addr, 0x1f, 0x80)`

### LIN-Bus Interfaces

#### LIN1 (Truma TIN Bus)
- **GPIO 14:** RXD1 LIN1
- **GPIO 15:** TXD1 LIN1
- **Baudrate:** 9600
- **Konfiguration:** 8N2 (8 Datenbits, keine Parität, 2 Stoppbits)
- **Modus:** Slave (für Inetbox-Emulation)

#### LIN2 (Alde TIN Bus)
- **GPIO 12:** TXD2 LIN2
- **GPIO 13:** RXD2 LIN2
- **Baudrate:** 9600
- **Konfiguration:** 8N2
- **Modus:** Master

#### CI Bus (Dometic Klima, optional)
- **GPIO 18:** CI RX
- **GPIO 19:** CI TX
- **Baudrate:** 19200
- **Konfiguration:** 8N2
- **Modus:** Master

## Pin-Belegung

| GPIO | Funktion | Richtung | Verwendung |
|------|----------|----------|------------|
| 12   | TXD2 LIN2 | Output | Alde TIN Bus (Ausgang) |
| 13   | RXD2 LIN2 | Input  | Alde TIN Bus (Ausgang) |
| 14   | RXD1 LIN1 | Input  | Truma TIN Bus (Eingang) |
| 15   | TXD1 LIN1 | Output | Truma TIN Bus (Eingang) |
| 16   | PHY Clock | Output | Ethernet PHY (25 MHz) |
| 17   | PHY Data  | I/O    | Ethernet PHY |
| 18   | CI RX     | Input  | CI Bus (optional) |
| 19   | CI TX     | Output | CI Bus (optional) |

## Wichtige Hinweise

### GPIO-Konflikte

**Problem:** Die Standard-UART 1-Pins IO16/17 werden vom Ethernet PHY verwendet.

**Lösung:** LIN-Interfaces müssen auf andere GPIO-Pins umgelegt werden:
- LIN1 verwendet GPIO 14/15 (statt IO16/17)
- LIN2 verwendet GPIO 12/13

### Bus-Konfiguration

**Truma TIN Bus (LIN1):**
- Für Inetbox-Emulation (z.B. mit Inetbox2MQTT)
- Betrieb als Slave
- 9600 Baud

**Alde TIN Bus (LIN2):**
- Direkte Steuerung der Alde-Heizung
- Betrieb als Master
- 9600 Baud

**CI Bus:**
- Für Dometic-Klimaanlagen
- Betrieb als Master
- 19200 Baud

## Beispiel-Konfigurationen

### ESPHome UART-Konfiguration

```yaml
# LIN1 (Truma TIN Bus)
uart:
  - id: truma_uart
    tx_pin: 15  # TXD1 LIN1
    rx_pin: 14  # RXD1 LIN1
    baud_rate: 9600
    data_bits: 8
    parity: NONE
    stop_bits: 2

# LIN2 (Alde TIN Bus)
  - id: alde_uart
    tx_pin: 12  # TXD2 LIN2
    rx_pin: 13  # RXD2 LIN2
    baud_rate: 9600
    data_bits: 8
    parity: NONE
    stop_bits: 2

# CI Bus (Dometic Klima)
  - id: ci_uart
    tx_pin: 19
    rx_pin: 18
    baud_rate: 19200
    data_bits: 8
    parity: NONE
    stop_bits: 2
```

## Verwendung

### Gleichzeitige Nutzung mehrerer Buses

Der WomoLIN kann alle drei Buses gleichzeitig verwenden:
- Truma-Heizung über TIN Bus (LIN1) steuern
- Alde-Heizung über TIN Bus (LIN2) steuern
- Dometic-Klima über CI Bus steuern

### Proxy-Modus

Für Protokoll-Analyse kann der WomoLIN als Proxy fungieren:
- Truma-Modul → LIN1 → ESP32 → LIN2 → Alde-Heizung
- Alle Daten werden durchgeleitet und protokolliert

## Troubleshooting

### Keine Kommunikation

1. **GPIO-Pins prüfen**
   - Korrekte Pin-Belegung verwenden
   - IO16/17 nicht für LIN-Bus verwenden!

2. **Baudrate prüfen**
   - TIN Bus: 9600 Baud
   - CI Bus: 19200 Baud

3. **Stoppbits prüfen**
   - Beide Buses verwenden 2 Stoppbits (8N2)

4. **GND verbinden**
   - Gemeinsame Masse erforderlich

### Ethernet PHY

Falls Ethernet verwendet wird:
- PHY benötigt 25 MHz Taktquelle über IO16
- PHY-Konfiguration erforderlich: `eth->phy_reg_write(eth, ksz80xx->addr, 0x1f, 0x80)`

## Referenzen

- [WomoLIN Website](https://womolin.de/)
- [TIN-Bus Protokoll (WomoNET Wiki)](https://wiki.womonet.io/de/protocols/tin/)
- [ESP32 Pinout](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html)

