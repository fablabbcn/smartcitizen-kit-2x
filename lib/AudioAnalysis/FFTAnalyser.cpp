#include "FFTAnalyser.h"

FFTAnalyser::FFTAnalyser(int bufferSize, int fftSize) {
  //BUFFER Sizes
  _fftSize = fftSize;
  _bufferSize = bufferSize;
  _bufferIndex = 0;
  //BUFFERS
  _sampleBuffer = NULL;
  _sampleRate = NULL;
  _bitsPerSample = NULL;
  _fftBuffer = NULL;
  _spectrumBuffer = NULL;
  _spectrumBufferDB = NULL;
  //RMS
  _rmsSpecB = 0;
}

FFTAnalyser::~FFTAnalyser() {
  if (_sampleBuffer){
    free(_sampleBuffer);
  }

  if (_fftBuffer) {
    free(_fftBuffer);
  }

  if (_spectrumBuffer) {
    free(_spectrumBuffer);
  }

  if (_spectrumBufferDB) {
    free(_spectrumBufferDB);
  }
}

bool FFTAnalyser::configure(int sampleRate){

  _sampleRate = sampleRate;
  _bitsPerSample = 32;

  //Initialise I2S
  begin(_sampleRate, _bitsPerSample);

  //Initialise fft
  if (ARM_MATH_SUCCESS != arm_rfft_init_q31(&_S31, _fftSize, 0, 1)) {
    return false;
  }

  //Allocate time buffer
  _sampleBuffer = calloc(_bufferSize, sizeof(q31_t));
  _fftBuffer = calloc(_fftSize, sizeof(q31_t));
  
  //Allocate frequency buffers
  _spectrumBuffer = calloc(_fftSize/2, sizeof(q31_t));
  _spectrumBufferDB = calloc(_fftSize/2, sizeof(q31_t));

  //Free all buffers in case of bad allocation
  if (_sampleBuffer == NULL || _fftBuffer == NULL || _spectrumBuffer == NULL || _spectrumBufferDB == NULL) {

    if (_sampleBuffer) {
      free(_sampleBuffer);
      _sampleBuffer = NULL;
    }

    if (_fftBuffer) {
      free(_fftBuffer);
      _fftBuffer = NULL;
    }

    if (_spectrumBuffer) {
      free(_spectrumBuffer);
      _spectrumBuffer = NULL;
    }

    if (_spectrumBufferDB) {
      free(_spectrumBufferDB);
      _spectrumBufferDB = NULL;
    }
    
    return false;
  }
  return true;
}

bool FFTAnalyser::bufferFilled() {

	int32_t* _buff = (int32_t*) _sampleBuffer;

	uint32_t started = millis();
	while(millis() - started < 2000) {
		
		begin(_sampleRate, _bitsPerSample);
  
		int32_t _sample = 0;

		if(_bufferIndex < _bufferSize) {

			I2S.read(&_sample, _bitsPerSample/8); 

			if (_sample != 0) {
				_buff[_bufferIndex] = _sample>>7;
				_bufferIndex++;
			}
		} else {
			return true;
			end();
		}
	}
	// TODO solve this I2S hangs, for now we reset if it is hanged
	SerialUSB.println("CRASHED!!!");
	NVIC_SystemReset();
	return false;
}

float FFTAnalyser::getReading(int spectrum[], WeightingType weighting_type){

  uint32_t time_after = micros();

  // Downscale the sample buffer for proper functioning
  scalingandwindow(_sampleBuffer, _bufferSize);

  // fft
  fft(_sampleBuffer, _spectrumBuffer, _fftSize);

  // Equalise
  equalising(_spectrumBuffer, _fftSize/2, _sampleRate);

  // Weighting
  switch (weighting_type) {

    case A_WEIGHTING:
    case C_WEIGHTING:
      weighting(_spectrumBuffer, _fftSize/2, weighting_type);
      _rmsSpecB = rms(_spectrumBuffer, _fftSize/2, 2, CONST_FACTOR); 
      convert2DB(_spectrumBuffer, _spectrumBufferDB, _fftSize/2,CONST_FACTOR);
      memcpy(spectrum, _spectrumBufferDB, sizeof(int) * _fftSize/2);
      break;

    case Z_WEIGHTING:
      _rmsSpecB = rms(_spectrumBuffer, _fftSize/2, 2, CONST_FACTOR);
      convert2DB(_spectrumBuffer, _spectrumBufferDB, _fftSize/2,CONST_FACTOR);
      memcpy(spectrum, _spectrumBufferDB, sizeof(int) * _fftSize/2);
      break;
  }

  _rmsSpecB = FULL_SCALE_DBSPL-(FULL_SCALE_DBFS-20*log10(sqrt(2)*_rmsSpecB));

  _bufferIndex = 0;
  return _rmsSpecB;
}

float FFTAnalyser::getReading(WeightingType weighting_type){

  // Apply Hann window and downscale by CONST_FACTOR
  scalingandwindow(_sampleBuffer, _bufferSize);

  // fft
  fft(_sampleBuffer, _spectrumBuffer, _fftSize);

  // Equalise
  equalising(_spectrumBuffer, _fftSize/2, _sampleRate);

  // Weighting
  if (weighting_type == A_WEIGHTING || weighting_type == C_WEIGHTING) weighting(_spectrumBuffer, _fftSize/2, weighting_type);

  _rmsSpecB = rms(_spectrumBuffer, _fftSize/2, 2, CONST_FACTOR);
  _rmsSpecB = (float) (FULL_SCALE_DBSPL-(FULL_SCALE_DBFS-20*log10(sqrt(2)*_rmsSpecB)));
  
  _bufferIndex = 0;

  return _rmsSpecB;
}

void FFTAnalyser::fft(void *_inputBuffer, void* _outputBuffer, int _fftBufferSize){
  //_inputBuffer is already treated for FFT (usable samples, averaged, windowed)

  // Calculate FFTBuffer ((r-i,r-i...))
  arm_rfft_q31(&_S31, (q31_t*)_inputBuffer, (q31_t*)_fftBuffer);

  //Calculate spectrumBuffer and normalize
  const q31_t* _pfftBuffer = (const q31_t*)_fftBuffer;
  q31_t* _pspectrumBuffer = (q31_t*) _outputBuffer;

  for (int i = 0; i < _fftBufferSize; i +=2) {
    *_pspectrumBuffer = (*_pfftBuffer) * (*_pfftBuffer);
    _pfftBuffer++;
    
    *_pspectrumBuffer += (*_pfftBuffer) * (*_pfftBuffer);
    *_pspectrumBuffer = sqrt(*_pspectrumBuffer);

    //Normalize SpectrumBuffer
    if (i) {
      *_pspectrumBuffer = 2 * (*_pspectrumBuffer);
    }

    _pfftBuffer++;
    _pspectrumBuffer++;
  }
}

void FFTAnalyser::convert2DB(void *inputVector, void *outputVector, int vectorSize, int factor){
    const q31_t* _vect = (const q31_t*) inputVector;
    q31_t* _vectDB = (q31_t*) outputVector;

    for (int i = 0; i<vectorSize;i++){
      if (*_vect>0){ 
        *_vectDB = FULL_SCALE_DBSPL-(FULL_SCALE_DBFS-20*log10(sqrt(2) * (*_vect) * factor));
        if (*_vectDB < 0 ) *_vectDB = 0;      
      } else {
        *_vectDB = 0;
      }
      _vect++;
      _vectDB++;
    }
}

void FFTAnalyser::weighting(void *inputBuffer, int inputSize, WeightingType weighting_type){
  //Apply weighting to Buffer
  q31_t* spB = (q31_t*)inputBuffer;
  double weighingfactor = 0;
  
  for (int i = 0; i < inputSize; i ++) {
    
    //Apply weighting
    switch (weighting_type) {

      case A_WEIGHTING: //A_WEIGHTING
        weighingfactor = A_WEIGHTINGTAB[i];
        break;

      /* case C_WEIGHTING: //C_WEIGHTING */
      /*   weighingfactor = C_WEIGHTINGTAB[i]; */
      /*   break; */
    }

    *spB *= weighingfactor;
    spB++;
  }
}

double FFTAnalyser::rms(void *inputBuffer, int inputSize, int typeRMS, int FACTOR){ 
  //typeRMS = 1 if time domain -- typeRMS = 2 if spectrum domain
  double _rmsOut = 0;
  const q31_t* _pBuffer = (const q31_t*) inputBuffer; 
 
  for (int i = 0; i < inputSize; i ++) { 
    _rmsOut += (*_pBuffer) * (*_pBuffer); 
    _pBuffer++; 
  } 
  _rmsOut = sqrt(_rmsOut/inputSize); 
  
  switch (typeRMS) {
    case 1: //TIME DOMAIN SIGNAL
      _rmsOut = _rmsOut * 1/RMS_HANN* FACTOR; 
      break;
    case 2: //SPECTRUM IN FREQ DOMAIN
      _rmsOut = _rmsOut * 1/RMS_HANN * FACTOR * sqrt(inputSize) / sqrt(2); 
      break;
    case 3:
      _rmsOut = _rmsOut * FACTOR; 
  }
  
  return _rmsOut;
} 
