cmake_minimum_required(VERSION 3.14)
project(Spectrolysis_test)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Added this line to get rid of FetchContent warning when using OpenMP
cmake_policy(SET CMP0135 NEW)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing with CTest
enable_testing()

# test array2d
add_executable(array2d_test array2d_test.cpp)
target_link_libraries(array2d_test PRIVATE array2d gtest gtest_main gmock)

# test pffft
add_executable(pffft_test pffft_test.cpp)
target_link_libraries(pffft_test PRIVATE pffft gtest gtest_main gmock)

# test smoothing
add_executable(smoothing_test smoothing_test.cpp)
target_link_libraries(smoothing_test PRIVATE pffft array2d smoothing)

include(GoogleTest)
gtest_discover_tests(array2d_test)
gtest_discover_tests(pffft_test)