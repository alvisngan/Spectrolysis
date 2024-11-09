
#include "smoothing.hpp"

#include <cstring>
#include <pffft.h>
#include <cmath>

#include "array2d.hpp" // for arr2dMoveRowsUp

void smoothingInsertRow(float* smoothingRow,     // fftLen/2 - 2
                        const float* row0,       // fftLen/2 - 2
                        const float* row1,       // fftLen/2 - 2
                        float* workSmoothing,    // fftLen/2
                        float* work0,            // fftLen/2
                        float* work1,            // fftLen/2
                        float* workRow,          // fftLen/2
                        const int fftLen,
                        const float a,
                        const float b,
                        const float c)
{
    // convolution kernel is 3x3: 
    //      row 0:  [[a b c]
    //      row 1:   [0 0 0]
    //      row 2:   [a b c]]
    //
    // kernel is separable
    // we can perform convolution on the row with kernel [a b c]
    // and convolution on the columns with kernel [1; 0; 1]
    //
    // row 0 and row 2 represent the new and old rows of data respectively
    // row 1, the smoothing row, 
    //        is all zero because it is a new row with no data
    //
    // zero-padding for linear convolution
    // -----------------------------------
    // new and old row have length fftLen/2 - 2
    // minus two because we are ignoring DC and the freq Nyquist when plotting
    // copying the new and last rows to work buffers
    memcpy(work0, row0, (fftLen/2 - 2) * sizeof(float));
    memcpy(work1, row1, (fftLen/2 - 2) * sizeof(float));

    // setting the last two cells of the work buffers as zero, for zero-padding 
    memset(&work0[fftLen/2 - 2], 0, 2 * sizeof(float));
    memset(&work1[fftLen/2 - 2], 0, 2 * sizeof(float));

    // zero-padding the row kernel
    workSmoothing[0] = a;
    workSmoothing[1] = b;
    workSmoothing[2] = c;
    memset(&workSmoothing[3], 0, (fftLen/2 - 3) * sizeof(float)); // zero's

    // FFT based convolution on the row direction
    // ------------------------------------------
    // FFT on each row (row 1 can be skipped since it's all zero)
    // real to complex FFT
    PFFFT_Setup* rowFFT = pffft_new_setup(fftLen/2, PFFFT_REAL);

    // pffft_transform input and output may alias, thank god!
    pffft_transform(rowFFT, work0, work0, workRow, PFFFT_FORWARD);
    pffft_transform(rowFFT, work1, work1, workRow, PFFFT_FORWARD);
    pffft_transform(rowFFT, workSmoothing, workSmoothing, workRow, PFFFT_FORWARD);

    // performing convolution on the two rows using the convolution theorem
    // pffft convolution accumulate results: dft_ab += (dft_a * fdt_b)*scaling
    // while, I can do input/output alias, that will acculmulate output to input
    // so, I need to do some copying with the workRow

    memset(&workRow[0], 0, (fftLen/2)*sizeof(float)); // clear the work row 

    pffft_zconvolve_accumulate(rowFFT, 
                            work0, 
                            workSmoothing,
                            workRow,
                            2.0f / (fftLen)); // this scaling seems to
                                                // account fot both forward
                                                // FFT's and the inverse FFT

    memcpy(&work0[0], &workRow[0], (fftLen/2)*sizeof(float));
    memset(&workRow[0], 0, (fftLen/2)*sizeof(float)); // clear the work row 

    pffft_zconvolve_accumulate(rowFFT, 
                            work1, 
                            workSmoothing,
                            workRow,
                                2.0f / (fftLen));

    memcpy(&work1[0], &workRow[0], (fftLen/2)*sizeof(float));

    // inverse FFT to get convolution result (complex to real)
    // inverse FFT needs to be scaled again!
    pffft_transform(rowFFT, work0, work0, workRow, PFFFT_BACKWARD);
    pffft_transform(rowFFT, work1, work1, workRow, PFFFT_BACKWARD);

    // finally done with FFT
    pffft_destroy_setup(rowFFT);


    // Spatial based convolution on the column direction
    // -------------------------------------------------
    // using spatial convolution since there are only two rows
    // finding the sum of each column elements will result
    // in effectively 2D convolution without doing more costly FFT
    // (sum because comlumn kernel has value [1; 0; 1])
    #pragma omp simd
    for (unsigned int i = 0; i < fftLen/2; ++i)
    {
        workSmoothing[i] = (work1[i] + work0[i]);
    }
    
    // result
    // ------
    // ignore the last two elements because they are paddings
    // copy here because complier auto SIMD the previous loop (hopefully)
    memcpy(smoothingRow, &workSmoothing[1], (fftLen/2 - 2) * sizeof(float));
}


void smoothingHalfGaussian(float* buffer, // len = n
                           const int n)
{
    float sum = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        // flip and scale to [0, 4*sigma]
        // sigma will canceled out later, no need to include it
        float x = 4 * (float)(n - i - 1) / (float)(n - 1);

        // Gaussian distribution centered at zero; scale later
        float prob = expf((-(x * x) / 2));
        buffer[i] = prob;

        sum += prob;
    }

    // scale to 1.0
    for (int i = 0; i < n; ++i)
    {
        buffer[i] /= sum;
    }
}


void smoothingBlurRow(const float* row,
                      float* bluredRow,
                      float* previousRows,
                      float* workRow,
                      float* workFFTRow,
                      float* workConvRow,
                      const float* colKernel,
                      const int fftLen,
                      const int nConvRows,
                      const float a,
                      const float b,
                      const float c)
{
    // perform FFT convolution on the row
    // ----------------------------------
    // populating and zero padding the row convolution buffer
    workConvRow[0] = a;
    workConvRow[1] = b;
    workConvRow[2] = c;
    memset(&workConvRow[3], 0, (fftLen/2 - 3) * sizeof(float));

    // zero-padding the row to workRow
    memcpy(&workRow[0], &row[0], (fftLen/2 - 2)*sizeof(float));
    memset(&workRow[fftLen/2 - 2], 0, 2 * sizeof(float));

    PFFFT_Setup* rowFFT = pffft_new_setup(fftLen/2, PFFFT_REAL);
    pffft_transform(rowFFT, workRow, workRow, workFFTRow, PFFFT_FORWARD);
    pffft_transform(rowFFT, workConvRow, workConvRow, workFFTRow, PFFFT_FORWARD);

    // performing convolution on the two rows using the convolution theorem
    // pffft convolution accumulate results: dft_ab += (dft_a * dft_b)*scaling
    // while, I can do input/output alias, that will acculmulate output to input
    // so, I need to do some copying with the workFFTRow
    memset(&workFFTRow[0], 0, (fftLen/2)*sizeof(float)); // clear the FFT row 

    pffft_zconvolve_accumulate(rowFFT, 
                                workRow, 
                                workConvRow, 
                                workFFTRow, // result is here
                                2.0f / (fftLen)); // this scaling seems to
                                                // account fot both forward
                                                // FFT's and the inverse FFT
    // inverse FFT:
    //      input:  workFFTRow  (stored convolved frequency domain data)
    //      output: workRow
    //      temp:   workConvRow (stored row kernel data, free to use now)
    pffft_transform(rowFFT, workFFTRow, workRow, workConvRow, PFFFT_BACKWARD);
    pffft_destroy_setup(rowFFT);
    
    // convolve the columns
    // --------------------

    // use a random useless work row to store the fully convolved data
    memset(&workFFTRow[0], 0, (fftLen/2) * sizeof(float));

    #pragma omp simd // auto SIMD maybe?
    for (int j = 0; j < fftLen/2; ++j)
    {
        // previous rows
        for (int i = 0; i < nConvRows - 1; ++i)
        {
            // earlier rows are stored at the top (lower i)
            workFFTRow[j] += (colKernel[i] * 
                                previousRows[array2dIdx(i, j, fftLen/2)]);
        }

        // current row
        workFFTRow[j] += colKernel[nConvRows - 1] * workRow[j];
    }

    // done with convolution, now manage data
    // --------------------------------------
    // stored to row convolved (but not column convlved) data
    // in the previousRows row-major array for later use
    // moveRowsUp func: row 1 ==> row 0; row 2 ==> row 1
    // (note: previousRows has dimension nConvRows-1 x fftLen/2)
    array2dMoveRowsUp(&previousRows[0], nConvRows-1, fftLen/2, 1);
    memcpy(&previousRows[array2dIdx(nConvRows-2, 0, fftLen/2)],
            &workRow[0], // row only convolution
            (fftLen/2) * sizeof(float));

    // copy the work data back to output
    memcpy(&bluredRow[0], &workFFTRow[1], (fftLen/2 - 2) * sizeof(float));
}


