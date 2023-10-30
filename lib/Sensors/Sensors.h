#pragma once

#define WITH_PMS                // Saves 1504 bytes (this also disables auxiliary external PM sensor from pmboard)
#define WITH_URBAN              // Saves 6408 bytes (none of the urban board sensor will be available, it also disables external SHT3X)
//#define WITH_LPS33              //
#define WITH_MPL

// Auxiliary Sensors (ALl this sensors use around 8kb)
// #define WITH_SENSOR_GROVE_OLED  // Saves 2496 bytes
// #define WITH_GASES_BOARD        // Saves 756 bytes
// #define WITH_GROVE_I2C_ADC      // Saves 244 bytes
// #define WITH_INA219             // Saves 388 bytes
// #define WITH_DS18B20            // Saves 260 bytes
// #define WITH_ATLAS              // Saves 884 bytes
// #define WITH_CHIRP              // Saves 404 bytes
// #define WITH_DALLAS_TEMP        // Saves 292 bytes
// #define WITH_GPS                // Saves 1508 bytes
// #define WITH_RANGE              // Saves 292 bytes
// #define WITH_BME68X             // Saves 644 bytes
// #define WITH_ADS1X15            // Saves 1172 bytes
// #define WITH_SCD30              // Saves 500 bytes
// #define WITH_SFA30              // Saves 468 bytes

#include <Arduino.h>

enum SensorLocation
{
    BOARD_BASE,
    BOARD_URBAN,
    BOARD_AUX
};

enum SensorType
{

    // Base Sensors
    SENSOR_BATT_PERCENT,
    SENSOR_BATT_VOLTAGE,
    SENSOR_SDCARD,
    SENSOR_RSSI,

#ifdef WITH_URBAN

    // Urban Sensors
    SENSOR_LIGHT,
    SENSOR_TEMPERATURE,
    SENSOR_HUMIDITY,
    SENSOR_NOISE_DBA,
    SENSOR_NOISE_DBC,
    SENSOR_NOISE_DBZ,
    SENSOR_NOISE_FFT,

#ifdef WITH_MPL
    SENSOR_MPL_ALTITUDE,
    SENSOR_MPL_PRESSURE,
    SENSOR_MPL_TEMP,
#endif

#ifdef WITH_LPS33
    SENSOR_LPS33_PRESS,
    SENSOR_LPS33_TEMP,
#endif

#ifdef WITH_CCS811
    SENSOR_CCS811_VOCS,
    SENSOR_CCS811_ECO2,
#endif

#ifdef WITH_PMS
    SENSOR_PMS_PM_1,
    SENSOR_PMS_PM_25,
    SENSOR_PMS_PM_10,
    SENSOR_PMS_PN_03,
    SENSOR_PMS_PN_05,
    SENSOR_PMS_PN_1,
    SENSOR_PMS_PN_25,
    SENSOR_PMS_PN_5,
    SENSOR_PMS_PN_10,
#endif

#ifdef WITH_SPS30
    SENSOR_SPS30_PM_1,
    SENSOR_SPS30_PM_25,
    SENSOR_SPS30_PM_4,
    SENSOR_SPS30_PM_10,
    SENSOR_SPS30_PN_05,
    SENSOR_SPS30_PN_1,
    SENSOR_SPS30_PN_25,
    SENSOR_SPS30_PN_4,
    SENSOR_SPS30_PN_10,
    SENSOR_SPS30_TPSIZE,
#endif

#ifdef WITH_SEN5X
    SENSOR_SEN5X_PM_1,
    SENSOR_SEN5X_PM_25,
    SENSOR_SEN5X_PM_4,
    SENSOR_SEN5X_PM_10,
    SENSOR_SEN5X_PN_05,
    SENSOR_SEN5X_PN_1,
    SENSOR_SEN5X_PN_25,
    SENSOR_SEN5X_PN_4,
    SENSOR_SEN5X_PN_10,
    SENSOR_SEN5X_TPSIZE,
    SENSOR_SEN5X_HUMIDITY,
    SENSOR_SEN5X_TEMPERATURE,
    SENSOR_SEN5X_VOCS_IDX,
    SENSOR_SEN5X_NOX_IDX,
    SENSOR_SEN5X_HUMIDITY_RAW,
    SENSOR_SEN5X_TEMPERATURE_RAW,
    SENSOR_SEN5X_VOCS_RAW,
    SENSOR_SEN5X_NOX_RAW,
#endif

#ifdef WITH_BME68X
    SENSOR_BME68X_TEMPERATURE,
    SENSOR_BME68X_HUMIDITY,
    SENSOR_BME68X_PRESSURE,
    SENSOR_BME68X_VOCS,
#endif

#ifdef WITH_AS7331
    SENSOR_AS7331_UVA,
    SENSOR_AS7331_UVB,
    SENSOR_AS7331_UVC, 
#endif
#endif

#ifdef WITH_GASES_BOARD
    // I2C Auxiliary Sensors
    SENSOR_GASESBOARD_SLOT_1A,
    SENSOR_GASESBOARD_SLOT_1W,
    SENSOR_GASESBOARD_SLOT_2A,
    SENSOR_GASESBOARD_SLOT_2W,
    SENSOR_GASESBOARD_SLOT_3A,
    SENSOR_GASESBOARD_SLOT_3W,
    SENSOR_GASESBOARD_TEMPERATURE,
    SENSOR_GASESBOARD_HUMIDITY,
#endif

#ifdef WITH_GROVE_I2C_ADC
    SENSOR_GROOVE_I2C_ADC,
#endif

#ifdef WITH_INA219
    SENSOR_INA219_BUSVOLT,
    SENSOR_INA219_SHUNT,
    SENSOR_INA219_CURRENT,
    SENSOR_INA219_LOADVOLT,
#endif

#ifdef WITH_DS18B20
    SENSOR_WATER_TEMP_DS18B20,
#endif

#ifdef WITH_ATLAS
    SENSOR_ATLAS_TEMPERATURE,
    SENSOR_ATLAS_PH,
    SENSOR_ATLAS_EC,
    SENSOR_ATLAS_EC_TDS,
    SENSOR_ATLAS_EC_SAL,
    SENSOR_ATLAS_EC_SG,
    SENSOR_ATLAS_DO,
    SENSOR_ATLAS_DO_SAT,
    SENSOR_ATLAS_ORP,
#endif

#ifdef WITH_CHIRP
    SENSOR_CHIRP_MOISTURE_RAW,
    SENSOR_CHIRP_MOISTURE,
    SENSOR_CHIRP_TEMPERATURE,
    SENSOR_CHIRP_LIGHT,
#endif

#ifdef WITH_PMS
    SENSOR_EXT_A_PM_1,
    SENSOR_EXT_A_PM_25,
    SENSOR_EXT_A_PM_10,
    SENSOR_EXT_A_PN_03,
    SENSOR_EXT_A_PN_05,
    SENSOR_EXT_A_PN_1,
    SENSOR_EXT_A_PN_25,
    SENSOR_EXT_A_PN_5,
    SENSOR_EXT_A_PN_10,
    SENSOR_EXT_B_PM_1,
    SENSOR_EXT_B_PM_25,
    SENSOR_EXT_B_PM_10,
    SENSOR_EXT_B_PN_03,
    SENSOR_EXT_B_PN_05,
    SENSOR_EXT_B_PN_1,
    SENSOR_EXT_B_PN_25,
    SENSOR_EXT_B_PN_5,
    SENSOR_EXT_B_PN_10,
#endif

#ifdef WITH_DALLAS_TEMP
    SENSOR_PM_DALLAS_TEMP,
    SENSOR_DALLAS_TEMP,
#endif

#ifdef WITH_URBAN
    SENSOR_SHT31_TEMP,
    SENSOR_SHT31_HUM,
    SENSOR_SHT35_TEMP,
    SENSOR_SHT35_HUM,
#endif

#ifdef WTIH_RANGE
    SENSOR_RANGE_LIGHT,
    SENSOR_RANGE_DISTANCE,
#endif

#ifdef WITH_GPS
    SENSOR_GPS_FIX_QUALITY,
    SENSOR_GPS_LATITUDE,
    SENSOR_GPS_LONGITUDE,
    SENSOR_GPS_ALTITUDE,
    SENSOR_GPS_SPEED,
    SENSOR_GPS_HDOP,
    SENSOR_GPS_SATNUM,
#endif

#ifdef WITH_ADS1X15
    SENSOR_ADS1X15_48_0,
    SENSOR_ADS1X15_48_1,
    SENSOR_ADS1X15_48_2,
    SENSOR_ADS1X15_48_3,
    SENSOR_ADS1X15_49_0,
    SENSOR_ADS1X15_49_1,
    SENSOR_ADS1X15_49_2,
    SENSOR_ADS1X15_49_3,
    SENSOR_ADS1X15_4A_0,
    SENSOR_ADS1X15_4A_1,
    SENSOR_ADS1X15_4A_2,
    SENSOR_ADS1X15_4A_3,
    SENSOR_ADS1X15_4B_0,
    SENSOR_ADS1X15_4B_1,
    SENSOR_ADS1X15_4B_2,
    SENSOR_ADS1X15_4B_3,
#endif

#ifdef WITH_SCD30
    SENSOR_SCD30_CO2,
    SENSOR_SCD30_TEMP,
    SENSOR_SCD30_HUM,
#endif

#ifdef  WITH_SFA30
    SENSOR_SFA30_TEMPERATURE,
    SENSOR_SFA30_HUMIDITY,
    SENSOR_SFA30_FORMALDEHYDE,
#endif

    // Actuators (This is temp)
#ifdef WITH_SENSOR_GROVE_OLED
    SENSOR_GROVE_OLED,
#endif

    SENSOR_COUNT
};

class OneSensor
{
    public:
        SensorLocation location;
        uint8_t priority;       // 0-249 lower first. 250 is reserved for sensor we don't want to read (actuators, ej. oled screen)
        SensorType type;
        const char *shortTitle;
        const char *title;
        const char *unit;
        String reading;
        uint32_t lastReadingTime;
        uint8_t id;
        uint8_t everyNint;      // Read this sensor every N intervals (default 1)
        uint8_t defaultEveryNint;
        bool enabled;
        bool defaultEnabled;
        int16_t state;      // -1:error on reading, 0:reading OK, >0:number of seconds until the reading is OK
        bool oled_display;

        OneSensor(SensorLocation nLocation, uint8_t nPriority, SensorType nType, const char *nShortTitle, const char *nTitle, uint8_t nId=0, bool nEnabled=false, uint8_t nEveryNint=1, const char *nUnit="", bool nOled_display=true) {
            location = nLocation;
            priority = nPriority;       // 0-250, 0:Max priority -> 250:Min priority
            type = nType;
            shortTitle = nShortTitle;
            title = nTitle;
            unit = nUnit;
            reading = "null";
            lastReadingTime = 0;
            id = nId;
            everyNint = nEveryNint;
            defaultEveryNint = nEveryNint;
            enabled = nEnabled;
            defaultEnabled = nEnabled;
            state = -1;
            oled_display = nOled_display;
        }
};

class AllSensors
{
    public:


        OneSensor list[SENSOR_COUNT+1] {

            //  SensorLocation          prio    SensorType                      shortTitle          title                                   id      enabled     everyNintervals     unit

            // Base Sensors
            OneSensor { BOARD_BASE,     100,    SENSOR_BATT_PERCENT,            "BATT",             "Battery",                              10,     true,       1,  "%"         },
            OneSensor { BOARD_BASE,     100,    SENSOR_BATT_VOLTAGE,            "BATT_VOLT",        "Battery voltage",                      222,    false,      1,  "V"         },
            OneSensor { BOARD_BASE,     100,    SENSOR_SDCARD,                  "SDCARD",           "SD card",                              221,    true,       1,  "Present"   },
            OneSensor { BOARD_BASE,     100,    SENSOR_RSSI,                    "RSSI",             "WiFi RSSI",                            220,    true,       3,  "dBm"       },
            // WIiFi count

#ifdef WITH_URBAN
            // Urban Sensors
            OneSensor { BOARD_URBAN,    100,    SENSOR_LIGHT,                   "LIGHT",            "Light",                                14,     true,       1,  "Lux"       },
            OneSensor { BOARD_URBAN,    0,      SENSOR_TEMPERATURE,             "TEMP",             "Temperature",                          55,     true,       1,  "C"         },
            OneSensor { BOARD_URBAN,    0,      SENSOR_HUMIDITY,                "HUM",              "Humidity",                             56,     true,       1,  "%"         },
            OneSensor { BOARD_URBAN,    100,    SENSOR_NOISE_DBA,               "NOISE_A",          "Noise dBA",                            53,     true,       1,  "dBA"       },
            OneSensor { BOARD_URBAN,    100,    SENSOR_NOISE_DBC,               "NOISE_B",          "Noise dBC",                            217,    false,      1,  "dBC"       },
            OneSensor { BOARD_URBAN,    100,    SENSOR_NOISE_DBZ,               "NOISE_Z",          "Noise dBZ",                            218,    false,      1,  "dB"        },
            OneSensor { BOARD_URBAN,    100,    SENSOR_NOISE_FFT,               "NOISE_FFT",        "Noise FFT",                            0,      false,      1,              },

#ifdef WITH_MPL
            OneSensor { BOARD_URBAN,    100,    SENSOR_MPL_ALTITUDE,            "MPL_ALT",          "MPL Altitude",                         219,    false,      1,  "m"         },
            OneSensor { BOARD_URBAN,    100,    SENSOR_MPL_PRESSURE,            "MPL_PRESS",        "MPL Barometric pressure",              58,     true,       1,  "kPa"       },
            OneSensor { BOARD_URBAN,    100,    SENSOR_MPL_TEMP,                "MPL_TEMP",         "MPL internal temperature",             0,      false,      1,  "C"         },
#endif

#ifdef WITH_CCS811
            OneSensor { BOARD_URBAN,    100,    SENSOR_CCS811_VOCS,             "CCS811_VOCS",      "VOC Gas CCS811",                       113,    true,       1,  "ppb"       },
            OneSensor { BOARD_URBAN,    100,    SENSOR_CCS811_ECO2,             "CCS811_ECO2",      "eCO2 Gas CCS811",                      112,    true,       1,  "ppm"       },
#endif

#ifdef WITH_LPS33
            OneSensor { BOARD_URBAN,    100,    SENSOR_LPS33_PRESS,             "LPS33_PRESS",      "LPS33 Barometric pressure",            227,    true,       1,  "kPa"       },
            OneSensor { BOARD_URBAN,    100,    SENSOR_LPS33_TEMP,              "LPS33_TEMP",       "LPS33 internal temperature",           0,      false,      1,  "C"         },
#endif

#ifdef WITH_PMS
        // TODO cambiar los nombres a este pm para que no se confunda con los demas
            OneSensor { BOARD_URBAN,    240,    SENSOR_PMS_PM_1,                "PMS_PM_1",         "PMS PM 1.0",                           89,     true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    240,    SENSOR_PMS_PM_25,               "PMS_PM_25",        "PMS PM 2.5",                           87,     true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    240,    SENSOR_PMS_PM_10,               "PMS_PM_10",        "PMS PM 10.0",                          88,     true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    240,    SENSOR_PMS_PN_03,               "PMS_PN_03",        "PMS PN 0.3",                           165,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    240,    SENSOR_PMS_PN_05,               "PMS_PN_05",        "PMS PN 0.5",                           166,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    240,    SENSOR_PMS_PN_1,                "PMS_PN_1",         "PMS PN 1.0",                           167,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    240,    SENSOR_PMS_PN_25,               "PMS_PN_25",        "PMS PN 2.5",                           168,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    240,    SENSOR_PMS_PN_5,                "PMS_PN_5",         "PMS PN 5.0",                           169,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    240,    SENSOR_PMS_PN_10,               "PMS_PN_10",        "PMS PN 10.0",                          170,    true,       5,  "#/0.1l"    },
#endif

#ifdef WITH_SPS30
            OneSensor { BOARD_URBAN,    100,    SENSOR_SPS30_PM_1,              "SPS30_PM_1",       "SPS30 PM 1.0",                         182,    true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SPS30_PM_25,             "SPS30_PM_25",      "SPS30 PM 2.5",                         183,    true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SPS30_PM_4,              "SPS30_PM_4",       "SPS30 PM 4.0",                         184,    true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SPS30_PM_10,             "SPS30_PM_10",      "SPS30 PM 10.0",                        185,    true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SPS30_PN_05,             "SPS30_PN_05",      "SPS30 PN 0.5",                         186,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SPS30_PN_1,              "SPS30_PN_1",       "SPS30 PN 1.0",                         187,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SPS30_PN_25,             "SPS30_PN_25",      "SPS30 PN 2.5",                         188,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SPS30_PN_4,              "SPS30_PN_4",       "SPS30 PN 4.0",                         189,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SPS30_PN_10,             "SPS30_PN_10",      "SPS30 PN 10.0",                        190,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SPS30_TPSIZE,            "SPS30_TPSIZE",     "SPS30 Typical Particle Size",          191,    true,       5,  "um"        },
#endif

#ifdef WITH_SEN5X
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_PM_1,              "SEN5X_PM_1",       "SEN5X PM 1.0",                         193,    true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_PM_25,             "SEN5X_PM_25",      "SEN5X PM 2.5",                         194,    true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_PM_4,              "SEN5X_PM_4",       "SEN5X PM 4.0",                         195,    true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_PM_10,             "SEN5X_PM_10",      "SEN5X PM 10.0",                        196,    true,       5,  "ug/m3"     },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_PN_05,             "SEN5X_PN_05",      "SEN5X PN 0.5",                         197,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_PN_1,              "SEN5X_PN_1",       "SEN5X PN 1.0",                         198,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_PN_25,             "SEN5X_PN_25",      "SEN5X PN 2.5",                         199,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_PN_4,              "SEN5X_PN_4",       "SEN5X PN 4.0",                         200,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_PN_10,             "SEN5X_PN_10",      "SEN5X PN 10.0",                        201,    true,       5,  "#/0.1l"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_TPSIZE,            "SEN5X_TPSIZE",     "SEN5X Typical Particle Size",          202,    true,       5,  "um"        },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_HUMIDITY,          "SEN5X_HUM",        "SEN5X Humidity",                       203,    true,       5,  "%"         },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_TEMPERATURE,       "SEN5X_TEMP",       "SEN5X Temperature",                    204,    true,       5,  "C"         },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_VOCS_IDX,          "SEN5X_VOCS_IDX",   "SEN5X Vocs Index",                     205,    true,       5,              },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_NOX_IDX,           "SEN5X_NOX_IDX",    "SEN5X NOx Index",                      206,    true,       5,              },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_HUMIDITY_RAW,      "SEN5X_HUM_RAW",    "SEN5X Humidity Raw",                   0,      true,       5,              },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_TEMPERATURE_RAW,   "SEN5X_TEMP_RAW",   "SEN5X Temperature Raw",                0,      true,       5,              },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_VOCS_RAW,          "SEN5X_VOCS_RAW",   "SEN5X Vocs Raw",                       207,    true,       5,              },
            OneSensor { BOARD_URBAN,    100,    SENSOR_SEN5X_NOX_RAW,           "SEN5X_NOX_RAW",    "SEN5X NOx Raw",                        208,    true,       5,              },
#endif

#ifdef WITH_BME68X
            OneSensor { BOARD_URBAN,    0,      SENSOR_BME68X_TEMPERATURE,      "BME68X_TEMP",      "Temperature BME680",                   0,      true,       1,  "C"         },
            OneSensor { BOARD_URBAN,    0,      SENSOR_BME68X_HUMIDITY,         "BME68X_HUM",       "Humidity BME680",                      0,      true,       1,  "%"         },
            OneSensor { BOARD_URBAN,    100,    SENSOR_BME68X_PRESSURE,         "BME68X_PRESS",     "Barometric pressure BME680",           0,      true,       1,  "kPa"       },
            OneSensor { BOARD_URBAN,    100,    SENSOR_BME68X_VOCS,             "BME68X_VOCS",      "VOC Gas BME680",                       0,      true,       1,  "Ohms"      },
#endif

#ifdef WITH_AS7331
            OneSensor { BOARD_URBAN,    100,    SENSOR_AS7331_UVA,              "AS7331_UVA",       "AS7331 UVA",                           214,    true,       1,  "uW/cm2"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_AS7331_UVB,              "AS7331_UVB",       "AS7331 UVB",                           215,    true,       1,  "uW/cm2"    },
            OneSensor { BOARD_URBAN,    100,    SENSOR_AS7331_UVC,              "AS7331_UVA",       "AS7331 UVC",                           216,    true,       1,  "uW/cm2"    },
#endif
#endif // WITH_URBAN

            // I2C Auxiliary Sensors
#ifdef WITH_GASES_BOARD
            // SCK Gases Board for Alphasense (3 Gas sensor Slots, + SHT31 Temp-Humidity)
            OneSensor { BOARD_AUX,      100,    SENSOR_GASESBOARD_SLOT_1A,      "GB_1A",            "Gases Board 1A",                       65,     true,       1,  "mV"        },
            OneSensor { BOARD_AUX,      100,    SENSOR_GASESBOARD_SLOT_1W,      "GB_1W",            "Gases Board 1W",                       64,     true,       1,  "mV"        },
            OneSensor { BOARD_AUX,      100,    SENSOR_GASESBOARD_SLOT_2A,      "GB_2A",            "Gases Board 2A",                       62,     true,       1,  "mV"        },
            OneSensor { BOARD_AUX,      100,    SENSOR_GASESBOARD_SLOT_2W,      "GB_2W",            "Gases Board 2W",                       61,     true,       1,  "mV"        },
            OneSensor { BOARD_AUX,      100,    SENSOR_GASESBOARD_SLOT_3A,      "GB_3A",            "Gases Board 3A",                       68,     true,       1,  "mV"        },
            OneSensor { BOARD_AUX,      100,    SENSOR_GASESBOARD_SLOT_3W,      "GB_3W",            "Gases Board 3W",                       67,     true,       1,  "mV"        },
            OneSensor { BOARD_AUX,      100,    SENSOR_GASESBOARD_TEMPERATURE,  "GB_TEMP",          "Gases Board Temperature",              79,     true,       1,  "C"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_GASESBOARD_HUMIDITY,     "GB_HUM",           "Gases Board Humidity",                 80,     true,       1,  "%"         },
#endif

#ifdef WITH_GROVE_I2C_ADC
            // Groove I2C ADC
            OneSensor { BOARD_AUX,      100,    SENSOR_GROOVE_I2C_ADC,          "GR_ADC",           "Groove ADC",                           25,     true,       1,  "V"         },
#endif

#ifdef WITH_INA219
            // Adafruit INA291 High Side DC Current Sensor
            OneSensor { BOARD_AUX,      100,    SENSOR_INA219_BUSVOLT,          "INA_VBUS",         "INA219 Bus voltage",                   0,      true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_INA219_SHUNT,            "INA_VSHUNT",       "INA219 Shunt voltage",                 0,      true,       1,  "mV"        },
            OneSensor { BOARD_AUX,      100,    SENSOR_INA219_CURRENT,          "INA_CURR",         "INA219 Current",                       0,      true,       1,  "mA"        },
            OneSensor { BOARD_AUX,      100,    SENSOR_INA219_LOADVOLT,         "INA_VLOAD",        "INA219 Load voltage",                  0,      true,       1,  "V"         },
#endif

#ifdef WITH_DS18B20
            OneSensor { BOARD_AUX,      100,    SENSOR_WATER_TEMP_DS18B20,      "DS_WAT_TEMP",      "DS18B20 Water temperature",            42,     true,       1,  "C"         },
#endif

#ifdef WITH_ATLAS
            OneSensor { BOARD_AUX,      100,    SENSOR_ATLAS_TEMPERATURE,       "AS_TEMP",          "Atlas Temperature",                    51,     true,       1,  "C"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ATLAS_PH,                "AS_PH",            "Atlas PH",                             43,     true,       1,  "pH"        },
            OneSensor { BOARD_AUX,      100,    SENSOR_ATLAS_EC,                "AS_COND",          "Atlas Conductivity",                   45,     true,       1,  "uS/cm"     },
            OneSensor { BOARD_AUX,      100,    SENSOR_ATLAS_EC_TDS,            "AS_TDS",           "Atlas Total Dissolved Solids",         122,    true,       1,  "ppm"       },
            OneSensor { BOARD_AUX,      100,    SENSOR_ATLAS_EC_SAL,            "AS_SAL",           "Atlas Salinity",                       123,    true,       1,  "PSU(ppt)"  },
            OneSensor { BOARD_AUX,      100,    SENSOR_ATLAS_EC_SG,             "AS_SG",            "Atlas Specific gravity",               46,     true,       1,              },
            OneSensor { BOARD_AUX,      100,    SENSOR_ATLAS_DO,                "AS_DO",            "Atlas Dissolved Oxygen",               48,     true,       1,  "mg/L"      },
            OneSensor { BOARD_AUX,      100,    SENSOR_ATLAS_DO_SAT,            "AS_DO_SAT",        "Atlas DO Saturation",                  49,     true,       1,  "%"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ATLAS_ORP,               "AS_ORP",           "Atlas Redox potential",                164,    true,       1,  "mV"        },
#endif

#ifdef WITH_CHIRP
            // I2C Moisture Sensor (chirp)
            // https://github.com/Miceuz/i2c-moisture-sensor
            OneSensor { BOARD_AUX,      100,    SENSOR_CHIRP_MOISTURE_RAW,      "CHRP_MOIS_RAW",    "Soil Moisture Raw",                    0,      true,       1,              },
            OneSensor { BOARD_AUX,      100,    SENSOR_CHIRP_MOISTURE,          "CHRP_MOIS",        "Soil Moisture Percent",                50,     true,       1,  "%"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_CHIRP_TEMPERATURE,       "CHRP_TEMP",        "Soil Temperature",                     0,      true,       1,  "C"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_CHIRP_LIGHT,             "CHRP_LIGHT",       "Soil Light",                           0,      true,       1,              },
#endif

#ifdef WITH_PMS
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_A_PM_1,              "EXT_PM_A_1",       "Ext PM_A 1.0",                         71,     true,       1,  "ug/m3"     },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_A_PM_25,             "EXT_PM_A_25",      "Ext PM_A 2.5",                         72,     true,       1,  "ug/m3"     },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_A_PM_10,             "EXT_PM_A_10",      "Ext PM_A 10.0",                        73,     true,       1,  "ug/m3"     },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_A_PN_03,             "EXT_PN_A_03",      "Ext PN_A 0.3",                         99,     false,      1,  "#/0.1l"    },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_A_PN_05,             "EXT_PN_A_05",      "Ext PN_A 0.5",                         100,    false,      1,  "#/0.1l"    },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_A_PN_1,              "EXT_PN_A_1",       "Ext PN_A 1.0",                         101,    false,      1,  "#/0.1l"    },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_A_PN_25,             "EXT_PN_A_25",      "Ext PN_A 2.5",                         102,    false,      1,  "#/0.1l"    },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_A_PN_5,              "EXT_PN_A_5",       "Ext PN_A 5.0",                         103,    false,      1,  "#/0.1l"    },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_A_PN_10,             "EXT_PN_A_10",      "Ext PN_A 10.0",                        104,    false,      1,  "#/0.1l"    },

            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_B_PM_1,              "EXT_PM_B_1",       "Ext PM_B 1.0",                         75,     true,       1,  "ug/m3"     },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_B_PM_25,             "EXT_PM_B_25",      "Ext PM_B 2.5",                         76,     true,       1,  "ug/m3"     },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_B_PM_10,             "EXT_PM_B_10",      "Ext PM_B 10.0",                        77,     true,       1,  "ug/m3"     },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_B_PN_03,             "EXT_PN_B_03",      "Ext PN_B 0.3",                         105,    false,      1,  "#/0.1l"    },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_B_PN_05,             "EXT_PN_B_05",      "Ext PN_B 0.5",                         106,    false,      1,  "#/0.1l"    },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_B_PN_1,              "EXT_PN_B_1",       "Ext PN_B 1.0",                         107,    false,      1,  "#/0.1l"    },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_B_PN_25,             "EXT_PN_B_25",      "Ext PN_B 2.5",                         108,    false,      1,  "#/0.1l"    },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_B_PN_5,              "EXT_PN_B_5",       "Ext PN_B 5.0",                         109,    false,      1,  "#/0.1l"    },
            OneSensor { BOARD_AUX,      200,    SENSOR_EXT_B_PN_10,             "EXT_PN_B_10",      "Ext PN_B 10.0",                        110,    false,      1,  "#/0.1l"    },
#endif

#ifdef WITH_DALLAS_TEMP
            OneSensor { BOARD_AUX,      0,      SENSOR_PM_DALLAS_TEMP,          "PM_DALLAS_TEMP",   "PM board Dallas Temperature",          96,     true,       1,  "C"         },
            OneSensor { BOARD_AUX,      0,      SENSOR_DALLAS_TEMP,             "DALLAS_TEMP",      "Direct Dallas Temperature",            96,     true,       1,  "C"         },
#endif

#ifdef WITH_URBAN
            OneSensor { BOARD_AUX,      0,      SENSOR_SHT31_TEMP,              "EXT_TEMP",         "Ext Temperature",                      79,     true,       1,  "C"         },
            OneSensor { BOARD_AUX,      0,      SENSOR_SHT31_HUM,               "EXT_HUM",          "Ext Humidity",                         80,     true,       1,  "%"         },
            OneSensor { BOARD_AUX,      0,      SENSOR_SHT35_TEMP,              "EXT_TEMP_SHT35",   "Ext SHT35 Temperature",                224,    true,       1,  "C"         },
            OneSensor { BOARD_AUX,      0,      SENSOR_SHT35_HUM,               "EXT_HUM_SHT35",    "Ext SHT35 Humidity",                   225,    true,       1,  "%"         },
#endif

#ifdef WTIH_RANGE
            OneSensor { BOARD_AUX,      100,    SENSOR_RANGE_LIGHT,             "EXT_RANGE_LIGHT",  "Ext Range Light",                      0,      true,       1,  "Lux"       },
            OneSensor { BOARD_AUX,      100,    SENSOR_RANGE_DISTANCE,          "EXT_RANGE_DIST",   "Ext Range Distance",                   98,     true,       1,  "mm"        },
#endif

#ifdef WITH_GPS
            OneSensor { BOARD_AUX,      100,    SENSOR_GPS_FIX_QUALITY,         "GPS_FIX",          "GPS Fix Quality",                      128,    true,       1,              },
            OneSensor { BOARD_AUX,      100,    SENSOR_GPS_LATITUDE,            "GPS_LAT",          "GPS Latitude",                         125,    true,       1,  "Deg"       },
            OneSensor { BOARD_AUX,      100,    SENSOR_GPS_LONGITUDE,           "GPS_LONG",         "GPS Longitude",                        126,    true,       1,  "Deg"       },
            OneSensor { BOARD_AUX,      100,    SENSOR_GPS_ALTITUDE,            "GPS_ALT",          "GPS Altitude",                         127,    true,       1,  "m"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_GPS_SPEED,               "GPS_SPEED",        "GPS Speed",                            129,    true,       1,  "m/s"       },
            OneSensor { BOARD_AUX,      100,    SENSOR_GPS_HDOP,                "GPS_HDOP",         "GPS Horizontal Dilution of Position",  131,    true,       1,              },
            OneSensor { BOARD_AUX,      100,    SENSOR_GPS_SATNUM,              "GPS_SATNUM",       "GPS Traked Satellites",                130,    true,       1,              },
#endif

#ifdef WITH_ADS1X15
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_48_0,            "ADC_48_0",         "ADS1x15 ADC 0x48 Ch0",                 133,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_48_1,            "ADC_48_1",         "ADS1x15 ADC 0x48 Ch1",                 134,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_48_2,            "ADC_48_2",         "ADS1x15 ADC 0x48 Ch2",                 135,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_48_3,            "ADC_48_3",         "ADS1x15 ADC 0x48 Ch3",                 136,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_49_0,            "ADC_49_0",         "ADS1x15 ADC 0x49 Ch0",                 138,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_49_1,            "ADC_49_1",         "ADS1x15 ADC 0x49 Ch1",                 139,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_49_2,            "ADC_49_2",         "ADS1x15 ADC 0x49 Ch2",                 140,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_49_3,            "ADC_49_3",         "ADS1x15 ADC 0x49 Ch3",                 141,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_4A_0,            "ADC_4A_0",         "ADS1x15 ADC 0x4A Ch0",                 143,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_4A_1,            "ADC_4A_1",         "ADS1x15 ADC 0x4A Ch1",                 144,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_4A_2,            "ADC_4A_2",         "ADS1x15 ADC 0x4A Ch2",                 145,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_4A_3,            "ADC_4A_3",         "ADS1x15 ADC 0x4A Ch3",                 146,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_4B_0,            "ADC_4B_0",         "ADS1x15 ADC 0x4B Ch0",                 148,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_4B_1,            "ADC_4B_1",         "ADS1x15 ADC 0x4B Ch1",                 149,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_4B_2,            "ADC_4B_2",         "ADS1x15 ADC 0x4B Ch2",                 150,    true,       1,  "V"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_ADS1X15_4B_3,            "ADC_4B_3",         "ADS1x15 ADC 0x4B Ch3",                 151,    true,       1,  "V"         },
#endif

#ifdef WITH_SCD30
            OneSensor { BOARD_AUX,      100,    SENSOR_SCD30_CO2,               "SCD30_CO2",        "SCD30 CO2",                            158,    true,       1,  "ppm"       },
            OneSensor { BOARD_AUX,      100,    SENSOR_SCD30_TEMP,              "SCD30_TEMP",       "SCD30 Temperature",                    160,    true,       1,  "C"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_SCD30_HUM,               "SCD30_HUM",        "SCD30 Humidity",                       161,    true,       1,  "%"         },
#endif

#ifdef  WITH_SFA30
            OneSensor { BOARD_AUX,      100,    SENSOR_SFA30_TEMPERATURE,       "SFA30_TEMP",       "SFA30 Temperature",                    211,    true,       1,  "C"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_SFA30_HUMIDITY,          "SFA30_HUM",        "SFA30 Humidity",                       210,    true,       1,  "%"         },
            OneSensor { BOARD_AUX,      100,    SENSOR_SFA30_FORMALDEHYDE,      "SFA30_HCHO",       "SFA30 Formaldehyde",                   212,    true,       1,  "ppb"       },
#endif

#ifdef WITH_SENSOR_GROVE_OLED
            // Later this will be moved to a Actuators.h file
            // Groove I2C Oled Display 96x96
            OneSensor { BOARD_AUX,      250,    SENSOR_GROVE_OLED,              "GR_OLED",          "Groove OLED",                          0,      true,       1,              },
#endif
            OneSensor { BOARD_BASE,     0,      SENSOR_COUNT,                   "NOT_FOUND",        "Not found",                            0,      false,      1,              }

            // Add New Sensor Here!!!
        };

        OneSensor & operator[](SensorType type) {
            return list[type];
        }

        OneSensor ordered(uint8_t place);
        SensorType getTypeFromText(const char* input);
        uint8_t sensorNameEndsIn(const char* input);
        SensorType sensorsPriorized(uint8_t index);
    private:
        uint8_t countMatchedWords(SensorType whichSensor, const char* input);
        SensorType prioSortedList[SENSOR_COUNT+1];
        bool sorted = false;
};
