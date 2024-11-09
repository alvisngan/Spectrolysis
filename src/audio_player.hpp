//===----------------------------------------------------------------------===//
//
// Library for audio player
//
// Audio playback using SDL2, MP3 and FLAC support provided by dr_libs
// 
//===----------------------------------------------------------------------===//
// Basic SDL2 .wav player https://www.youtube.com/watch?v=hZ0TGCUcY2g&t=711s

#ifndef AUDIO_PLAYER_HPP
#define AUDIO_PLAYER_HPP

#include <vector>
#include <mutex>
#include <atomic>

#include <SDL2/SDL.h>

/// TODO: maybe change it to single channel?

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    // AudioPlayer(const AudioPlayer&) = delete;
    // AudioPlayer& operator=(const AudioPlayer&) = delete;

    /// Load an audio file for playback, will convert buffer to Float-32
    /// 
    /// Currently supporting .wav .flac and .mp3
    ///
    /// \param filepath     filepath of the playback audio file
    ///
    void loadFile(const char* filepath);

    /// Start playing the audio at the current playback position
    ///
    void play();

    /// Pause the audio stream
    ///
    void pause();

    /// Return to the beginning of the audio buffer
    ///
    void skipBackward();

    /// \return whether the audio stream is pauseed
    ///
    bool getIsPaused();

    /// Set the current audio playback position to a specific time in sec
    ///
    /// \param toTimeSec    time in second, if exceeded the audio length
    ///                     the time will be set to the end of stream
    ///
    void setAudioPosition(Uint32 toTimeSec);

    /// \return     current playback time in seconds
    ///
    float getCurrentTimeSec();

    /// \return     audio stream length in seconds
    ///
    float getTotalTimeSec();

    /// \return     audio file sample frequency in Hz
    ///
    int getFreq();

    /// Fill in a buffer array with floating point audio samples from the 
    /// current playing point to user specified number of samples before the 
    /// current playing point.
    ///
    /// If the buffer size exceeds the sample avaliable, this function will
    /// fill the buffer with zeros. 
    ///
    /// Note:   will get numSamples amount of sample regardless
    ///         of the number of channels.
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

    /// Select and setup an audio playback device
    ///
    /// \param deviceName   defaulted to system default audio playback device
    ///
    void setupDevice(const char* deviceName = nullptr);

private:
    SDL_AudioDeviceID device;
    
    // .WAV file properties
    SDL_AudioSpec audioSpec;
    Uint8* audioStartPtr;   // pointer to audio stream
    Uint32 audioSize;       // total size of audio stream in bytes
    std::atomic_uint32_t audioBytePos;
   
    int numDevices;         

    std::atomic_bool isPaused;
    
    // for getting audio data
    Uint8 bytesPerSample;

    enum class AudioFormat
    {
        UNKNOWN,
        MP3,
        WAV,
        FLAC
    };

    AudioFormat audioFormat;

    void wavToFloat(const char* filepath);
    void mp3ToFloat(const char* filepath);
    void flacToFloat(const char* filepath);

    friend void audioPlayerAudioCallback(void* userdata, 
                                         Uint8* stream, 
                                         int callbackBufferSize);
};

#endif
