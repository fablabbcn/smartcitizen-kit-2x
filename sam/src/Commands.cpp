#include "Commands.h"
#include "SckBase.h"

void AllCommands::in(SckBase* base, String strIn) {

	strIn.replace("\n", "");
	strIn.replace("\r", "");
	strIn.trim();

	if (strIn.length() <= 0) return;

	CommandType reqComm = COM_COUNT;

	// Search in the command list
	for (uint8_t i=0; i < COM_COUNT; ++i) {

		CommandType thisType = static_cast<CommandType>(i);

		OneCom *thisCommand = &com_list[thisType];

		if (strIn.startsWith(thisCommand->title)) {
			reqComm = thisType;
			strIn.replace(thisCommand->title, "");
			strIn.trim();
			break;
		}
	}

	if (reqComm < COM_COUNT) com_list[reqComm].function(base, strIn);
	else base->sckOut("Unrecognized command!!");
}

void reset_com(SckBase* base, String parameters) {

	base->reset();
}
void getVersion_com(SckBase* base, String parameters) {
	base->sckOut("Hardware Version ");
	base->sckOut("SAM Version ");
	base->sckOut("ESP version ");
}
void outlevel_com(SckBase* base, String parameters) {

	// get
	if (parameters.length() <= 0) {
	
		sprintf(base->outBuff, "Current output level: %s", base->outLevelTitles[base->outputLevel]);
		base->sckOut();
	
	// set
	} else {
		
		uint8_t newLevel = parameters.toInt();
		
		// Parameter sanity check
		if (newLevel > 0 && newLevel < OUT_COUNT) {
			sprintf(base->outBuff, "New output level: %s", base->outLevelTitles[newLevel]);
			base->sckOut();
		} else {
			base->sckOut("unrecognized output level!!");
			return;
		}
	}
}
void help_com(SckBase* base, String parameters) {

	// list commands
	if (parameters.length() <= 0) {
	
		base->sckOut("Command list:\n");

		uint8_t numberOfColumns = 3;

		for (uint8_t i=0; i < COM_COUNT; ++i) {

			CommandType thisType = static_cast<CommandType>(i);
			OneCom *thisCommand = &base->commands[thisType];


			if ((i+1) % numberOfColumns == 0) {
				sprintf(base->outBuff, "%s%s", base->outBuff, thisCommand->title);
				base->sckOut();
			} else {
				sprintf(base->outBuff, "%s%s%s", base->outBuff, thisCommand->title, "\t");
				if (String(thisCommand->title).length() < 8) sprintf(base->outBuff, "%s%s", base->outBuff, "\t");
			}
		}
		base->sckOut();
		base->sckOut("Type help [command name] to see command description.");

	// Specific command help
	} else {

		for (uint8_t i=0; i < COM_COUNT; ++i) {

			CommandType thisType = static_cast<CommandType>(i);
			OneCom *thisCommand = &base->commands[thisType];

			if (parameters.startsWith(thisCommand->title)) {
				base->sckOut(thisCommand->help);
				break;
			}
		}

	}
}
void esp_com(SckBase* base, String parameters) {

	if (parameters.length() <= 0) {
		if (base->espStarted > 0) {
			sprintf(base->outBuff, "ESP is on since %lu seconds ago", (millis() - base->espStarted) / 1000);
			base->sckOut();
		} else base->sckOut("ESP is off");
	} else if (parameters.equals("on")) base->ESPcontrol(base->ESP_ON);
	else if (parameters.equals("off")) base->ESPcontrol(base->ESP_OFF);
	else if (parameters.equals("reboot")) base->ESPcontrol(base->ESP_REBOOT);
	else if (parameters.equals("flash")) base->ESPcontrol(base->ESP_FLASH);
	else if (parameters.equals("debug")) {
		// TODO toggle esp debug
	}
}
void resetCause_com(SckBase* base, String parameters) {

	uint8_t resetCause = PM->RCAUSE.reg;

	switch(resetCause){
		case 1:
			base->sckOut("POR: Power On Reset");
			break;
		case 2:
			base->sckOut("BOD12: Brown Out 12 Detector Reset");
			break;
		case 4:
			base->sckOut("BOD33: Brown Out 33 Detector Reset");
			break;
		case 16:
			base->sckOut("EXT: External Reset");
			break;
		case 32:
			base->sckOut("WDT: Watchdog Reset");
			break;
		case 64:
			base->sckOut("SYST: System Reset Request");
			break;
	}
}
