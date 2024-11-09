//===----------------------------------------------------------------------===//
//
// FFT library with pffft (Pretty Fast Fast Fourier Transform)
// This library is for fixed length FFT defined in fftInit()
// 
//===----------------------------------------------------------------------===//

#ifndef FFT_HPP
#define FFT_HPP

/// Initialize a fft instance, currently only support one instance a time
///
/// \param fftLen   length of FFT data, must be larger than 32,
///                  and of power of 2
///
void fftInit(const int fftLen);


void fftCleanUp();


/// Return one sided FFT spectrum in complex form to outputBuffer
///
/// Must initiate an fft instance before using this methon
///
/// \param inputBuffer      input data, must be fully populated,
///                         i.e. it won't zero pad for you.
///                         (array must have a length of fftLen)
///
/// \param  outputBuffer    where the output data resides, the output data 
///                         is one-sided, interleaved complex number,
///                         aside from the first two elements which are DC 
///                         and Nyquist magnitudes
///                         values are scaled to fftLen/2 for one-sided FFT
///                         inputBuffer and outputBuffer may alias
///                         (array must have a length of fftLen)
///
/// \param workBuffer       buffer holding temporary data
///                         (array must have a length of fftLen)
///
void fftForwardFFT(const float* inputBuffer,
                   float* outputBuffer,
                   float* workBuffer);


/// Converts complex FFT data to real values
/// 
/// \param realBuffer       where the one-sided real value result resides
///                         order: [DC bin1 bin2 ... Nyquist]
///                         (array must have a length of realBufferLen)
///
/// \param complexBuffer    where the input complex array resides
///                         order: [DC Nyquist real1 imag1 real2 imag2 ...]
///                         (array must have a length of realBufferLen)
///
/// \param realBufferLen    the length of the realBuffer
///                         must be smaller or equal to fftLen/2 + 1
///                         if realBufferLen < fftLen/2 the data will only be
///                         filled to the [realBufferLen -1] element
///
/// \param normalize        normalized to 1/2N
///                         defaulted to false
///
void fftComplexToReal(float* realBuffer, 
                      const float* complexBuffer,
                      const int realBufferLen,
                      const bool normalize = true);


/// Converts complex FFT data to real values in Decibel scale
/// 
/// \param realBuffer       where the one-sided real value result resides
///                         order: [DC bin1 bin2 ...Nyquist]
///                         (array must have a length of realBufferLen)
///
/// \param complexBuffer    where the input complex array resides
///                         order: [DC Nyquist real1 imag1 real2 imag2 ...]
///                         (array must have a length of realBufferLen)
///
/// \param realBufferLen    the length of the realBuffer, 
///                         must be smaller or equal to fftLen/2 + 1
///                         if realBufferLen < fftLen/2 the data will only be
///                         filled to the [realBufferLen -1] element
///
/// \param scale            scale output to [0, 1], representing [floor, 0]dB
///
/// \param floorDB          noise floor intensity in dB, 
///                         prevent -inf for zero magnitude 
///                         automatically convert to negative
///
void fftComplexToRealDB(float* realBuffer, 
                        const float* complexBuffer,
                        const int realBufferLen,
                        const bool scale = false,
                        const float floorDB = -120);


/// Obtain the frequency binwidth
///
/// \param sampleFreq       sample frequency
///
/// \return bin width
///
float fftBinWidth(const float sampleFreq);


/// Obtain frequency of each bin from the result of fftComplexToReal()
///
/// \param freqArray        array which the output, the frequencies for each 
///                         element, will reside
///                         order: [DC bin1 bin2 ...]
///
/// \param sampleFreq       sample frequency
///
/// \param freqLen          length of freqArray
///
void fftFrequency(float* freqArray, 
                  const int sampleFreq, 
                  const int freqLen);


#endif