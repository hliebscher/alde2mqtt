#include "lin_bus_proxy.h"
#include "esphome/core/log.h"

static const char *TAG = "lin_bus_proxy";

namespace esphome {
namespace lin_bus_proxy {

void LinBusProxyComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LIN Bus Proxy Component...");
  ESP_LOGCONFIG(TAG, "  Proxy Mode: %s", this->proxy_mode_ ? "enabled" : "disabled");
  ESP_LOGCONFIG(TAG, "  Logging: %s", this->logging_enabled_ ? "enabled" : "disabled");
  
  this->truma_rx_pos_ = 0;
  this->alde_rx_pos_ = 0;
  this->truma_to_alde_count_ = 0;
  this->alde_to_truma_count_ = 0;
  this->last_log_time_ = millis();
}

void LinBusProxyComponent::loop() {
  if (!this->proxy_mode_) {
    return;
  }
  
  // Daten von Truma-Modul empfangen und an Alde weiterleiten
  if (this->truma_uart_ != nullptr && this->alde_uart_ != nullptr) {
    this->forward_data(this->truma_uart_, this->alde_uart_, 
                      this->truma_rx_buffer_, this->truma_rx_pos_, 
                      "Truma->Alde");
  }
  
  // Daten von Alde empfangen und an Truma weiterleiten
  if (this->alde_uart_ != nullptr && this->truma_uart_ != nullptr) {
    this->forward_data(this->alde_uart_, this->truma_uart_, 
                      this->alde_rx_buffer_, this->alde_rx_pos_, 
                      "Alde->Truma");
  }
}

void LinBusProxyComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LIN Bus Proxy:");
  ESP_LOGCONFIG(TAG, "  Proxy Mode: %s", this->proxy_mode_ ? "enabled" : "disabled");
  ESP_LOGCONFIG(TAG, "  Logging: %s", this->logging_enabled_ ? "enabled" : "disabled");
  ESP_LOGCONFIG(TAG, "  Truma->Alde Frames: %u", this->truma_to_alde_count_);
  ESP_LOGCONFIG(TAG, "  Alde->Truma Frames: %u", this->alde_to_truma_count_);
}

void LinBusProxyComponent::forward_data(uart::UARTComponent *from, 
                                        uart::UARTComponent *to,
                                        uint8_t *buffer, 
                                        uint8_t &pos,
                                        const char *direction) {
  static uint32_t last_byte_time[2] = {0, 0};
  uint32_t direction_idx = (strcmp(direction, "Truma->Alde") == 0) ? 0 : 1;
  
  // Daten von "from" UART lesen
  while (from->available() && pos < sizeof(buffer) - 1) {
    uint8_t byte = from->read();
    buffer[pos++] = byte;
    last_byte_time[direction_idx] = millis();
    
    // Frame erkannt? (Sync Byte 0x55)
    if (pos >= 3 && buffer[0] == 0x55) {
      // Frame könnte komplett sein, warte kurz auf weitere Bytes
      delayMicroseconds(500);
      
      // Restliche Bytes lesen
      while (from->available() && pos < sizeof(buffer) - 1) {
        buffer[pos++] = from->read();
      }
      
      // Frame analysieren und loggen
      if (this->logging_enabled_) {
        this->analyze_frame(buffer, pos, direction);
        this->log_frame(buffer, pos, direction);
      }
      
      // Daten an "to" UART weiterleiten
      for (uint8_t i = 0; i < pos; i++) {
        to->write_byte(buffer[i]);
      }
      to->flush();
      
      // Statistik aktualisieren
      if (direction_idx == 0) {
        this->truma_to_alde_count_++;
      } else {
        this->alde_to_truma_count_++;
      }
      
      // Buffer zurücksetzen
      pos = 0;
      return;
    }
  }
  
  // Timeout: Wenn länger keine Daten kommen, Buffer zurücksetzen
  uint32_t now = millis();
  if (pos > 0 && (now - last_byte_time[direction_idx] > 200)) {
    // Unvollständiger Frame, trotzdem weiterleiten
    if (this->logging_enabled_) {
      ESP_LOGW(TAG, "%s: Unvollständiger Frame (%d Bytes)", direction, pos);
    }
    for (uint8_t i = 0; i < pos; i++) {
      to->write_byte(buffer[i]);
    }
    to->flush();
    pos = 0;
  }
}

void LinBusProxyComponent::analyze_frame(uint8_t *data, uint8_t len, const char *direction) {
  if (len < 3) return;
  
  // Frame-ID extrahieren
  uint8_t pid = data[1];
  uint8_t frame_id = pid & 0x3F;
  
  // Bekannte Frame-IDs
  const char *frame_name = "Unknown";
  switch (frame_id) {
    case 0x03: frame_name = "Air Heater Command"; break;
    case 0x04: frame_name = "Water Heater Command"; break;
    case 0x05: frame_name = "Fuel Command"; break;
    case 0x06: frame_name = "Electro Command"; break;
    case 0x07: frame_name = "Vent Command"; break;
    case 0x16: frame_name = "Info (Status)"; break;
  }
  
  ESP_LOGD(TAG, "%s: Frame 0x%02X (%s), %d Bytes", direction, frame_id, frame_name, len);
  
  // Temperatur-Daten extrahieren (für Frame 0x03, 0x04, 0x16)
  if (frame_id == 0x03 || frame_id == 0x04) {
    if (len >= 6) {
      uint16_t temp_kelvin10 = data[3] | (data[4] << 8);
      float temp_celsius = (temp_kelvin10 / 10.0) - 273.15;
      ESP_LOGD(TAG, "  -> Temperatur: %.1f°C (0x%04X)", temp_celsius, temp_kelvin10);
    }
  }
  
  if (frame_id == 0x16) {
    if (len >= 10) {
      uint16_t air_temp = data[2] | (data[3] << 8);
      uint16_t water_temp = data[4] | (data[5] << 8);
      float air_celsius = (air_temp / 10.0) - 273.15;
      float water_celsius = (water_temp / 10.0) - 273.15;
      ESP_LOGD(TAG, "  -> Raumtemperatur: %.1f°C", air_celsius);
      ESP_LOGD(TAG, "  -> Wassertemperatur: %.1f°C", water_celsius);
      ESP_LOGD(TAG, "  -> Status: 0x%02X", data[6]);
    }
  }
}

void LinBusProxyComponent::log_frame(uint8_t *data, uint8_t len, const char *direction) {
  if (this->log_sensor_ == nullptr) return;
  
  // Hex-String erstellen
  char hex_string[512];
  char *ptr = hex_string;
  
  ptr += sprintf(ptr, "[%lu] %s: ", millis(), direction);
  for (uint8_t i = 0; i < len && i < 32; i++) {
    ptr += sprintf(ptr, "%02X ", data[i]);
  }
  if (len > 32) {
    ptr += sprintf(ptr, "...");
  }
  
  // Text-Sensor aktualisieren (nur letzte Nachricht)
  this->log_sensor_->publish_state(hex_string);
}

}  // namespace lin_bus_proxy
}  // namespace esphome

