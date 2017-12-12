// #pragma once

// // Timer
// 	bool timerRun();
// 	enum TimerAction { 
// 		ACTION_NULL,						// 0
// 		ACTION_CLEAR_ESP_BOOTING,			// 1
// 		ACTION_ESP_ON,						// 2
// 		ACTION_ESP_REBOOT,					// 3
// 		ACTION_GET_ESP_STATUS,				// 4
// 		ACTION_LONG_PRESS,					// 5
// 		ACTION_VERY_LONG_PRESS,				// 6
// 		ACTION_RESET,						// 7
// 		ACTION_UPDATE_SENSORS,				// 8
// 		ACTION_CHECK_ESP_PUBLISH_TIMEOUT,	// 9
// 		ACTION_READ_NETWORKS,				// 10
// 		ACTION_DEBUG_LOG,					// 11
// 		ACTION_GOTO_SETUP,					// 12
// 		ACTION_RECOVER_ERROR,				// 13
// 		ACTION_START_AP_MODE,				// 14
// 		ACTION_SAVE_SD_CONFIG,				// 15
// 		ACTION_MQTT_SUBSCRIBE,				// 16
// 		ACTION_RETRY_READ_SENSOR,			// 17
// 		ACTION_SLEEP						// 18
// 	};
// 	struct OneTimer	{
// 		TimerAction action = ACTION_NULL;
// 		bool periodic = false;
// 		uint32_t interval = 0;
// 		uint32_t started = 0;
// 	};
// 	static const uint8_t timerSlots = 12;
// 	OneTimer timers[timerSlots];
// 	void timerSet(TimerAction action, uint32_t interval, bool isPeriodic=false);		// interval is in milliseconds
// 	bool timerClear(TimerAction action);
// 	void timerClearTasks(bool clearAll=false);
// 	bool timerExists(TimerAction action);