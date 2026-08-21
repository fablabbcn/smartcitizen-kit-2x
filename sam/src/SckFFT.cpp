#include "SckFFT.h"
#include <Sensors.h>

#if defined(SCK_WITH_NOISE) || defined(SCK_WITH_AS7341)

#include "SckTables.h"

// Each Sck_FFT<N> needs its own tables
template<uint16_t SAMPLE_NUM_> struct FFTTables;

template<> struct FFTTables<512>
    {
    static constexpr const uint16_t *hann = hannWindow;
    static constexpr const int16_t *twiddle = twiddleCoefQ15_512;
    static constexpr const uint16_t *bitRev = armBitRevTable8;
    };

template<> struct FFTTables<128>
    {
    static constexpr const uint16_t *hann = hannWindow128;
    static constexpr const int16_t *twiddle = twiddleCoefQ15_128;
    static constexpr const uint16_t *bitRev = armBitRevTable128;
    };

template<uint16_t SAMPLE_NUM_>
void Sck_FFT<SAMPLE_NUM_>::computeSpectrum(int32_t *source, int32_t *magnitude)
{
    double divider = dynamicScale(source, scaledSource);

    applyWindow(scaledSource, FFTTables<SAMPLE_NUM_>::hann, SAMPLE_NUM);

    static int16_t ALIGN4 scratchData[SAMPLE_NUM * 2];

    // Split the data
    for (int i=0; i<SAMPLE_NUM*2; i+=2) {
        scratchData[i] = scaledSource[i/2]; // Real
        scratchData[i+1] = 0; // Imaginary
    }

    arm_radix2_butterfly(scratchData, (int16_t)SAMPLE_NUM, (int16_t *)FFTTables<SAMPLE_NUM_>::twiddle);
    arm_bitreversal(scratchData, SAMPLE_NUM, (uint16_t *)FFTTables<SAMPLE_NUM_>::bitRev);

    for (int i=0; i<SAMPLE_NUM/2; i++) {

        // Calculate result and normalize SpectrumBuffer, also revert dynamic scaling
        uint32_t myReal = pow(scratchData[i*2], 2);
        uint32_t myImg = pow(scratchData[(i*2)+1], 2);

        magnitude[i] = sqrt(myReal + myImg) * divider * 4;
    }

    // Exception for the first bin
    magnitude[0] = magnitude[0] / 2;
}
template<uint16_t SAMPLE_NUM_>
uint16_t Sck_FFT<SAMPLE_NUM_>::dominantBin(int32_t *magnitude, uint16_t skipBins)
{
    uint16_t peak = skipBins;
    for (uint16_t i=skipBins+1; i<FFT_NUM; i++) if (magnitude[i] > magnitude[peak]) peak = i;
    return peak;
}
template<uint16_t SAMPLE_NUM_>
double Sck_FFT<SAMPLE_NUM_>::modulationDepth(int32_t *magnitude, uint16_t bin)
{
    if (magnitude[0] <= 0) return 0;
    return ((double)magnitude[bin] / (double)magnitude[0]) * 100.0;
}
template<uint16_t SAMPLE_NUM_>
double Sck_FFT<SAMPLE_NUM_>::rms(int32_t *magnitude)
{
    long long rmsSum = 0;
    for (uint16_t i=0; i<FFT_NUM; i++) rmsSum += pow(magnitude[i], 2) / FFT_NUM;
    double rmsOut = sqrt((double)rmsSum);
    return rmsOut * 1 / RMS_HANN * sqrt(FFT_NUM) / sqrt(2);
}
template<uint16_t SAMPLE_NUM_>
void Sck_FFT<SAMPLE_NUM_>::minMax(int32_t *source, int32_t *min, int32_t *max)
{
    *min = source[0];
    *max = source[0];
    for (uint16_t i=1; i<SAMPLE_NUM; i++) {
        if (source[i] < *min) *min = source[i];
        if (source[i] > *max) *max = source[i];
    }
}
template<uint16_t SAMPLE_NUM_>
double Sck_FFT<SAMPLE_NUM_>::dynamicScale(int32_t *source, int16_t *scaledSource)
{
    int32_t maxLevel = 0;
    for (uint16_t i=0; i<SAMPLE_NUM; i++) if (abs(source[i]) > maxLevel) maxLevel = abs(source[i]);
    double divider = (maxLevel+1) / 32768.0; // 16 bits
    if (divider < 1) divider = 1;

    for (uint16_t i=0; i<SAMPLE_NUM; i++) scaledSource[i] = source[i] / divider;

    return divider;
}
template<uint16_t SAMPLE_NUM_>
void Sck_FFT<SAMPLE_NUM_>::applyWindow(int16_t *src, const uint16_t *window, uint16_t len)
{
    /* This code is from https://github.com/adafruit/Adafruit_ZeroFFT thank you!
        -------
        This is an FFT library for ARM cortex M0+ CPUs
        Adafruit invests time and resources providing this open source code,
        please support Adafruit and open-source hardware by purchasing products from Adafruit!
        Written by Dean Miller for Adafruit Industries. MIT license, all text above must be included in any redistribution
        ------
    */

    while (len--) {
        int32_t val = *src * *window++;
        *src = val >> 15;
        src++;
    }
}
template<uint16_t SAMPLE_NUM_>
void Sck_FFT<SAMPLE_NUM_>::arm_radix2_butterfly(int16_t * pSrc, int16_t fftLen, int16_t * pCoef)
{
    /* This code is from https://github.com/adafruit/Adafruit_ZeroFFT thank you!
        -------
        This is an FFT library for ARM cortex M0+ CPUs
        Adafruit invests time and resources providing this open source code,
        please support Adafruit and open-source hardware by purchasing products from Adafruit!
        Written by Dean Miller for Adafruit Industries. MIT license, all text above must be included in any redistribution
        ------
    */

    int i, j, k, l;
    int n1, n2, ia;
    int16_t xt, yt, cosVal, sinVal;

    n2 = fftLen;

    n1 = n2;
    n2 = n2 >> 1;
    ia = 0;

    // loop for groups
    for (j=0; j<n2; j++) {
        cosVal = pCoef[ia * 2];
        sinVal = pCoef[(ia * 2) + 1];
        ia++;

        // loop for butterfly
        for (i=j; i<fftLen; i+=n1) {
            l = i + n2;
            xt = (pSrc[2 * i] >> 2u) - (pSrc[2 * l] >> 2u);
            pSrc[2 * i] = ((pSrc[2 * i] >> 2u) + (pSrc[2 * l] >> 2u)) >> 1u;

            yt = (pSrc[2 * i + 1] >> 2u) - (pSrc[2 * l + 1] >> 2u);
            pSrc[2 * i + 1] =
                ((pSrc[2 * l + 1] >> 2u) + (pSrc[2 * i + 1] >> 2u)) >> 1u;

            pSrc[2u * l] = (((int16_t) (((int32_t) xt * cosVal) >> 16)) +
                ((int16_t) (((int32_t) yt * sinVal) >> 16)));

            pSrc[2u * l + 1u] = (((int16_t) (((int32_t) yt * cosVal) >> 16)) -
                ((int16_t) (((int32_t) xt * sinVal) >> 16)));

        }                           // butterfly loop end
    }                             // groups loop end

    uint16_t twidCoefModifier = 2;

    // loop for stage
    for (k = fftLen / 2; k > 2; k = k >> 1) {
        n1 = n2;
        n2 = n2 >> 1;
        ia = 0;

        // loop for groups
        for (j=0; j<n2; j++) {
            cosVal = pCoef[ia * 2];
            sinVal = pCoef[(ia * 2) + 1];

            ia = ia + twidCoefModifier;

            // loop for butterfly
            for (i=j; i<fftLen; i+=n1) {
                l = i + n2;
                xt = pSrc[2 * i] - pSrc[2 * l];
                pSrc[2 * i] = (pSrc[2 * i] + pSrc[2 * l]) >> 1u;

                yt = pSrc[2 * i + 1] - pSrc[2 * l + 1];
                pSrc[2 * i + 1] = (pSrc[2 * l + 1] + pSrc[2 * i + 1]) >> 1u;

                pSrc[2u * l] = (((int16_t) (((int32_t) xt * cosVal) >> 16)) +
                    ((int16_t) (((int32_t) yt * sinVal) >> 16)));

                pSrc[2u * l + 1u] = (((int16_t) (((int32_t) yt * cosVal) >> 16)) -
                    ((int16_t) (((int32_t) xt * sinVal) >> 16)));

            }                         // butterfly loop end
        }                           // groups loop end
        twidCoefModifier = twidCoefModifier << 1u;
    }                             // stages loop end

    n1 = n2;
    n2 = n2 >> 1;
    ia = 0;
    // loop for groups
    for (j=0; j<n2; j++) {
        cosVal = pCoef[ia * 2];
        sinVal = pCoef[(ia * 2) + 1];

        ia = ia + twidCoefModifier;

        // loop for butterfly
        for (i=j; i<fftLen; i+=n1) {
            l = i + n2;
            xt = pSrc[2 * i] - pSrc[2 * l];
            pSrc[2 * i] = (pSrc[2 * i] + pSrc[2 * l]);

            yt = pSrc[2 * i + 1] - pSrc[2 * l + 1];
            pSrc[2 * i + 1] = (pSrc[2 * l + 1] + pSrc[2 * i + 1]);

            pSrc[2u * l] = xt;

            pSrc[2u * l + 1u] = yt;

        }                           // butterfly loop end
    }                             // groups loop end
}
template<uint16_t SAMPLE_NUM_>
void Sck_FFT<SAMPLE_NUM_>::arm_bitreversal(int16_t * pSrc16, uint32_t fftLen, uint16_t * pBitRevTab)
{
    /* This code is from https://github.com/adafruit/Adafruit_ZeroFFT thank you!
        -------
        This is an FFT library for ARM cortex M0+ CPUs
        Adafruit invests time and resources providing this open source code,
        please support Adafruit and open-source hardware by purchasing products from Adafruit!
        Written by Dean Miller for Adafruit Industries. MIT license, all text above must be included in any redistribution
        ------
    */

    int32_t *pSrc = (int32_t *) pSrc16;
    int32_t in;
    uint32_t fftLenBy2, fftLenBy2p1;
    uint32_t i, j;

    /*  Initializations */
    j = 0u;
    fftLenBy2 = fftLen / 2u;
    fftLenBy2p1 = (fftLen / 2u) + 1u;

    /* Bit Reversal Implementation */
    for (i = 0u; i <= (fftLenBy2 - 2u); i += 2u) {
        if (i < j) {
            in = pSrc[i];
            pSrc[i] = pSrc[j];
            pSrc[j] = in;

            in = pSrc[i + fftLenBy2p1];
            pSrc[i + fftLenBy2p1] = pSrc[j + fftLenBy2p1];
            pSrc[j + fftLenBy2p1] = in;
        }

        in = pSrc[i + 1u];
        pSrc[i + 1u] = pSrc[j + fftLenBy2];
        pSrc[j + fftLenBy2] = in;

        /*  Reading the index for the bit reversal */
        j = *pBitRevTab;

        /*  Updating the bit reversal index depending on the fft length  */
        pBitRevTab++;
    }
}

// Explicit classes for noise and as7341
#ifdef SCK_WITH_NOISE
template class Sck_FFT<512>;
#endif
#ifdef SCK_WITH_AS7341
template class Sck_FFT<128>;
#endif

#endif // SCK_WITH_NOISE || SCK_WITH_AS7341
