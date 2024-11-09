#include <vector>
#include <gtest/gtest.h>
#include "gmock/gmock.h"
#include "../src/array2d.hpp"

TEST(Array2DTest, IdxTest)
{
    EXPECT_EQ(array2dIdx(1, 2, 3), 5);
    EXPECT_EQ(array2dIdx(5, 6, 9), 51);
    EXPECT_EQ(array2dIdx(0, 0, 61), 0);
}


TEST(Array2DTest, MoveRowsUpTest)
{
    const unsigned int nRows = 6;
    const unsigned int nCols = 4;
    const float tolerance = 0.01f;

    float array[] = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20,
        21, 22, 23, 24
    };

    std::vector<float> vector = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20,
        21, 22, 23, 24        
    };

    const float expectedArray[] = {
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20,
        21, 22, 23, 24,
        21, 22, 23, 24  
    };

    // modifies the input arrays
    array2dMoveRowsUp(array, nRows, nCols, 1);
    array2dMoveRowsUp(vector.data(), nRows, nCols, 1);
    // moveRowsUp(pointer, nRows, nCols);

    EXPECT_THAT(
        array,
        testing::Pointwise(testing::FloatNear(tolerance), expectedArray)
    );

    EXPECT_THAT(
        vector,
        testing::Pointwise(testing::FloatNear(tolerance), expectedArray)
    );

    // multiple rows
    float multiRowsArray[] = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20,
        21, 22, 23, 24
    };

    const float expectedMultiRowsArray[] = {
        9, 10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20,
        21, 22, 23, 24,
        17, 18, 19, 20,
        21, 22, 23, 24  
    };

    array2dMoveRowsUp(multiRowsArray, nRows, nCols, 2);

    EXPECT_THAT(
        multiRowsArray,
        testing::Pointwise(testing::FloatNear(tolerance), 
                           expectedMultiRowsArray)
    );
}

TEST(Array2DTest, ElementIndicesTest)
{
    // 1x1 cell
    std::vector<int> indices1x1(6);
    array2dElementIndices(indices1x1.data(), 1, 1);
    const std::vector<int> expected1x1 = {1, 0, 2, 2, 3, 1};

    EXPECT_EQ(indices1x1, expected1x1);

    // 3x4 cells
    std::vector<int> indices3x4(3 * 4 * 6);
    array2dElementIndices(indices3x4.data(), 3, 4);
    const std::vector<int> expected3x4 = {
         1,  0,  5,   5,  6,  1,
         2,  1,  6,   6,  7,  2,
         3,  2,  7,   7,  8,  3,
         4,  3,  8,   8,  9,  4,
         6,  5, 10,  10, 11,  6,
         7,  6, 11,  11, 12,  7,
         8,  7, 12,  12, 13,  8,
         9,  8, 13,  13, 14,  9,
        11, 10, 15,  15, 16, 11,
        12, 11, 16,  16, 17, 12,
        13, 12, 17,  17, 18, 13,
        14, 13, 18,  18, 19, 14
    };

    EXPECT_EQ(indices3x4, expected3x4);
}

TEST(Array2DTest, PatchIndicesTest)
{
    // 1x1 cell
    std::vector<int> indices1x1(4);
    array2dPatchIndices(indices1x1.data(), 1, 1, ARRAY2D_CCW);
    std::vector<int> expected1x1 = {1, 0, 2, 3};

    EXPECT_EQ(indices1x1, expected1x1);

    // 3x4 cells
    std::vector<int> indices3x4(48);
    array2dPatchIndices(indices3x4.data(), 3, 4, ARRAY2D_CCW);
    std::vector<int> expected3x4 = {
        1, 0, 5, 6,
        2, 1, 6, 7,
        3, 2, 7, 8,
        4, 3, 8, 9,
        6, 5, 10, 11,
        7, 6, 11, 12,
        8, 7, 12, 13,
        9, 8, 13, 14,
        11, 10, 15, 16,
        12, 11, 16, 17,
        13, 12, 17, 18,
        14, 13, 18, 19
    };

    EXPECT_EQ(indices3x4, expected3x4);

    // 1x1 cell with OpenGL tessellation ordering
    std::vector<int> indices1x1GL(4);
    array2dPatchIndices(indices1x1GL.data(), 1, 1, ARRAY2D_GL_CCW);
    std::vector<int> expected1x1GL = {0, 1, 2, 3};

    EXPECT_EQ(indices1x1GL, expected1x1GL);

    // 3x4 cells with OpenGL tessellation ordering
    std::vector<int> indices3x4GL(48);
    array2dPatchIndices(indices3x4GL.data(), 3, 4, ARRAY2D_GL_CCW);
    std::vector<int> expected3x4GL = {
        0, 1, 5, 6,
        1, 2, 6, 7,
        2, 3, 7, 8,
        3, 4, 8, 9,
        5, 6, 10, 11,
        6, 7, 11, 12,
        7, 8, 12, 13,
        8, 9, 13, 14,
        10, 11, 15, 16,
        11, 12, 16, 17,
        12, 13, 17, 18,
        13, 14, 18, 19
    };

    EXPECT_EQ(indices3x4GL, expected3x4GL);
}

TEST(Array2DTest, GridTest)
{
    const float tolerance = 0.01f;

    // interleaved 6x7 vertices
    // ------------------------
    std::vector<float> result(6 * 7 * 2);
    array2dGrid(result.data(), 6, 7);

    std::vector<float> expected = {
        // row 1
        -1.0f, 1.0f,  -0.666f, 1.0f,  -0.333f, 1.0f,  0.0f, 1.0f,
        0.333f, 1.0f,  0.666f, 1.0f,  1.0f, 1.0f, 

        // row 2
        -1.0f, 0.6f,  -0.666f, 0.6f,  -0.333f, 0.6f,  0.0f, 0.6f,
        0.333f, 0.6f,  0.666f, 0.6f,  1.0f, 0.6f,

        // row 3
        -1.0f, 0.2f,  -0.666f, 0.2f, -0.333f, 0.2f,  0.0f, 0.2f,
        0.333f, 0.2f,  0.666f, 0.2f,  1.0f, 0.2f,

        // row 4
        -1.0f, -0.2f,  -0.666f, -0.2f, -0.333f, -0.2f,  0.0f, -0.2f,
        0.333f, -0.2f,  0.666f, -0.2f,  1.0f, -0.2f,

        // row 5
        -1.0f, -0.6f,  -0.666f, -0.6f,  -0.333f, -0.6f,  0.0f, -0.6f,
        0.333f, -0.6f,  0.666f, -0.6f,  1.0f, -0.6f,

        // row 6
        -1.0f, -1.0f,  -0.666f, -1.0f,  -0.333f, -1.0f,  0.0f, -1.0f,
        0.333f, -1.0f,  0.666f, -1.0f,  1.0f, -1.0f         
    };

    EXPECT_THAT(
        result,
        testing::Pointwise(testing::FloatNear(tolerance), expected)
    );

    // interleaved 3x7 vertices with uv map
    // ------------------------------------
    std::vector<float> resultUV(3 * 7 *4);
    array2dGrid(resultUV.data(), 3, 7, true);

    std::vector<float> expectedUV = { 
        // row 1
        -1.0f, 1.0f, 0.0f, 1.0f,  -0.666f, 1.0f, 0.167f, 1.0f, 
        -0.333f, 1.0f, 0.333f, 1.0f,  0.0f, 1.0f, 0.5f, 1.0f,
        0.333f, 1.0f, 0.666f, 1.0f,  0.666f, 1.0f, 0.833f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,

        // row 2
        -1.0f, 0.0f, 0.0f, 0.5f,  -0.666f, 0.0f, 0.167f, 0.5f, 
        -0.333f, 0.0f, 0.333f, 0.5f,  0.0f, 0.0f, 0.5f, 0.5f,
        0.333f, 0.0f, 0.666f, 0.5f,  0.666f, 0.0f, 0.833f, 0.5f,
        1.0f, 0.0f, 1.0f, 0.5f,

        // row 3
        -1.0f, -1.0f, 0.0f, 0.0f,  -0.666f, -1.0f, 0.167f, 0.0f, 
        -0.333f, -1.0f, 0.333f, 0.0f,  0.0f, -1.0f, 0.5f, 0.0f,
        0.333f, -1.0f, 0.666f, 0.0f,  0.666f, -1.0f, 0.833f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f
    };

    EXPECT_THAT(
        resultUV,
        testing::Pointwise(testing::FloatNear(tolerance), expectedUV)
    ); 
}


TEST(Array2DTest, GridBatchedTest)
{
    const float tolerance = 0.01f;

    // batched 6x7 vertices
    // --------------------
    std::vector<float> bResult(6 * 7 * 2);
    array2dGridBatched(bResult.data(), 6, 7);

    std::vector<float> bExpected = {
        // x
        -1.0f, -0.666f, -0.333f, 0.0f, 0.333f, 0.666f, 1.0f, // row 1
        -1.0f, -0.666f, -0.333f, 0.0f, 0.333f, 0.666f, 1.0f,
        -1.0f, -0.666f, -0.333f, 0.0f, 0.333f, 0.666f, 1.0f,
        -1.0f, -0.666f, -0.333f, 0.0f, 0.333f, 0.666f, 1.0f,
        -1.0f, -0.666f, -0.333f, 0.0f, 0.333f, 0.666f, 1.0f,
        -1.0f, -0.666f, -0.333f, 0.0f, 0.333f, 0.666f, 1.0f,
        
        // y
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // row 1
        0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f,
        0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f,
        -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f,
        -0.6f, -0.6f, -0.6f, -0.6f, -0.6f, -0.6f, -0.6f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f
    };

    EXPECT_THAT(
        bResult,
        testing::Pointwise(testing::FloatNear(tolerance), bExpected)
    );

    // batched 3x7 vertices with uv map
    // --------------------------------
    std::vector<float> bResultUV(3 * 7 * 4);
    array2dGridBatched(bResultUV.data(), 3, 7, true);
    
    std::vector<float> bExpectedUV = {
        // x
        -1.0f, -0.666f, -0.333f, 0.0f, 0.333f, 0.666f, 1.0f, // row 1
        -1.0f, -0.666f, -0.333f, 0.0f, 0.333f, 0.666f, 1.0f,
        -1.0f, -0.666f, -0.333f, 0.0f, 0.333f, 0.666f, 1.0f,
        
        // y
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // row 1
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,

        // u
        0.0f, 0.167f, 0.333f, 0.5f, 0.666f, 0.833f, 1.0f, // row 1
        0.0f, 0.167f, 0.333f, 0.5f, 0.666f, 0.833f, 1.0f,
        0.0f, 0.167f, 0.333f, 0.5f, 0.666f, 0.833f, 1.0f,   

        // v
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // row 1
        0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };

    EXPECT_THAT(
        bResultUV,
        testing::Pointwise(testing::FloatNear(tolerance), bExpectedUV)
    );
}