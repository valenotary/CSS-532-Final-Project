#include <ble_helper.hpp>

namespace SDGC {
    namespace BLE_helper {
        void ble_setup() {
            BLE.stopScan();
            BLE.disconnect();
            BLE.end();
            if (!BLE.begin()) {
                Serial.print("Starting BLE failed!");
                while (1) {
                    Serial.print("Starting BLE failed!");
                }
                        }
            Serial.println("BLE Central");
            // start scanning for peripherals
            BLE.scan();
        }
    } // namespace BLE_helper
} // namespace SDGC