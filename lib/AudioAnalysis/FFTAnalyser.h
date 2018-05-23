/* #ifndef _FFT_ANALYSER_H_INCLUDED */
/* #define _FFT_ANALYSER_H_INCLUDED */

#include <Arduino.h>

/* #define ARM_MATH_CM0PLUS */
/* #include <arm_math.h> */

#include "AudioAnalyser.h"

//CLASS
class FFTAnalyser : public AudioAnalyser
{
public:

  FFTAnalyser(int bufferSize=512, int fftSize=512);
  ~FFTAnalyser();

  bool configure(int sampleRate=40000);
  float getReading(int spectrum[], WeightingType weighting_type);
  float getReading(WeightingType weighting_type);
  bool bufferFilled();

private:

  void weighting(void *inputBuffer, int inputSize, WeightingType weighting_type);
  void convert2DB(void *inputVector, void *outputVector, int vectorSize, int factor);
  double rms(void *inputBuffer, int inputSize, int typeRMS, int factor);
  void fft(void *inputBuffer, void* outputBuffer, int fftBufferSize);

  //BUFFER Sizes
  int _fftSize;
  int _bufferSize; //Already usable bufferSize
  int _bufferIndex;
  int _sampleRate;
  int _bitsPerSample;
  float _rmsSpecB;
  //BUFFERS
  void* _sampleBuffer;
  void* _fftBuffer;
  void* _spectrumBuffer;
  void* _spectrumBufferDB;
  //FFT
  arm_rfft_instance_q31 _S31;
};

/* #endif */
