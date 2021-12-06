#include <device_credentials.hpp>
#include <wifi_helper.hpp>
namespace SDGC {
    namespace wifi_helper {

        unsigned long getTime() { return WiFi.getTime(); }

        void onMessageReceived(int messageSize) {
            // todo... need to parse lambda function msg in here
            // and send bluetooth message to the peripheral
        }

        void wifi_setup(BearSSLClient &sslClient, MqttClient &mqttClient) {
            if (!ECCX08.begin()) {
                Serial.println("No ECCX08 present!");
                while (1)
                    ;
            }
            ArduinoBearSSL.onGetTime(getTime);
            sslClient.setEccSlot(0, SECRET_CERTIFICATE);
            mqttClient.onMessage(onMessageReceived);
        }

        void wifi_connect() {
            Serial.print("Attempting to connect to SSID: ");
            Serial.print(SECRET_SSID);
            Serial.print(" ");

            while (WiFi.begin(SECRET_SSID, SECRET_PASS) != WL_CONNECTED) {
                // failed, retry
                Serial.print(".");
                delay(5000);
            }
            Serial.println();

            Serial.println("You're connected to the network");
            Serial.println();
        }

        void mqtt_connect(MqttClient &mqttClient) {
            Serial.print("Attempting to MQTT broker: ");
            Serial.print(SECRET_BROKER);
            Serial.println(" ");
            mqttClient.setId("SDGC-Arduino");
            while (!mqttClient.connect(SECRET_BROKER, 8883)) {
                // failed, retry
                Serial.print(".");
                delay(5000);
            }
            Serial.println();

            Serial.println("You're connected to the MQTT broker");
            Serial.println();

            // subscribe to a topic
            mqttClient.subscribe("SDGC/arduino/incoming"); // reusing stuff from HW 1 :)
            // mqttClient.onMessage(onMessageReceived);
        }
    } // namespace wifi_helper
} // namespace SDGC