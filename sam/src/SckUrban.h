#pragma once

#include <Wire.h>
#include <Arduino.h>
#include <RTCZero.h>

#include <Sensors.h>
#include "Pins.h"
#ifdef WITH_URBAN
#ifdef WITH_MPL
#include <Adafruit_MPL3115A2.h>
#endif
#ifdef WITH_LPS33
#include <Adafruit_LPS35HW.h>
#endif
#include "SckSoundTables.h"
#include <I2S.h>

#ifdef WITH_CCS811
#include <SparkFunCCS811.h>
#endif

#ifdef WITH_SPS30
// Sensirion Library for SPS30
// https://github.com/Sensirion/arduino-sps
#include <sps30.h>
#endif

#ifdef WITH_BME68X
// Adafruit BME608 library
#include <Adafruit_BME680.h>
#endif
#endif

// Firmware for SmartCitizen Kit - Urban Sensor Board SCK 2.2
// It includes drivers for this sensors:
//
// * Light - BH1730FVC -> (0x29)
// * Temperature and Humidity - SHT31 -> (0x44)
// * Noise - Invensense ICS43432 I2S microphone -> (I2S)
// 2.2 DEPRECATED (MOVE TO AUX) -> * Barometric pressure - MPL3115 -> (0x60)
// 2.2 DEPRECATED (MOVE TO AUX) -> * VOC and ECO2 - CCS811 -> (0x5a)
// * Pressure - ST LPS33 -> (0x5d)
// * UVA A,B and C - AS7331 -> (0x14)

class SckBase;

// Pins
const uint8_t pinPM_SERIAL_RX = pinBOARD_CONN_11;
const uint8_t pinPM_SERIAL_TX = pinBOARD_CONN_13;

enum SensorState
{
    SENSOR_BUSY,
    SENSOR_READY,
    SENSOR_ERROR
};

#ifdef WITH_URBAN
// Light
class Sck_BH1730FVC
    {
        // Datasheet
        // https://www.mouser.es/datasheet/2/348/bh1730fvc-e-1018573.pdf

    private:
        bool updateValues();

        // Config values
        uint8_t ITIME;          // Integration Time (datasheet page 9)
        float ITIME_ms;
        const uint8_t ITIME_max = 252;
        const uint8_t ITIME_min = 1;
        const uint16_t goUp = 32768;    // On the high part
        float Tmt;          // Measurement time (datasheet page 9)
        const float Tint = 2.8;     // Internal Clock Period (datasheet page 4 --> 2.8 typ -- 4.0 max)
        uint8_t Gain = 1;

        float DATA0;            // Visible Light
        float DATA1;            // Infrared Light
    public:
        bool debug = false;
        const uint8_t address = 0x29;
        int reading;
        bool start();
        bool stop();
        bool get();
    };

// Temperature and Humidity
class Sck_SHT31
    {
        // Datasheet
        // https://www.sensirion.com/fileadmin/user_upload/customers/sensirion/Dokumente/2_Humidity_Sensors/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital.pdf
        // This code is based on Adafruit SHT31 library, thanks! (https://github.com/adafruit/Adafruit_SHT31)
    private:

        TwoWire *_Wire;

        // Commands
        const uint16_t SOFT_RESET = 0x30A2;
        const uint16_t SINGLE_SHOT_HIGH_REP = 0x2400;

        const uint32_t timeout = 100; // Time in ms to wait for a reading
        const uint8_t retrys = 3;
        bool debug = false;
        bool update();
        bool sendComm(uint16_t comm);
        uint8_t crc8(const uint8_t *data, int len);
    public:
        uint8_t address;
        // Conntructor
        Sck_SHT31(TwoWire *localWire, uint8_t customAddress=0x44);

        float temperature;
        float humidity;
        bool start();
        bool stop();
        bool getReading();
    };

// Noise
class Sck_Noise
    {
    public:
        bool debugFlag = false;
        const uint32_t sampleRate = 44100;
        static const uint16_t SAMPLE_NUM = 512;
        static const uint16_t FFT_NUM = 256;
        float readingDB;
        int32_t readingFFT[FFT_NUM];
        bool start();
        bool stop();
        bool getReading(SensorType wichSensor);

    private:
        bool alreadyStarted = false;
        const double RMS_HANN = 0.61177;
        const uint8_t FULL_SCALE_DBSPL = 120;
        const uint8_t BIT_LENGTH = 24;
        const double FULL_SCALE_DBFS = 20*log10(pow(2,(BIT_LENGTH)));
        int32_t source[SAMPLE_NUM]; // 2k
        int16_t scaledSource[SAMPLE_NUM]; // 1k
        bool FFT(int32_t *source);
        void arm_bitreversal(int16_t * pSrc16, uint32_t fftLen, uint16_t * pBitRevTab);
        void arm_radix2_butterfly( int16_t * pSrc, int16_t fftLen, int16_t * pCoef);
        void applyWindow(int16_t *src, const uint16_t *window, uint16_t len);
        double dynamicScale(int32_t *source, int16_t *scaledSource);
        void fft2db();

    };

#ifdef WITH_MPL
// Barometric pressure and Altitude
class Sck_MPL3115A2
    {
        // Datasheet
        // https://cache.freescale.com/files/sensors/doc/data_sheet/MPL3115A2.pdf

    private:
        Adafruit_MPL3115A2 Adafruit_mpl3115A2 = Adafruit_MPL3115A2();

    public:
        const uint8_t address = 0x60;
        float altitude;
        float pressure;
        float temperature;
        bool start();
        bool stop();
        bool getAltitude();
        bool getPressure();
        bool getTemperature();
    };
#endif

#ifdef WITH_LPS33
// Barometric pressure and Altitude LPS33K
class Sck_LPS33
    {
        // Datasheet
        // https://www.st.com/resource/en/datasheet/lps33k.pdf

    private:
        Adafruit_LPS35HW Adafruit_lps35hw = Adafruit_LPS35HW();

    public:
        const uint8_t address = 0x5d;
        float pressure;
        float temperature;
        bool start();
        bool stop();
        bool getPressure();
        bool getTemperature();
    };
#endif

#ifdef WITH_PMS
//PM sensors
class Sck_PMS
    {
    private:
        uint32_t lastReading = 0;

        static const uint8_t buffLong = 31; // + Start_byte_1 = 32
        uint8_t buffer[buffLong];

        const byte PM_START_BYTE_1      = 0x42;
        const byte PM_START_BYTE_2      = 0x4d;

        const byte PM_CMD_GET_PASSIVE_READING   = 0xe2;
        const byte PM_CMD_CHANGE_MODE       = 0xe1;
        const byte PM_CMD_SLEEP_ACTION      = 0xe4;

        const byte PM_MODE_PASSIVE      = 0x00;
        const byte PM_MODE_ACTIVE       = 0x01;
        const byte PM_SLEEP             = 0x00;
        const byte PM_WAKEUP            = 0x01;

        // Serial transmission from PMS
        // 0: Start char 1 0x42 (fixed)
        // 1: Start char 2 0x4d (fixed)
        // 2-3 : Frame length = 2x13 + 2 (data + parity)

        // 4-5: PM1.0 concentration (CF = 1, standard particles) Unit ug/m^3
        // 6-7: PM2.5 concentration (CF = 1, standard particulates) Unit ug/m^3
        // 8-9: PM10 concentration (CF = 1, standard particulate matter) Unit ug/m^3

        // 10-11: PM1.0 concentration (in the atmosphere) Unit ug/m^3
        // 12-13: PM2.5 concentration (in the atmosphere) Unit ug/m^3
        // 14-15: PM10 concentration (in the atmosphere) Unit ug/m^3

        // 16-17: Particles in 0.1 liter of air > 0.3um
        // 18-19: Particles in 0.1 liter of air > 0.5um
        // 20-21: Particles in 0.1 liter of air > 1.0um
        // 22-23: Particles in 0.1 liter of air > 2.5um
        // 24-25: Particles in 0.1 liter of air > 5.0um
        // 26-27: Particles in 0.1 liter of air > 10um

        // 28: Version number
        // 29: Error code

        // 30-31: Sum of each byte from start_1 ... error_code

        RTCZero* rtc;
        bool started = false;
        bool alreadyFailed = false;
        uint32_t wakeUpTime = 0;
        uint8_t retries = 0;
        const uint8_t MAX_RETRIES = 3;
        bool oldSensor = false;

        bool fillBuffer();
        bool processBuffer();
        bool sendCmd(byte cmd, byte data=0x00, bool checkResponse=true);
        bool sleep();
        bool wake();

    public:
        Sck_PMS(RTCZero* myrtc) {
            rtc = myrtc;
        }

        // Readings
        uint16_t pm1;
        uint16_t pm25;
        uint16_t pm10;
        uint16_t pn03;
        uint16_t pn05;
        uint16_t pn1;
        uint16_t pn25;
        uint16_t pn5;
        uint16_t pn10;

        // Other data provided by the sensor
        uint8_t version = 0;
        uint8_t errorCode;  // Not documented on Datasheet

        // Default values for this are in config.h
        bool powerSave;     // If true the sensor will be turned off between readings.
        uint32_t warmUpPeriod;  // Wait this time (sec) before taking readings to let sensor stabilize

        bool start();
        bool stop();
        bool getReading(OneSensor *wichSensor, SckBase *base);
        bool debug = false;
        bool monitor = false;
    };
#endif

#ifdef WITH_CCS811
// VOC ans ECO2 - CCS811
class Sck_CCS811
    {
        // Datasheet https://ams.com/documents/20143/36005/CCS811_DS000459_7-00.pdf/3cfdaea5-b602-fe28-1a14-18776b61a35a
        // TODO review  the utility of baseline on datasheet and implement control interface if needed
        // TODO check consumption and quality in different drive modes: 1 sec [default], 10 sec, 60 sec or 0.25 sec (RAW mode)

    public:
        Sck_CCS811(RTCZero* myrtc) {
            rtc = myrtc;
        }

        const byte address = 0x5a;
        bool start();
        bool stop();
        bool getReading(SckBase *base);
        uint16_t getBaseline();
        bool setBaseline(uint16_t wichBaseline);
        bool setDriveMode(uint8_t wichDrivemode);

        //Mode 0 = Idle
        //Mode 1 = read every 1s
        //Mode 2 = every 10s
        //Mode 3 = every 60s
        //Mode 4 = RAW mode
        uint8_t driveMode = 3;

        bool debug = false;
        bool compensate = true;     // Compensation is for both sensors or none
        float VOCgas;
        float ECO2gas;
    private:
        uint32_t startTime = 0;
        uint32_t lastReadingMill = 0;
        const uint32_t warmingTime = 300;   // Minimal time for sensor stabilization in seconds(the kit will not return readings during this period) 5 minutes as default
        bool alreadyStarted = false;
        CCS811 ccs = CCS811(address);
        RTCZero* rtc;
    };
#endif

#ifdef WITH_SPS30
class Sck_SPS30
    {
        // TODO
        // implement average option and test it

    public:
        Sck_SPS30(RTCZero* myrtc) {
            rtc = myrtc;
        }

        const byte address = 0x69;
        bool start(SensorType wichSensor);
        bool stop(SensorType wichSensor);
        bool getReading(OneSensor* wichSensor);
        bool sleep();
        bool startCleaning();
        bool debug = true;
        bool monitor = false;

        struct lastCleaning { uint32_t time; bool valid=true; };

        // A struct of float to store pm readings
        struct sps30_measurement pm_readings;

    private:
        static const uint8_t totalMetrics = 10;
        uint8_t enabled[totalMetrics][2] = { {SENSOR_SPS30_PM_1, 0}, {SENSOR_SPS30_PM_25, 0}, {SENSOR_SPS30_PM_4, 0}, {SENSOR_SPS30_PM_10, 0},
            {SENSOR_SPS30_PN_05, 0}, {SENSOR_SPS30_PN_1, 0}, {SENSOR_SPS30_PN_25, 0}, {SENSOR_SPS30_PN_4, 0}, {SENSOR_SPS30_PN_10, 0},
            {SENSOR_SPS30_TPSIZE, 0} };

        enum SPS30State { SPS30_OFF, SPS30_IDLE, SPS30_SLEEP, SPS30_WARM_UP_1, SPS30_WARM_UP_2, SPS30_CLEANING, SPS30_NOT_DETECTED };
        SPS30State state = SPS30_OFF;


        uint32_t lastReading = 0;
        uint32_t measureStarted = 0;

        // Sensirion recommends taking a reading after 16 seconds, if the Particle Number reading is over 300#/cm3 the reading is OK, but if it is lower wait until 30 seconds and takeit again.
        // https://sensirion.com/media/documents/8600FF88/616542B5/Sensirion_PM_Sensors_Datasheet_SPS30.pdf
        const uint16_t warmUpPeriod[2] = { 16, 30 }; // Warm up period
        const uint16_t concentrationThreshold = 300;

        RTCZero* rtc;
        bool update(SensorType wichSensor);
        bool wake();
    };
#endif

#define ONE_WEEK_IN_SECONDS 604800
#ifdef WITH_SEN5X
class Sck_SEN5X
    {

    public:
        Sck_SEN5X(RTCZero* myrtc) {
            rtc = myrtc;
        }

        const byte address = 0x69;

        bool start(SensorType wichSensor);
        bool stop(SensorType wichSensor);
        bool getReading(OneSensor* wichSensor);
        bool idle();
        bool startCleaning();
        bool debug = false;
        bool monitor = false;
        bool continousMode = false;
        bool forcedContinousMode = false;

        bool getVer();
        float firmwareVer = -1;
        float hardwareVer = -1;
        float protocolVer = -1;

        // PM metrics
        float pM1p0;
        float pM2p5;
        float pM4p0;
        float pM10p0;
        float pN0p5;
        float pN1p0;
        float pN2p5;
        float pN4p0;
        float pN10p0;
        float tSize;

        // Other
        float humidity;
        float temperature;
        float vocIndex;
        float noxIndex;
        int16_t rawHumidity;
        int16_t rawTemperature;
        uint16_t rawVoc;
        uint16_t rawNox;

        struct lastCleaning { uint32_t time; bool valid=true; };

        #define SEN5X_VOC_STATE_BUFFER_SIZE 8
        uint8_t VOCstate[SEN5X_VOC_STATE_BUFFER_SIZE];
        struct VOCstateStruct { uint8_t state[SEN5X_VOC_STATE_BUFFER_SIZE]; uint32_t time; bool valid=true; };
        bool vocStateToEeprom();

        // Sensirion recommends taking a reading after 16 seconds, if the Perticle number reading is over 100#/cm3 the reading is OK, but if it is lower wait until 30 seconds and take it again.
        // https://sensirion.com/resource/application_note/low_power_mode/sen5x
        uint16_t warmUpPeriod[2] = { 16, 30 }; // Warm up period
        uint16_t concentrationThreshold = 100;

    private:

        // Commands
        #define SEN5X_RESET                        0xD304
        #define SEN5X_GET_PRODUCT_NAME             0xD014
        #define SEN5X_GET_FIRMWARE_VERSION         0xD100
        #define SEN5X_START_MEASUREMENT            0x0021
        #define SEN5X_START_MEASUREMENT_RHT_GAS    0x0037
        #define SEN5X_STOP_MEASUREMENT             0x0104
        #define SEN5X_READ_DATA_READY              0x0202
        #define SEN5X_START_FAN_CLEANING           0x5607
        #define SEN5X_RW_VOCS_STATE                0x6181

        #define SEN5X_READ_VALUES                  0x03C4
        #define SEN5X_READ_RAW_VALUES              0x03D2
        #define SEN5X_READ_PM_VALUES               0x0413


        enum SEN5Xmodel { SEN5X_UNKNOWN = 0, SEN50 = 0b001, SEN54 = 0b010, SEN55 = 0b100 };
        SEN5Xmodel model = SEN5X_UNKNOWN;

        static const uint8_t totalMetrics = 18;
        // Each metric has { SENSOR_TYPE, enabled/disabled, SUPPORTED_MODELS }
        // SUPPORTED_MODELS: b001:SEN50, b010:SEN54, b100:SEN55 and any combination
        uint8_t enabled[totalMetrics][3] = {
            {SENSOR_SEN5X_PM_1, 0, 0b111}, {SENSOR_SEN5X_PM_25, 0, 0b111}, {SENSOR_SEN5X_PM_4, 0, 0b111}, {SENSOR_SEN5X_PM_10, 0, 0b111},
            {SENSOR_SEN5X_PN_05, 0, 0b111}, {SENSOR_SEN5X_PN_1, 0, 0b111}, {SENSOR_SEN5X_PN_25, 0, 0b111}, {SENSOR_SEN5X_PN_4, 0, 0b111}, {SENSOR_SEN5X_PN_10, 0, 0b111}, {SENSOR_SEN5X_TPSIZE, 0, 0b111},
            {SENSOR_SEN5X_HUMIDITY, 0, 0b110}, {SENSOR_SEN5X_TEMPERATURE, 0, 0b110}, {SENSOR_SEN5X_VOCS_IDX, 0, 0b110}, {SENSOR_SEN5X_NOX_IDX, 0, 0b100},
            {SENSOR_SEN5X_HUMIDITY_RAW, 0, 0b110}, {SENSOR_SEN5X_TEMPERATURE_RAW, 0, 0b110}, {SENSOR_SEN5X_VOCS_RAW, 0, 0b110}, {SENSOR_SEN5X_NOX_RAW, 0, 0b100} };

        bool sensorNeedsContinousMode(SensorType wichSensor) {
            if (wichSensor == SENSOR_SEN5X_VOCS_RAW ||
                wichSensor == SENSOR_SEN5X_VOCS_IDX ||
                wichSensor == SENSOR_SEN5X_NOX_RAW  ||
                wichSensor == SENSOR_SEN5X_NOX_IDX) return true;
            return false;
        }

        enum SEN5XState { SEN5X_OFF, SEN5X_IDLE, SEN5X_MEASUREMENT, SEN5X_MEASUREMENT_2, SEN5X_CLEANING, SEN5X_NOT_DETECTED };
        SEN5XState state = SEN5X_OFF;

        uint32_t lastReading = 0;
        uint32_t measureStarted = 0;

        RTCZero* rtc;
        uint8_t update(SensorType wichSensor); // returns: 0: ok, 1: data is not yet ready, 2: error
        bool findModel();

        bool sen_sendCommand(uint16_t wichCommand);
        bool sen_sendCommand(uint16_t wichCommand, uint8_t* buffer, uint8_t byteNumber=0);
        uint8_t sen_readBuffer(uint8_t* buffer, uint8_t byteNumber); // Return number of bytes received
        uint8_t sen_CRC(uint8_t* buffer);
        bool sen_readValues();
        bool sen_readPmValues();
        bool sen_readRawValues();

        bool vocStateFromEeprom();
        bool vocStateToSensor();
        bool vocStateFromSensor();
    };
#endif

#ifdef WITH_BME68X
class Sck_BME68X
    {
    public:
        // Conntructor
        Sck_BME68X(byte customAddress=0x76);

        byte address;
        bool start();
        bool stop();
        bool getReading();

        float temperature;
        float humidity;
        float pressure;
        float VOCgas;
    private:
        Adafruit_BME680 _bme = Adafruit_BME680(&Wire);
        uint32_t lastTime = 0;
        const uint32_t minTime = 1000;    // Avoid taking readings more often than this value (ms)
        bool alreadyStarted = false;
    };
#endif

#ifdef WITH_AS7331
// UVA
class Sck_AS7331
    {
     public:
        const byte address = 0x74;

        bool start(SensorType wichSensor);
        bool stop(SensorType wichSensor);
        bool getReading(OneSensor* wichSensor);

        float uva;
        float uvb;
        float uvc;

        bool debug = false;

     private:
        // Datasheet: https://ams.com/documents/20143/9106314/AS7331_DS001047_4-00.pdf

        #define AS7331_OSR      0x00
        // OSR (0x00):  Page 49 of datasheet
        //      DOS     0:2 -> 00X: NOP (no change of DOS), 010: Operational state CONFIGURATION, 011: Operational state MEASUREMENT, 1XX: NOP (no change of DOS).
        //      SW_RES  3 -> 1: Software reset
        //      PD      6 -> 0: Power Down state switched OFF, 1: Power Down state switched ON.
        //      SS      7 -> 0: stop measurement, 1: Start of measurement ((only if DOS = MEASUREMENT)

        // This are the posible (useful) configurations for the OSR register
        #define AS7331_CONFIG               0b00000010
        #define AS7331_MEASUREMENT          0b00000011
        #define AS7331_START_MEASUREMENT    0b10000011
        #define AS7331_RESET                0b00001010
        #define AS7331_OFF                  0b01000010

        // Measurement Modes
        #define AS7331_CONT_MODE 0x00   // Continuous Measurement Mode – CONT
        #define AS7331_CMD_MODE  0x01   // Command Measurement Mode – CMD
        #define AS7331_SYNS_MODE 0x02   // Synchronous Measurement Mode – SYNS
        #define AS7331_SYND_MODE 0x03   // Synchronous Measurement Start and End Mode – SYND

        // Internal clock frequency
        #define AS7331_1024 0x00
        #define AS7331_2048 0x01
        #define AS7331_4096 0x02
        #define AS7331_8192 0x03

        #define AS7331_AGEN     0x02 // ChipIP info (it shoiuld be 0x21)

        #define AS7331_CREG1    0x06
        // CREG1 (0x07) page 52 of datasheet.
        //      TIME    3:0 -> Integration time from 0000: 2^10 (1024) to 1110 2^24 (16,384). The unit is number of clocks periods
        //      GAIN    7:4 -> 0000: 2048x, 0001: 1024x, ... 1011: 1x

        #define AS7331_CREG2    0x07
        // CREG2 (0x07) page 53
        //      DIV     2:0 -> Value of the divider, 000: 2^1, ... 111: 2^8
        //      EN_DIV  3   -> 0: Digital divider of the measurement result registers is disabled. 1: enabled
        //      EN_TM   6   -> 0: In combination with SYND mode, the internal measurement of the conversion time is disabled and no temperature measurement takes place.

        #define AS7331_CREG3    0x08
        // CREG3 (0x08) page 55
        //      CCLK    1:0 -> Internal clock frequency. 00:1024 Mhz, ... 11:8192 MHz
        //      RDYOD   3   -> 0: Pin READY operates as Push Pull output, 1:operates as Open Drain output
        //      SB      4   -> 0: Standby is switched OFF, 1: Standby is switched ON
        //      MMODE   7:6 -> 00 CONT mode (continuous measurement), 01 CMD mode (measurement per command). 10 SYNS mode (externally synchronized start of measurement), 11 SYND mode (start and end of measurement are externally synchronized).

        #define AS7331_BREAK    0x09
        // BREAK (0x09) page 56 of datasheet
        //      BREAK   7:0 -> reak time TBREAK between two measurements (except CMD mode): from 0 to 2040 μs, step size 8 μs. The value 0h results in a minimum time of 3 clocks of fCLK.

        #define AS7331_STATUS   0x00
        // STATUS (0x00) page 59 of datasheet
        //      POWERSTATE  0 -> Power Down state. 0: OFF, 1: ON
        //      STANDBYSTATE 1 -> 0: OFF, 1: ON
        //      NOTREADY    2 -> Corresponds to the inverted signal at the output pin READY. 0: Measurement progress is finished or not started yet
        //      NDATA       3 -> New measurement results were transferred from the temporary storage to the output result registers.
        //      LDATA       4 -> Measurement results in the buffer registers were overwritten before they were transferred to the output result registers. A transfer takes place as part of an I²C read process of at least one register of the output register bank.
        //      ADCOF       5 -> Overflow of at least one of the internal conversion channels during the measurement (e.g.caused by pulsed light) – analog evaluation is made
        //      MRESOF      6 -> Overflow of at least one of the measurement result registers MRES1 … MRES3.
        //      OUTCONVOF   7 -> Digital overflow of the internal 24 bit time reference OUTCONV.

        #define AS7331_TEMP      0x01
        #define AS7331_MRES1     0x02
        #define AS7331_MRES2     0x03
        #define AS7331_MRES3     0x04
        #define AS7331_OUTCONVL  0x05
        #define AS7331_OUTCONVH  0x06
        // Output Register Bank Page 58 of datasheet
        //      TEMP    1 -> Temperature Measurement Result (0h +12 bits for the value).
        //      MRES1   2 -> Measurement Result A-Channel.
        //      MRES2   3 -> Measurement Result B-Channel.
        //      MRES3   4 -> Measurement Result C-Channel
        //      OUTCONVL 5 -> Time reference, result of conversion time measurement (least significant byte and middle byte).
        //      OUTCONVH 6 -> Time reference, result of conversion time measurement (most significant byte and one empty byte with 00h).


        byte getByte(byte wichByte);
        byte writeByte(byte wichByte, byte wichValue);
        uint8_t readBytes(uint8_t wichReg, uint8_t howMany, uint8_t * buff);
        bool started = false;
        uint32_t lastReading = 0;

        struct SensorStatus { SensorType type; bool enabled; bool readed; };
        static const uint8_t totalMetrics = 3;
        SensorStatus sensorStatus[totalMetrics] = { {SENSOR_AS7331_UVA, false, true}, {SENSOR_AS7331_UVB, false, true}, {SENSOR_AS7331_UVC, false, true} };

        // Config values
        uint8_t mmode       = AS7331_CONT_MODE; // choices are modes are CONT, CMD, SYNS, SYND
        uint8_t sb          = 0x01;             // standby enabled 0x01 (to save power), standby disabled 0x00
        uint8_t cclk        = AS7331_1024;      // choices are 1.024, 2.048, 4.096, or 8.192 MHz
        uint8_t breakTime   = 40;               // sample time == 8 us x breakTime (0 - 255, or 0 - 2040 us range), CONT or SYNX modes
        uint8_t gain        = 8;                // ADCGain = 2^(11-gain), by 2s, 1 - 2048 range,  0 < gain = 11 max, default 10
        uint8_t time        = 9;                // 2^time in ms, so 0x07 is 2^6 = 64 ms, 0 < time = 15 max, default  6
    };
#endif
#endif

class SckUrban
    {
    private:
        RTCZero* rtc;
    public:
        SckUrban(RTCZero* myrtc) {
            rtc = myrtc;
        }

        bool start(SensorType wichSensor);
        bool stop(SensorType wichSensor);

        // String getReading(); https://stackoverflow.com/questions/14840173/c-same-function-parameters-with-different-return-type
        void getReading(SckBase *base, OneSensor *wichSensor);
        bool control(SckBase *base, SensorType wichSensor, String command);

#ifdef WITH_URBAN
        // Light
        Sck_BH1730FVC sck_bh1730fvc;

        // Temperature and Humidity
        Sck_SHT31 sck_sht31 = Sck_SHT31(&Wire);

        // Noise
        Sck_Noise sck_noise;

#ifdef WITH_MPL
        // Barometric pressure and Altitude
        Sck_MPL3115A2 sck_mpl3115A2;
#endif

#ifdef WITH_LPS33
        // Barometric pressure LPS
        Sck_LPS33 sck_lps33;
#endif

#ifdef WITH_CCS811
        // VOC and ECO2
        Sck_CCS811 sck_ccs811 = Sck_CCS811(rtc);
#endif

#ifdef WITH_PMS
        // PM sensor
        Sck_PMS sck_pms = Sck_PMS(rtc);
#endif

#ifdef WITH_SPS30
        // SPS30 PM sensor
        Sck_SPS30 sck_sps30 = Sck_SPS30(rtc);
#endif

#ifdef WITH_SEN5X
        // SEN5X PM, [temp, hum, vocs, nox] sensor
        Sck_SEN5X sck_sen5x = Sck_SEN5X(rtc);
#endif
#ifdef WITH_BME68X
        // BME68X, Temperature, Humidity, Barometric Pressure, Gases
        Sck_BME68X sck_bme68x = Sck_BME68X();
#endif
#ifdef WITH_AS7331
        // AMS7331 UVA, UVB, and UVC sensor
        Sck_AS7331 sck_as7331;
#endif
#endif
    };
