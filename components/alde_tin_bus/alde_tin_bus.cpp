#include "alde_tin_bus.h"
#include "esphome/core/log.h"

static const char *TAG = "alde_tin_bus";

namespace esphome {
namespace alde_tin_bus {

void AldeTinBusComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Alde TIN Bus Component...");
  this->last_update_ = millis();
  this->rx_buffer_pos_ = 0;
}

void AldeTinBusComponent::loop() {
  // Empfangene Daten verarbeiten
  while (this->available()) {
    uint8_t byte = this->read();
    
    if (this->rx_buffer_pos_ < sizeof(this->rx_buffer_)) {
      this->rx_buffer_[this->rx_buffer_pos_++] = byte;
    } else {
      this->rx_buffer_pos_ = 0; // Buffer overflow, zurücksetzen
    }
    
    // Prüfen ob vollständiger Frame empfangen wurde
    if (this->rx_buffer_pos_ >= 3 && this->rx_buffer_[0] == LIN_SYNC_BYTE) {
      uint8_t pid = this->rx_buffer_[1];
      uint8_t frame_id = pid & 0x3F;
      
      if (frame_id == TIN_FRAME_INFO) {
        // Info-Frame empfangen
        if (this->rx_buffer_pos_ >= 10) {
          this->parse_status_frame(this->rx_buffer_, this->rx_buffer_pos_);
          this->rx_buffer_pos_ = 0;
        }
      } else {
        // Anderer Frame, verwerfen nach kurzer Zeit
        if (this->rx_buffer_pos_ >= sizeof(this->rx_buffer_)) {
          this->rx_buffer_pos_ = 0;
        }
      }
    }
    
    // Timeout: Wenn länger keine Daten kommen, Buffer zurücksetzen
    static uint32_t last_byte_time = 0;
    uint32_t now = millis();
    if (now - last_byte_time > 100 && this->rx_buffer_pos_ > 0) {
      this->rx_buffer_pos_ = 0;
    }
    last_byte_time = now;
  }
  
  // Regelmäßig Status abfragen
  uint32_t now = millis();
  if (now - this->last_update_ >= this->update_interval_) {
    this->request_status();
    this->last_update_ = now;
  }
}

void AldeTinBusComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Alde TIN Bus Component:");
  ESP_LOGCONFIG(TAG, "  Update Interval: %u ms", this->update_interval_);
  if (this->air_temp_sensor_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Air Temperature Sensor: enabled");
  }
  if (this->water_temp_sensor_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Water Temperature Sensor: enabled");
  }
  if (this->power_switch_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Power Switch: enabled");
  }
}

uint8_t AldeTinBusComponent::calculate_protected_id(uint8_t frame_id) {
  uint8_t pid = frame_id & 0x3F;
  uint8_t p0 = ((pid >> 0) ^ (pid >> 1) ^ (pid >> 2) ^ (pid >> 4)) & 0x01;
  uint8_t p1 = ((pid >> 1) ^ (pid >> 3) ^ (pid >> 4) ^ (pid >> 5)) & 0x01;
  return pid | (p0 << 6) | (p1 << 7);
}

void AldeTinBusComponent::send_tin_frame(uint8_t frame_id, uint8_t *data, uint8_t data_len) {
  // Break-Sequenz (für LIN Bus Master)
  this->write_byte(LIN_BREAK_SEQUENCE);
  delayMicroseconds(100);
  
  // Sync Byte
  this->write_byte(LIN_SYNC_BYTE);
  
  // Protected ID
  uint8_t pid = this->calculate_protected_id(frame_id);
  this->write_byte(pid);
  
  // Daten senden
  for (int i = 0; i < data_len; i++) {
    this->write_byte(data[i]);
  }
  
  // Checksumme (Classic Checksum für Legacy TIN)
  uint8_t checksum = 0;
  for (int i = 0; i < data_len; i++) {
    checksum += data[i];
    if (checksum > 255) checksum -= 255;
  }
  this->write_byte(checksum);
  
  delay(10); // Kurze Pause zwischen Frames
  ESP_LOGD(TAG, "Frame 0x%02X gesendet, %d Bytes Daten", frame_id, data_len);
}

uint16_t AldeTinBusComponent::celsius_to_kelvin10(float celsius) {
  return (uint16_t)((celsius + 273.15) * 10);
}

float AldeTinBusComponent::kelvin10_to_celsius(uint16_t kelvin10) {
  return (kelvin10 / 10.0) - 273.15;
}

void AldeTinBusComponent::set_air_temperature(float celsius) {
  if (celsius < 5.0 || celsius > 35.0) {
    ESP_LOGW(TAG, "Raumtemperatur außerhalb des gültigen Bereichs: %.1f°C", celsius);
    return;
  }
  
  uint16_t kelvin10 = this->celsius_to_kelvin10(celsius);
  uint8_t data[2];
  data[0] = (uint8_t)(kelvin10 & 0xFF);
  data[1] = (uint8_t)((kelvin10 >> 8) & 0xFF);
  this->send_tin_frame(TIN_FRAME_AIR_HEATER_CMD, data, 2);
  ESP_LOGI(TAG, "Raumtemperatur gesetzt: %.1f°C (0x%04X)", celsius, kelvin10);
}

void AldeTinBusComponent::set_water_temperature(float celsius) {
  if (celsius < 20.0 || celsius > 70.0) {
    ESP_LOGW(TAG, "Wassertemperatur außerhalb des gültigen Bereichs: %.1f°C", celsius);
    return;
  }
  
  uint16_t kelvin10 = this->celsius_to_kelvin10(celsius);
  uint8_t data[2];
  data[0] = (uint8_t)(kelvin10 & 0xFF);
  data[1] = (uint8_t)((kelvin10 >> 8) & 0xFF);
  this->send_tin_frame(TIN_FRAME_WATER_HEATER_CMD, data, 2);
  ESP_LOGI(TAG, "Wassertemperatur gesetzt: %.1f°C (0x%04X)", celsius, kelvin10);
}

void AldeTinBusComponent::set_fuel_mode(uint8_t mode) {
  uint8_t data[1] = {mode & 0x03};
  this->send_tin_frame(TIN_FRAME_FUEL_CMD, data, 1);
  ESP_LOGI(TAG, "Kraftstoffmodus gesetzt: %d", mode);
}

void AldeTinBusComponent::set_electro_power(uint8_t power) {
  uint8_t data[1] = {power & 0x03};
  this->send_tin_frame(TIN_FRAME_ELECTRO_CMD, data, 1);
  ESP_LOGI(TAG, "Elektroleistung gesetzt: %d", power);
}

void AldeTinBusComponent::set_vent_speed(uint8_t speed) {
  uint8_t data[1] = {speed & 0x07};
  this->send_tin_frame(TIN_FRAME_VENT_CMD, data, 1);
  ESP_LOGI(TAG, "Lüftergeschwindigkeit gesetzt: %d", speed);
}

void AldeTinBusComponent::request_status() {
  uint8_t data[1] = {0x00};
  this->send_tin_frame(TIN_FRAME_INFO, data, 1);
}

void AldeTinBusComponent::parse_status_frame(uint8_t *data, uint8_t len) {
  if (len < 10) {
    ESP_LOGW(TAG, "Status-Frame zu kurz: %d Bytes", len);
    return;
  }
  
  // Info-Frame Struktur (muss an tatsächliche Daten angepasst werden):
  // [0] = 0x55 (Sync)
  // [1] = Protected ID
  // [2-3] = Raumtemperatur (Kelvin × 10, Little Endian)
  // [4-5] = Wassertemperatur (Kelvin × 10, Little Endian)
  // [6] = Status-Byte
  // [7] = Kraftstoffmodus
  // [8] = Elektroleistung
  // [9] = Lüftergeschwindigkeit
  // [10] = Checksumme
  
  uint16_t air_temp = data[2] | (data[3] << 8);
  uint16_t water_temp = data[4] | (data[5] << 8);
  uint8_t status = data[6];
  uint8_t fuel_mode = data[7];
  uint8_t electro_power = data[8];
  uint8_t vent_speed = data[9];
  
  float air_celsius = this->kelvin10_to_celsius(air_temp);
  float water_celsius = this->kelvin10_to_celsius(water_temp);
  
  // Sensoren aktualisieren
  if (this->air_temp_sensor_ != nullptr) {
    this->air_temp_sensor_->publish_state(air_celsius);
  }
  if (this->water_temp_sensor_ != nullptr) {
    this->water_temp_sensor_->publish_state(water_celsius);
  }
  if (this->status_sensor_ != nullptr) {
    this->status_sensor_->publish_state(status);
  }
  
  // Schalter aktualisieren
  if (this->power_switch_ != nullptr) {
    this->power_switch_->publish_state((fuel_mode & 0x03) != 0);
  }
  if (this->fuel_gas_switch_ != nullptr) {
    this->fuel_gas_switch_->publish_state((fuel_mode & 0x01) != 0);
  }
  if (this->fuel_electro_switch_ != nullptr) {
    this->fuel_electro_switch_->publish_state((fuel_mode & 0x02) != 0);
  }
  
  // Auswahl aktualisieren
  if (this->vent_speed_select_ != nullptr && vent_speed <= 7) {
    this->vent_speed_select_->publish_state(vent_speed);
  }
  if (this->electro_power_select_ != nullptr && electro_power <= 3) {
    this->electro_power_select_->publish_state(electro_power);
  }
  
  ESP_LOGD(TAG, "Status empfangen: Raum=%.1f°C, Wasser=%.1f°C, Status=0x%02X", 
           air_celsius, water_celsius, status);
}

}  // namespace alde_tin_bus
}  // namespace esphome

