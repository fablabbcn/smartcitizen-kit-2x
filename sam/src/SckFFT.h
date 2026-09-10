#pragma once

#include <Arduino.h>

/*
    Fixed-point real FFT engine
*/
template<uint16_t SAMPLE_NUM_>
class Sck_FFT
    {
    public:
        static const uint16_t SAMPLE_NUM = SAMPLE_NUM_;
        static const uint16_t FFT_NUM = SAMPLE_NUM_ / 2;   // usable bins: DC .. Nyquist-1

        void computeSpectrum(int32_t *source, int32_t *magnitude);

        // Index (within magnitude[FFT_NUM]) of the strongest bin
        uint16_t dominantBin(int32_t *magnitude, uint16_t skipBins = 1);
        double modulationDepth(int32_t *magnitude, uint16_t bin);

        // Hann-window-compensated RMS of the whole spectrum
        double rms(int32_t *magnitude);

        // Min and max of source[SAMPLE_NUM], for a peak-to-peak metric
        void minMax(int32_t *source, int32_t *min, int32_t *max);

    private:
        const double RMS_HANN = 0.61177;
        int16_t scaledSource[SAMPLE_NUM];

        double dynamicScale(int32_t *source, int16_t *scaledSource);
        void applyWindow(int16_t *src, const uint16_t *window, uint16_t len);
        void arm_radix2_butterfly(int16_t *pSrc, int16_t fftLen, int16_t *pCoef);
        void arm_bitreversal(int16_t *pSrc16, uint32_t fftLen, uint16_t *pBitRevTab);
    };
