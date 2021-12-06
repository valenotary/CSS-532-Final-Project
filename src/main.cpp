/*
    Project Title: Smart Device Gesture Control
    Course: CSS 532 (Internet of Things)
    Instructor: Prof. Yang Peng
    Author: Franz Anthony Varela
    Description: This is a prototype of a small wearable device that can interface with BT enabled devices. It has two
   main modes of operation due to dev board limitations:
    1. WiFi Mode: Gesture prediction is uploaded to cloud, lambda function response is forwarded to laptop hub, laptop
   hub parses the response and sends the byte data via bluetooth to smart device.
    2. BLE Mode: Gesture prediction is parsed on device and sent directly via BLE.
    The Arduino MKR 1010 WiFi supports both WiFi and BLE, but apparently not simultaneously. Thus multiple options were
   explored, but ultimately I chose to go this path to fulfill the project requirements while also demonstrating the
   expected behavior.

    Generalized Order of Execution:
        Initialize Components:
            - Wifi V
            - Bluetooth V
            - imu6050 (Accelerometer/Gyroscope) V
            - SVM V


    Empirical testing shows that the average absolute sum of readings should be
    ~300 as the threshold between rest and possible gesture.
*/

#if defined MODE_WIFI
#include <ArduinoJson.h>
#include <wifi_helper.hpp>
#elif defined MODE_BLE
#include <ble_helper.hpp>
#endif
#include <imu_helper.hpp>
#include <model.hpp>


// ENUM CLASS FOR FLAGGING
enum class CurrentState {
    Idle,
    GestureRecording,
    GestureFinished,
    InferenceFinished,
    UploadingToCloud,
    WaitingForCloud,
    SendingToBT
};

enum class CurrentPrediction { SwipeUp = 0, SwipeDown, Misc };

const int16_t ACCEL_THRESHOLD = 300;

// BLUETOOTH STUFF

// ACCELEROEMETER STUFF
MPU6050 accelgyro(0x68);

// WIFI STUFF
#if defined MODE_WIFI
WiFiClient wifiClient;
BearSSLClient sslClient(wifiClient);
MqttClient mqttClient(sslClient);
#endif

// GENERAL STUFF
const int ledPin = LED_BUILTIN;

bool recording_gesture = false;

float gesture_buffer[70];
CurrentPrediction currentPrediction;
size_t buffer_index = 0;

size_t record_length = 0;
unsigned long time_now = 0;

// ML MODEL
Eloquent::ML::Port::SVM svm;

CurrentState currentState;

void setup() {
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);
#if defined MODE_WIFI
    Serial.println("Starting in WiFi mode...");
    SDGC::wifi_helper::wifi_setup(sslClient, mqttClient);
    SDGC::wifi_helper::wifi_connect();
    SDGC::wifi_helper::mqtt_connect(mqttClient);
    
    Serial.println("Letting Hub know that I am alive...");
    mqttClient.beginMessage("SDGC/arduino/outgoing");
    StaticJsonDocument<256> connectedJSONMessage;
    connectedJSONMessage["action"] = "connecting";
    connectedJSONMessage["clientID"] = "SDGC-Arduino";
    serializeJson(connectedJSONMessage, mqttClient);
    mqttClient.endMessage();
    bool hub_is_connected = false;
    Serial.println("Awaiting notice from Hub...");
    while (!hub_is_connected) { // loop in setup until we know that the hub is connected
        if (mqttClient.parseMessage()) { // if we got something
            Serial.print("Got message from: ");
            Serial.println(mqttClient.messageTopic());
            StaticJsonDocument<256> doc;
            deserializeJson(doc, mqttClient);
            if (doc["action"] == "connecting" && doc["clientID"] == "SDGC-Hub") {
                Serial.println("Hub is connected to AWS IoT as well!");
                hub_is_connected = true;
            }
        }
    }

#elif defined MODE_BLE
    Serial.println("Starting in BLE mode...");
    SDGC::BLE_helper::ble_setup(); // central device
#endif
    SDGC::IMU_helper::imu_setup(accelgyro);
    currentState = CurrentState::Idle;
    currentPrediction = CurrentPrediction::Misc;
    Serial.println("Starting main loop now...");
    time_now = millis();
}

void loop() {
#if defined MODE_BLE
    BLEDevice peripheral = BLE.available();
    if (!peripheral)
        return;
    Serial.println("Peripheral found...");
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();
    if (peripheral.localName() != "LED")
        return;
    Serial.println("LED name found...");
    if (!peripheral.connect())
        return;
    Serial.println("Connected!");
    BLE.stopScan();
    Serial.println("Discovering attributes...");
    if (!peripheral.discoverAttributes())
        return;
    BLECharacteristic ledCharacteristic = peripheral.characteristic("19B10001-E8F2-537E-4F6C-D104768A1214");
    while (peripheral.connected()) {
#endif
        // regardless, the common loop here is to get the IMU readings...
        if (!SDGC::IMU_helper::dmpReady)
#if defined MODE_BLE
            continue;
#elif defined MODE_WIFI
        return;
#endif
        if (!accelgyro.dmpGetCurrentFIFOPacket(SDGC::IMU_helper::fifoBuffer))
#if defined MODE_BLE
            continue;
#elif defined MODE_WIFI
        return;
#endif
        Quaternion q;
        VectorInt16 aa, aaReal, aaWorld;
        VectorFloat gravity;


        // get the readings from the FIFO buffer and calculate aaWorld and q
        accelgyro.dmpGetQuaternion(&q, SDGC::IMU_helper::fifoBuffer);
        accelgyro.dmpGetAccel(&aa, SDGC::IMU_helper::fifoBuffer);
        accelgyro.dmpGetGravity(&gravity, &q);
        accelgyro.dmpGetLinearAccel(&aaReal, &aa, &gravity);
        accelgyro.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
        // get the absolute sum to detect motion
        int16_t sum_abs = SDGC::IMU_helper::imu_sum_abs(aaWorld.x, aaWorld.y, aaWorld.z);

        if (currentState == CurrentState::Idle &&
            sum_abs > ACCEL_THRESHOLD) { // you've detected motion, start recording
            Serial.println("Gesture starting to record...");
            // digitalWrite(ledPin, HIGH);
            currentState = CurrentState::GestureRecording;
        }
        else if (currentState == CurrentState::GestureRecording && record_length >= 10) { // wrap up the recording
            Serial.println("Stopping the recording...");
            digitalWrite(ledPin, LOW);
            record_length = 0;
            buffer_index = 0;
            currentState = CurrentState::GestureFinished;
        }
        if (currentState == CurrentState::GestureRecording) {
            gesture_buffer[buffer_index] = aaWorld.x;
            gesture_buffer[buffer_index + 1] = aaWorld.y;
            gesture_buffer[buffer_index + 2] = aaWorld.z;

            gesture_buffer[buffer_index + 3] = q.w;
            gesture_buffer[buffer_index + 4] = q.x;
            gesture_buffer[buffer_index + 5] = q.y;
            gesture_buffer[buffer_index + 6] = q.z;

            buffer_index += 7;
            record_length++;
        }
        else if (currentState ==
                 CurrentState::GestureFinished) { // determine how to send the data to the connected device
            Serial.println("Running inference...");
            currentPrediction = static_cast<CurrentPrediction>(svm.predict(gesture_buffer));
            String currentPredictionString = svm.idxToLabel(static_cast<uint8_t>(currentPrediction));
            Serial.print("Predicted action: ");
            Serial.println(currentPredictionString);
            if (currentPrediction != CurrentPrediction::Misc) {
#if defined MODE_WIFI // send to cloud, let hub send the BLE byte message
                Serial.println("Publishing MQTT message...");
                mqttClient.beginMessage("SDGC/arduino/outgoing");
                StaticJsonDocument<256> doc;
                doc["gesture"] = currentPredictionString;
                serializeJson(doc, mqttClient);
                mqttClient.endMessage();
                Serial.println("MQTT message sent. Waiting for reply...");
                bool hub_reply_success = false;
                while (!hub_reply_success) { // wait until we get a reply
                    if (mqttClient.parseMessage()) {
                        Serial.print("Got message from: ");
                        Serial.println(mqttClient.messageTopic());
                        StaticJsonDocument<256> doc;
                        deserializeJson(doc, mqttClient);
                        if (doc["action"] == "notify" && doc["fromClientID"] == "SDGC-Hub" &&
                            doc["toClientID"] == "SDGC-Arduino" && doc["message"] == "actionCompleted") {
                            Serial.println("Hub has propagated the action to the BLE device");
                            hub_reply_success = true;
                        }
                    }
                }
                Serial.println("Got response from cloud...");
#elif defined MODE_BLE // we send the BLE byte message ourselves, no cloud involved
            Serial.println("Propagating message to bluetooth device...");
            ledCharacteristic.writeValue(static_cast<byte>(currentPrediction));
#endif
            }
            else
                Serial.println("Action was determined as misc. No point forwarding it anywhere...");
            // ... after everything ...
            delay(2000);
            currentState = CurrentState::Idle;
            Serial.println("Ready for next action!");
            digitalWrite(ledPin, HIGH);
        }
        delay(100);
#if defined MODE_BLE
    }
    Serial.println("Disconnected from peripheral. Rescanning...");
    BLE.scan();
#endif
}
