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
		}
	}

	if (reqComm < COM_COUNT) com_list[reqComm].function(base, strIn);
	else base->sckOut("Unrecognized command!!");
}

void reset_com(SckBase* base, String parameters) {
	base->sckOut("Bye!!");
	NVIC_SystemReset();
}

void getVersion_com(SckBase* base, String parameters) {
	base->sckOut("Version");
}