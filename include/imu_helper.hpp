#ifndef IMU_HELPER
#define IMU_HELPER

#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>
#include <Wire.h>

namespace SDGC {
    namespace IMU_helper {
        // MPU control/status vars
        extern bool dmpReady; // set true if DMP init was successful
        extern uint8_t mpuIntStatus; // holds actual interrupt status byte from MPU
        extern uint8_t devStatus; // return status after each device operation (0 = success, !0 = error)
        extern uint16_t packetSize; // expected DMP packet size (default is 42 bytes)
        extern uint16_t fifoCount; // count of all bytes currently in FIFO
        extern uint8_t fifoBuffer[64]; // FIFO storage buffer
        extern volatile bool mpuInterrupt; // indicates whether MPU interrupt pin has gone high
        extern void dmpDataReady();
        // set up the initialization of the IMU stuff
        void imu_setup(MPU6050 &);
        // very rudimentary calibration method
        void imu_calibrate(MPU6050 &);
        // calculating the absolute sum of every data point
        int16_t imu_sum_abs(const int16_t, const int16_t, const int16_t);
        
    } // namespace IMU_helper
} // namespace SDGC

#endif
