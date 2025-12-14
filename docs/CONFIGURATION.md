# Konfigurations-Guide

## Was gehört in secrets.yaml und was nicht?

### ✅ In secrets.yaml (gemeinsame Werte)

Diese Werte sind für alle Geräte gleich oder sollten nicht im Repository committet werden:

```yaml
# WiFi-Konfiguration (gemeinsam für alle Geräte)
wifi_ssid: "MeinWLAN"
wifi_password: "MeinPasswort"

# MQTT-Broker (gemeinsam für alle Geräte)
mqtt_broker: "192.168.1.100"
mqtt_username: "mqtt_user"  # Optional
mqtt_password: "mqtt_pass"  # Optional
```

### ❌ NICHT in secrets.yaml (gerätespezifisch)

Diese Werte sind **gerätespezifisch** und sollten direkt in der YAML-Datei bleiben:

#### 1. ESP32 Hardware-Konfiguration

```yaml
esphome:
  name: alde-proxy  # Eindeutiger Name pro Gerät
  platform: ESP32
  board: esp32dev
  framework:
    type: arduino
  flash_size: 16MB  # Hardware-spezifisch (WomoLIN hat 16MB)
```

**Warum nicht in secrets?**
- `name`: Muss eindeutig sein für jedes Gerät
- `flash_size`: Hardware-spezifisch, aber bei allen WomoLIN-Boards gleich

#### 2. API Encryption Key

```yaml
api:
  encryption:
    key: " . . . "
```

**Warum nicht in secrets?**
- **Sicherheit:** Jeder ESP32 sollte einen eigenen, eindeutigen Encryption Key haben
- **Gerätespezifisch:** Wird beim ersten Setup generiert und ist gerätegebunden
- **Nicht teilen:** Sollte nicht zwischen Geräten geteilt werden

**Wichtig:** Wenn du den Key teilst, können andere Geräte sich als dieses Gerät ausgeben!

#### 3. OTA Password

```yaml
ota:
  - platform: esphome
    passwor " . . . "
```

**Warum nicht in secrets?**
- **Sicherheit:** Jeder ESP32 sollte ein eigenes OTA-Passwort haben
- **Gerätespezifisch:** Wird beim ersten Setup generiert
- **Schutz:** Verhindert unbefugte OTA-Updates

**Wichtig:** Wenn du das Passwort teilst, können andere Geräte Updates auf dieses Gerät pushen!

## Empfohlene Struktur

### secrets.yaml (gemeinsam)

```yaml
# Gemeinsame Werte für alle Geräte
wifi_ssid: "MeinWLAN"
wifi_password: "MeinPasswort"
mqtt_broker: "192.168.1.100"
```

### alde2mqtt.yaml / alde2mqtt_proxy.yaml (gerätespezifisch)

```yaml
esphome:
  name: alde-heizung  # Eindeutig pro Gerät
  # ... Hardware-Konfiguration

api:
  encryption:
    key: "..."  # Gerätespezifisch

ota:
  - platform: esphome
    password: "..."  # Gerätespezifisch
```

## Neue Geräte einrichten

### 1. YAML-Datei kopieren

```bash
cp alde2mqtt.yaml alde2mqtt-neues-geraet.yaml
```

### 2. Gerätespezifische Werte ändern

```yaml
esphome:
  name: alde-neues-geraet  # Neuer Name

api:
  encryption:
    key: "NEUER_KEY_HIER"  # Neuer Key generieren

ota:
  - platform: esphome
    password: "NEUES_PASSWORT_HIER"  # Neues Passwort generieren
```

### 3. Keys/Passwörter generieren

**API Encryption Key generieren:**
```bash
openssl rand -base64 32
```

**OTA Password generieren:**
```bash
openssl rand -hex 16
```

Oder in ESPHome Dashboard: Beim ersten Kompilieren werden diese automatisch generiert.

## Sicherheitshinweise

1. **Niemals API Keys oder OTA Passwords teilen**
   - Jedes Gerät braucht eigene Werte
   - Nicht in secrets.yaml für gemeinsame Nutzung

2. **Secrets.yaml schützen**
   - Nicht ins Repository committen (ist in .gitignore)
   - Sichere Speicherung

3. **Gerätespezifische Werte**
   - In der YAML-Datei lassen
   - Bei Bedarf in gerätespezifischen YAML-Dateien

## Zusammenfassung

| Wert | In secrets.yaml? | Grund |
|------|------------------|-------|
| WiFi SSID/Password | ✅ Ja | Gemeinsam für alle Geräte |
| MQTT Broker | ✅ Ja | Gemeinsam für alle Geräte |
| ESP32 Board Config | ❌ Nein | Hardware-spezifisch |
| API Encryption Key | ❌ Nein | Gerätespezifisch, Sicherheit |
| OTA Password | ❌ Nein | Gerätespezifisch, Sicherheit |
| Gerätename | ❌ Nein | Muss eindeutig sein |

