#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace uart_readline {

class UartReadlineTextSensor : public Component, public text_sensor::TextSensor, public uart::UARTDevice {
 public:
  void setup() override {
    ESP_LOGD("uart_readline", "UART Readline sensor setup complete");
  }

  void loop() override {
    static std::string buffer;
    static uint32_t last_read_time = 0;
    
    while (available()) {
      uint8_t byte;
      read_byte(&byte);
      
      uint32_t now = millis();
      
      // Log every byte received for debugging
      ESP_LOGV("uart_readline", "Received byte: 0x%02X (%c)", byte, (byte >= 32 && byte <= 126) ? byte : '.');
      
      // If more than 50ms since last byte, consider it a new message
      if (now - last_read_time > 50 && !buffer.empty()) {
        publish_state(buffer);
        ESP_LOGD("uart_readline", "Timeout publish: %s", buffer.c_str());
        buffer.clear();
      }
      last_read_time = now;
      
      // Skip protocol response headers
      if (buffer.empty() && (byte == 0x22 || byte == 0x24 || byte == 0x33 || byte == 0x44)) {
        ESP_LOGV("uart_readline", "Skipping protocol header: 0x%02X", byte);
        continue;
      }
      
      // Add printable characters to buffer
      if (byte >= 0x20 && byte <= 0x7E) {
        buffer += (char)byte;
      }
      
      // Check for null terminator
      if (byte == 0x00 && !buffer.empty()) {
        publish_state(buffer);
        ESP_LOGD("uart_readline", "Null terminated: %s", buffer.c_str());
        buffer.clear();
      }
      
      // Prevent buffer overflow
      if (buffer.length() > 256) {
        publish_state(buffer);
        ESP_LOGD("uart_readline", "Buffer overflow, publishing: %s", buffer.c_str());
        buffer.clear();
      }
    }
    
    // Final timeout check
    if (!buffer.empty() && (millis() - last_read_time > 50)) {
      publish_state(buffer);
      ESP_LOGD("uart_readline", "Final timeout publish: %s", buffer.c_str());
      buffer.clear();
    }
  }
};

}  // namespace uart_readline
}  // namespace esphome