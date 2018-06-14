/* #ifndef _AUDIO_ANALYZER_H_INCLUDED */
/* #define _AUDIO_ANALYZER_H_INCLUDED */

#include <I2S.h>
#include <Arduino.h>

#include "ConstantsSound.h"

#define ARM_MATH_CM0PLUS
#include <arm_math.h>

enum WeightingType{
	A_WEIGHTING,
   	C_WEIGHTING,
   	Z_WEIGHTING
};

//CLASS
class AudioAnalyser
{
public:

  	bool begin(long sampleRate, int bitsPerSample);
  	void end();
	void scalingandwindow(void *vector, int vectorSize);
  	void equalising(void *inputBuffer, int inputSize, int sampleRate);	

};

