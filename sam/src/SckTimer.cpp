#include "SckBase.h"
#include "SckTimer.h"


void SckBase::setTimer(uint16_t lapse, Task task) {

	nextTask = task;

	if(alarmRunning_TC3 || lapse == 0) {

		TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
		while(TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		alarmRunning_TC3 = false;

		nextTask = TASK_COUNT;

	} else {

		// Clock the timer with the core cpu clock (48MHz)
		GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3 );
		while(GCLK->STATUS.bit.SYNCBUSY);

		// Reset the TC
		TC3->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
		while(TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		while(TC3->COUNT16.CTRLA.bit.SWRST);

		// Set Timer counter Mode to 16 bits
		TC3->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;

		// Set TC5 mode as match frequency
		TC3->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

		TC3->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024 | TC_CTRLA_ENABLE;

		TC3->COUNT16.CC[0].reg = uint32_t(46.875 * lapse);
		while(TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		
		// Configure interrupt request
		NVIC_DisableIRQ(TC3_IRQn);
		NVIC_ClearPendingIRQ(TC3_IRQn);
		NVIC_SetPriority(TC3_IRQn, 2); //you can change priority between 0 (Highest priority) and 2 (lowest)
		NVIC_EnableIRQ(TC3_IRQn);

		// Enable the TC5 interrupt request
		TC3->COUNT16.INTENSET.bit.MC0 = 1;
		while(TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		
		//enable the counter (from now your getting interrupt)
		TC3->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
		while(TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);

		alarmRunning_TC3 = true;
	}
}

void SckBase::timerAlarm() {
	SerialUSB.println("caca");
	sckOut("timer alarm!!!");

	switch (nextTask) {

		case T_OTG_ON: {
			charger.OTG(1);
			break;
		} case T_OTG_OFF : {
			charger.OTG(0);
			break;
		} default: break;
	}
	
	setTimer();
}


// // 	-----------------
// // 	|	 Timers 	|
// // 	-----------------
// //
// bool SckBase::timerRun() {

// 	for (uint8_t i=0; i<timerSlots; i++) {
// 		if (timers[i].action != ACTION_NULL) {
// 			if (millis() - timers[i].started > timers[i].interval) {
				
// 				// Check for action to execute
// 				switch(timers[i].action) {
// 					case ACTION_CLEAR_ESP_BOOTING:{
// 						sckOut(F("ESP ready!!!"));
// 						timerSet(ACTION_GET_ESP_STATUS, statusPoolingInterval, true);
// 						break;

// 					} case ACTION_ESP_ON: {
// 						ESPcontrol(ESP_ON);
// 						break;

// 					} case ACTION_ESP_REBOOT: {
// 						ESPcontrol(ESP_REBOOT);
// 						break;

// 					} case ACTION_GET_ESP_STATUS: {
// 						if (!readLightEnabled) {
// 							if (!digitalRead(POWER_WIFI)) getStatus();
// 						}
// 						break;
					
// 					} case ACTION_LONG_PRESS: {
// 						longPress();
// 						break;

// 					} case ACTION_VERY_LONG_PRESS: {
// 						veryLongPress();
// 						break;

// 					} case ACTION_RESET: {

// 						softReset();
// 						break;

// 					} case ACTION_UPDATE_SENSORS: {

// 						updateSensors();
// 						break;

// 					} case ACTION_DEBUG_LOG: {

// 						sdLogADC();
// 						break;

// 					} case ACTION_GOTO_SETUP: {

// 						changeMode(MODE_SETUP);
// 						break;

// 					} case ACTION_RECOVER_ERROR: {

// 						if (config.persistentMode == MODE_SD) {

// 							if (onTime && sdPresent()) {
// 								timerClear(ACTION_RECOVER_ERROR);
// 								changeMode(MODE_SD);
// 							}
						
// 						} else if (config.persistentMode == MODE_NET) {

// 							if (digitalRead(POWER_WIFI)) ESPcontrol(ESP_ON);

// 							if (onTime && onWifi() && tokenSet) {
// 								timerClear(ACTION_RECOVER_ERROR);
// 								timerClear(ACTION_START_AP_MODE);
// 								changeMode(MODE_NET);
// 							}
// 						}

// 						break;

// 					} case ACTION_START_AP_MODE: {

// 						msgBuff.com = ESP_START_AP_COM;
// 						ESPqueueMsg(false, false);
// 						break;

// 					} case ACTION_SAVE_SD_CONFIG: {
						
// 						// How much time for next publish...
// 						uint32_t timeToNextPublish = rtc.getEpoch() - lastPublishTime;

// 						// If there is no message on queue and publish time is at least 5 seconds away
// 						if (BUS_queueCount == 0 && timeToNextPublish > 5 && !triggerHello) {
							
// 							// Save sd config (esp off)
// 							saveSDconfig();

// 							// Clear timer
// 							timerClear(ACTION_SAVE_SD_CONFIG);
// 						}
// 						break;

// 					} case ACTION_MQTT_SUBSCRIBE: {

// 						if (onWifi()) mqttConfig(true);
// 						else timerSet(ACTION_MQTT_SUBSCRIBE, 1000);
// 						break;

// 					} case ACTION_RETRY_READ_SENSOR: {

// 						sckIn(String("read ") + String(sensors[retrySensor].title));
// 						break;

// 					} case ACTION_SLEEP :{

// 						goToSleep();
// 						break;

// 					} default: {
// 						;
// 					}
// 				}

// 				// Clear Timer
// 				if (!timers[i].periodic) {
// 					timers[i].action = ACTION_NULL;
// 					timers[i].interval = 0;
// 					timers[i].started = 0;
// 				} else {

// 					// Restart timer for periodic tasks
// 					timers[i].started = millis();
// 				}

// 				return true;
// 			}
// 		}
// 	}

// 	return false;
// }
// void SckBase::timerSet(TimerAction action, uint32_t interval, bool isPeriodic) {

// 	bool slotsFree = false;

// 	for (uint8_t i=0; i<timerSlots; i++) {
// 		if (timers[i].action == ACTION_NULL) {
// 			timers[i].action = action;
// 			timers[i].interval = interval;
// 			timers[i].started = millis();
// 			timers[i].periodic = isPeriodic;
// 			slotsFree = true;
// 			break;
// 		}
// 	}

// 	if (!slotsFree) {
// 		sckOut(F("We need more Timer slots!!!"), PRIO_HIGH);
// 		for (uint8_t i=0; i<timerSlots; i++) sckOut(String(timers[i].action));
// 	}
// }
// bool SckBase::timerClear(TimerAction action) {

// 	for (uint8_t i=0; i<timerSlots; i++) {
// 		if (timers[i].action == action) {
// 			timers[i].action = ACTION_NULL;
// 			timers[i].interval = 0;
// 			timers[i].started = 0;
// 			timers[i].periodic = false;
// 			return true;
// 		}
// 	}
// 	return false;
// }
// void SckBase::timerClearTasks(bool clearAll) {

// 	for (uint8_t i=0; i<timerSlots; i++) {

// 		if ((timers[i].action != ACTION_LONG_PRESS &&
// 			timers[i].action != ACTION_VERY_LONG_PRESS && 
// 			timers[i].action != ACTION_GET_ESP_STATUS && 
// 			timers[i].action != ACTION_RECOVER_ERROR &&
// 			timers[i].action != ACTION_SAVE_SD_CONFIG) || clearAll) {

// 				timers[i].action = ACTION_NULL;
// 				timers[i].interval = 0;
// 				timers[i].started = 0;
// 				timers[i].periodic = false;
// 		}
// 	}
// }
// bool SckBase::timerExists(TimerAction action) {
// 	for (uint8_t i=0; i<timerSlots; i++) {
// 		if (timers[i].action == action) {
// 			return true;
// 		}
// 	}
// 	return false;
// }
