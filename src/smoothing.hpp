//===----------------------------------------------------------------------===//
//
// Library smoothing a 3D plot, not really a portable one unless your 
// length of the row of data is of power of two minus 2 and larger than 30
// for performing FFT with pffft
//
// Note:    fftLen is NOT the FFT sample size for convolution, but the FFT
//          sample size for generating the spectrogram
// 
//===----------------------------------------------------------------------===//

#ifndef SMOOTHING_HPP
#define SMOOTHING_HPP

/// TODO:   add nCols for original data, which must be 
///         larger than  fftLen/2 - 2 for linear convolution

/// Introducing a new row by performing FFT/Spatail based convolution smoothing.
///
/// convolution kernel is 3x3: 
///  row 0:  [[a b c]
///  row 1:   [0 0 0]
///  row 2:   [a b c]]
///
/// which is seperable [1 [a b c]
///                     0
///                     1]
///
/// \param smoothingRow     pointer to the new row made with convolution
///                         len = fftLen/2 - 2
///
/// \param row0             one of the row that contribute to the smoothing row
///                         len = fftLen/2 - 2
///
/// \param row1             one of the row that contribute to the smoothing row
///                         len = fftLen/2 - 2
///
/// \param workSmoothing    holds temporary data, 
///                         len = fftLen/2
///                         must be aligned 64 bytes
///
/// \param work0            holds temporary data, 
///                         len = fftLen/2
///                         must be aligned 64 bytes
/// 
/// \param work1            holds temporary data, 
///                         len = fftLen/2
///                         must be aligned 64 bytes
/// 
/// \param workRow          holds temporary data, 
///                         len = fftLen/2
///                         must be aligned 64 bytes
/// 
/// \param fftLen           FFT sample size for generating the spectrogram
///                         NOT the FFT sample size for convolution 
///                         (i.e. fftLen/2)
///                         this should be fftLen
///     
/// \param a                the left row-kernel value, a + b + c = 0.5
/// 
/// \param b                the middle row-kernel value, a + b + c = 0.5
/// 
/// \param c                the right row-kernel value, a + b + c = 0.5 
///
void smoothingInsertRow(float* smoothingRow,         // fftLen/2 - 2
                        const float* row0,           // fftLen/2 - 2
                        const float* row1,           // fftLen/2 - 2
                        float* workSmoothing,        // fftLen/2
                        float* work0,                // fftLen/2
                        float* work1,                // fftLen/2
                        float* workRow,              // fftLen/2
                        const int fftLen,
                        const float a = 1.0f/6.0f,
                        const float b = 1.0f/6.0f,
                        const float c = 1.0f/6.0f);


/// Fill the first n elements with half Gaussian distribution.
///
/// [p(4*sigma)  ......  p(0)]
/// scaled to sum = 1.0
///
/// \param buffer   len = n
///
/// \param n        number of elements
///
void smoothingHalfGaussian(float* buffer,
                           const int n);


/// Bluring a row of data using previous nConvRows amount of rows.
///
/// Perfrom a half Gaussian blur* on the new row for smoothing out the plot,
/// thereby increases GPU quad utilization hence increases framerate.
///
///   * half Gaussian blur kernel is provided by smoothingHalfGaussian(),
///     the user can provide their own kernel.
///
/// Kernel is seperable to [colKernel]T * [a b c]
///
/// Row direction kernel len = 3 so that we can perform FFT convolution.
///     (dataLen = fftLen/2 - 2, with DC and freq before Nyquist removed)
///     (linear convolution padding = (fftLen/2 - 2) + 3 - 1)
///
/// Column direction len depends on how much data we want to store.
///
/// \param row          pointer to row that will be blured, 
///                     len = fftLen/2 - 2
///
/// \param bluredRow    blured row, can alias parameter row
///                     len = fftLen/2 - 2
///
/// \param previousRows previously row-driection convolved rows
///                     NOT the same as fully convolved rows
///                     will get updated with new data
///                     len = (nConvRows - 1) * fftLen/2
///
/// \param workRow      holds temporary data, 
///                     len = fftLen/2
///                     must be aligned 64 bytes
///
/// \param workFFTRow   holds temporary data, 
///                     len = fftLen/2
///                     must be aligned 64 bytes
///
/// \param workConvRow  holds temporary data, 
///                     len = fftLen/2
///                     must be aligned 64 bytes
///
/// \param colKernel    array of column-direction kernel, 
///                     the last element is at blurring row
///                     array elements must sum to 1
///                     len = nConvRows
///
/// \param fftLen       FFT sample size for generating the spectrogram
///                     NOT the FFT sample size for convolution (i.e. fftLen/2)
///                     this should be fftLen
/// 
/// \param nConvRows    number of rows used for convolution
///
/// \param a            the left row-kernel value, a + b + c = 1.0
///
/// \param b            the middle row-kernel value, a + b + c = 1.0
///
/// \param c            the right row-kernel value, a + b + c = 1.0 
///
void smoothingBlurRow(const float* row,
                      float* bluredRow,
                      float* previousRows,
                      float* workRow,
                      float* workFFTRow,
                      float* workConvRow,
                      const float* colKernel,
                      const int fftLen,
                      const int nConvRows,
                      const float a = 1.0f/4.0f,
                      const float b = 1.0f/2.0f,
                      const float c = 1.0f/4.0f);


#endif 