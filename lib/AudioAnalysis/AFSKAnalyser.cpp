#include "AFSKAnalyser.h"

AFSKAnalyser::AFSKAnalyser(int bufferSize, int fftSize) :
  //BUFFER Sizes
  _fftSize(fftSize),
  _bufferSize(bufferSize),
  _sampleRate(-1),
  _bitsPerSample(-1),
  _carrierFreqSize(-1),
  _pointFreq(NULL),
  _windowSize(-1),
  // _ordRight(-1),
  _slopeRight(-1),
  // _ordLeft(-1),
  _slopeLeft(-1),
  //BUFFERs
  _sampleBuffer(NULL),
  _fftBuffer(NULL),
  _spectrumBuffer(NULL),
  _windowBuffer(NULL),
  _windowBufferFloat(NULL),
  //CHECK TEMP
  _timePostBuffer(-1)
{
}

AFSKAnalyser::~AFSKAnalyser()
{
  if (_sampleBuffer){
    free(_sampleBuffer);
  }

  if (_fftBuffer) {
    free(_fftBuffer);
  }

  if (_spectrumBuffer) {
    free(_spectrumBuffer);
  }

  if (_windowBuffer) {
    free(_windowBuffer);
  }

  if (_windowBufferFloat) {
    free(_windowBufferFloat);
  }

}

bool AFSKAnalyser::configure(int sampleRate, int bitsPerSample, int carrierFreq[], int carrierFreqSize){
  _sampleRate = sampleRate;
  _bitsPerSample = bitsPerSample;
  _carrierFreqSize = carrierFreqSize;

  //Initialise I2S
  begin(_sampleRate, _bitsPerSample);

  //Initialize fft
  if (ARM_MATH_SUCCESS != arm_rfft_init_q31(&_S31, _fftSize, 0, 1)) {
    return false;
  }

  double _freqResolution = _sampleRate/_fftSize;
  int minDeltaFreq = carrierFreq[1]-carrierFreq[0];
  //_windowSize = round(minDeltaFreq/_freqResolution);

  _windowSize = 8;
  SerialUSB.println("windowSize\t" + String(_windowSize));

  //Allocate frequency index pointers
  _pointFreq = calloc(_carrierFreqSize, sizeof(int32_t)); // Index frequency

  //Allocate time buffer
  _sampleBuffer = calloc(_bufferSize, sizeof(q31_t));
  _fftBuffer = calloc(_fftSize, sizeof(q31_t));

  //Allocate frequency buffers
  _spectrumBuffer = calloc(_fftSize/2, sizeof(q31_t));
  _windowBuffer = calloc(_windowSize, sizeof(q31_t));
  _windowBufferFloat = calloc(_windowSize, sizeof(float)); 

  // Determine the index of carrierFrequencies with respect to fftSize Vector and SampleRate
  int32_t* ptFreq = (int32_t*) _pointFreq;
  float checkCarrierFreq = 0;

  for (int i = 0; i < _carrierFreqSize; i++) {
    ptFreq[i] = round(_fftSize/2*carrierFreq[i])/(_sampleRate/2); 
    checkCarrierFreq = ptFreq[i]*_sampleRate/2/(_fftSize/2);
    SerialUSB.println(String(ptFreq[i]) + '\t'+ String(checkCarrierFreq));
  }

  //Free all buffers in case of bad allocation
  if (_sampleBuffer == NULL || _fftBuffer == NULL || _spectrumBuffer == NULL) {

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

    return false;
  }
  return true;
}

bool AFSKAnalyser::bufferFilled() {
    
  int32_t _sample = 0;
  int32_t* _buff = (int32_t*) _sampleBuffer;

  while(_bufferIndex < _bufferSize) {
    _sample = I2S.read();
    if (_sample) {
      _buff[_bufferIndex] = _sample>>7;
      _bufferIndex++;
    } else {
      return false;
    }
  }
  _timePostBuffer = micros();
  return true;
}

unsigned char AFSKAnalyser::signalGet(){

  unsigned char _signal;

  if(bufferFilled()) {

    // SerialUSB.println(micros()-_timePostBuffer);
    
    // Downscale the sample buffer for proper functioning and apply Hann window
    // scalingandwindow(_sampleBuffer, _bufferSize);
    // SerialUSB.println(micros()-_timePostBuffer);
    
    // fft 
    fft(_sampleBuffer, _spectrumBuffer, _fftSize);
    // SerialUSB.println(micros()-_timePostBuffer);
    
    // Equalise
    // equalising(_spectrumBuffer, _fftSize/2, _sampleRate);
    // SerialUSB.println(micros()-_timePostBuffer);
    
    // Detect Peak on _spectrumBuffer and assign them to the mask
    _signal = freqDetect(_spectrumBuffer, _pointFreq, _carrierFreqSize);
    // SerialUSB.println(micros()-_timePostBuffer);    
    
    SerialUSB.println(_signal);
    _bufferIndex = 0;

    //// Make the LED blink
    digitalWrite(6, LOW); //ROJO
    delay(5);
    //// Make the LED blink
    digitalWrite(6, HIGH); //ROJO

  }

  return _signal;
}

void AFSKAnalyser::fft(void *inputBuffer, void* outputBuffer, int fftBufferSize){
  //_inputBuffer is already treated for FFT (usable samples, averaged, windowed)
    
  // Calculate FFTBuffer ((r-i,r-i...))
  arm_rfft_q31(&_S31, (q31_t*)inputBuffer, (q31_t*)_fftBuffer);

  //Calculate spectrumBuffer and normalize
  const q31_t* _pfftBuffer = (const q31_t*)_fftBuffer;
  q31_t* _pspectrumBuffer = (q31_t*) outputBuffer;
    

  for (int i = 0; i < fftBufferSize; i +=2) {
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

unsigned char AFSKAnalyser::freqDetect(void* refBuffer, void* indexBuffer, int carrierFreqSize) {

  q31_t* _inputBuffer = (q31_t*) _spectrumBuffer;
  const int* _indexFreq = (const int*) indexBuffer;
  // double _iBufferDB = 0;

  unsigned char _flags = 0;
  int _powerof2 = 0;

  // Calculate stdDev
  double _stdDev = 0;
  _stdDev = standardDeviation(_inputBuffer, _fftSize/2);

  // Loop over the amount of carrier frequencies and retrieve the active ones, giving back a bitmask
  for (int i = 0; i<carrierFreqSize; i++) {

    //If the _indexFreq[i]th component in _inputBuffer is non-zero
    if (_inputBuffer[_indexFreq[i]]>_stdDev) { 
      _powerof2 = pow(2,i);
      
      if (isPeak(_inputBuffer, _indexFreq[i], _windowSize, _stdDev)) {
        _flags |= _powerof2;
      } else {
        _flags &= ~_powerof2;
      }
    
    } else {
      _flags &= ~_powerof2;
    }

  }

  return _flags;
}

bool AFSKAnalyser::isPeak (const void* inputBuffer, int index, int windowSize, double stdDev) {

  bool _isPeak = false;

  // Determine the indexes without counting the middle one
  int _indexLeft = max(0,index-windowSize/2);
  int _indexRight = min(_fftSize/2, index+windowSize/2);

  // Buffers
  q31_t* _inBuffer = (q31_t*) inputBuffer;
  q31_t* _winBuffer = (q31_t*) _windowBuffer;
  float* _winBufferFloat = (float*) _windowBufferFloat;

  // Max of window variables
  q31_t _maxWindow = _winBuffer[_indexLeft];
  int _indexMax = _indexLeft;

  //Extract the _windowBuffer and get the peak (value,index)
  for (int i = 0; i<(_indexRight-_indexLeft)+1 ; i++) {

    //Copy the value to the window
    _winBuffer[i] = _inBuffer[_indexLeft+i];

    //Check for the max value and update if needed
    if (_winBuffer[i] > _maxWindow) {
      _maxWindow = _winBuffer[i];
      _indexMax = _indexLeft + i;
    }
  }

  // If only the maximum extracted complies with the detect criteria
  if (_maxWindow>stdDev*NOISE_FACTOR) {

    //Convert to float since there is no INV matrix function for q31_t type in the arm cortex
    arm_q31_to_float(_winBuffer, _winBufferFloat, windowSize+1);
    int _mLeft = _indexMax - _indexLeft+1;

    if (_mLeft>1) {
      //Initialise Left Matrix
      float _windowLeft [_mLeft];

      for (int i = 0 ; i<_mLeft; i++){
        _windowLeft[i] = _winBuffer[i];
      }

      normalEquation(_mLeft,_windowLeft, &_slopeLeft);

    } else {
      _slopeLeft = -1;
    }

    int _mRight = _indexRight-_indexMax+1;
    // SerialUSB.println("_mRight\t" + String(_mRight));

    if (_mRight>1) {

      float _windowRight [_mRight];

      for (int i = 0; i<_mRight; i++){
        _windowRight[i] = _winBuffer[i+_mLeft-1];
      }

      normalEquation(_mRight,_windowRight, &_slopeRight);

    } else {
      _slopeRight = 1;
    }

    if (_slopeLeft>0 && _slopeRight<0) {
      _isPeak = true;
    } 
  } 

  return _isPeak;
}

// BELOW THIS LINE COULD GO TO MATH LIBRARY
void AFSKAnalyser::normalEquation(int vectorLength, float window[], void* slope) {
  // Linear regression of buffer points
  // We basicly aim to do this:
  // window = vector of points to be regressed with equation like window = ordinate + slope*index
  // vectorLength = length(window);
  // X = [ones(vectorLength, 1) [1:1:vectorLength]'];
  // index = [1:1:vectorLength]
  // Solve the normal equation INV(X'*X)*X'*window

  // float* ord = (float*) ordinate;
  float* slp = (float*) slope;

  // INITIALISE ALL MATRIXES
  float _XN [vectorLength*2]; // Indexes matrix
  float _XT [vectorLength*2]; // Transpose of XN
  float _XTM [4]; // XT*XN
  float _XTMI [4];  // Inv(XT*XN)
  float _XTMIT [vectorLength*2]; // Inv(XT*XN)*XT
  float _theta [2]; //Vector containing ordinate and theta

  arm_matrix_instance_f32 _F32XTM; // Instance for XTM
  arm_matrix_instance_f32 _F32XTMI; // Instance for XTMI
  
  // Initialise them as arm matrixes - we need this to calculate the inverse of the matrix
  arm_mat_init_f32 (&_F32XTM, 2, 2, (float*) _XTM);
  arm_mat_init_f32 (&_F32XTMI, 2, 2, (float*) _XTMI);

  // Fill XN
  for (int i = 0; i<vectorLength; i++){
    _XN[i] = 1;
    _XN[i+vectorLength] = i+1;
  }

  //Calculate Transpose (XT)
  int _counterUP = 0;
  int _counterDOWN = vectorLength;

  for (int i = 0; i < vectorLength*2; i+=2) {
    _XT[i] = _XN[_counterUP];
    _counterUP ++;
    _XT[i+1] = _XN[_counterDOWN];
    _counterDOWN ++;
  }

  //Calculate (XT) * (XN)
  int sumCol1 = 0;
  int sumCol2 = 0;

  for (int i = 0; i<2; i++) {

    for (int j = 0; j <vectorLength; j++) {
      sumCol1 += _XT[j*2+i] * _XN[j];
      sumCol2 += _XT[j*2+i] * _XN[j+vectorLength];
    }

    _XTM[i] = sumCol1;
    _XTM[i+2] = sumCol2;
    sumCol1 = 0;
    sumCol2 = 0;
  }

  //Calculate Inv((Transpose(XT) * XN)
  arm_mat_inverse_f32(&_F32XTM, &_F32XTMI);

  //Calculate (Inv(XT * XN) * XT)
  for (int j = 0; j <vectorLength*2+1; j+=2) {
    _XTMIT[j] = _XTMI[0] * _XT[j] + _XTMI[2]*_XT[j+1];
    _XTMIT[j+1] = _XTMI[1] * _XT[j] + _XTMI[3]*_XT[j+1];
  }

 
  //Calculate Theta = ((Inverse((Transpose(A) * A)) *  Transpose(A)) * B)
  // _theta [0] = 0; // Saving some time here
  _theta [1] = 0;

  for (int i = 0; i<vectorLength; i++) {
    // _theta[0] += _XTMIT[i*2]* window[i];
    _theta[1] += _XTMIT[i*2+1]* window[i];
  }

  // *ord = _theta[0];
  *slp = _theta[1];
}

double AFSKAnalyser::standardDeviation(void* inputBuffer, int bufferSize) {
  const q31_t* _inBuf = (const q31_t*) inputBuffer;
  double _stdDev = 0;
  double _avg = 0;

  _avg = average(inputBuffer, bufferSize);

  for (int i = 0; i < bufferSize; i++) {
    _stdDev += (*_inBuf-_avg)*(*_inBuf-_avg);
    _inBuf++;
  }

  _stdDev = sqrt(_stdDev/bufferSize);

  return _stdDev;
}

double AFSKAnalyser::average(void* inputBuffer, int bufferSize) {

  const q31_t* inBuf = (const q31_t*) inputBuffer;
  double _avg = 0;

  for (int i = 0; i < bufferSize; i++) {
    _avg += *inBuf;
    inBuf++;
  }

  _avg = _avg/bufferSize;

  return _avg;
}
