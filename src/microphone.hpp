//===----------------------------------------------------------------------===//
//
// Library for real-time sound recording into float-32
//
// Made with SDL2
// 
//===----------------------------------------------------------------------===//
#ifndef MICROPHONE_HPP
#define MICROPHONE_HPP

#include <vector>
#include <string>
#include <mutex>

#include <SDL2/SDL.h>

class Microphone
{
public:
    /// \param maxRecordingSec  buffer size, won't limit the recording time;
    ///                         however it only holds maxRecordingSec of data
    ///                         in the buffer should the user wants to save it
    ///
    Microphone(int maxRecordingSec = 300);
    ~Microphone();

    /// Start recording and start filling in the ring buffer
    ///
    void record();

    /// Pause recording and fill ring buffer with zero
    /// 
    /// Note:   Pause logic is implemented in the main loop such that 
    //          pause means no data is filled in the buffer
    ///
    void pause();

    /// \return     whether the recording is pauseed
    ///
    bool getIsPaused();

    /// \return     recording device sample frequency in Hz
    ///
    int getFreq();

    /// Fill in a buffer array with floating point audio samples from the 
    /// current recording point to user specified number of samples before the 
    /// current recording point.
    ///
    /// If the buffer size exceeds the sample avaliable, this function will
    /// fill the buffer with zeros. 
    ///
    /// \param buffer       pointer to the buffer array, the buffer must have
    ///                     enough space to hold all the samples
    ///
    /// \param numSamples   number of sample points to extra from the audio 
    ///                     stream, regardless of the number of channels
    ///
    void getAudioData(float* buffer, int numSamples);


    /// Note:   Uses std::string because char* pointer might change
    ///
    /// \return     a list of avaliable recording devices
    ///
    std::vector<std::string> getAvailableDevices();

    /// Manually closing the current audio device
    /// 
    /// Note:   The audio device will automatically close during the 
    ///         destruction of the object
    ///
    void closeDevice();

    /// Select and setup an audio recording device
    ///
    /// \param deviceName       defaulted to system default recording device
    ///
    /// \param desiredFreq      recording frequency in Hz, 
    ///                         defaulted to 44100Hz
    ///
    /// \param desiredSamples   number of samples per callback invocation
    ///                         NOT total buffer size
    ///                         defaulted to 2048
    ///
    void setupDevice(const char* deviceName = nullptr, 
                     int desiredFreq = 44100,
                     int desiredSamples = 2048);

private:
    SDL_AudioDeviceID device;
    
    // recording properties
    SDL_AudioSpec audioSpec;

    // ring buffer storing audio data
    int maxRecordingSec;
    std::vector<float> ringBuffer;
    Uint8* ringBufferPtr;
    Uint32 ringBufferSize; // ringBuffer size in bytes
    Uint32 audioBytePos;   // current position in the ring buffer, in bytes
    Uint32 remainingRingBufferSize;
    
    int numDevices;    

    std::mutex audioMutex;  // for thread safety during audio callback

    bool isPaused;

    friend void microphoneAudioCallback(void* userdata, Uint8* stream, 
                                        int callbackBufferSize); 
};
#endif