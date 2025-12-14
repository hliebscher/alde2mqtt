#pragma once

#include "esphome.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"

namespace esphome {
namespace alde_tin_bus {

// Legacy TIN Bus Signal-Frames
#define TIN_FRAME_AIR_HEATER_CMD    0x03
#define TIN_FRAME_WATER_HEATER_CMD  0x04
#define TIN_FRAME_FUEL_CMD          0x05
#define TIN_FRAME_ELECTRO_CMD        0x06
#define TIN_FRAME_VENT_CMD           0x07
#define TIN_FRAME_INFO               0x16

// LIN Bus Konstanten
#define LIN_SYNC_BYTE 0x55
#define LIN_BREAK_SEQUENCE 0x00

class AldeTinBusComponent : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  // Sensoren
  void set_air_temperature_sensor(sensor::Sensor *sensor) { air_temp_sensor_ = sensor; }
  void set_water_temperature_sensor(sensor::Sensor *sensor) { water_temp_sensor_ = sensor; }
  void set_status_sensor(sensor::Sensor *sensor) { status_sensor_ = sensor; }
  
  // Zahlen (Number)
  void set_air_temperature_number(number::Number *number) { air_temp_number_ = number; }
  void set_water_temperature_number(number::Number *number) { water_temp_number_ = number; }
  
  // Schalter
  void set_power_switch(switch_::Switch *sw) { power_switch_ = sw; }
  void set_fuel_gas_switch(switch_::Switch *sw) { fuel_gas_switch_ = sw; }
  void set_fuel_electro_switch(switch_::Switch *sw) { fuel_electro_switch_ = sw; }
  
  // Auswahl (Select)
  void set_vent_speed_select(select::Select *sel) { vent_speed_select_ = sel; }
  void set_electro_power_select(select::Select *sel) { electro_power_select_ = sel; }
  
  // Update-Intervall
  void set_update_interval(uint32_t interval) { update_interval_ = interval; }
  
  // Öffentliche Methoden für Lambda-Aufrufe
  void set_air_temperature(float celsius);
  void set_water_temperature(float celsius);

 protected:
  // TIN Bus Kommunikation
  void send_tin_frame(uint8_t frame_id, uint8_t *data, uint8_t data_len);
  uint8_t calculate_protected_id(uint8_t frame_id);
  bool receive_tin_frame(uint8_t *buffer, uint8_t max_len, uint8_t expected_frame_id);
  
  // Temperatur-Konvertierung
  uint16_t celsius_to_kelvin10(float celsius);
  float kelvin10_to_celsius(uint16_t kelvin10);
  
  // Befehle
  void set_fuel_mode(uint8_t mode);
  void set_electro_power(uint8_t power);
  void set_vent_speed(uint8_t speed);
  void request_status();
  void parse_status_frame(uint8_t *data, uint8_t len);
  
  // Sensoren
  sensor::Sensor *air_temp_sensor_{nullptr};
  sensor::Sensor *water_temp_sensor_{nullptr};
  sensor::Sensor *status_sensor_{nullptr};
  
  // Zahlen
  number::Number *air_temp_number_{nullptr};
  number::Number *water_temp_number_{nullptr};
  
  // Schalter
  switch_::Switch *power_switch_{nullptr};
  switch_::Switch *fuel_gas_switch_{nullptr};
  switch_::Switch *fuel_electro_switch_{nullptr};
  
  // Auswahl
  select::Select *vent_speed_select_{nullptr};
  select::Select *electro_power_select_{nullptr};
  
  uint32_t update_interval_{10000}; // 10 Sekunden
  uint32_t last_update_{0};
  uint8_t rx_buffer_[32];
  uint8_t rx_buffer_pos_{0};
};

}  // namespace alde_tin_bus
}  // namespace esphome

