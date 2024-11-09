#include <cstring>
#include <iostream>
#include <iostream>

#include "../src/smoothing.hpp"
// #include "../src/array.hpp"

int main()
{
    return 0;
}

// int main() 
// {
//     constexpr int fftLen = 128;

//     // this clusterfuck is for one single smoothing row smh
//     alignas(64) float smoothingRow[fftLen/2 - 2];
//     alignas(64) float oldRow[fftLen/2 - 2];
//     alignas(64) float newRow[fftLen/2 - 2]; 
//     alignas(64) float workSmoothing[fftLen/2];
//     alignas(64) float workOld[fftLen/2];
//     alignas(64) float workNew[fftLen/2];
//     alignas(64) float workRow[fftLen/2];

//     for (unsigned int i = 0; i < fftLen; ++i)
//     {
//         oldRow[i] = 1.0f;
//         newRow[i] = 1.0f;
//     }

//     smoothRow(smoothingRow,    
//               oldRow,        
//               newRow,        
//               workSmoothing, 
//               workOld,      
//               workNew,       
//               workRow,       
//               fftLen);



//     std::vector<float> smoothRowVec(fftLen/2 - 2);
//     memcpy(&smoothRowVec[0], &smoothingRow[0], (fftLen/2 - 2) * sizeof(float));
//     array::printVector(smoothRowVec);


//     return 0;
// }

// int main() 
// {
//     constexpr int fftLen = 128;

//     // this clusterfuck is for one single smoothing row smh
//     alignas(64) float smoothingRow[fftLen/2 - 2];
//     alignas(64) float oldRow[fftLen/2 - 2];
//     alignas(64) float newRow[fftLen/2 - 2]; 
//     alignas(64) float workSmoothing[fftLen/2];
//     alignas(64) float workOld[fftLen/2];
//     alignas(64) float workNew[fftLen/2];
//     alignas(64) float workRow[fftLen/2];

//     for (unsigned int i = 0; i < fftLen; ++i)
//     {
//         oldRow[i] = 0.5f;
//         newRow[i] = 0.5f;
//     }

//     oldRow[32] = 2.0f;
//     newRow[32] = 2.0f;

//     smoothRow(smoothingRow,    
//               oldRow,        
//               newRow,        
//               workSmoothing, 
//               workOld,      
//               workNew,       
//               workRow,       
//               fftLen);



//     std::vector<float> smoothRowVec(fftLen/2 - 2);
//     memcpy(&smoothRowVec[0], &smoothingRow[0], (fftLen/2 - 2) * sizeof(float));
//     array::printVector(smoothRowVec);


//     return 0;
// }

// int main() 
// {
//     constexpr int fftLen = 128;

//     // this clusterfuck is for one single smoothing row smh
//     alignas(64) float smoothingRow[fftLen/2 - 2];
//     alignas(64) float oldRow[fftLen/2 - 2];
//     alignas(64) float newRow[fftLen/2 - 2]; 
//     alignas(64) float workSmoothing[fftLen/2];
//     alignas(64) float workOld[fftLen/2];
//     alignas(64) float workNew[fftLen/2];
//     alignas(64) float workRow[fftLen/2];

//     for (unsigned int i = 0; i < fftLen; ++i)
//     {
//         oldRow[i] = 0.5f;
//         newRow[i] = 0.5f;
//     }

//     oldRow[31] = 2.0f;
//     newRow[33] = 2.0f;

//     smoothRow(smoothingRow,    
//               oldRow,        
//               newRow,        
//               workSmoothing, 
//               workOld,      
//               workNew,       
//               workRow,       
//               fftLen);



//     std::vector<float> smoothRowVec(fftLen/2 - 2);
//     memcpy(&smoothRowVec[0], &smoothingRow[0], (fftLen/2 - 2) * sizeof(float));
//     array::printVector(smoothRowVec);


//     return 0;
// }

// // test half guassian
// int main()
// {
//     constexpr int n = 9;
//     std::vector<float> buffer(n);

//     halfGaussian(&buffer[0], n); //, stddev);
//     array::printVector(buffer);

//     float sum = 0;
//     for (unsigned i = 0; i < n; ++i)
//     {
//         sum += buffer[i];
//     }

//     std::cout << "sum: " << sum << std::endl;

//     return 0;
// }

// /// test smooth row with 1s
// /// TODO: test moveRowsUp 
// int main()
// {
//     int fftLen = 256;
//     int nConvRows = 4;

//     // the row we want to blur
//     std::vector<float> row(fftLen/2 - 2);

//     // work buffer for temporary buffers
//     alignas(64) float previousRows[(nConvRows - 1) * fftLen/2];
//     alignas(64) float workRow[fftLen/2];
//     alignas(64) float workFFTRow[fftLen/2];
//     alignas(64) float workConvRow[fftLen/2];

//     // fill in stand-in data
//     for (unsigned int i = 0; i < (nConvRows - 1) * fftLen/2; ++i)
//     {
//         previousRows[i] = 1.0f;
//     }

//     for (unsigned int i = 0; i < (fftLen/2 - 2); ++i)
//     {
//         row[i] = 1.0f;
//     }

//     // column kernel
//     float colKernel[nConvRows];
//     halfGaussian(&colKernel[0], nConvRows);

//     // blur!!
//     blurRow(&row[0],
//             &row[0],
//             &previousRows[0],
//             &workRow[0],
//             &workFFTRow[0],
//             &workConvRow[0],
//             &colKernel[0],
//             fftLen,
//             nConvRows);

//     array::printVector(row);
    
//     // std::vector<float> tempVec(fftLen/2);
//     // memcpy(&tempVec[0], &workFFTRow[0], fftLen/2 * sizeof(float));
//     // array::printVector(tempVec);

// }

// /// test smooth row with staggered rows
// /// test for move rows up
// int main()
// {
//     int fftLen = 256;
//     int nConvRows = 4;

//     // the row we want to blur
//     std::vector<float> row(fftLen/2 - 2);

//     // work buffer for temporary buffers
//     alignas(64) float previousRows[(nConvRows - 1) * fftLen/2];
//     alignas(64) float workRow[fftLen/2];
//     alignas(64) float workFFTRow[fftLen/2];
//     alignas(64) float workConvRow[fftLen/2];

//     // fill in stand-in data
//     for (unsigned int j = 0; j < fftLen/2; ++j)
//     {
//         for (unsigned int i = 0; i < (nConvRows - 1); ++i)
//         {
//             if ((i % 2) == 0) // even row
//             {
//                 previousRows[array::idx(i, j, fftLen/2)] = 1.0f;
//             }
//             else 
//             {
//                 previousRows[array::idx(i, j, fftLen/2)] = 0.0f;
//             }
//         }
//     }

//     for (unsigned int j = 0; j < (fftLen/2 - 2); ++j)
//     {
//         row[j] = 0.5f;
//     }

//     // column kernel
//     float colKernel[nConvRows];
//     halfGaussian(&colKernel[0], nConvRows);

//     // blur!!
//     blurRow(&row[0],
//             &row[0],
//             &previousRows[0],
//             &workRow[0],
//             &workFFTRow[0],
//             &workConvRow[0],
//             &colKernel[0],
//             fftLen,
//             nConvRows);

//     // array::printVector(row);

//     std::vector<float> tempVec((nConvRows - 1) * fftLen/2);
//     memcpy(&tempVec[0], &previousRows[0], ((nConvRows - 1) * fftLen/2) * sizeof(float));
//     array::printVector(tempVec);

//     // std::vector<float> tempVec(fftLen/2);
//     // memcpy(&tempVec[0], &workFFTRow[0], fftLen/2 * sizeof(float));
//     // array::printVector(tempVec);

// }