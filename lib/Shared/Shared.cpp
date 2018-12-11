#include "Shared.h"

VersionInt parseVersionStr(String versionStr)
{
	// We receive a String like this: 0.5.1-aabbcc

	VersionInt versionInt;

	// Get mayor version number from string
	uint8_t sep = versionStr.indexOf(".");
	String mayorSTR = versionStr.substring(0, sep);
	versionStr.remove(0, sep+1);
	versionInt.mayor = mayorSTR.toInt();

	// Get minor version number from string
	sep = versionStr.indexOf(".");
	String minorSTR = versionStr.substring(0, sep);
	versionStr.remove(0, sep+1);
	versionInt.minor = minorSTR.toInt();

	// Get build version number from string
	sep = versionStr.indexOf("-");
	String buildSTR = versionStr.substring(0, sep);
	versionInt.build = buildSTR.toInt();

	return versionInt;
}
