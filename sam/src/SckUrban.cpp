#include "SckUrban.h"
#include "SckBase.h"

// Hardware Serial UART PM
Uart SerialPM (&sercom5, pinPM_SERIAL_RX, pinPM_SERIAL_TX, SERCOM_RX_PAD_1, UART_TX_PAD_0);
void SERCOM5_Handler() {
    SerialPM.IrqHandler();
}


#ifdef WITH_URBAN
#ifdef WITH_SEN5X
// SEN5X flash space to save last cleaning date and VOC index algorithm state
FlashStorage(eepromSEN5xLastCleaning, Sck_SEN5X::lastCleaning);
FlashStorage(eepromSEN5xVOCstate, Sck_SEN5X::VOCstateStruct);
#endif
#ifdef WITH_SPS30
// SPS30 flash space to save last cleaning date
FlashStorage(eepromSPS30LastCleaning, Sck_SPS30::lastCleaning);
#endif
#endif

bool SckUrban::start(SensorType wichSensor)
{
    switch(wichSensor) {
#ifdef WITH_URBAN
        case SENSOR_LIGHT:              return sck_bh1730fvc.start();
        case SENSOR_TEMPERATURE:
        case SENSOR_HUMIDITY:           return sck_sht31.start();
        case SENSOR_NOISE_DBA:
        case SENSOR_NOISE_DBC:
        case SENSOR_NOISE_DBZ:
        case SENSOR_NOISE_FFT:          return sck_noise.start();
        case SENSOR_ALTITUDE:
        case SENSOR_PRESSURE:
        case SENSOR_PRESSURE_TEMP:      return sck_mpl3115A2.start();
        case SENSOR_LPS33_PRESS:
        case SENSOR_LPS33_TEMP:         return sck_lps33.start();
#ifdef WITH_CCS811
        case SENSOR_CCS811_VOCS:        return sck_ccs811.start();
        case SENSOR_CCS811_ECO2:        return sck_ccs811.start();
#endif
#ifdef WITH_PM
        case SENSOR_PM_1:
        case SENSOR_PM_25:
        case SENSOR_PM_10:
        case SENSOR_PN_03:
        case SENSOR_PN_05:
        case SENSOR_PN_1:
        case SENSOR_PN_25:
        case SENSOR_PN_5:
        case SENSOR_PN_10:              return sck_pm.start();
#endif
#ifdef WITH_SPS30
        case SENSOR_SPS30_PM_1:
        case SENSOR_SPS30_PM_25:
        case SENSOR_SPS30_PM_4:
        case SENSOR_SPS30_PM_10:
        case SENSOR_SPS30_PN_05:
        case SENSOR_SPS30_PN_1:
        case SENSOR_SPS30_PN_25:
        case SENSOR_SPS30_PN_4:
        case SENSOR_SPS30_PN_10:
        case SENSOR_SPS30_TPSIZE:        return sck_sps30.start(wichSensor);
#endif
#ifdef WITH_SEN5X
        case SENSOR_SEN5X_PM_1:
        case SENSOR_SEN5X_PM_25:
        case SENSOR_SEN5X_PM_4:
        case SENSOR_SEN5X_PM_10:
        case SENSOR_SEN5X_PN_05:
        case SENSOR_SEN5X_PN_1:
        case SENSOR_SEN5X_PN_25:
        case SENSOR_SEN5X_PN_4:
        case SENSOR_SEN5X_PN_10:
        case SENSOR_SEN5X_TPSIZE:
        case SENSOR_SEN5X_HUMIDITY:
        case SENSOR_SEN5X_TEMPERATURE:
        case SENSOR_SEN5X_VOCS_IDX:
        case SENSOR_SEN5X_NOX_IDX:
        case SENSOR_SEN5X_HUMIDITY_RAW:
        case SENSOR_SEN5X_TEMPERATURE_RAW:
        case SENSOR_SEN5X_VOCS_RAW:
        case SENSOR_SEN5X_NOX_RAW:      return sck_sen5x.start(wichSensor);
#endif
#ifdef WITH_BME68X
        case SENSOR_BME68X_TEMPERATURE:
        case SENSOR_BME68X_HUMIDITY:
        case SENSOR_BME68X_PRESSURE:
        case SENSOR_BME68X_VOCS:        return sck_bme68x.start();
#endif
#ifdef WITH_AS7331
        case SENSOR_AS7331_UVA:
        case SENSOR_AS7331_UVB:
        case SENSOR_AS7331_UVC:         return sck_as7331.start(wichSensor);
#endif
#endif
        default: break;
    }

    return false;
}
bool SckUrban::stop(SensorType wichSensor)
{
    switch(wichSensor) {
#ifdef WITH_URBAN
        case SENSOR_LIGHT:              return sck_bh1730fvc.stop();
        case SENSOR_TEMPERATURE:
        case SENSOR_HUMIDITY:           return sck_sht31.stop();
        case SENSOR_NOISE_DBA:
        case SENSOR_NOISE_DBC:
        case SENSOR_NOISE_DBZ:
        case SENSOR_NOISE_FFT:          return sck_noise.stop();
        case SENSOR_ALTITUDE:
        case SENSOR_PRESSURE:
        case SENSOR_PRESSURE_TEMP:      return sck_mpl3115A2.stop();
        case SENSOR_LPS33_PRESS:
        case SENSOR_LPS33_TEMP:         return sck_lps33.stop();
#ifdef WITH_CCS811
        case SENSOR_CCS811_VOCS:
        case SENSOR_CCS811_ECO2:        return sck_ccs811.stop();
#endif
#ifdef WITH_PM
        case SENSOR_PM_1:
        case SENSOR_PM_25:
        case SENSOR_PM_10:
        case SENSOR_PN_03:
        case SENSOR_PN_05:
        case SENSOR_PN_1:
        case SENSOR_PN_25:
        case SENSOR_PN_5:
        case SENSOR_PN_10:              return sck_pm.stop();
#endif
#ifdef WITH_SPS30
        case SENSOR_SPS30_PM_1:
        case SENSOR_SPS30_PM_25:
        case SENSOR_SPS30_PM_4:
        case SENSOR_SPS30_PM_10:
        case SENSOR_SPS30_PN_05:
        case SENSOR_SPS30_PN_1:
        case SENSOR_SPS30_PN_25:
        case SENSOR_SPS30_PN_4:
        case SENSOR_SPS30_PN_10:
        case SENSOR_SPS30_TPSIZE:       return sck_sps30.stop(wichSensor);
#endif
#ifdef WITH_SEN5X
        case SENSOR_SEN5X_PM_1:
        case SENSOR_SEN5X_PM_25:
        case SENSOR_SEN5X_PM_4:
        case SENSOR_SEN5X_PM_10:
        case SENSOR_SEN5X_PN_05:
        case SENSOR_SEN5X_PN_1:
        case SENSOR_SEN5X_PN_25:
        case SENSOR_SEN5X_PN_4:
        case SENSOR_SEN5X_PN_10:
        case SENSOR_SEN5X_TPSIZE:
        case SENSOR_SEN5X_HUMIDITY:
        case SENSOR_SEN5X_TEMPERATURE:
        case SENSOR_SEN5X_VOCS_IDX:
        case SENSOR_SEN5X_NOX_IDX:
        case SENSOR_SEN5X_HUMIDITY_RAW:
        case SENSOR_SEN5X_TEMPERATURE_RAW:
        case SENSOR_SEN5X_VOCS_RAW:
        case SENSOR_SEN5X_NOX_RAW:      return sck_sen5x.stop(wichSensor);
#endif
#ifdef WITH_BME68X
        case SENSOR_BME68X_TEMPERATURE:
        case SENSOR_BME68X_HUMIDITY:
        case SENSOR_BME68X_PRESSURE:
        case SENSOR_BME68X_VOCS:        return sck_bme68x.stop();
#endif
#ifdef WITH_AS7331
        case SENSOR_AS7331_UVA:
        case SENSOR_AS7331_UVB:
        case SENSOR_AS7331_UVC:         return sck_as7331.stop(wichSensor);
#endif
#endif
        default: break;
    }

    return false;
}

void SckUrban::getReading(SckBase *base, OneSensor *wichSensor)
{
    wichSensor->state = 0;
    switch(wichSensor->type) {
#ifdef WITH_URBAN
        case SENSOR_LIGHT:                  if (sck_bh1730fvc.get())                    { wichSensor->reading = String(sck_bh1730fvc.reading);                          return; } break;
        case SENSOR_TEMPERATURE:            if (sck_sht31.getReading())                 { wichSensor->reading = String(sck_sht31.temperature);                          return; } break;
        case SENSOR_HUMIDITY:               if (sck_sht31.getReading())                 { wichSensor->reading = String(sck_sht31.humidity);                             return; } break;
        case SENSOR_NOISE_DBA:              if (sck_noise.getReading(SENSOR_NOISE_DBA)) { wichSensor->reading = String(sck_noise.readingDB);                            return; } break;
        case SENSOR_NOISE_DBC:              if (sck_noise.getReading(SENSOR_NOISE_DBC)) { wichSensor->reading = String(sck_noise.readingDB);                            return; } break;
        case SENSOR_NOISE_DBZ:              if (sck_noise.getReading(SENSOR_NOISE_DBZ)) { wichSensor->reading = String(sck_noise.readingDB);                            return; } break;
        case SENSOR_NOISE_FFT:              if (sck_noise.getReading(SENSOR_NOISE_FFT)) {
            // TODO find a way to give access to readingsFFT instead of storing them on a String (too much RAM)
            // For now it just prints the values to console
            for (uint16_t i=1; i<sck_noise.FFT_NUM; i++) SerialUSB.println(sck_noise.readingFFT[i]);
            return;
        }
#ifdef WITH_CCS811
        case SENSOR_CCS811_VOCS:            if (sck_ccs811.getReading(base))            { wichSensor->reading = String(sck_ccs811.VOCgas);                              return; } break;
        case SENSOR_CCS811_ECO2:            if (sck_ccs811.getReading(base))            { wichSensor->reading = String(sck_ccs811.ECO2gas);                             return; } break;
#endif
        case SENSOR_ALTITUDE:               if (sck_mpl3115A2.getAltitude())            { wichSensor->reading = String(sck_mpl3115A2.altitude);                         return; } break;
        case SENSOR_PRESSURE:               if (sck_mpl3115A2.getPressure())            { wichSensor->reading = String(sck_mpl3115A2.pressure);                         return; } break;
        case SENSOR_PRESSURE_TEMP:          if (sck_mpl3115A2.getTemperature())         { wichSensor->reading = String(sck_mpl3115A2.temperature);                      return; } break;
        case SENSOR_LPS33_PRESS:            if (sck_lps33.getPressure())                { wichSensor->reading = String(sck_lps33.pressure);                             return; } break;
        case SENSOR_LPS33_TEMP:             if (sck_lps33.getTemperature())             { wichSensor->reading = String(sck_lps33.temperature);                          return; } break;
#ifdef WITH_PM
        case SENSOR_PM_1:                   if (sck_pm.getReading(wichSensor, base))    { wichSensor->reading = String(sck_pm.pm1);                                     return; } break;
        case SENSOR_PM_25:                  if (sck_pm.getReading(wichSensor, base))    { wichSensor->reading = String(sck_pm.pm25);                                    return; } break;
        case SENSOR_PM_10:                  if (sck_pm.getReading(wichSensor, base))    { wichSensor->reading = String(sck_pm.pm10);                                    return; } break;
        case SENSOR_PN_03:                  if (sck_pm.getReading(wichSensor, base))    { wichSensor->reading = String(sck_pm.pn03);                                    return; } break;
        case SENSOR_PN_05:                  if (sck_pm.getReading(wichSensor, base))    { wichSensor->reading = String(sck_pm.pn05);                                    return; } break;
        case SENSOR_PN_1:                   if (sck_pm.getReading(wichSensor, base))    { wichSensor->reading = String(sck_pm.pn1);                                     return; } break;
        case SENSOR_PN_25:                  if (sck_pm.getReading(wichSensor, base))    { wichSensor->reading = String(sck_pm.pn25);                                    return; } break;
        case SENSOR_PN_5:                   if (sck_pm.getReading(wichSensor, base))    { wichSensor->reading = String(sck_pm.pn5);                                     return; } break;
        case SENSOR_PN_10:                  if (sck_pm.getReading(wichSensor, base))    { wichSensor->reading = String(sck_pm.pn10);                                    return; } break;
#endif
#ifdef WITH_SPS30
        case SENSOR_SPS30_PM_1:             if (sck_sps30.getReading(wichSensor))       { wichSensor->reading = String(sck_sps30.pm_readings.mc_1p0);                   return; } break;
        case SENSOR_SPS30_PM_25:            if (sck_sps30.getReading(wichSensor))       { wichSensor->reading = String(sck_sps30.pm_readings.mc_2p5);                   return; } break;
        case SENSOR_SPS30_PM_4:             if (sck_sps30.getReading(wichSensor))       { wichSensor->reading = String(sck_sps30.pm_readings.mc_4p0);                   return; } break;
        case SENSOR_SPS30_PM_10:            if (sck_sps30.getReading(wichSensor))       { wichSensor->reading = String(sck_sps30.pm_readings.mc_10p0);                  return; } break;
        case SENSOR_SPS30_PN_05:            if (sck_sps30.getReading(wichSensor))       { wichSensor->reading = String(sck_sps30.pm_readings.nc_0p5);                   return; } break;
        case SENSOR_SPS30_PN_1:             if (sck_sps30.getReading(wichSensor))       { wichSensor->reading = String(sck_sps30.pm_readings.nc_1p0);                   return; } break;
        case SENSOR_SPS30_PN_25:            if (sck_sps30.getReading(wichSensor))       { wichSensor->reading = String(sck_sps30.pm_readings.nc_2p5);                   return; } break;
        case SENSOR_SPS30_PN_4:             if (sck_sps30.getReading(wichSensor))       { wichSensor->reading = String(sck_sps30.pm_readings.nc_4p0);                   return; } break;
        case SENSOR_SPS30_PN_10:            if (sck_sps30.getReading(wichSensor))       { wichSensor->reading = String(sck_sps30.pm_readings.nc_10p0);                  return; } break;
        case SENSOR_SPS30_TPSIZE:           if (sck_sps30.getReading(wichSensor))       { wichSensor->reading = String(sck_sps30.pm_readings.typical_particle_size);    return; } break;
#endif
#ifdef WITH_SEN5X
        case SENSOR_SEN5X_PM_1:             if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.pM1p0);                                return; } break;
        case SENSOR_SEN5X_PM_25:            if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.pM2p5);                                return; } break;
        case SENSOR_SEN5X_PM_4:             if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.pM4p0);                                return; } break;
        case SENSOR_SEN5X_PM_10:            if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.pM10p0);                               return; } break;
        case SENSOR_SEN5X_PN_05:            if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.pN0p5);                                return; } break;
        case SENSOR_SEN5X_PN_1:             if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.pN1p0);                                return; } break;
        case SENSOR_SEN5X_PN_25:            if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.pN2p5);                                return; } break;
        case SENSOR_SEN5X_PN_4:             if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.pN4p0);                                return; } break;
        case SENSOR_SEN5X_PN_10:            if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.pN10p0);                               return; } break;
        case SENSOR_SEN5X_TPSIZE:           if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.tSize);                                return; } break;
        case SENSOR_SEN5X_HUMIDITY:         if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.humidity);                             return; } break;
        case SENSOR_SEN5X_TEMPERATURE:      if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.temperature);                          return; } break;
        case SENSOR_SEN5X_VOCS_IDX:         if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.vocIndex);                             return; } break;
        case SENSOR_SEN5X_NOX_IDX:          if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.noxIndex);                             return; } break;
        case SENSOR_SEN5X_HUMIDITY_RAW:     if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.rawHumidity);                          return; } break;
        case SENSOR_SEN5X_TEMPERATURE_RAW:  if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.rawTemperature);                       return; } break;
        case SENSOR_SEN5X_VOCS_RAW:         if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.rawVoc);                               return; } break;
        case SENSOR_SEN5X_NOX_RAW:          if (sck_sen5x.getReading(wichSensor))       { wichSensor->reading = String(sck_sen5x.rawNox);                               return; } break;
#endif
#ifdef WITH_BME68X
        case SENSOR_BME68X_TEMPERATURE:     if (sck_bme68x.getReading())                { wichSensor->reading = String(sck_bme68x.temperature);                         return; } break;
        case SENSOR_BME68X_HUMIDITY:        if (sck_bme68x.getReading())                { wichSensor->reading = String(sck_bme68x.humidity);                            return; } break;
        case SENSOR_BME68X_PRESSURE:        if (sck_bme68x.getReading())                { wichSensor->reading = String(sck_bme68x.pressure);                            return; } break;
        case SENSOR_BME68X_VOCS:            if (sck_bme68x.getReading())                { wichSensor->reading = String(sck_bme68x.VOCgas);                              return; } break;
#endif
#ifdef WITH_AS7331
        case SENSOR_AS7331_UVA:             if (sck_as7331.getReading(wichSensor))      { wichSensor->reading = String(sck_as7331.uva);                                 return; } break;
        case SENSOR_AS7331_UVB:             if (sck_as7331.getReading(wichSensor))      { wichSensor->reading = String(sck_as7331.uvb);                                 return; } break;
        case SENSOR_AS7331_UVC:             if (sck_as7331.getReading(wichSensor))      { wichSensor->reading = String(sck_as7331.uvc);                                 return; } break;
#endif
#endif
        default: break;
    }
    wichSensor->state = -1;
}
bool SckUrban::control(SckBase *base, SensorType wichSensor, String command)
{
    switch (wichSensor) {
#ifdef WITH_URBAN
        case SENSOR_NOISE_DBA:
        case SENSOR_NOISE_DBC:
        case SENSOR_NOISE_DBZ:
        case SENSOR_NOISE_FFT:
            {
                if (command.startsWith("debug")) {
                    sck_noise.debugFlag = !sck_noise.debugFlag;
                    sprintf(base->outBuff, "Noise debug: %s", sck_noise.debugFlag  ? "true" : "false");
                    base->sckOut();
                    return true;
                }
            } 
#ifdef WITH_CCS811
        case SENSOR_CCS811_VOCS:
        case SENSOR_CCS811_ECO2:
            {
                if (command.startsWith("compensate")) {

                    sck_ccs811.compensate = !sck_ccs811.compensate;
                    return (sck_ccs811.compensate ? "True" : "False");

                } else if (command.startsWith("mode")) {

                    command.replace("mode", "");
                    command.trim();

                    if (command.length() == 0) {
                        sprintf(base->outBuff, "Current drive mode: %u", sck_ccs811.driveMode);
                        base->sckOut();
                        return "\r\n";
                    }

                    uint8_t newDriveMode = command.toInt();
                    if ((newDriveMode == 0 and !command.equals("0")) || newDriveMode > 4 || newDriveMode < 0) return F("Wrong drive mode value received, try again");

                    //Mode 0 = Idle
                    //Mode 1 = read every 1s
                    //Mode 2 = every 10s
                    //Mode 3 = every 60s
                    //Mode 4 = RAW mode
                    if (sck_ccs811.setDriveMode(newDriveMode) != CCS811Core::CCS811_Stat_SUCCESS) return F("Failed to set new drive mode");
                    else return String F("Drivemode set to ") + String(sck_ccs811.driveMode);

                } else if (command.startsWith("help") || command.length() == 0) {

                    sprintf(base->outBuff, "Available commands:\r\n* compensate (toggles temp/hum compensation)\r\n* mode [0-4] (0-idle, 1-1s, 2-10s, 3-60s, 4-raw)");
                    base->sckOut();
                    return "\r\n";
                }

            }
#endif
#ifdef WITH_PM
        case SENSOR_PM_1:
        case SENSOR_PM_25:
        case SENSOR_PM_10:
            {
                if (command.startsWith("debug")) {

                    command.replace("debug", "");
                    command.trim();
                    if (command.startsWith("1")) sck_pm.debug = true;
                    else if (command.startsWith("0")) sck_pm.debug = false;

                    sprintf(base->outBuff, "%s", sck_pm.debug  ? "true" : "false");
                    base->sckOut();
                    return true;

                } else if (command.startsWith("powersave")) {

                    bool oldValue = sck_pm.powerSave;
                    command.replace("powersave", "");
                    command.trim();
                    if (command.startsWith("1")) sck_pm.powerSave = true;
                    else if (command.startsWith("0")) sck_pm.powerSave = false;

                    sprintf(base->outBuff, "%s", sck_pm.powerSave  ? "true" : "false");
                    base->sckOut();

                    if (oldValue != sck_pm.powerSave) {
                        base->config.extra.pmPowerSave = sck_pm.powerSave;
                        base->saveConfig();
                    }
                    return true;

                } else if (command.startsWith("warmup")) {

                    uint32_t oldValue = sck_pm.warmUpPeriod;
                    command.replace("warmup", "");
                    command.trim();

                    // If user provide a value set new period
                    if (command.length() > 0) {
                        int newPeriod = command.toInt();
                        sck_pm.warmUpPeriod = newPeriod;
                    }

                    sprintf(base->outBuff, "%u seconds", sck_pm.warmUpPeriod);
                    base->sckOut();

                    if (oldValue != sck_pm.warmUpPeriod) {
                        base->config.extra.pmWarmUpPeriod = sck_pm.warmUpPeriod;
                        base->saveConfig();
                    }
                    return true;

                } else if (command.startsWith("help") || command.length() == 0) {

                    sprintf(base->outBuff, "Available commands:\r\n* powersave: [0-1] Sets powersave (turn off PMS after reading it)\r\n* warmup [seconds] Changes warm up period for sensor\r\n* debug [0-1] Sets debug messages");
                    base->sckOut();
                    return "\r\n";
                }
            }
#endif
#ifdef WITH_SPS30
        case SENSOR_SPS30_PM_1:
        case SENSOR_SPS30_PM_25:
        case SENSOR_SPS30_PM_4:
        case SENSOR_SPS30_PM_10:
        case SENSOR_SPS30_PN_05:
        case SENSOR_SPS30_PN_1:
        case SENSOR_SPS30_PN_25:
        case SENSOR_SPS30_PN_4:
        case SENSOR_SPS30_PN_10:
        case SENSOR_SPS30_TPSIZE:
            {
                if (command.startsWith("debug")) {

                    command.replace("debug", "");
                    command.trim();
                    if (command.startsWith("1")) sck_sps30.debug = true;
                    else if (command.startsWith("0")) sck_sps30.debug = false;

                    sprintf(base->outBuff, "Debug: %s", sck_sps30.debug  ? "true" : "false");
                    base->sckOut();
                    return true;

                } else if (command.startsWith("doclean")) {

                    base->sckOut("SPS30 cleaning started, it will take 10s...");
                    sck_sps30.startCleaning();
                    base->sckOut("Done!!!");
                    return "\r\n";

                } else if (command.startsWith("lastclean")) {

                    Sck_SPS30::lastCleaning when = eepromSPS30LastCleaning.read();
 
                    if (!when.valid) {
                        base->sckOut("No valid date for last cleaning");

                    } else {
                        base->epoch2iso(when.time, base->ISOtimeBuff);
                        sprintf(base->outBuff, "Last cleaning date: %s (UTC)", base->ISOtimeBuff);
                        base->sckOut();
                    }
                    return "\r\n";

                }  else if (command.startsWith("help") || command.length() == 0) {
                    sprintf(base->outBuff, "Available commands:\r\n* debug: [0-1] Sets debug messages\r\n* doclean: Starts a cleaning\r\n* lastClean\r\n");
                    base->sckOut();
                    return "\r\n";
                }
            }
#endif
#ifdef WITH_SEN5X
        case SENSOR_SEN5X_PM_1:
        case SENSOR_SEN5X_PM_25:
        case SENSOR_SEN5X_PM_4:
        case SENSOR_SEN5X_PM_10:
        case SENSOR_SEN5X_PN_05:
        case SENSOR_SEN5X_PN_1:
        case SENSOR_SEN5X_PN_25:
        case SENSOR_SEN5X_PN_4:
        case SENSOR_SEN5X_PN_10:
        case SENSOR_SEN5X_TPSIZE:
        case SENSOR_SEN5X_HUMIDITY:
        case SENSOR_SEN5X_TEMPERATURE:
        case SENSOR_SEN5X_VOCS_IDX:
        case SENSOR_SEN5X_NOX_IDX:
        case SENSOR_SEN5X_HUMIDITY_RAW:
        case SENSOR_SEN5X_TEMPERATURE_RAW:
        case SENSOR_SEN5X_VOCS_RAW:
        case SENSOR_SEN5X_NOX_RAW:
            {
                if (command.startsWith("debug")) {

                    command.replace("debug", "");
                    command.trim();
                    if (command.startsWith("1")) sck_sen5x.debug = true;
                    else if (command.startsWith("0")) sck_sen5x.debug = false;

                    sprintf(base->outBuff, "Debug: %s", sck_sen5x.debug  ? "true" : "false");
                    base->sckOut();
                    return true;

                } else if (command.startsWith("doclean")) {

                    sck_sen5x.startCleaning();
                    return "\r\n";

                } else if (command.startsWith("lastclean")) {

                    Sck_SEN5X::lastCleaning when = eepromSEN5xLastCleaning.read();
 
                    if (!when.valid) {
                        base->sckOut("No valid date for last cleaning");

                    } else {
                        base->epoch2iso(when.time, base->ISOtimeBuff);
                        sprintf(base->outBuff, "Last cleaning date: %s (UTC)", base->ISOtimeBuff);
                        base->sckOut();
                    }
                    return "\r\n";

                } else if (command.startsWith("doclean")) {

                } else if (command.startsWith("version")) {

                    if (sck_sen5x.getVer()) {
                        sprintf(base->outBuff, "SEN5X Firmware: %f\r\nSEN5X Hardware: %f\r\nSEN5X Protocol: %f", sck_sen5x.firmwareVer, sck_sen5x.hardwareVer, sck_sen5x.protocolVer);
                        base->sckOut();
					return "\r\n";
                    }

                }  else if (command.startsWith("help") || command.length() == 0) {
					sprintf(base->outBuff, "Available commands:\r\n* debug: [0-1] Sets debug messages\r\n* doclean: Starts a cleaning\r\n* lastClean\r\n* version");
					base->sckOut();
					return "\r\n";
                }

            }
#endif
#endif

        default: break;
    }

    base->sckOut("Sensor not recognized, or no control interface available for this sensor");
    return false;
}

#ifdef WITH_URBAN
// Light
bool Sck_BH1730FVC::start()
{
    if (!I2Cdetect(&Wire, address)) return false;
    return true;
}
bool Sck_BH1730FVC::stop()
{
    // 0x00 register - CONTROL
    uint8_t CONTROL = B000000;
    // ADC_INTR:    5   0:Interrupt is inactive.
    //          1:Interrupt is active.
    // ADC_VALID:   4   0:ADC data is not updated after last reading.
    //          1:ADC data is updated after last reading.
    // ONE_TIME:    3   0:ADC measurement is continuous.
    //          1:ADC measurement is one time.
    //          ADC transits to power down automatically.
    // DATA_SEL:    2   0:ADC measurement Type0 and Type1.
    //          1:ADC measurement Type0 only.
    // ADC_EN:  1   0:ADC measurement stop.
    //          1:ADC measurement start.
    // POWER:   0   0:ADC power down.
    //          1:ADC power on.

    // Send Configuration
    // This will save around 150 uA
    Wire.beginTransmission(address);
    Wire.write(0x80);
    Wire.write(CONTROL);
    Wire.endTransmission();

    return true;
}
bool Sck_BH1730FVC::updateValues()
{
    // Datasheet http://rohmfs.rohm.com/en/products/databook/datasheet/ic/sensor/light/bh1730fvc-e.pdf

    // 0x00 register - CONTROL
    uint8_t CONTROL = B000011;
    // ADC_INTR:    5   0:Interrupt is inactive.
    //          1:Interrupt is active.
    // ADC_VALID:   4   0:ADC data is not updated after last reading.
    //          1:ADC data is updated after last reading.
    // ONE_TIME:    3   0:ADC measurement is continuous.
    //          1:ADC measurement is one time.
    //          ADC transits to power down automatically.
    // DATA_SEL:    2   0:ADC measurement Type0 and Type1.
    //          1:ADC measurement Type0 only.
    // ADC_EN:  1   0:ADC measurement stop.
    //          1:ADC measurement start.
    // POWER:   0   0:ADC power down.
    //          1:ADC power on.

    // 0x01 register - TIMMING
    // uint8_t ITIME  = 0xDA;   // Datasheet default value (218 DEC)

    // 00h: Start / Stop of measurement is set by special command. (ADC manual integration mode)
    // 01h to FFh: Integration time is determined by ITIME value (defaultt is oxDA)
    // Integration Time : ITIME_ms = Tint * 964 * (256 - ITIME)
    // Measurement time : Tmt= ITIME_ms + Tint * 714

    // 0x02 register - INTERRUPT
    uint8_t INTERRUPT = B00000000;
    // RES      7   Write 0
    // INT_STOP 6   0 : ADC measurement does not stop.
    //              1 : ADC measurement stops and transits to power down mode when interrupt becomes active.
    // RES      5   Write 0
    // INT_EN   4   0 : Disable interrupt function.
    //              1 : Enable interrupt function.
    // PERSIST  3:0 Interrupt persistence function.
    //  0000 : Interrupt becomes active at each measurement end.
    //  0001 : Interrupt status is updated at each measurement end.
    //  0010 : Interrupt status is updated if two consecutive threshold judgments are the same.
    //  When  set  0011  or  more,  interrupt  status  is  updated  if same threshold judgments continue consecutively same times as the number set in PERSIST.

    // 0x03, 0x04 registers - TH_LOW Low interrupt threshold
    uint8_t TH_LOW0 = 0x00;     // Lower byte of low interrupt threshold
    uint8_t TH_LOW1 = 0x00;     // Upper byte of low interrupt threshold

    // 0x05, 0x06 registers - TH_UP High interrupt threshold
    uint8_t TH_UP0 = 0xFF;      // Lower byte of high interrupt threshold
    uint8_t TH_UP1 = 0xFF;      // Upper byte of high interrupt threshold

    // 0x07 - GAIN
    uint8_t GAIN = 0x00;
    // GAIN  2:0    ADC resolution setting
    //    X00 : x1 gain mode
    //    X01 : x2 gain mode
    //    X10 : x64 gain mode
    //    X11 : x128 gain mode

    uint8_t DATA[8] = {CONTROL, ITIME, INTERRUPT, TH_LOW0, TH_LOW1, TH_UP0, TH_UP1, GAIN};

    // Send Configuration
    Wire.beginTransmission(address);
    Wire.write(0x80);
    for (int i= 0; i<8; i++) Wire.write(DATA[i]);
    Wire.endTransmission();

    // Calculate timming values
    ITIME_ms = (Tint * 964 * (256 - ITIME)) / 1000;
    Tmt =  ITIME_ms + (Tint * 714);

    // Wait for ADC to finish
    uint32_t started = millis();
    uint8_t answer = 0;
    while ((answer & 0x10) == 0) {
        delay(10);
        Wire.beginTransmission(address);
        Wire.write(0x80);
        Wire.endTransmission();
        Wire.requestFrom(address, 1);
        answer = Wire.read();
        if (millis() - started > Tmt) {
            if (debug) SerialUSB.println("ERROR: Timeout waiting for reading");
            return false;
        }
    }

    // Ask for reading
    Wire.beginTransmission(address);
    Wire.write(0x94);
    Wire.endTransmission();
    Wire.requestFrom(address, 4);

    // Get result
    uint16_t IDATA0 = 0;
    uint16_t IDATA1 = 0;
    IDATA0 = Wire.read();
    IDATA0 = IDATA0 | (Wire.read()<<8);
    IDATA1 = Wire.read();
    IDATA1 = IDATA1 | (Wire.read()<<8);
    DATA0 = (float)IDATA0;
    DATA1 = (float)IDATA1;

    // Setup gain
    Gain = 1;
    if (GAIN == 0x01) Gain = 2;
    else if (GAIN == 0x02) Gain = 64;
    else if (GAIN == 0x03) Gain = 128;

    return true;
}
bool Sck_BH1730FVC::get()
{
    // Start in the default integration time
    ITIME = 218;

    if (!updateValues()) return false;

    // Adjust the Integration Time (ITIME)
    for (uint8_t i=0; i<6; i++) {

        if (DATA0 > goUp || DATA1 > goUp) {
            ITIME += (((ITIME_max - ITIME) / 2) + 1);
            if (ITIME > 250) ITIME = ITIME_max;

            if (debug) {
                SerialUSB.print(DATA0);
                SerialUSB.print(" -- ");
                SerialUSB.print(DATA1);
                SerialUSB.print(" >> ");
                SerialUSB.println(ITIME);
            }
        } else break;

        if (!updateValues()) return false;
    }

    // Lux calculation (Datasheet page 13)
    float Lx = 0;
    if (DATA0 > 0 && DATA1 > 0) {
        if (DATA1/DATA0 < 0.26) Lx = ((1.290 * DATA0 - 2.733 * DATA1) / Gain) * (102.6 / ITIME_ms);
        else if (DATA1/DATA0 < 0.55) Lx = ((0.795 * DATA0 - 0.859 * DATA1) / Gain) * (102.6 / ITIME_ms);
        else if (DATA1/DATA0 < 1.09) Lx = ((0.510 * DATA0 - 0.345 * DATA1) / Gain) * (102.6 / ITIME_ms);
        else if (DATA1/DATA0 < 2.13) Lx = ((0.276 * DATA0 - 0.130 * DATA1) / Gain) * (102.6 / ITIME_ms);
        else Lx = 0;
    }

    Lx = max(0, Lx);
    reading  = (int)Lx;

    if (debug) {
        SerialUSB.print("Integration Time ITIME_ms: ");
        SerialUSB.println(ITIME_ms);
        SerialUSB.print("Measurement Time Tmt: ");
        SerialUSB.println(Tmt);
        SerialUSB.print("Gain: ");
        SerialUSB.println(Gain);
        SerialUSB.print("Visible Light DATA0: ");
        SerialUSB.println(DATA0);
        SerialUSB.print("Infrared Light DATA1: ");
        SerialUSB.println(DATA1);
        SerialUSB.print("Calculated Lux: ");
        SerialUSB.println(Lx);
    }

    stop();

    return true;
}

// SHT31 (Temperature and Humidity)
// TODO Implement calibration routine
// TODO implement heater controller
Sck_SHT31::Sck_SHT31(TwoWire *localWire, uint8_t customAddress)
{
    _Wire = localWire;
    address = customAddress;
}
bool Sck_SHT31::start()
{
    _Wire->begin();
    if (!I2Cdetect(_Wire, address)) return false;

    delay(1);       // In case the device was off
    sendComm(SOFT_RESET);   // Send reset command
    delay(50);      // Give time to finish reset
    update();

    return true;
}
bool Sck_SHT31::stop()
{

    // It will go to idle state by itself after 1ms
    return true;
}
bool Sck_SHT31::update()
{
    uint8_t readbuffer[6];
    if (!sendComm(SINGLE_SHOT_HIGH_REP)) return false;

    uint32_t started = millis();
    while (_Wire->requestFrom(address, 6) < 6) {
        if (millis() - started > timeout) {
            if (debug) SerialUSB.println("ERROR: Timed out waiting for SHT31 sensor!!!");
            return false;
        }
    }

    // Read response
    if (debug) SerialUSB.print("Response: ");
    for (uint8_t i=0; i<6; i++) {
        readbuffer[i] = _Wire->read();
        if (debug) SerialUSB.print(readbuffer[i]);
    }
    if (debug) SerialUSB.println();

    uint16_t ST, SRH;
    ST = readbuffer[0];
    ST <<= 8;
    ST |= readbuffer[1];

    // Check Temperature crc
    if (readbuffer[2] != crc8(readbuffer, 2)) {
        if (debug) SerialUSB.println("ERROR: Temperature reading CRC failed!!!");
        return false;
    }
    SRH = readbuffer[3];
    SRH <<= 8;
    SRH |= readbuffer[4];

    // check Humidity crc
    if (readbuffer[5] != crc8(readbuffer+3, 2)) {
        if (debug) SerialUSB.println("ERROR: Humidity reading CRC failed!!!");
        return false;
    }

    double temp = ST;
    temp *= 175;
    temp /= 0xffff;
    temp = -45 + temp;
    temperature = (float)temp;

    double shum = SRH;
    shum *= 100;
    shum /= 0xFFFF;
    humidity = (float)shum;

    return true;
}
bool Sck_SHT31::sendComm(uint16_t comm)
{
    _Wire->beginTransmission(address);
    _Wire->write(comm >> 8);
    _Wire->write(comm & 0xFF);
    if (_Wire->endTransmission() != 0) return false;

    return true;
}
uint8_t Sck_SHT31::crc8(const uint8_t *data, int len)
{

    /* CRC-8 formula from page 14 of SHT spec pdf */

    /* Test data 0xBE, 0xEF should yield 0x92 */

    /* Initialization data 0xFF */
    /* Polynomial 0x31 (x8 + x5 +x4 +1) */
    /* Final XOR 0x00 */

    const uint8_t POLYNOMIAL(0x31);
    uint8_t crc(0xFF);

    for ( int j = len; j; --j ) {
        crc ^= *data++;
        for ( int i = 8; i; --i ) {
            crc = ( crc & 0x80 )
                ? (crc << 1) ^ POLYNOMIAL
                : (crc << 1);
        }
    }
    return crc;
}
bool Sck_SHT31::getReading()
{
    uint8_t tried = retrys;
    while (tried > 0) {
        if (update()) return true;
        tried--;
    }

    return false;
}

// Noise
bool Sck_Noise::start()
{
    if (alreadyStarted) return true;

    REG_GCLK_GENCTRL = GCLK_GENCTRL_ID(4);  // Select GCLK4
    while (GCLK->STATUS.bit.SYNCBUSY);

    if (!getReading(SENSOR_NOISE_DBA)) return false;

    alreadyStarted = true;
    return true;
}
bool Sck_Noise::stop()
{
    return true;
}
bool Sck_Noise::getReading(SensorType wichSensor)
{
    if (!I2S.begin(I2S_PHILIPS_MODE, sampleRate, 32)) return false;

    // Wait 263000 I2s cycles or 85 ms at 441000 hz
    uint32_t startPoint = millis();
    while (millis() - startPoint < 200) I2S.read();

    // Fill buffer with samples from I2S bus
    uint16_t bufferIndex = 0;

    startPoint = millis();
    uint8_t timeOut = 30;   // (ms) Timeout to avoid hangs if the I2S is not responfing
    while (bufferIndex < SAMPLE_NUM) {
        int32_t buff = I2S.read();
        if (buff) {
            source[bufferIndex] = buff>>7;
            bufferIndex ++;
        }

        if (millis() - startPoint > timeOut) {
            I2S.end();
            return false;
        }
    }
    I2S.end();

    // Get de average of recorded samples
    int32_t sum = 0;
    for (uint16_t i=0; i<SAMPLE_NUM; i++) sum += source[i];
    int32_t avg = sum / SAMPLE_NUM;

    // Center samples in zero
    for (uint16_t i=0; i<SAMPLE_NUM; i++) source[i] = source[i] - avg;

    // FFT
    FFT(source);

    switch(wichSensor) {

        case SENSOR_NOISE_DBA:
            // Equalization and A weighting
            for (uint16_t i=0; i<FFT_NUM; i++) readingFFT[i] *= (double)(equalWeight_A[i] / 65536.0);
            break;
        case SENSOR_NOISE_DBC:
            // Equlization and C weighting
            for (uint16_t i=0; i<FFT_NUM; i++) readingFFT[i] *= (double)(equalWeight_C[i] / 65536.0);
            break;
        case SENSOR_NOISE_DBZ:
            // Just Equalization
            for (uint16_t i=0; i<FFT_NUM; i++) readingFFT[i] *= (double)(equalTab[i] / 65536.0);
            break;
        case SENSOR_NOISE_FFT:
            // Convert FFT to dB
            fft2db();
            return true;
            break;
        default: break;
    }

    // RMS
    long long rmsSum = 0;
    double rmsOut = 0;
    for (uint16_t i=0; i<FFT_NUM; i++) rmsSum += pow(readingFFT[i], 2) / FFT_NUM;
    rmsOut = sqrt(rmsSum);
    rmsOut = rmsOut * 1 / RMS_HANN * sqrt(FFT_NUM) / sqrt(2);

    // Convert to dB
    readingDB = (double) (FULL_SCALE_DBSPL - (FULL_SCALE_DBFS - (20 * log10(rmsOut * sqrt(2)))));

    if (debugFlag) {
        SerialUSB.println("samples, FFT_weighted");
        for (uint16_t i=0; i<SAMPLE_NUM; i++) {
            SerialUSB.print(source[i]);
            SerialUSB.print(",");
            if (i < 256) SerialUSB.println(readingFFT[i]);
            else SerialUSB.println();
        }
    }

    return true;
}
bool Sck_Noise::FFT(int32_t *source)
{
    double divider = dynamicScale(source, scaledSource);

    applyWindow(scaledSource, hannWindow, SAMPLE_NUM);

    static int16_t ALIGN4 scratchData[SAMPLE_NUM * 2];

    // Split the data
    for(int i=0; i<SAMPLE_NUM*2; i+=2){
        scratchData[i] = scaledSource[i/2]; // Real
        scratchData[i+1] = 0; // Imaginary
    }

    arm_radix2_butterfly(scratchData, (int16_t)SAMPLE_NUM, (int16_t *)twiddleCoefQ15_512);
    arm_bitreversal(scratchData, SAMPLE_NUM, (uint16_t *)armBitRevTable8);

    for (int i=0; i<SAMPLE_NUM/2; i++) {

        // Calculate result and normalize SpectrumBuffer, also revert dynamic scaling
        uint32_t myReal = pow(scratchData[i*2], 2);
        uint32_t myImg = pow(scratchData[(i*2)+1], 2);

        readingFFT[i] = sqrt(myReal + myImg) * divider * 4;
    }

    // Exception for the first bin
    readingFFT[0] = readingFFT[0] / 2;

    return 0;
}
double Sck_Noise::dynamicScale(int32_t *source, int16_t *scaledSource)
{
    int32_t maxLevel = 0;
    for (uint16_t i=0; i<SAMPLE_NUM; i++) if (abs(source[i]) > maxLevel) maxLevel = abs(source[i]);
    double divider = (maxLevel+1) / 32768.0; // 16 bits
    if (divider < 1) divider = 1;

    for (uint16_t i=0; i<SAMPLE_NUM; i++) scaledSource[i] = source[i] / divider;

    return divider;
}
void Sck_Noise::applyWindow(int16_t *src, const uint16_t *window, uint16_t len)
{
    /* This code is from https://github.com/adafruit/Adafruit_ZeroFFT thank you!
        -------
        This is an FFT library for ARM cortex M0+ CPUs
        Adafruit invests time and resources providing this open source code,
        please support Adafruit and open-source hardware by purchasing products from Adafruit!
        Written by Dean Miller for Adafruit Industries. MIT license, all text above must be included in any redistribution
        ------
    */

    while(len--){
        int32_t val = *src * *window++;
        *src = val >> 15;
        src++;
    }
}
void Sck_Noise::arm_radix2_butterfly(int16_t * pSrc, int16_t fftLen, int16_t * pCoef)
{
    /* This code is from https://github.com/adafruit/Adafruit_ZeroFFT thank you!
        -------
        This is an FFT library for ARM cortex M0+ CPUs
        Adafruit invests time and resources providing this open source code,
        please support Adafruit and open-source hardware by purchasing products from Adafruit!
        Written by Dean Miller for Adafruit Industries. MIT license, all text above must be included in any redistribution
        ------
    */

    int i, j, k, l;
    int n1, n2, ia;
    int16_t xt, yt, cosVal, sinVal;

    n2 = fftLen;

    n1 = n2;
    n2 = n2 >> 1;
    ia = 0;

    // loop for groups
    for (j=0; j<n2; j++) {
        cosVal = pCoef[ia * 2];
        sinVal = pCoef[(ia * 2) + 1];
        ia++;

        // loop for butterfly
        for (i=j; i<fftLen; i+=n1) {
            l = i + n2;
            xt = (pSrc[2 * i] >> 2u) - (pSrc[2 * l] >> 2u);
            pSrc[2 * i] = ((pSrc[2 * i] >> 2u) + (pSrc[2 * l] >> 2u)) >> 1u;

            yt = (pSrc[2 * i + 1] >> 2u) - (pSrc[2 * l + 1] >> 2u);
            pSrc[2 * i + 1] =
                ((pSrc[2 * l + 1] >> 2u) + (pSrc[2 * i + 1] >> 2u)) >> 1u;

            pSrc[2u * l] = (((int16_t) (((int32_t) xt * cosVal) >> 16)) +
                ((int16_t) (((int32_t) yt * sinVal) >> 16)));

            pSrc[2u * l + 1u] = (((int16_t) (((int32_t) yt * cosVal) >> 16)) -
                ((int16_t) (((int32_t) xt * sinVal) >> 16)));

        }                           // butterfly loop end
    }                             // groups loop end

    uint16_t twidCoefModifier = 2;

    // loop for stage
    for (k = fftLen / 2; k > 2; k = k >> 1) {
        n1 = n2;
        n2 = n2 >> 1;
        ia = 0;

        // loop for groups
        for (j=0; j<n2; j++) {
            cosVal = pCoef[ia * 2];
            sinVal = pCoef[(ia * 2) + 1];

            ia = ia + twidCoefModifier;

            // loop for butterfly
            for (i=j; i<fftLen; i+=n1) {
                l = i + n2;
                xt = pSrc[2 * i] - pSrc[2 * l];
                pSrc[2 * i] = (pSrc[2 * i] + pSrc[2 * l]) >> 1u;

                yt = pSrc[2 * i + 1] - pSrc[2 * l + 1];
                pSrc[2 * i + 1] = (pSrc[2 * l + 1] + pSrc[2 * i + 1]) >> 1u;

                pSrc[2u * l] = (((int16_t) (((int32_t) xt * cosVal) >> 16)) +
                    ((int16_t) (((int32_t) yt * sinVal) >> 16)));

                pSrc[2u * l + 1u] = (((int16_t) (((int32_t) yt * cosVal) >> 16)) -
                    ((int16_t) (((int32_t) xt * sinVal) >> 16)));

            }                         // butterfly loop end
        }                           // groups loop end
        twidCoefModifier = twidCoefModifier << 1u;
    }                             // stages loop end

    n1 = n2;
    n2 = n2 >> 1;
    ia = 0;
    // loop for groups
    for (j=0; j<n2; j++) {
        cosVal = pCoef[ia * 2];
        sinVal = pCoef[(ia * 2) + 1];

        ia = ia + twidCoefModifier;

        // loop for butterfly
        for (i=j; i<fftLen; i+=n1) {
            l = i + n2;
            xt = pSrc[2 * i] - pSrc[2 * l];
            pSrc[2 * i] = (pSrc[2 * i] + pSrc[2 * l]);

            yt = pSrc[2 * i + 1] - pSrc[2 * l + 1];
            pSrc[2 * i + 1] = (pSrc[2 * l + 1] + pSrc[2 * i + 1]);

            pSrc[2u * l] = xt;

            pSrc[2u * l + 1u] = yt;

        }                           // butterfly loop end
    }                             // groups loop end
}
void Sck_Noise::arm_bitreversal(int16_t * pSrc16, uint32_t fftLen, uint16_t * pBitRevTab)
{
    /* This code is from https://github.com/adafruit/Adafruit_ZeroFFT thank you!
        -------
        This is an FFT library for ARM cortex M0+ CPUs
        Adafruit invests time and resources providing this open source code,
        please support Adafruit and open-source hardware by purchasing products from Adafruit!
        Written by Dean Miller for Adafruit Industries. MIT license, all text above must be included in any redistribution
        ------
    */

    int32_t *pSrc = (int32_t *) pSrc16;
    int32_t in;
    uint32_t fftLenBy2, fftLenBy2p1;
    uint32_t i, j;

    /*  Initializations */
    j = 0u;
    fftLenBy2 = fftLen / 2u;
    fftLenBy2p1 = (fftLen / 2u) + 1u;

    /* Bit Reversal Implementation */
    for (i = 0u; i <= (fftLenBy2 - 2u); i += 2u) {
        if(i < j) {
            in = pSrc[i];
            pSrc[i] = pSrc[j];
            pSrc[j] = in;

            in = pSrc[i + fftLenBy2p1];
            pSrc[i + fftLenBy2p1] = pSrc[j + fftLenBy2p1];
            pSrc[j + fftLenBy2p1] = in;
        }

        in = pSrc[i + 1u];
        pSrc[i + 1u] = pSrc[j + fftLenBy2];
        pSrc[j + fftLenBy2] = in;

        /*  Reading the index for the bit reversal */
        j = *pBitRevTab;

        /*  Updating the bit reversal index depending on the fft length  */
        pBitRevTab++;
    }
}
void Sck_Noise::fft2db()
{
    for (uint16_t i=0; i<FFT_NUM; i++) {
        if (readingFFT[i] > 0) readingFFT[i] = FULL_SCALE_DBSPL - (FULL_SCALE_DBFS - (20 * log10(readingFFT[i] * sqrt(2))));
        if (readingFFT[i] < 0) readingFFT[i] = 0;
    }
}

// Barometric pressure and Altitude
bool Sck_MPL3115A2::start()
{
    if (!I2Cdetect(&Wire, address)) return false;
    if (Adafruit_mpl3115A2.begin()) return true;
    return false;
}
bool Sck_MPL3115A2::stop()
{

    return true;
}
bool Sck_MPL3115A2::getAltitude()
{

    Adafruit_mpl3115A2.begin();

    // TODO callibration with control interface
    // Maybe we could implement get online data to calibrate this
    // mpl3115A2.setSeaPressure(102250.0);

    // TODO timeout to prevent hangs on external lib
    altitude = Adafruit_mpl3115A2.getAltitude();

    return true;
}
bool Sck_MPL3115A2::getPressure()
{

    Adafruit_mpl3115A2.begin();

    // TODO timeout to prevent hangs on external lib
    pressure = Adafruit_mpl3115A2.getPressure() / 1000;

    return true;
}
bool Sck_MPL3115A2::getTemperature()
{

    Adafruit_mpl3115A2.begin();

    // TODO timeout to prevent hangs on external lib
    altitude = Adafruit_mpl3115A2.getAltitude();
    temperature =  Adafruit_mpl3115A2.getTemperature(); // Only works after a getAltitude! don't call this allone

    return true;
}

// Barometric pressure and Altitude
bool Sck_LPS33::start()
{

    if (!I2Cdetect(&Wire, address)) return false;
    if (Adafruit_lps35hw.begin_I2C()) {
        Adafruit_lps35hw.setDataRate(LPS35HW_RATE_ONE_SHOT);
        return true;
    }
    return false;
}
bool Sck_LPS33::stop()
{

    return true;
}

bool Sck_LPS33::getPressure()
{

    Adafruit_lps35hw.takeMeasurement();
    // TODO add calibration of zeroPressure via lps35hw.zeroPressure();
    // TODO timeout to prevent hangs on external lib
    pressure = Adafruit_lps35hw.readPressure() / 10; // convert from hPa to kPa

    return true;
}
bool Sck_LPS33::getTemperature()
{

    Adafruit_lps35hw.takeMeasurement();

    // TODO timeout to prevent hangs on external lib
    temperature =  Adafruit_lps35hw.readTemperature();

    return true;
}

#ifdef WITH_PM
// PM sensor
bool Sck_PM::start()
{
    if (started) return true;
    else if (alreadyFailed) return false;

    pinMode(pinPM_ENABLE, OUTPUT);
    digitalWrite(pinPM_ENABLE, HIGH);
    SerialPM.begin(9600);
    delay(250);
    SerialPM.setTimeout(5000);

    if (fillBuffer()) {
        if (!sendCmd(PM_CMD_CHANGE_MODE, PM_MODE_PASSIVE, true)) {
            if (debug) Serial.println("PM: Failed setting passive mode");
            stop();
            alreadyFailed = true;
            return false;
        }

        started = true;
        wakeUpTime = rtc->getEpoch();
        if (debug) Serial.println("PM: Started OK");
        return true;
    }

    if (debug) Serial.println("PM: serial port didn't started correctly");
    stop();

    alreadyFailed = true;
    return false;
}
bool Sck_PM::stop()
{
    if (debug) Serial.println("PM: Stoping sensor");
    digitalWrite(pinPM_ENABLE, LOW);
    SerialPM.end();
    started = false;
    wakeUpTime = 0;
    return true;
}
bool Sck_PM::getReading(OneSensor *wichSensor, SckBase *base)
{
    uint32_t now = rtc->getEpoch();

    // If last reading is recent doesn't make sense to get a new one
    if (now - lastReading < warmUpPeriod && !monitor) {
        if (debug) Serial.println("PM: Less than warmUp period since last update, data is still valid...");
        return true;
    }

    // Check if sensor is on
    if (!started) start();

    // Check if sensor is sleeping
    if (wakeUpTime == 0) wake();

    // Are we still warming up?
    uint32_t warmUpPassed = now - wakeUpTime;
    if (warmUpPassed < warmUpPeriod) {
        wichSensor->state = warmUpPeriod - warmUpPassed;    // Report how many seconds are missing to cover the warm up period
        if (debug) Serial.println("PM: Still on warm up period");

        // Old sensors seem to wakeUp on active mode so we need to set them to passive each time.
        if (SerialPM.available()) {
            if (debug) Serial.println("PM: This seems to be an old sensor... changing to passive mode");
            oldSensor = true;
            while (SerialPM.available()) SerialPM.read();
            if (!sendCmd(PM_CMD_CHANGE_MODE, PM_MODE_PASSIVE, true)) {
                if (debug) Serial.println("PM: Failed setting passive mode");
                stop();
                return false;
            }
        }
        return true;
    }

    // Empty SerialPM internal buffer
    while (SerialPM.available()) SerialPM.read();

    if (!sendCmd(PM_CMD_GET_PASSIVE_READING, 0x00, false)) return false;

    if (!fillBuffer()) return false;

    if (!processBuffer()) return false;

    // Only go to sleep if these conditions are met
    if (    powerSave &&                                    // PowerSave is enabled
        ((wichSensor->everyNint * base->config.readInterval) > (warmUpPeriod * 2)) &&   // Reading interval is twice the warmUpPeriod
        !base->st.dynamic &&                                // We are not in dynamic mode
        !monitor) {                                     // We are not in monitor mode

        if (debug) Serial.println("PM: going to sleep");
        if (oldSensor) delay(50);   // Old sensors don't work without a small delay between commands
        if (!sleep()) return false;
    }

    monitor = false;
    wichSensor->state = 0;
    return true;
}
bool Sck_PM::fillBuffer()
{
    // Wait for start char 1 (0x42)
    if (!SerialPM.find(PM_START_BYTE_1)) {
        if (debug) Serial.println("PM: Timeout waiting for data start");
        return false;
    }

    size_t readedBytes = SerialPM.readBytes(buffer, buffLong);

    // Did we get all needed bytes?
    if (readedBytes < buffLong - 1) {
        if (debug) Serial.println("PM: Error: received less data than expected");
        return false;
    }

    if (debug) Serial.println("PM: Buffer filled OK");
    return true;
}
bool Sck_PM::processBuffer()
{
    if (buffer[0] != PM_START_BYTE_2) {
        if (debug) Serial.println("PM: Error on received data");
        return false;
    }

    // Checksum
    uint16_t checkSum = (buffer[buffLong - 2]<<8) + buffer[buffLong - 1];
    uint16_t sum = 0x42; // Start_byte_1 is not included in buffer, but it needs to be on the sum
    for(int i=0; i<(buffLong - 2); i++) sum += buffer[i];
    if(sum != checkSum) {
        if (debug) Serial.println("PM: Checksum error");
        return false;
    }

    // Get the values
    pm1 = (buffer[3]<<8) + buffer[4];
    pm25 = (buffer[5]<<8) + buffer[6];
    pm10 = (buffer[7]<<8) + buffer[8];
    pn03 = (buffer[15]<<8) + buffer[16];
    pn05 = (buffer[17]<<8) + buffer[18];
    pn1 = (buffer[19]<<8) + buffer[20];
    pn25 = (buffer[21]<<8) + buffer[22];
    pn5 = (buffer[23]<<8) + buffer[24];
    pn10 = (buffer[25]<<8) + buffer[26];

    // This values are just for debug
    version = buffer[27];
    errorCode = buffer[28];

    if (debug) {
        Serial.print("PM: version: ");
        Serial.println(version);
        Serial.print("PM: Error code: ");
        Serial.println(errorCode);
        Serial.println("PM: Reading data received OK");
    }

    lastReading = rtc->getEpoch();
    return true;
}
bool Sck_PM::sendCmd(byte cmd, byte data, bool checkResponse)
{
    // Based on datasheet: Appendix B：Transport Protocol Passive Mode

    if (debug) {
        Serial.print("PM: Sending command: 0x");
        Serial.print(cmd, HEX);
        Serial.print(" with data: 0x");
        Serial.println(data, HEX);
    }

    uint8_t msgLong = 7;
    unsigned char buff[msgLong];

    buff[0] = PM_START_BYTE_1;
    buff[1] = PM_START_BYTE_2;
    buff[2] = cmd;  // Command
    buff[3] = 0x00; // Data 1 (DATAH)
    buff[4] = data; // Data 2 (DATAL)

    // Checksum
    uint16_t sum = 0;
    for(uint8_t i=0; i<(msgLong - 2); i++) sum += buff[i];
    buff[5] = ((sum >> 8) & 0xFF);  // Verify byte 1 (LRCH)
    buff[6] = (sum & 0xFF) ;    // Verify byte 2 (LRCL)

    // Clear buffer
    if (retries > 0) {
        while (SerialPM.available()) SerialPM.read();
    }

    // Send message
    SerialPM.write(buff, msgLong);

    // When asking for readings we don't check response
    if (!checkResponse) return true;

    // Wait for start char 1 (0x42)
    if (!SerialPM.find(PM_START_BYTE_1)) {
        if (debug) Serial.println("PM: Timeout waiting for response");
        if (retries < MAX_RETRIES) {
            if (debug) Serial.println("PM: Retrying command");
            retries++;
            return sendCmd(cmd, data, checkResponse);
        } else {
            retries = 0;
            return false;
        }
    }

    // Get response
    uint8_t resLong = 7;
    unsigned char res[resLong];
    uint8_t received = SerialPM.readBytes(res, resLong);

    // Checksum
    sum = 0x42;
    uint16_t checkSum = (res[resLong - 2]<<8) + (res[resLong - 1]);
    for(int i=0; i<(resLong - 2); i++) sum += res[i];
    if(sum != checkSum) {
        if (debug) Serial.println("PM: Checksum error on command response");
        if (retries < MAX_RETRIES) {
            if (debug) Serial.println("PM: Retrying command");
            retries++;
            return sendCmd(cmd, data, checkResponse);
        } else {
            retries = 0;
            return false;
        }
    }

    // Check response
    if (    (res[0] != 0x4d) ||
        (res[1] != 0x00) ||
        (res[2] != 0x04) ||
        (res[3] != cmd)  ||
        (res[4] != data)) {

        if (debug) Serial.println("PM: Error on command response");
        if (retries < MAX_RETRIES) {
            if (debug) Serial.println("PM: Retrying command");
            retries++;
            return sendCmd(cmd, data, checkResponse);
        } else {
            retries = 0;
            return false;
        }
    }

    if (debug) {
        Serial.print("PM: Success on command 0x");
        Serial.print(res[3], HEX);
        Serial.print(" with data: 0x");
        Serial.println(res[4], HEX);
    }

    return true;
}
bool Sck_PM::sleep()
{
    if (debug) Serial.println("PM: Sending sensor to sleep");

    if (wakeUpTime == 0) {
        if (debug) Serial.println("PM: Sensor is already sleeping");
        return true;
    }

    if (!sendCmd(PM_CMD_SLEEP_ACTION, PM_SLEEP)) {
        if (debug) Serial.println("PM: Error on going to sleep");
        return false;
    }

    wakeUpTime = 0;
    return true;
}
bool Sck_PM::wake()
{
    if (debug) Serial.println("PM: Waking up sensor");

    if (wakeUpTime > 0) {
        if (debug) Serial.println("PM: Sensor is already waked up");
        return true;
    }

    if (!sendCmd(PM_CMD_SLEEP_ACTION, PM_WAKEUP, false)) {
        if (debug) Serial.println("PM: Error on waking up");
        return false;
    }

    wakeUpTime = rtc->getEpoch();
    return true;
}
#endif

#ifdef WITH_CCS811
// VOC and ECO2
bool Sck_CCS811::start()
{
    if (alreadyStarted) return true;

    if (!I2Cdetect(&Wire, address)) return false;

    if (!ccs.begin()) return false;

    if (ccs.setDriveMode(driveMode) != CCS811Core::CCS811_Stat_SUCCESS) return false;

    startTime = rtc->getEpoch();
    alreadyStarted = true;
    return true;
}
bool Sck_CCS811::stop()
{
    // If the sensor is not there we don't need to stop it
    if (!I2Cdetect(&Wire, address)) return true;

    if (ccs.setDriveMode(0) != CCS811Core::CCS811_Stat_SUCCESS) return false;
    alreadyStarted = false;
    startTime = 0;
    return true;
}
bool Sck_CCS811::getReading(SckBase *base)
{
    if (!alreadyStarted) start();
    uint32_t rtcNow = rtc->getEpoch();
    if (((startTime == 0) || ((rtcNow - startTime) < warmingTime)) && !base->inTest) {
        if (debug) {
            SerialUSB.println("CCS811: in warming period!!");
            SerialUSB.print("CCS811: Readings will be ready in (sec): ");
            SerialUSB.println(warmingTime - (rtcNow - startTime));
        }
        return false;
    }
    if (millis() - lastReadingMill < 5000) {
        if (debug) SerialUSB.println("CCS811: (not enough time passed)");
        return true; // This prevents getting different updates for ECO2 and VOCS
    }

    if (!ccs.dataAvailable()) {
        uint32_t Uinterval = 60000;     // Interval between sensor update (ms)
        switch (driveMode) {
            case 1: Uinterval = 1000; break;
            case 2: Uinterval = 10000; break;
            case 3: Uinterval = 60000; break;
        }

        if (debug) {
            SerialUSB.print("CCS811: Drivemode interval (ms): ");
            SerialUSB.println(Uinterval);
        }

        if (millis() - lastReadingMill < Uinterval) {
            if (debug) SerialUSB.println("CCS811: using old readings (not enough time passed)");
            return true;  // We will use last reading because  sensor is not programmed to give us readings so often
        }
        if (debug) SerialUSB.println("CCS811: ERROR obtaining reading!!");
        return false;
    }

    lastReadingMill = millis();

    ccs.readAlgorithmResults();

    VOCgas = ccs.getTVOC();
    ECO2gas = ccs.getCO2();

    if (compensate) {
        if (base->sensors[SENSOR_TEMPERATURE].enabled && base->sensors[SENSOR_HUMIDITY].enabled) {
            base->getReading(&base->sensors[SENSOR_HUMIDITY]);
            base->getReading(&base->sensors[SENSOR_TEMPERATURE]);
            if (base->sensors[SENSOR_HUMIDITY].state == 0 && base->sensors[SENSOR_TEMPERATURE].state == 0) {
                if (debug) SerialUSB.println("CCS811: Compensating readings with temp/hum");
                ccs.setEnvironmentalData(base->sensors[SENSOR_HUMIDITY].reading.toFloat(), base->sensors[SENSOR_TEMPERATURE].reading.toFloat());
            } else {
                if (debug) SerialUSB.println("CCS811: Compensation failed Error obtaining temp/hum readings!!");
            }
        } else {
            if (debug) SerialUSB.println("CCS811: temp/hum compensation failed, some sensors are disabled");
        }
    } else {
        if (debug) SerialUSB.println("CCS811: temp/hum compensation is disabled!");
    }
    return true;
}
uint16_t Sck_CCS811::getBaseline()
{
    if (!alreadyStarted) {
        if (!start()) return false;
    }
    return ccs.getBaseline();
}
bool Sck_CCS811::setBaseline(uint16_t wichBaseline)
{
    if (!alreadyStarted) {
        if (!start()) return false;
    }

    if (ccs.setBaseline(wichBaseline) != ccs.CCS811_Stat_SUCCESS) return false;

    return true;
}
bool Sck_CCS811::setDriveMode(uint8_t wichDrivemode)
{
    driveMode = wichDrivemode;
    if (ccs.setDriveMode(driveMode) != CCS811Core::CCS811_Stat_SUCCESS) return false;
    return true;
}
#endif

#ifdef WITH_SPS30
// Sensirion SPS30 PM sensor option
bool Sck_SPS30::start(SensorType wichSensor)
{
    // If detection already failed dont try again until next reset
    if (state == SPS30_NOT_DETECTED) return false;

    if (state != SPS30_OFF) {
        // Mark this specific metric as enabled
        for (uint8_t i=0; i<totalMetrics; i++) if (enabled[i][0] == wichSensor) enabled[i][1] = 1;
        return true;
    }

    // The SCK needs battery to survive the sensor startup current draw, with only usb power it normally does not work
    // If battery is not present it enters a reset loop
    pinMode(pinPM_ENABLE, OUTPUT);
    digitalWrite(pinPM_ENABLE, HIGH);
    delay(25);

    state = SPS30_NOT_DETECTED;

    if (!I2Cdetect(&Wire, address)) {
        if (debug) Serial.println("SPS30 ERROR no i2c address found");
        return false;
    }

    sensirion_i2c_init();

    int result = sps30_probe();
    if (result != 0) {
        Serial.print("Error probing for SPS30: ");
        Serial.println(result);
        return false;
    }

    // Detection succeeded
    if (debug) Serial.println("SPS30 Started OK");
    state = SPS30_IDLE;

    // Check if it is time to do a cleaning
    lastCleaning when = eepromSPS30LastCleaning.read();
    if (when.valid) {
        uint32_t passed = rtc->getEpoch() - when.time;
        if (passed > ONE_WEEK_IN_SECONDS) {
            if (debug) Serial.println("SPS30: More than a week since las cleaning, doing it...");
            startCleaning();
        } else {
            if (debug) {
                Serial.print("Last cleaning date (in epoch): ");
                Serial.println(when.time);
            }
        }
    } else {
        // We asume the SCK  has just been updated or it is new, so no need to trigger a cleaning.
        // Just save the timestamp to do a cleaning one week from now.
        when.time = rtc->getEpoch();
        when.valid = true;
        eepromSPS30LastCleaning.write(when);
        if (debug) Serial.println("SPS30: No valid last cleaning date found, saving it now");
    }

    // Call start again to just enable the corresponding metric
    return start(wichSensor);
}
bool Sck_SPS30::stop(SensorType wichSensor)
{
    bool changed = false;

    // Mark this specific metric as disabled
    for (uint8_t i=0; i<totalMetrics; i++) {
        if (enabled[i][0] == wichSensor) {
            enabled[i][1] = 0;
            changed = true;
        }
    }

    // Check if any metric is still enabled
    for (uint8_t i=0; i<totalMetrics; i++) {
        if (enabled[i][1]) return changed;
    }

    // If no metric is enabled turn off power
    if (debug) Serial.println("SPS30 Stoping sensor");
    digitalWrite(pinPM_ENABLE, LOW);

    state = SPS30_OFF;

    return true;
}
bool Sck_SPS30::getReading(OneSensor* wichSensor)
{
    uint32_t now = rtc->getEpoch();

    switch (state) {
        case SPS30_SLEEP: {
            // If last reading is recent doesn't make sense to get a new one
            if (now - lastReading < warmUpPeriod[0] && !monitor) {
                if (debug) Serial.println("SPS30: Less than warmUp period since last update, data is still valid...");
                return true;
            }
            wake();
        }
        case SPS30_IDLE: {
            int16_t result = sps30_start_measurement();
            if (result < 0) {
                if (debug) {
                    Serial.print("Error starting measurement on SPS30: ");
                    Serial.println(result);
                }
                return false;
            }

            if (debug) Serial.println("SPS30: Started measurement");
            measureStarted = now;
            state = SPS30_WARM_UP_1;
        }
        case SPS30_WARM_UP_1: {

            // On monitor mode we don't wait for warmUP'
            if (monitor) {
                if (!update(wichSensor->type)) return false;
                else {
                    lastReading = now;
                    wichSensor->state = 0;
                    return true;
                }
            }

            uint32_t passed = now - measureStarted;

            // If warmUp period has finished get a reading
            if (passed >= warmUpPeriod[0]) {

                if (!update(wichSensor->type)) return false;

                // If the reading is low (the tyhreshold is in #/cm3) and second warmUp hasn't passed we keep waiting
                if ((pm_readings.nc_4p0 / 100) < concentrationThreshold && passed < warmUpPeriod[1]) {
                    if (debug) Serial.println("SPS30: Concentration is low, we will wait for the second warm Up");
                    state = SPS30_WARM_UP_2;
                    wichSensor->state = warmUpPeriod[1] - passed;   // Report how many seconds are missing to cover the first warm up period
                    return true;
                }

                // return the reading
                sleep();
                lastReading = now;
                wichSensor->state = 0;
                return true;
            }

            wichSensor->state = warmUpPeriod[0] - passed;
            if (debug) Serial.println("SPS30: Still on first warm up period");
            return true;
        }
        case SPS30_WARM_UP_2: {

            uint32_t passed = now - measureStarted;

            if (passed < warmUpPeriod[1]) {
                wichSensor->state = warmUpPeriod[1] - passed;   // Report how many seconds are missing to cover the second warm up period
                if (debug) Serial.println("SPS30: Still on second warm up period");
                return true;
            }

            if (!update(wichSensor->type)) return false;

            if (!monitor) sleep();
            lastReading = now;
            wichSensor->state = 0;
            return true;
        }
    }

    return false;
}
bool Sck_SPS30::startCleaning()
{
    wake();

    state = SPS30_CLEANING;

    // * Note that this command can only be run when the sensor is in measurement
    // * mode, i.e. after sps30_start_measurement() without subsequent
    // * sps30_stop_measurement().
    int16_t result = sps30_start_measurement();
    if (result < 0) {
        if (debug) Serial.print("SPS30: Error before starting cleaning");
        return false;
    }

    int16_t response = sps30_start_manual_fan_cleaning();
    if (response < 0) {
        if (debug) Serial.println("SPS30: error on fan cleaning");
        return false;
    }

    // Block while the cleaning is happening (msg to the user is sent by the control funcion)
    uint16_t started = millis();
    while (millis() - started < 10500) {
        Serial.print(".");
        delay(500);
    }
    if (debug) Serial.println(" done!!");
    sleep();

    // Save timestamp in flash so we know when a week has passed
    lastCleaning when;
    when.time = rtc->getEpoch();
    eepromSPS30LastCleaning.write(when);

    return true;
}
bool Sck_SPS30::update(SensorType wichSensor)
{
    // Try to get new data
    uint16_t data_ready;
    int response = sps30_read_data_ready(&data_ready);
    if (response < 0) {
        if (debug) Serial.println("SPS30: error reading SPS30 data ready flag");
        return false; // TODO this shouldn't fail this soon, some retrys and diagnostics should be done
    }

    // TODO what happens if I haven't read data since 5 seconds, do I get the most recent?? CHECK THE DATASHEET
    if (!data_ready) {
        if (debug) Serial.println("SPS30: Data is not ready");
        return false;
    }

    // Ask for the new data
    response = sps30_read_measurement(&pm_readings);

    // Convert PN readings from #/cm3 to #/0.1l
    pm_readings.nc_0p5  *= 100;
    pm_readings.nc_1p0  *= 100;
    pm_readings.nc_2p5  *= 100;
    pm_readings.nc_4p0  *= 100;
    pm_readings.nc_10p0 *= 100;

    if (response < 0) {
        if (debug) Serial.println("SPS30: error getting SPS30 data");
        return false;
    }

    return true;
}
bool Sck_SPS30::sleep()
{
    if (state != SPS30_IDLE) {
        int16_t result = sps30_stop_measurement();
        if (result < 0) {
            if (debug) {
                Serial.print("Error on stoping measurement SPS30: ");
                Serial.println(result);
            }
            return false;
        }

        measureStarted = 0;
        state = SPS30_IDLE;
    }

    int16_t result = sps30_sleep();
    if (result < 0) {
        if (debug) {
            Serial.print("Error on sleep SPS30: ");
            Serial.println(result);
        }
        return false;
    }

    if (debug) Serial.println("SPS30: going to sleep...");
    monitor = false;
    state = SPS30_SLEEP;
    return true;
}
bool Sck_SPS30::wake()
{
    int16_t result = sps30_wake_up();
    if (result < 0) {
        if (debug) {
            Serial.print("Error starting measurement on SPS30: ");
            Serial.println(result);
        }
        return false;
    }

    if (debug) Serial.println("SPS30: Waking Up...");
    state = SPS30_IDLE;
    return true;
}
#endif

#ifdef WITH_SEN5X
bool Sck_SEN5X::start(SensorType wichSensor)
{
    // If detection already failed dont try again until next reset
    if (state == SEN5X_NOT_DETECTED) return false;

    // if state is not OFF the sensor is already started so we only enable the metric but we don't need to init the sensor again
    if (state != SEN5X_OFF) {
        for (uint8_t i=0; i<totalMetrics; i++) {
            if (enabled[i][0] == wichSensor && (enabled[i][2] & model) > 0) {
                enabled[i][1] = 1;
                return true;
            }
        }
        return false;
    }

    // The SCK needs battery to survive the sensor startup current draw, with only usb power it normally does not work
    // If battery is not present it  can enter a reset loop, (not always)
    pinMode(pinPM_ENABLE, OUTPUT);
    digitalWrite(pinPM_ENABLE, HIGH);
    delay(25);

    state = SEN5X_NOT_DETECTED;

    if (!I2Cdetect(&Wire, address)) {
        if (debug) Serial.println("SEN5X ERROR no device found on adress");
        return false;
    }

    delay(25); // without this there is an error on the deviceReset function

    if (!sen_sendCommand(SEN5X_RESET)) {
        if (debug) Serial.println("SEN5X: Error reseting device");
        return false;
    }
    delay(200); // From Sensirion Arduino library

    if (!findModel()) {
        Serial.println("SEN5X: error finding sensor model");
        return false;
    }

    // Check if firmware version allows The direct switch between Measurement and RHT/Gas-Only Measurement mode
    if (!getVer()) return false;
    if (firmwareVer < 2) {
        Serial.println("SEN5X: error firmware is too old and will not work with this implementation");
        return false;
    }

    // Detection succeeded
    state = SEN5X_IDLE;

    // Check if it is time to do a cleaning
    lastCleaning when = eepromSEN5xLastCleaning.read();
    if (when.valid) {

        uint32_t now = rtc->getEpoch();
        uint32_t passed = now - when.time;

        if (passed > ONE_WEEK_IN_SECONDS && (now > 1514764800)) {       // If current date greater than 01/01/2018 (validity check)
            if (debug) Serial.println("SEN5X: More than a week since las cleaning, doing it...");
            startCleaning();
        } else {
            if (debug) {
                Serial.print("Last cleaning date (in epoch): ");
                Serial.println(when.time);
            }
        }
    } else {
        // We asume the SCK  has just been updated or it is new, so no need to trigger a cleaning.
        // Just save the timestamp to do a cleaning one week from now.
        when.time = rtc->getEpoch();
        when.valid = true;
        eepromSEN5xLastCleaning.write(when);
        if (debug) Serial.println("SEN5X: No valid last cleaning date found, saving it now");
    }

    // Get VOCstate from eeprom memory to restore it on next start measure
    vocStateFromEeprom();

    // Call start again to just enable the corresponding metric
    return start(wichSensor);
}
bool Sck_SEN5X::stop(SensorType wichSensor)
{
    bool changed = false;

    // Mark this specific metric as disabled
    for (uint8_t i=0; i<totalMetrics; i++) {
        if (enabled[i][0] == wichSensor) {
            enabled[i][1] = 0;
            changed = true;
        }
    }

    // Check if any metric is still enabled
    for (uint8_t i=0; i<totalMetrics; i++) {
        if (enabled[i][1]) return changed;
    }

    // If no metric is enabled turn off power
    if (debug) Serial.println("SEN5X: Stoping sensor");
    digitalWrite(pinPM_ENABLE, LOW);

    state = SEN5X_OFF;

    return true;
}
bool Sck_SEN5X::getReading(OneSensor* wichSensor)
{
    uint32_t now = rtc->getEpoch();

    switch (state) {
        case SEN5X_OFF: {
            return false;
        }
        case SEN5X_IDLE: {

            // If last reading is recent doesn't make sense to get a new one
            if (now - lastReading < warmUpPeriod[0] && !monitor) {
                if (debug) Serial.println("SEN5X: Less than warmUp period since last update, data is still valid...");
                return true;
            }

            // Restore latest VOC state 
            vocStateToSensor();

            if (!sen_sendCommand(SEN5X_START_MEASUREMENT)) {
                if (debug) Serial.println("SEN5X: Error starting measurement");
                return false;
            }
            delay(50); // From Sensirion Arduino library

            if (debug) Serial.println("SEN5X: Started measurement (with PMs)");
            measureStarted = now;
            state = SEN5X_MEASUREMENT;
        }
        case SEN5X_MEASUREMENT: {

            // MONITOR MODE
            // On monitor mode we don't wait for warmUP'
            if (monitor) {
                if (update(wichSensor->type) != 0) return false;
                else {
                    lastReading = now;
                    wichSensor->state = 0;
                    return true;
                }
            }

            uint32_t passed = now - measureStarted;

            if (passed < warmUpPeriod[0]) {
                wichSensor->state = warmUpPeriod[0] - passed; 	// Report how many seconds are missing to cover the warm up period
                if (debug) Serial.println("SEN5X: Still on first warm up period");
                return true;
            }

            // Lets get the readings
            if (update(wichSensor->type) != 0) return false;

            // If the reading is low (the tyhreshold is in #/cm3) and second warmUp hasn't passed we keep waiting
            if ((pN4p0 / 100) < concentrationThreshold && passed < warmUpPeriod[1]) {
                if (debug) Serial.println("SEN5X: Concentration is low, we will wait for the second warm Up");
                state = SEN5X_MEASUREMENT_2;
                wichSensor->state = warmUpPeriod[1] - passed; 	// Report how many seconds are missing to cover the first warm up period
                return true;
            }

            // Save power
            idle();

            // Return the reading
            lastReading = now;
            wichSensor->state = 0;
            return true;

        } 
        case SEN5X_MEASUREMENT_2: {

            uint32_t passed = now - measureStarted;

            if (passed < warmUpPeriod[1]) {
                wichSensor->state = warmUpPeriod[1] - passed; 	// Report how many seconds are missing to cover the second warm up period
                if (debug) Serial.println("SEN5X: Still on second warm up period");
                return true;
            }

            if (update(wichSensor->type) != 0) return false;

            // Save power
            idle();

            lastReading = now;
            wichSensor->state = 0;
            return true;
        }
        case SEN5X_NOT_DETECTED: {
            return false;
        }
    }
    return false;
}
bool Sck_SEN5X::idle()
{
    // Get VOC state before going to idle mode
    vocStateFromSensor();

    if (!sen_sendCommand(SEN5X_STOP_MEASUREMENT)) {
        if (debug) Serial.println("SEN5X: Error stoping measurement");
        return false;
    }
    delay(200); // From Sensirion Arduino library

    if (debug) Serial.println("SEN5X: Stoping measurement mode");

    monitor = false;
    state = SEN5X_IDLE;
    measureStarted = 0;
}
bool Sck_SEN5X::startCleaning()
{
    state = SEN5X_CLEANING;

    // Note that this command can only be run when the sensor is in measurement mode
    if (!sen_sendCommand(SEN5X_START_MEASUREMENT)) {
        if (debug) Serial.println("SEN5X: Error starting measurment mode");
        return false;
    }
    delay(50); // From Sensirion Arduino library
 
    if (!sen_sendCommand(SEN5X_START_FAN_CLEANING)) {
        if (debug) Serial.println("SEN5X: Error starting fan cleaning");
        return false;
    }
    delay(20); // From Sensirion Arduino library

    // This message will be always printed so the user knows the SCK it's not hanged
    Serial.println("SEN5X: Started fan cleaning it will take 10 seconds...");
 
    uint16_t started = millis();
    while (millis() - started < 10500) {
        Serial.print(".");
        delay(500);
    }
    Serial.println(" done!!");

    // Save timestamp in flash so we know when a week has passed
    lastCleaning when;
    when.time = rtc->getEpoch();
    eepromSEN5xLastCleaning.write(when);

    idle();
    return true;
}
uint8_t Sck_SEN5X::update(SensorType wichSensor)
{
    // Try to get new data
    if (!sen_sendCommand(SEN5X_READ_DATA_READY)){
        if (debug) Serial.println("SEN5X: Error sending command data ready flag");
        return 2;
    }
    delay(20); // From Sensirion Arduino library

    uint8_t dataReadyBuffer[3];
    size_t charNumber = sen_readBuffer(&dataReadyBuffer[0], 3);
    if (charNumber == 0) {
        if (debug) Serial.println("SEN5X: Error getting device version value");
        return 2;
    }

    bool data_ready = dataReadyBuffer[1];

    if (!data_ready) {
        if (debug) Serial.println("SEN5X: Data is not ready");
        return 1;
    }

    if(!sen_readValues()) {
        if (debug) Serial.println("SEN5X: Error getting readings");
        return 2;
    }

    if(!sen_readPmValues()) {
        if (debug) Serial.println("SEN5X: Error getting PM readings");
        return 2;
    }

    if(!sen_readRawValues()) {
        if (debug) Serial.println("SEN5X: Error getting Raw readings");
        return 2;
    }

    return 0;
}
bool Sck_SEN5X::findModel()
{
    if (!sen_sendCommand(SEN5X_GET_PRODUCT_NAME)) {
        if (debug) Serial.println("SEN5X: Error asking for product name");
        return false;
    }
    delay(50); // From Sensirion Arduino library

    const uint8_t nameSize = 48;
    uint8_t name[nameSize];
    size_t charNumber = sen_readBuffer(&name[0], nameSize);
    if (charNumber == 0) {
        if (debug) Serial.println("SEN5X: Error getting device name");
        return false;
    }

    // We only check the last character that defines the model SEN5X
    switch(name[4])
    {
        case 48:
            model = SEN50;
            break;
        case 52:
            model = SEN54;
            break;
        case 53:
            model = SEN55;
            break;
    }

    if (debug) {
        Serial.print("SEN5X: found sensor model SEN5");
        Serial.println((char)name[4]);
    }

    return true;
}
bool Sck_SEN5X::getVer()
{
    if (!sen_sendCommand(SEN5X_GET_FIRMWARE_VERSION)){
        if (debug) Serial.println("SEN5X: Error sending version command");
        return false;
    }
    delay(20); // From Sensirion Arduino library

    uint8_t versionBuffer[12];
    size_t charNumber = sen_readBuffer(&versionBuffer[0], 3);
    if (charNumber == 0) {
        if (debug) Serial.println("SEN5X: Error getting data ready flag value");
        return false;
    }

    firmwareVer = versionBuffer[0] + (versionBuffer[1] / 10);
    hardwareVer = versionBuffer[3] + (versionBuffer[4] / 10);
    protocolVer = versionBuffer[5] + (versionBuffer[6] / 10);

    if (debug) {
        Serial.print("SEN5X Firmware Version: ");
        Serial.println(firmwareVer);
        Serial.print("SEN5X Hardware Version: ");
        Serial.println(hardwareVer);
        Serial.print("SEN5X Protocol Version: ");
        Serial.println(protocolVer);
    }

    return true;
}
bool Sck_SEN5X::sen_readValues()
{
    if (!sen_sendCommand(SEN5X_READ_VALUES)){
        if (debug) Serial.println("SEN5X: Error sending read command");
        return false;
    }
    delay(20); // From Sensirion Arduino library

    uint8_t dataBuffer[24];
    size_t receivedNumber = sen_readBuffer(&dataBuffer[0], 24);
    if (receivedNumber == 0) {
        if (debug) Serial.println("SEN5X: Error getting values");
        return false;
    }

    // First get the integers
    uint16_t uint_pM1p0        = static_cast<uint16_t>((dataBuffer[0]  << 8) | dataBuffer[1]);
    uint16_t uint_pM2p5        = static_cast<uint16_t>((dataBuffer[2]  << 8) | dataBuffer[3]);
    uint16_t uint_pM4p0        = static_cast<uint16_t>((dataBuffer[4]  << 8) | dataBuffer[5]);
    uint16_t uint_pM10p0       = static_cast<uint16_t>((dataBuffer[6]  << 8) | dataBuffer[7]);
    int16_t  int_humidity      = static_cast<int16_t>((dataBuffer[8]   << 8) | dataBuffer[9]);
    int16_t  int_temperature   = static_cast<int16_t>((dataBuffer[10]  << 8) | dataBuffer[11]);
    int16_t  int_vocIndex      = static_cast<int16_t>((dataBuffer[12]  << 8) | dataBuffer[13]);
    int16_t  int_noxIndex      = static_cast<int16_t>((dataBuffer[14]  << 8) | dataBuffer[15]);

    // TODO we should check if values are NAN before converting them
    // convert them based on Sensirion Arduino lib
    pM1p0          = uint_pM1p0      / 10.0f; 
    pM2p5          = uint_pM2p5      / 10.0f; 
    pM4p0          = uint_pM4p0      / 10.0f; 
    pM10p0         = uint_pM10p0     / 10.0f; 
    humidity       = int_humidity    / 100.0f;
    temperature    = int_temperature / 200.0f; 
    vocIndex       = int_vocIndex    / 10.0f;
    noxIndex       = int_noxIndex    / 10.0f;

    return true;
}
bool Sck_SEN5X::sen_readPmValues()
{
    if (!sen_sendCommand(SEN5X_READ_PM_VALUES)){
        if (debug) Serial.println("SEN5X: Error sending read command");
        return false;
    }
    delay(20); // From Sensirion Arduino library
 
    uint8_t dataBuffer[30];
    size_t receivedNumber = sen_readBuffer(&dataBuffer[0], 30);
    if (receivedNumber == 0) {
        if (debug) Serial.println("SEN5X: Error getting PM values");
        return false;
    }

    // First get the integers
    // uint16_t uint_pM1p0   = static_cast<uint16_t>((dataBuffer[0]   << 8) | dataBuffer[1]);
    // uint16_t uint_pM2p5   = static_cast<uint16_t>((dataBuffer[2]   << 8) | dataBuffer[3]);
    // uint16_t uint_pM4p0   = static_cast<uint16_t>((dataBuffer[4]   << 8) | dataBuffer[5]);
    // uint16_t uint_pM10p0  = static_cast<uint16_t>((dataBuffer[6]   << 8) | dataBuffer[7]);
    uint16_t uint_pN0p5   = static_cast<uint16_t>((dataBuffer[8]   << 8) | dataBuffer[9]);
    uint16_t uint_pN1p0   = static_cast<uint16_t>((dataBuffer[10]  << 8) | dataBuffer[11]);
    uint16_t uint_pN2p5   = static_cast<uint16_t>((dataBuffer[12]  << 8) | dataBuffer[13]);
    uint16_t uint_pN4p0   = static_cast<uint16_t>((dataBuffer[14]  << 8) | dataBuffer[15]);
    uint16_t uint_pN10p0  = static_cast<uint16_t>((dataBuffer[16]  << 8) | dataBuffer[17]);
    uint16_t uint_tSize   = static_cast<uint16_t>((dataBuffer[18]  << 8) | dataBuffer[19]);
 
    // convert them based on Sensirion Arduino lib
    // pM1p0   = uint_pM1p0  / 10.0f;
    // pM2p5   = uint_pM2p5  / 10.0f;
    // pM4p0   = uint_pM4p0  / 10.0f;
    // pM10p0  = uint_pM10p0 / 10.0f;
    pN0p5   = uint_pN0p5  / 10.0f;
    pN1p0   = uint_pN1p0  / 10.0f;
    pN2p5   = uint_pN2p5  / 10.0f;
    pN4p0   = uint_pN4p0  / 10.0f;
    pN10p0  = uint_pN10p0 / 10.0f;
    tSize   = uint_tSize  / 1000.0f;

    // Convert PN readings from #/cm3 to #/0.1l
    pN0p5  *= 100;
    pN1p0  *= 100;
    pN2p5  *= 100;
    pN4p0  *= 100;
    pN10p0 *= 100;
    tSize  *= 100;
 
    return true;
}
bool Sck_SEN5X::sen_readRawValues()
{
    if (!sen_sendCommand(SEN5X_READ_RAW_VALUES)){
        if (debug) Serial.println("SEN5X: Error sending read command");
        return false;
    }
    delay(20); // From Sensirion Arduino library
 
    uint8_t dataBuffer[12];
    size_t receivedNumber = sen_readBuffer(&dataBuffer[0], 12);
    if (receivedNumber == 0) {
        if (debug) Serial.println("SEN5X: Error getting Raw values");
        return false;
    }

    // Get values
    rawHumidity     = static_cast<int16_t>((dataBuffer[0]   << 8) | dataBuffer[1]);
    rawTemperature  = static_cast<int16_t>((dataBuffer[2]   << 8) | dataBuffer[3]);
    rawVoc          = static_cast<uint16_t>((dataBuffer[4]  << 8) | dataBuffer[5]);
    rawNox          = static_cast<uint16_t>((dataBuffer[6]  << 8) | dataBuffer[7]);

    return true;
}
bool Sck_SEN5X::sen_sendCommand(uint16_t wichCommand)
{
    uint8_t nothing;
    return sen_sendCommand(wichCommand, &nothing, 0);
}
bool Sck_SEN5X::sen_sendCommand(uint16_t wichCommand, uint8_t* buffer, uint8_t byteNumber)
{
    // At least we need two bytes for the command
    uint8_t bufferSize = 2;

    // Add space for CRC bytes (one every two bytes)
    if (byteNumber > 0) bufferSize += byteNumber + (byteNumber / 2);

    uint8_t toSend[bufferSize];
    uint8_t i = 0;
    toSend[i++] = static_cast<uint8_t>((wichCommand & 0xFF00) >> 8);
    toSend[i++] = static_cast<uint8_t>((wichCommand & 0x00FF) >> 0);

    // Prepare buffer with CRC every third byte
    uint8_t bi = 0;
    if (byteNumber > 0) {
        while (bi < byteNumber) {
            toSend[i++] = buffer[bi++];
            toSend[i++] = buffer[bi++];
            uint8_t calcCRC = sen_CRC(&buffer[bi - 2]);
            toSend[i++] = calcCRC;
        }
    }

    // Transmit the data
    Wire.beginTransmission(address);
    size_t writtenBytes = Wire.write(toSend, bufferSize);
    uint8_t i2c_error = Wire.endTransmission();

    if (writtenBytes != bufferSize) {
        if (debug) Serial.println("SEN5X: Error writting on I2C bus");
        return false;
    }

    if (i2c_error != 0) {
        if (debug) {
            Serial.print("SEN5X: Error on I2c communication: ");
            Serial.println(i2c_error);
        }
        return false;
    }
    return true;
}
uint8_t Sck_SEN5X::sen_readBuffer(uint8_t* buffer, uint8_t byteNumber)
{
    size_t readedBytes = Wire.requestFrom(address, byteNumber);

    if (readedBytes != byteNumber) {
        if (debug) Serial.println("SEN5X: Error reading I2C bus");
        return 0;
    }

    uint8_t i = 0;
    uint8_t receivedBytes = 0;
    while (readedBytes > 0) {
        buffer[i++] = Wire.read(); // Just as a reminder: i++ returns i and after that increments.
        buffer[i++] = Wire.read();
        uint8_t recvCRC = Wire.read();
        uint8_t calcCRC = sen_CRC(&buffer[i - 2]);
        if (recvCRC != calcCRC) {
            if (debug) Serial.println("SEN5X: Checksum error while receiving msg");
            return 0;
        }
        readedBytes -=3;
        receivedBytes += 2;
    }

    return receivedBytes;
}
uint8_t Sck_SEN5X::sen_CRC(uint8_t* buffer)
{
    // This code is based on Sensirion's own implementation https://github.com/Sensirion/arduino-core/blob/41fd02cacf307ec4945955c58ae495e56809b96c/src/SensirionCrc.cpp
    uint8_t crc = 0xff;

    for (uint8_t i=0; i<2; i++){

        crc ^= buffer[i];

        for (uint8_t bit=8; bit>0; bit--) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }

    return crc;
}
bool Sck_SEN5X::vocStateToEeprom()
{
    VOCstateStruct temp;
    for (uint8_t i=0; i<SEN5X_VOC_STATE_BUFFER_SIZE; i++) temp.state[i] = VOCstate[i];
    eepromSEN5xVOCstate.write(temp);

    if (debug) {
        Serial.println("SEN5X: VOC state saved to eeprom");
        for (uint8_t i=0; i<SEN5X_VOC_STATE_BUFFER_SIZE; i++) Serial.print(temp.state[i]);
        Serial.println();
    }

    return true;
}
bool Sck_SEN5X::vocStateFromEeprom()
{
    VOCstateStruct temp = eepromSEN5xVOCstate.read();

    if (!temp.valid) {
        Serial.println("SEN5X No valid VOC's state found on eeprom");
        return false;
    } else {
        for (uint8_t i=0; i<SEN5X_VOC_STATE_BUFFER_SIZE; i++) VOCstate[i] = temp.state[i];
        if (debug) {
            Serial.println("SEN5X Loaded VOC's state from eeprom");
            for (uint8_t i=0; i<SEN5X_VOC_STATE_BUFFER_SIZE; i++) Serial.print(VOCstate[i]);
            Serial.println();
        }
    }
    return true;
}
bool Sck_SEN5X::vocStateToSensor()
{
    if (!sen_sendCommand(SEN5X_RW_VOCS_STATE, VOCstate, SEN5X_VOC_STATE_BUFFER_SIZE)){
        if (debug) Serial.println("SEN5X: Error sending VOC's state command'");
        return false;
    }

    if (debug) {
        Serial.println("SEN5X: VOC state sent to sensor");
        for (uint8_t i=0; i<SEN5X_VOC_STATE_BUFFER_SIZE; i++) Serial.print(VOCstate[i]);
        Serial.println();
    }

    return true;
}
bool Sck_SEN5X::vocStateFromSensor()
{
    //  Ask VOCs state from the sensor
    if (!sen_sendCommand(SEN5X_RW_VOCS_STATE)){
        if (debug) Serial.println("SEN5X: Error sending VOC's state command'");
        return false;
    }

    // Retrieve the data
    size_t receivedNumber = sen_readBuffer(&VOCstate[0], SEN5X_VOC_STATE_BUFFER_SIZE + (SEN5X_VOC_STATE_BUFFER_SIZE / 2));
    delay(20);
    if (receivedNumber == 0) {
        if (debug) Serial.println("SEN5X: Error getting VOC's state'");
        return false;
    }

    // Print the state (if debug is on)
    if (debug) {
        Serial.println("SEN5X: VOC state retrieved from sensor");
        for (uint8_t i=0; i<SEN5X_VOC_STATE_BUFFER_SIZE; i++) Serial.print(VOCstate[i]);
        Serial.println();
    }

    return true;
}
#endif
#ifdef WITH_BME68X
Sck_BME68X::Sck_BME68X(byte customAddress)
{
    address = customAddress;
}
bool Sck_BME68X::start()
{

    if (alreadyStarted) return true;

    if (!_bme.begin(address)) {
        return false;
    }

    alreadyStarted = true;
    return true;
}
bool Sck_BME68X::stop()
{
    alreadyStarted = false;
    return true;
}
bool Sck_BME68X::getReading()
{
    if (millis() - lastTime > minTime) {
        if (!_bme.performReading()) return false;
        lastTime = millis();
    }

    temperature = _bme.temperature;
    humidity = _bme.humidity;
    pressure = _bme.pressure / 1000;  // Converted to kPa
    VOCgas = _bme.gas_resistance;

    return true;
}
#endif
#ifdef WITH_AS7331
bool Sck_AS7331::start(SensorType wichSensor)
{
    if (!I2Cdetect(&Wire, address)) return false;

    // Mark this specific metric as enabled
    for (uint8_t i=0; i<totalMetrics; i++) if (enabled[i][0] == wichSensor) enabled[i][1] = 1;

    if (started) return true;

    // PowerUp and set configuration mode
    writeByte(AS7331_OSR, AS7331_CONFIG);

    // Software reset
    writeByte(AS7331_OSR, AS7331_RESET);

    uint8_t chipID = getByte(AS7331_AGEN);

    if (chipID != 0x21) {
        if (debug) {
            Serial.print("AS7331 ERROR: Wrong chip ID: ");
            Serial.println(chipID);
        }
        return false;
    }

    // TODO set default configuration ESTOS son los valores que usan en el codigo de ejemplo que estoy viendo
    // Specify sensor parameters
    // MMODE   mmode = AS7331_CONT_MODE;  // choices are modes are CONT, CMD, SYNS, SYND
    // CCLK    cclk  = AS7331_1024;      // choices are 1.024, 2.048, 4.096, or 8.192 MHz
    // uint8_t sb    = 0x01;             // standby enabled 0x01 (to save power), standby disabled 0x00                    
    // uint8_t breakTime = 40;           // sample time == 8 us x breakTime (0 - 255, or 0 - 2040 us range), CONT or SYNX modes
    // uint8_t gain = 8; // ADCGain = 2^(11-gain), by 2s, 1 - 2048 range,  0 < gain = 11 max, default 10
    // uint8_t time = 9; // 2^time in ms, so 0x07 is 2^6 = 64 ms, 0 < time = 15 max, default  6
    // AS7331.init(mmode, cclk, sb, breakTime, gain, time);

    return true;
}
bool Sck_AS7331::stop(SensorType wichSensor)
{
    // Mark this specific metric as disabled
    for (uint8_t i=0; i<totalMetrics; i++) if (enabled[i][0] == wichSensor) enabled[i][1] = 0;

    // Turn sensor off only if all metrics are disabled
    for (uint8_t i=0; i<totalMetrics; i++) if (enabled[i][1] == 1) return false;

    // Power off
    writeByte(AS7331_OSR, AS7331_OFF);

    return true;
}
bool Sck_AS7331::getReading(OneSensor* wichSensor)
{

    return true;
}
byte Sck_AS7331::getByte(byte wichReg)
{
    Wire.beginTransmission(address);
    Wire.write(wichReg);
    Wire.endTransmission(false);
    Wire.requestFrom(address, 1);
    return Wire.read();
}
byte Sck_AS7331::writeByte(byte wichReg, byte wichValue)
{
    Wire.beginTransmission(address);
    Wire.write(wichReg);
    Wire.write(wichValue);
    Wire.endTransmission();

}
#endif
#endif
