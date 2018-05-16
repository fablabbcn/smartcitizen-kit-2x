#include "AudioAnalyser.h"

bool AudioAnalyser::begin(long sampleRate, int bitsPerSample) {
  if (!I2S.begin(I2S_PHILIPS_MODE, sampleRate, bitsPerSample)) {
    return 0;
  }

  //Initialisation
  int _delay = 263000;
  for (int i = 0; i< _delay; i++) {
      // Trigger a read to kick things off
      I2S.read();
  }

  return 1;
}

void AudioAnalyser::end() {
  I2S.end();
}

void AudioAnalyser::scalingandwindow(void *vector, int vectorSize){
  // SCALE signal by factor
  q31_t* _vectDW = (q31_t*) vector;
  double window = 0;
  for (int i = 0; i<vectorSize;i++){
    window = HANN_REDUCED[i];
    *_vectDW *= window;
    _vectDW++;
  }
}

void AudioAnalyser::equalising(void *inputBuffer, int inputSize, int sampleRate){
  //Deconvolution of the spectrumBuffer by division of the microphone frequency response

  q31_t* _spBE = (q31_t*)inputBuffer;
  int _step = EQUALTABSIZE / inputSize * sampleRate / EQUALTABSAMPLERATE; // THIS SHOULD ALWAYS BE LARGER THAN 1
  
  if (!(_step<1)) {
    double equalfactor = 0;

    for (int i = 0; i < inputSize; i ++) {
      equalfactor = EQUALTAB[i*_step];
      *_spBE *= equalfactor;
      _spBE++;
    }
  }
}


