#include <pffft.h>
#include <gtest/gtest.h>
#include "gmock/gmock.h"
#include <iostream>
#include <vector>
#include <cmath>
// #include "../src/array.hpp" // for the printVector function

/// simple forward fft test (bin width = 1Hz)
///
/// \param fftLen must be a power of 2 & larger than 32,
///                equals to sampling frequency in Hz
/// \param  waveFreq test signal frequency in Hz; integer for now
void basicFowardFFTTest(int fftLen, int waveFreq)
{
    // for testing
    const float tolerance = 0.1f;

    // initialize pffft for real to complex forward fft
    PFFFT_Setup* setup = pffft_new_setup(fftLen, PFFFT_REAL);

    // allocate aligned memory for input, output, and temporary buffer
    float* input = (float*)pffft_aligned_malloc(fftLen * sizeof(float));
    float* output = (float*)pffft_aligned_malloc(fftLen * sizeof(float)); // TODO: check if complex needs extra memory, as do work
    float* work = (float*)pffft_aligned_malloc(fftLen * sizeof(float));

    // fill input data with simple sine wave
    for (int i = 0; i < fftLen; ++i) 
    {
        // fftLen = samplingFreq
        input[i] = sinf(2.0f * M_PI * waveFreq * i / fftLen); 
    }

    // perform the forward fft, must be ordered for the result to make sense
    pffft_transform_ordered(setup, input, output, work, PFFFT_FORWARD);

    // store resultant magnitudes in a std::vector for testing
    std::vector<float> outputVec(fftLen/2 - 1); // Nyquist Frequency

    // compute the magnitude of each frequency, and assigning it to vector
    for (int i = 0; i < fftLen/2 - 1; ++i) 
    {
        // real and imaginary parts are interleaved in the output array
        float real = output[2 * i];
        float imag = output[2 * i + 1];

        float magnitude = sqrtf(real * real + imag * imag);

        outputVec[i] = magnitude;
    }

    // clean up resources
    pffft_aligned_free(input);
    pffft_aligned_free(output);
    pffft_aligned_free(work);
    pffft_destroy_setup(setup);

    // expected result std::vector
    std::vector<float> expectedVec(fftLen/2 - 1, 0.0f);

    // fftLen / 2 because we are only looking at one side of the FFT spectrum
    expectedVec[waveFreq] = static_cast<float>(fftLen) / 2;

    EXPECT_THAT(
        outputVec,
        testing::Pointwise(testing::FloatNear(tolerance), expectedVec)
    );
}

TEST(PffftTest, BasicFowardFFTTest)
{
    // testing 2^5=32 to 2^15
    std::vector<int> fftLen(15);
    
    // size must be divisible by 32
    for (int i = 4; i < 15; i++)
    {
        fftLen[i] = std::pow(2, i + 1);
    }
    
    // test signal frequency in Hz, a random integer for now
    const int waveFreq = 9; 

    for (int i = 4; i < 15; i++)
    {
        basicFowardFFTTest(fftLen[i], waveFreq);
    }
}

/// forward fft test with floating point sample and test frequency
///
/// \param fftLen number of floating point elements,
///                must be a power of 2 & larger than 32,
///                equals to sampling frequency in Hz
/// \param sampleFreq sampling frequency or rate in Hz
/// \param  waveFreq test signal frequency in Hz
void fowardFFTTest(int fftLen, float sampleFreq, float waveFreq)
{
    // for testing
    const float tolerance = 0.1f;

    // initialize pffft for real to complex forward fft
    PFFFT_Setup* setup = pffft_new_setup(fftLen, PFFFT_REAL);

    // allocate aligned memory for input, output, and temporary buffer
    float* input = (float*)pffft_aligned_malloc(fftLen * sizeof(float));
    float* output = (float*)pffft_aligned_malloc(fftLen * sizeof(float));
    float* work = (float*)pffft_aligned_malloc(fftLen * sizeof(float));

    // fill input data with simple sine wave
    for (int i = 0; i < fftLen; ++i) 
    {
        input[i] = sinf(2.0f * M_PI * waveFreq * i / sampleFreq);
    }

    // perform the forward fft, must be ordered for the result to make sense
    pffft_transform_ordered(setup, input, output, work, PFFFT_FORWARD);

    float binWidth = sampleFreq / fftLen;

    std::vector<int> idxNearest;
    std::vector<float> magNearest;

    // saving indices and magnitude for later comparison
    for (int i = 0; i < fftLen/2 - 1; ++i) 
    {
        // the magnitude closest to test signal frequency should be
        // much larger than the frequencies futher away
        if (abs(i * binWidth - waveFreq) < binWidth)
        {
            // real and imaginary parts are interleaved in the output array
            float real = output[2 * i];
            float imag = output[2 * i + 1];
            float magnitude = sqrtf(real * real + imag * imag);

            // saving indices and magnitudes for later comparison
            idxNearest.push_back(i);
            magNearest.push_back(magnitude);
        }
        else
        {
            // nothing
        }
    }

    // test only region near the test signal frequency
    for (int i = 0; i < 2; ++i)
    {
        // let far away frequency = 7 * binWidth (no particular reasons)   
        int idxFar = idxNearest[i] + 7 * std::pow(-1, i + 1); // -7 and 7

        // real and imaginary parts are interleaved in the output array
        float real = output[2 * idxFar];
        float imag = output[2 * idxFar + 1];
        float magnitude = sqrtf(real * real + imag * imag);

        // doing comparisons immediately
        float minFactor = 30.0f; // the magnitude must be minFactor times larger

        EXPECT_TRUE(magnitude < minFactor * magNearest[i]);
    }

    // clean up resources
    pffft_aligned_free(input);
    pffft_aligned_free(output);
    pffft_aligned_free(work);
    pffft_destroy_setup(setup);
}

TEST(PffftTest, FowardFFTTest)
{
    // testing 2^5=32 to 2^15
    std::vector<int> fftLen(15);
    
    // size must be divisible by 32
    for (int i = 4; i < 15; i++)
    {
        fftLen[i] = std::pow(2, i + 1);
    }
    
    // test signal frequency in Hz
    const float waveFreq = 3090.0f; 

    for (int i = 4; i < 15; i++)
    {
        fowardFFTTest(fftLen[i], 44100.0f, waveFreq);
    }
}


/// forward fft test with floating point sample and test frequency
/// with a DC component and a test signal at Nyquist frequency
///
/// \param fftLen number of floating point elements,
///                must be a power of 2 & larger than 32,
///                equals to sampling frequency in Hz
/// \param sampleFreq sampling frequency or rate in Hz
/// \param  waveFreq test signal frequency in Hz
/// \param waveDC DC component amplitude
void fowardDCFFTTest(int fftLen, 
                     float sampleFreq, 
                     float waveFreq, 
                     float waveDC)
{
    // for testing
    const float tolerance = 0.1f;

    // initialize pffft for real to complex forward fft
    PFFFT_Setup* setup = pffft_new_setup(fftLen, PFFFT_REAL);

    // allocate aligned memory for input, output, and temporary buffer
    float* input = (float*)pffft_aligned_malloc(fftLen * sizeof(float));
    float* output = (float*)pffft_aligned_malloc(fftLen * sizeof(float));
    float* work = (float*)pffft_aligned_malloc(fftLen * sizeof(float));

    // fill input data with simple sine wave
    for (int i = 0; i < fftLen; ++i) 
    {
        // Add a DC component and a signal at Nyquist frequency
        input[i] = waveDC + cosf(2.0f * M_PI * waveFreq * i / sampleFreq)
                   + cosf(M_PI * sampleFreq * i / sampleFreq);
    }

    // perform the forward fft, must be ordered for the result to make sense
    pffft_transform_ordered(setup, input, output, work, PFFFT_FORWARD);

    float binWidth = sampleFreq / fftLen;

    std::vector<int> idxNearest;
    std::vector<float> magNearest;

    // saving indices and magnitude for later comparison
    for (int i = 0; i < fftLen/2 - 1; ++i) 
    {
        // the magnitude closest to test signal frequency should be
        // much larger than the frequencies futher away
        if (abs(i * binWidth - waveFreq) < binWidth)
        {
            // real and imaginary parts are interleaved in the output array
            float real = output[2 * i];
            float imag = output[2 * i + 1];
            float magnitude = sqrtf(real * real + imag * imag);

            // saving indices and magnitudes for later comparison
            idxNearest.push_back(i);
            magNearest.push_back(magnitude);
        }
        else
        {
            // nothing
        }
    }

    float minFactor = 30.0f; // the magnitude must be minFactor times larger
    // test only region near the test signal frequency
    for (int i = 0; i < 2; ++i)
    {
        // let far away frequency = 7 * binWidth (no particular reasons)   
        int idxFar = idxNearest[i] + 7 * std::pow(-1, i + 1); // -7 and 7

        // real and imaginary parts are interleaved in the output array
        float real = output[2 * idxFar];
        float imag = output[2 * idxFar + 1];
        float magnitude = sqrtf(real * real + imag * imag);

        // doing comparisons immediately
        EXPECT_TRUE(magnitude < minFactor * magNearest[i]);
    }

    // test DC component and Nyquist component
    // DC is stored at output[0]
    EXPECT_TRUE(minFactor * output[0] > output[idxNearest[0] + 7]);
    // Nyquist is stored at output[1]
    EXPECT_TRUE(minFactor * output[1] > output[idxNearest[0] + 7]);

    // clean up resources
    pffft_aligned_free(input);
    pffft_aligned_free(output);
    pffft_aligned_free(work);
    pffft_destroy_setup(setup);
}

TEST(PffftTest, FowardDCFFTTest)
{
    // testing 2^5=32 to 2^15
    std::vector<int> fftLen(15);
    
    // size must be divisible by 32
    for (int i = 4; i < 15; i++)
    {
        fftLen[i] = std::pow(2, i + 1);
    }
    
    // test signal frequency in Hz
    const float waveFreq = 3090.0f; 

    for (int i = 4; i < 15; i++)
    {
        fowardDCFFTTest(fftLen[i], 44100.0f, waveFreq, 2.0f);
    }
}

/// forward fft test with floating point sample and test frequency
/// and pointers to an input, output, and work array without malloc
///
/// \param fftLen number of floating point elements,
///                must be a power of 2 & larger than 32,
///                equals to sampling frequency in Hz
/// \param input pointer to the input array
/// \param output pointer to the output array
/// \param work pointer to the temporary work buffer
/// \param sampleFreq sampling frequency or rate in Hz
/// \param  waveFreq test signal frequency in Hz
void fowardArrFFTTest(int fftLen, 
                      float* input, 
                      float* output,
                      float* work,
                      float sampleFreq,
                      float waveFreq)
{
    // for testing
    const float tolerance = 0.1f;

    // initialize pffft for real to complex forward fft
    PFFFT_Setup* setup = pffft_new_setup(fftLen, PFFFT_REAL);

    // perform the forward fft, must be ordered for the result to make sense
    pffft_transform_ordered(setup, input, output, work, PFFFT_FORWARD);

    float binWidth = sampleFreq / fftLen;

    std::vector<int> idxNearest;
    std::vector<float> magNearest;

    // saving indices and magnitude for later comparison
    for (int i = 0; i < fftLen/2 - 1; ++i) 
    {
        // the magnitude closest to test signal frequency should be
        // much larger than the frequencies futher away
        if (abs(i * binWidth - waveFreq) < binWidth)
        {
            // real and imaginary parts are interleaved in the output array
            float real = output[2 * i];
            float imag = output[2 * i + 1];
            float magnitude = sqrtf(real * real + imag * imag);

            // saving indices and magnitudes for later comparison
            idxNearest.push_back(i);
            magNearest.push_back(magnitude);
        }
        else
        {
            // nothing
        }
    }

    // test only region near the test signal frequency
    for (int i = 0; i < 2; ++i)
    {
        // let far away frequency = 7 * binWidth (no particular reasons)   
        int idxFar = idxNearest[i] + 7 * std::pow(-1, i + 1); // -7 and 7

        // real and imaginary parts are interleaved in the output array
        float real = output[2 * idxFar];
        float imag = output[2 * idxFar + 1];
        float magnitude = sqrtf(real * real + imag * imag);

        // doing comparisons immediately
        float minFactor = 30.0f; // the magnitude must be minFactor times larger

        EXPECT_TRUE(magnitude < minFactor * magNearest[i]);
    }

    // clean up resources
    pffft_destroy_setup(setup);
}

/// TODO: needs aligned arrays
// TEST(PffftTest, FowardArrFFTTest)
// {
//     // testing 2^5=32 to 2^15
//     std::vector<int> fftLen(15);
    
//     // size must be divisible by 32
//     for (int i = 4; i < 15; i++)
//     {
//         fftLen[i] = std::pow(2, i + 1);
//     }
    
//     // test signal frequency in Hz
//     const float waveFreq = 3090.0f; 
//     const float sampleFreq = 44100.0f;

//     // loop through fftLen[]
//     for (int i = 4; i < 15; i++)
//     {
//         // create input array
//         std::vector<float> input(fftLen[i]); 
//         std::vector<float> output(fftLen[i]);
//         std::vector<float> work(fftLen[i]);

//         // fill input data with simple sine wave
//         for (int j = 0; j < fftLen[i]; ++j) 
//         {
//             input[i] = sinf(2.0f * M_PI * waveFreq * j / sampleFreq);
//         }

//         fowardArrFFTTest(fftLen[i], 
//                          &input[0], &output[0], &work[0], 
//                          sampleFreq, waveFreq);
//     }
// }
