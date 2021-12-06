#ifndef WIFI_HELPER
#define WIFI_HELPER

#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000

namespace SDGC {
    namespace wifi_helper {
        void wifi_setup(BearSSLClient &, MqttClient &);
        void wifi_connect();
        void mqtt_connect(MqttClient&);
    } // namespace wifi_helper
} // namespace SDGC

#endif