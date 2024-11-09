#include "fft.hpp"

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#include <pffft.h>

static PFFFT_Setup* s_setup = nullptr;
static int s_fftLen = 0;
  

void fftInit(const int fftLen)
{
    // initialize pffft for real to complex forward fft
    s_setup = pffft_new_setup(fftLen, PFFFT_REAL);
    s_fftLen = fftLen;
}


void fftCleanUp()
{
    pffft_destroy_setup(s_setup);
}


void fftForwardFFT(const float* inputBuffer,
                   float* outputBuffer,
                   float* workBuffer)
{
 
    // perform the forward fft, must be ordered for the result to make sense
    pffft_transform_ordered(s_setup, 
                            inputBuffer, 
                            outputBuffer, 
                            workBuffer, 
                            PFFFT_FORWARD);

}


/// TODO: maybe make sure realBufferLen don't exceed s_fftLen/2 + 1
void fftComplexToReal(float* realBuffer, 
                      const float* complexBuffer,
                      const int realBufferLen,
                      const bool normalize)
{
    // filling in the buffer with magnitude in accending frequency order
    //-------------------------------------------------------------------
    
    // complexBuffer[0] = DC
    realBuffer[0] = complexBuffer[0];
    
    // complexBuffer[1] = Nyquist
    if (realBufferLen >= s_fftLen / 2 + 1)
    {
        realBuffer[s_fftLen / 2] = complexBuffer[1];
    }
    else
    {
        // nothing: realBuffer is smaller than Nyquist frequency
    }

    // the rest of the complexBuffer is interleaved real and imaginary parts
    // for loop start with i = 1, i.e. complexBuffer[2] 
    float real;
    float imag;
    float magnitude;
    float maxMag = 0;
    for (int i = 1; i < realBufferLen; ++i)
    {
        // real and imaginary parts are interleaved in the output array
        real = complexBuffer[2 * i];
        imag = complexBuffer[2 * i + 1];
        magnitude = sqrtf(real * real + imag * imag);

        // filling in the buffer
        // normalized to 1/2N to get amplitude (1/2N because of one-sided FFT)
        realBuffer[i] = (normalize) ? magnitude/(2 * s_fftLen) : magnitude;
    }
}


/// TODO: maybe make sure realBufferLen don't exceed s_fftLen/2 + 1
/// ChatGPT o1-preview written fast version
void fftComplexToRealDB(float* realBuffer, 
                        const float* complexBuffer,
                        const int realBufferLen,
                        const bool scale,
                        const float floorDB)
{
    // Ensure floorDB is negative
    float floorDBNeg = -std::fabs(floorDB);

    const float epsilon = 1e-20f; // Small value to avoid log(0)
    const float fftLenFloat = static_cast<float>(s_fftLen);
    const float fftLenLog10 = 20.0f * log10f(fftLenFloat);
    const float fftLenSquared = fftLenFloat * fftLenFloat;

    // DC component
    float magnitudeSquared = complexBuffer[0] * complexBuffer[0];
    magnitudeSquared = fmaxf(magnitudeSquared, epsilon);
    float dB = 10.0f * log10f(magnitudeSquared) - fftLenLog10;
    dB = fmaxf(dB, floorDBNeg);
    realBuffer[0] = scale ? 1.0f - dB / floorDBNeg : dB;

    // Nyquist component
    if (realBufferLen >= s_fftLen / 2 + 1)
    {
        magnitudeSquared = complexBuffer[1] * complexBuffer[1];
        magnitudeSquared = fmaxf(magnitudeSquared, epsilon);
        dB = 10.0f * log10f(magnitudeSquared) - fftLenLog10;
        dB = fmaxf(dB, floorDBNeg);
        realBuffer[s_fftLen / 2] = scale ? 1.0f - dB / floorDBNeg : dB;
    }

    // Process the rest of the frequencies
    for (int i = 1; i < realBufferLen; ++i)
    {
        float real = complexBuffer[2 * i];
        float imag = complexBuffer[2 * i + 1];
        magnitudeSquared = real * real + imag * imag;
        magnitudeSquared = fmaxf(magnitudeSquared, epsilon);
        dB = 10.0f * log10f(magnitudeSquared) - fftLenLog10;
        dB = fmaxf(dB, floorDBNeg);
        realBuffer[i] = scale ? 1.0f - dB / floorDBNeg : dB;
    }
}


float fftBinWidth(const float sampleFreq)
{
    return sampleFreq / s_fftLen;
}


void fftFrequency(float* freqArray, 
                  const int sampleFreq, 
                  const int freqLen)
{
    float binWidth = fftBinWidth(sampleFreq);

    for (int i = 0; i < freqLen; ++i)
    {
        freqArray[i] = i * binWidth;
    }
}


// /// TODO: SLOW; Also the floor isn't really floor (will penetrate the floor)
// ///       slow mainly on side view, may due to poor quad utilization
// ///       but no DB works just fine, idk 
// void fftComplexToRealDB(float* realBuffer, 
//                         const float* complexBuffer,
//                         const int realBufferLen,
//                         const bool scale,
//                         const float floorDB)
// {
//     float real;
//     float imag;
//     float magnitude;
//     float dB;

//     // convert floor level to negative
//     floorDB = -1 * std::abs(floorDB); 

//     // filling in the buffer with magnitude in accending frequency order
//     //-------------------------------------------------------------------
//     magnitude = complexBuffer[0];
//     dB = (magnitude == 0) ? floorDB : 20 * log10f(magnitude / s_fftLen);
//     realBuffer[0] = (scale) ? 1 - dB / floorDB : dB;

//     // complexBuffer[1] = Nyquist; putting Nyquist at the last cell
//     if (realBufferLen >= s_fftLen / 2 + 1)
//     {
//         magnitude = complexBuffer[1];
//         dB = (magnitude == 0) ? floorDB : 20 * log10f(magnitude / s_fftLen);
//         realBuffer[s_fftLen/2] = (scale) ? 1 - dB / floorDB : dB;
//     }
//     else
//     {
//         // nothing: realBuffer is smaller than Nyquist frequency
//     }

//     // the rest of the complexBuffer is interleaved real and imaginary parts
//     // for loop start with i = 1, i.e. complexBuffer[2] 
//     for (int i = 1; i < realBufferLen; ++i)
//     {
//         real = complexBuffer[2 * i];
//         imag = complexBuffer[2 * i + 1];
//         magnitude = sqrtf(real * real + imag * imag);

//         // filling in the buffer
//         dB = (magnitude == 0) ? floorDB : 20 * log10f(magnitude / s_fftLen);
//         realBuffer[i] = (scale) ? 1 - dB / floorDB : dB;
//     }    
// } 
