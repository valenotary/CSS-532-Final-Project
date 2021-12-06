#include <imu_helper.hpp>

namespace SDGC {
    namespace IMU_helper {

        // MPU control/status vars
        bool dmpReady = false; // set true if DMP init was successful
        uint8_t mpuIntStatus; // holds actual interrupt status byte from MPU
        uint8_t devStatus; // return status after each device operation (0 = success, !0 = error)
        uint16_t packetSize; // expected DMP packet size (default is 42 bytes)
        uint16_t fifoCount; // count of all bytes currently in FIFO
        uint8_t fifoBuffer[64]; // FIFO storage buffer
        volatile bool mpuInterrupt = false; // indicates whether MPU interrupt pin has gone high
        void dmpDataReady() { mpuInterrupt = true; }


        void imu_setup(MPU6050 &imu) {
            Wire.begin();
            // initialize device
            Serial.println("Initializing IMU...");
            imu.initialize();

            // verify connection
            Serial.println("Testing device connections...");
            Serial.println(imu.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

            // load and configure the DMP
            Serial.println(F("Initializing DMP..."));
            imu.dmpInitialize();

            // make sure it worked (returns 0 if so)
            if (devStatus == 0) {
                imu_calibrate(imu);
                // turn on the DMP, now that it's ready
                Serial.println(F("Enabling DMP..."));
                imu.setDMPEnabled(true);

                // enable Arduino interrupt detection
                Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
                Serial.print(digitalPinToInterrupt(2));
                Serial.println(F(")..."));
                attachInterrupt(digitalPinToInterrupt(2), dmpDataReady, RISING);
                mpuIntStatus = imu.getIntStatus();

                // set our DMP Ready flag so the main loop() function knows it's okay to use it
                Serial.println(F("DMP ready! Waiting for first interrupt..."));
                dmpReady = true;

                // get expected DMP packet size for later comparison
                packetSize = imu.dmpGetFIFOPacketSize();
            }
            else {
                // ERROR!
                // 1 = initial memory load failed
                // 2 = DMP configuration updates failed
                // (if it's going to break, usually the code will be 1)
                Serial.print(F("DMP Initialization failed (code "));
                Serial.print(devStatus);
                Serial.println(F(")"));
            }
        }
        void imu_calibrate(MPU6050 &imu) {
            Serial.println("Calibrating device. Please hold it still...");
            delay(1000);

            // the following calibration offset values
            // were precalculated from a different sketch
            imu.setXAccelOffset(-3337);
            imu.setYAccelOffset(-2202);
            imu.setZAccelOffset(1188);

            imu.setXGyroOffset(154);
            imu.setYGyroOffset(-74);
            imu.setZGyroOffset(46);
        }
        int16_t imu_sum_abs(const int16_t ax, const int16_t ay, const int16_t az) {
            return abs(ax) + abs(ay) + abs(az);
        }
    } // namespace IMU_helper
} // namespace SDGC