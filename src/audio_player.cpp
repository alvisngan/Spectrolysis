#include "audio_player.hpp"
#include "SDL_stdinc.h"

#include <cstring>
#include <iostream>
#include <string>
#include <algorithm> // std::min

#define DR_MP3_IMPLEMENTATION
#define DR_MP3_FLOAT_OUTPUT
#define DR_FLAC_IMPLEMENTATION
#include "dr_libs/dr_mp3.h"
#include "dr_libs/dr_flac.h"

void audioPlayerAudioCallback(void* userdata, Uint8* stream, int callbackBufferSize);

AudioPlayer::AudioPlayer()//const char* filepath)
    :   device(0),
        audioStartPtr(nullptr), audioSize(0), // will be changed after loading
         audioBytePos(0),
        numDevices(0),
        isPaused(true),
        bytesPerSample(sizeof(float)) // will convert everything to float
{
    
}


AudioPlayer::~AudioPlayer()
{
    if (audioStartPtr.load() != nullptr)
    {
        switch (audioFormat)
        {
            case AudioFormat::MP3:
                drmp3_free(audioStartPtr.load(), nullptr);
                break;
            case AudioFormat::WAV:
                SDL_FreeWAV(audioStartPtr.load());
                break;
            case AudioFormat::FLAC:
                drflac_free(audioStartPtr.load(), NULL);
                break;
            default:
                break;
        }

        audioStartPtr.store(nullptr);
    }
    else
    {
        // nothing, already cleared
    }


    if (device != 0)
    {
        SDL_CloseAudioDevice(device);
    }
}


void AudioPlayer::wavToFloat(const char* filepath)
{
    Uint32 wavSize;
    Uint8* wavStartPtr = nullptr;

    if (SDL_LoadWAV(filepath, &audioSpec, &wavStartPtr, &wavSize) == nullptr)
    {
        std::cout << "Failed to load audio: " << SDL_GetError() << std::endl;
    }
    else
    {
        // no need to callback now, we will callback after setting desired specs
    }

    this->audioSize.store(wavSize);
    this->audioStartPtr.store(wavStartPtr);

    // copying audioSpec to desiredSpec and change format to float
    SDL_AudioSpec desiredSpec;
    desiredSpec.freq = audioSpec.freq;
    desiredSpec.format = AUDIO_F32SYS;
    desiredSpec.channels = audioSpec.channels;
    desiredSpec.samples = audioSpec.samples;
    desiredSpec.callback = audioPlayerAudioCallback;  
    desiredSpec.userdata = this;            // pass the AudioPlayer object to callback

    // build the audio converter
    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt,
                          audioSpec.format,
                          audioSpec.channels,
                          audioSpec.freq,
                          desiredSpec.format,
                          desiredSpec.channels,
                          desiredSpec.freq) < 0)
    {
        std::cout << "Failed to build audio converter: " << SDL_GetError() << std::endl;
    }
    else if (cvt.needed)
    {
        cvt.len = audioSize.load();
        cvt.buf = (Uint8*)SDL_malloc(cvt.len * cvt.len_mult);
        
        // if cvt buffer allocation failed, i.e. nullptr
        if (!cvt.buf) // if nullptr
        {
            std::cout << "Failed to allocate memory for audio conversion: " << SDL_GetError() << std::endl;
        }
        // cvt buffer allocation success 
        else
        {
            // copy the original audio data into the converter buffer
            memcpy(cvt.buf, audioStartPtr.load(), audioSize.load());

            // perform the conversion
            if (SDL_ConvertAudio(&cvt) < 0)
            {
                std::cout << "Audio conversion failed: " << SDL_GetError() << std::endl;
                SDL_free(cvt.buf);
            }
            else
            {
                // replace audioStartPtr and audioSize with the converted data
                SDL_FreeWAV(audioStartPtr.load()); // free the original WAV data
                audioStartPtr.store(cvt.buf);
                audioSize.store(cvt.len_cvt);

                // update the audioSpec to the desired format
                audioSpec = desiredSpec;
            }
        }
    }
    else
    {
        // nothing
    }
}


void AudioPlayer::mp3ToFloat(const char* filepath)
{
    drmp3_config config;
    drmp3_uint64 frameCount;

    float* pSampleData = drmp3_open_file_and_read_pcm_frames_f32(filepath, 
                                                                  &config, 
                                                                  &frameCount, 
                                                                  nullptr);
    
    if (pSampleData == nullptr)
    {
        std::cout << "Failed to load MP3 file: " << filepath << std::endl;
        return; 
    }

    // assign dr_mp3 specs into SDL_Audio specs
    audioSpec.channels = config.channels;
    audioSpec.freq = config.sampleRate;
    audioSpec.format = AUDIO_F32SYS;
    audioSpec.samples = 4096; // default SDL buffer size
    audioSpec.callback = audioPlayerAudioCallback;
    audioSpec.userdata = this;

    // assign dr_mp3 data to AudioPlayer 
    this->audioSize.store(frameCount * config.channels * sizeof(float));

    // using the buffer for dr_mp3 directly
    // yeah, casting ptr, not the best, but I need the memory address
    this->audioStartPtr.store((Uint8*)pSampleData);
    
    // we need the data later, no freeing pSampleData
    pSampleData = nullptr;
    
}


void AudioPlayer::flacToFloat(const char* filepath)
{
    unsigned int channels;
    unsigned int sampleRate;
    drflac_uint64 totalPCMFrameCount;
    float* pSampleData = drflac_open_file_and_read_pcm_frames_f32(filepath, 
                                                                  &channels, 
                                                                  &sampleRate, 
                                                                  &totalPCMFrameCount, 
                                                                  NULL);
    if (pSampleData == NULL) 
    {
        std::cout << "Failed to load FLAC file: " << filepath << std::endl;
    }    

    // assign dr_flac specs into SDL_Audio specs
    audioSpec.channels = channels;
    audioSpec.freq = sampleRate;
    audioSpec.format = AUDIO_F32SYS;
    audioSpec.samples = 4096; // default SDL buffer size
    audioSpec.callback = audioPlayerAudioCallback;
    audioSpec.userdata = this;

    // assign dr_flac data to AudioPlayer 
    // yeah, casting ptr, not the best, but I need the memory address
    this->audioStartPtr.store((Uint8*)pSampleData);
    this->audioSize.store(totalPCMFrameCount * channels * sizeof(float));

    // we need the data later, no freeing pSampleData
    pSampleData = nullptr;
    
}


void AudioPlayer::loadFile(const char* filepath)
{
    SDL_LockAudioDevice(device);

    // clean up existing audio buffer
    if (audioStartPtr.load() != nullptr)
    {
        switch (audioFormat)
        {
            case AudioFormat::MP3:
                drmp3_free(audioStartPtr.load(), nullptr);
                break;
            case AudioFormat::WAV:
                SDL_FreeWAV(audioStartPtr.load());
                break;
            case AudioFormat::FLAC:
                drflac_free(audioStartPtr.load(), NULL);
                break;
            default:
                break;
        }

        audioStartPtr.store(nullptr);
    }
    else
    {
        // nothing, already cleared
    }
    
    // determine the file type base on the extension
    std::string filePathStr(filepath);
    std::string extension = filePathStr.substr(filePathStr.find_last_of('.') + 1);

    if (extension == "mp3" or extension == "MP3")
    {
        mp3ToFloat(filepath);
        this->audioFormat = AudioFormat::MP3;
    }    
    else if (extension == "wav" or extension == "WAV")
    {
        wavToFloat(filepath);
        this->audioFormat = AudioFormat::WAV;
    }
    else if (extension == "flac" or extension == "FLAC")
    {
        flacToFloat(filepath);
        this->audioFormat = AudioFormat::FLAC;
    }
    else
    {
        std::cout << "Unsupported audio format: " << extension << std::endl;
        return;
    }

    SDL_UnlockAudioDevice(device);

    // re-setup device since audioSpec has changed
    this->setupDevice();
}


void AudioPlayer::play()
{
    // queue the audio (so we play when available,
    // as oppososed to a callback function)
    this->isPaused.store(false);
    SDL_PauseAudioDevice(device, 0);    
}


void AudioPlayer::pause()
{
    SDL_PauseAudioDevice(device, 1);
    this->isPaused.store(true);
}


void AudioPlayer::skipBackward()
{
    this-> audioBytePos = 0;
}


bool AudioPlayer::getIsPaused()
{
    return this->isPaused.load();
}


void AudioPlayer::setAudioPosition(Uint32 toTimeSec)
{
    // std::lock_guard<std::mutex> lock(audioMutex);

    Uint32 bytesPerSec = audioSpec.freq * audioSpec.channels * bytesPerSample;

    // clamp
    this->audioBytePos.store(std::min(toTimeSec * bytesPerSec, audioSize.load()));
}


float AudioPlayer::getCurrentTimeSec()
{
    Uint32 bytesPerSec = audioSpec.freq * audioSpec.channels * bytesPerSample;

    if (bytesPerSec == 0)
    {
        return 0;
    }

    // std::lock_guard<std::mutex> lock(audioMutex);
    Uint32 playedBytes = this->audioBytePos.load();

    float playedTimeSec =   static_cast<float>(playedBytes) 
                          / static_cast<float>(bytesPerSec);

    float totalDurationSec = this->getTotalTimeSec();
    if (playedTimeSec > totalDurationSec)
    {
        playedTimeSec = totalDurationSec;
    }

    return playedTimeSec;
}


float AudioPlayer::getTotalTimeSec()
{
    Uint32 bytesPerSec = audioSpec.freq * audioSpec.channels * bytesPerSample;

    if (bytesPerSec == 0)
    {
        return 0;
    }

    return static_cast<float>(audioSize) / static_cast<float>(bytesPerSec); 
}


int AudioPlayer::getFreq()
{
    return audioSpec.freq;
}


void AudioPlayer::getAudioData(float* buffer, int numSamples)
{   
    // determining the buffer size 
    Uint32 bufferSize = (numSamples * sizeof(float));
    
    if (!(isPaused.load()))
    {
        Uint32 currentBytePos = this->audioBytePos.load();
        
        // ignore audio data when it is smaller than the buffer (i.e start)
        if (( currentBytePos > bufferSize) &&
            ( currentBytePos < audioSize))
        {
            // buffer start position in the audio stream, in bytes
            Uint32 bufferStartPosition = currentBytePos - bufferSize;

            // pointer to the data start point
            Uint8* dataPtr = audioStartPtr.load() + bufferStartPosition;

            // copy audio stream data to the buffer
            // (already converted to float in constructor)
            memcpy(buffer, dataPtr, bufferSize);
        }
        else
        {
            // fill in with zero if there is not enough audio samples in the stream
            memset(buffer, 0, bufferSize);
        }
    }
    else
    {
        // fill in with zero if audio stream is paused
        memset(buffer, 0, bufferSize);        
    }
}


std::vector<std::string> AudioPlayer::getAvailableDevices()
{
    this->numDevices = SDL_GetNumAudioDevices(0); // 0 for playback devices
    std::vector<std::string> devices;

    for (int i = 0; i < numDevices; ++i) 
    {
        const char* deviceName = SDL_GetAudioDeviceName(i, 0);
        if (deviceName) 
        {
            devices.push_back(std::string(deviceName));
        }
    }
    return devices;
}


void AudioPlayer::closeDevice()
{
    if (device != 0)
    {
        SDL_CloseAudioDevice(device);
    }
    else
    {
        // nothing, already closed
    }
}


void AudioPlayer::setupDevice(const char* deviceName)
{
    // close the previously opened audio device if it exists
    if (device != 0)
    {
        SDL_CloseAudioDevice(device);
    }

    // set the device for playback for 0, or '1' for recording.
    device = SDL_OpenAudioDevice(
        deviceName, 0, &audioSpec, nullptr, 
        // disallow frequency change and channel change
        SDL_AUDIO_ALLOW_FORMAT_CHANGE
    );  

    if (device == 0)
    {
        std::cout << "AudioPlayer device error: " << SDL_GetError() << std::endl; 
    }  
}


void audioPlayerAudioCallback(void* userdata, Uint8* stream, 
                              int callbackBufferSize)
{
    // need a callback to keep track of audioBytePos

    AudioPlayer* audioPlayer = static_cast<AudioPlayer*>(userdata);

    Uint32 currentSize = audioPlayer->audioSize.load();
    Uint8* currentStartPtr = audioPlayer->audioStartPtr.load();
    Uint32 currentBytePos = audioPlayer->audioBytePos.load(std::memory_order_relaxed);
    
    if (audioPlayer->isPaused.load() || 
        currentStartPtr == nullptr || 
        currentSize == 0)
    {
        // fill the stream with silence if paused
        SDL_memset(stream, 0, callbackBufferSize);
        return;
    }

    // pause the audio when finish playback
    if (currentBytePos >= currentSize)
    {
        
        audioPlayer->isPaused.store(true);
        SDL_PauseAudioDevice(audioPlayer->device, 1);
        SDL_memset(stream, 0, callbackBufferSize);
        return;
    }

    Uint32 remaining = currentSize - currentBytePos;
    Uint32 toCopy = (callbackBufferSize > static_cast<int>(remaining)) ? remaining : callbackBufferSize;

    if (toCopy > 0)
    {
        // Copy audio data from audioStartPtr to the stream
        SDL_memcpy(stream, 
                   currentStartPtr + currentBytePos, 
                   toCopy);
        audioPlayer->audioBytePos.fetch_add(toCopy, std::memory_order_relaxed);
    }

    if (toCopy < static_cast<Uint32>(callbackBufferSize))
    {
        // Fill the rest of the stream with silence (if we've reached the end)
        SDL_memset(stream + toCopy, 0, callbackBufferSize - toCopy);
    }
}
