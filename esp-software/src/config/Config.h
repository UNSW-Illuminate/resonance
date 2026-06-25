#pragma once

#include <Arduino.h>

namespace Config {
    // UART for Teensy
    constexpr uint8_t TEENSY_TX_PIN = 17;
    constexpr uint8_t TEENSY_RX_PIN = 18;
    constexpr uint32_t TEENSY_BAUD = 115200;

    // Wi-Fi AP settings
    constexpr const char* AP_SSID = "Resonance";
    constexpr const char* AP_PASSWORD = "0123456789";
    constexpr const char* AP_IP = "192.168.4.1";
    constexpr const char* AP_GATEWAY = "192.168.4.1";
    constexpr const char* AP_SUBNET = "255.255.255.0";

    // WLED integration
    constexpr const char* WLED_HOST = "192.168.4.3";
    constexpr uint32_t WLED_TIMEOUT_MS = 750;
    constexpr bool WLED_ENABLED = true;
}
