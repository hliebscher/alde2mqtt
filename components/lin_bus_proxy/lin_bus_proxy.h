#pragma once

#include "esphome.h"
#include "esphome/components/uart/uart.h"

#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

namespace esphome {
namespace lin_bus_proxy {

class LinBusProxyComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  // UART-Konfiguration
  void set_truma_uart(uart::UARTComponent *uart) { truma_uart_ = uart; }
  void set_alde_uart(uart::UARTComponent *uart) { alde_uart_ = uart; }
  
  // Text-Sensor für Logging
#ifdef USE_TEXT_SENSOR
  void set_log_sensor(text_sensor::TextSensor *sensor) { log_sensor_ = sensor; }
#else
  void set_log_sensor(void *sensor) { (void)sensor; }  // Dummy wenn nicht verfügbar
#endif
  
  // Proxy-Modus
  void set_proxy_mode(bool enabled) { proxy_mode_ = enabled; }
  void set_logging_enabled(bool enabled) { logging_enabled_ = enabled; }
  
  // Statistik
  uint32_t get_truma_to_alde_count() { return truma_to_alde_count_; }
  uint32_t get_alde_to_truma_count() { return alde_to_truma_count_; }

 protected:
  // UART-Komponenten
  uart::UARTComponent *truma_uart_{nullptr};
  uart::UARTComponent *alde_uart_{nullptr};
  
  // Text-Sensor für Logging
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *log_sensor_{nullptr};
#else
  void *log_sensor_{nullptr};
#endif
  
  // Proxy-Modus
  bool proxy_mode_{true};
  bool logging_enabled_{true};
  
  // Buffer für Datenübertragung
  uint8_t truma_rx_buffer_[256];
  uint8_t alde_rx_buffer_[256];
  uint8_t truma_rx_pos_{0};
  uint8_t alde_rx_pos_{0};
  
  // Statistik
  uint32_t truma_to_alde_count_{0};
  uint32_t alde_to_truma_count_{0};
  uint32_t last_log_time_{0};
  
  // Frame-Analyse
  void analyze_frame(uint8_t *data, uint8_t len, const char *direction);
  void log_frame(uint8_t *data, uint8_t len, const char *direction);
  void forward_data(uart::UARTComponent *from, uart::UARTComponent *to, 
                    uint8_t *buffer, uint8_t &pos, const char *direction);
};

}  // namespace lin_bus_proxy
}  // namespace esphome

