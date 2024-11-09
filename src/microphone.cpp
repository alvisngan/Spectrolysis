#include "microphone.hpp"

#include <iostream>

void microphoneAudioCallback(void* userdata, Uint8* stream, int len);

Microphone::Microphone(int maxRecordingSec)
    :   device(0),
        maxRecordingSec(maxRecordingSec),
        ringBufferPtr(nullptr),
        ringBufferSize(maxRecordingSec * 44100 * sizeof(float)),
        audioBytePos(0),
        remainingRingBufferSize(0),
        numDevices(0),
        isPaused(true)
{
    // reserve buffer size, will force recording to single channel
    this->ringBuffer.resize(ringBufferSize);
    // set initial ring buffer to zero's, so I don't need to
    // implement states in getAudioData
    memset(this->ringBuffer.data(), 0, ringBufferSize);

    this->ringBufferPtr = reinterpret_cast<Uint8*>(ringBuffer.data());

    this->setupDevice();
}

Microphone::~Microphone()
{
    // ringBuffer is a vector, will deallocate automatically
    this->ringBufferPtr = nullptr;

    if (device != 0)
    {
        SDL_CloseAudioDevice(device);
    }
}


void Microphone::record()
{
    this->isPaused = false;
    SDL_PauseAudioDevice(device, 0);  
}


void Microphone::pause()
{
    SDL_PauseAudioDevice(device, 1);
    this->isPaused = true;    
}


bool Microphone::getIsPaused()
{
    return this->isPaused;
}


int Microphone::getFreq()
{
    return this->audioSpec.freq;
}


/// TODO:   if (bufferSize > ringBufferSize), should throw an error
///
void Microphone::getAudioData(float* buffer, int numSamples)
{
    // buffer:      external buffer which data is copied to
    // ringBuffer:  internal ring buffer which data is originally held

    // determining the buffer size 
    Uint32 bufferSize = (numSamples * sizeof(float));

    if (!isPaused)
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        
        if ((audioBytePos <= bufferSize) && (bufferSize <= ringBufferSize))
        {
            // need to copy end and front data due to ring buffer wrapping
            // first section (older data):
            Uint32 dataBytePos = ringBufferSize - (bufferSize - audioBytePos);
            Uint8* dataPtr = ringBufferPtr + dataBytePos;
            Uint32 copySize = bufferSize - audioBytePos;

            memcpy(buffer, dataPtr, copySize);

            // second section (newer data):
            Uint8* buffer2ndPtr =   reinterpret_cast<Uint8*>(buffer) 
                                  + (bufferSize - audioBytePos);
            dataPtr = ringBufferPtr;
            copySize = audioBytePos;

            memcpy(buffer2ndPtr,
                   dataPtr,
                   copySize);
        }
        else if ((audioBytePos > bufferSize) && (bufferSize <= ringBufferSize))
        { 
            //   simply copy and paste
            Uint32 dataBytePos = audioBytePos - bufferSize;
            Uint8* dataPtr = ringBufferPtr + dataBytePos;
            
            memcpy(buffer, dataPtr, bufferSize);
        }
        else
        {
            // bufferSize > ringBufferSize

            // Not sure how to handle the start of recording
            // since it is a ring buffer, like there is no
            // way to know if it is the start or buffer wrapping
            // unless implementing states (I dun wanna :p).
            //
            // Easiest way is to start with a buffer initialized
            // with zeros, which it is now.

            memset(buffer , 0, bufferSize);
            std::cout << "buffer exceeded recording size" << std::endl;
        }
    }
    else
    {
        // fill in with zero if recording is paused
        memset(buffer, 0, bufferSize);           
    }
}


std::vector<std::string> Microphone::getAvailableDevices()
{
    this->numDevices = SDL_GetNumAudioDevices(1); // 1 for recording devices
    std::vector<std::string> devices;

    for (int i = 0; i < numDevices; ++i) 
    {
        const char* deviceName = SDL_GetAudioDeviceName(i, 1);
        if (deviceName) 
        {
            devices.push_back(std::string(deviceName));
        }
    }
    return devices;
}


void Microphone::closeDevice()
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


void Microphone::setupDevice(const char* deviceName, 
                             int desiredFreq,
                             int desiredSamples)
{
    // close the previously opened audio device if it exists
    if (device != 0)
    {
        SDL_CloseAudioDevice(device);
    }

    SDL_AudioSpec desiredSpec;
    desiredSpec.freq = desiredFreq;
    desiredSpec.format = AUDIO_F32SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = desiredSamples;
    desiredSpec.callback = microphoneAudioCallback;  
    desiredSpec.userdata = this;

    // set the device for playback for 0, or '1' for recording.
    device = SDL_OpenAudioDevice(
        deviceName, 1, &desiredSpec, &audioSpec, 
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE // don't allow format change
    );
    

    if (device == 0)
    {
        std::cout << "Microphone device error: " << SDL_GetError() << std::endl; 
    }  
    else
    {
        Uint32 obtainedRingBufferSize =   audioSpec.freq
                                        * maxRecordingSec 
                                        * SDL_AUDIO_BITSIZE(audioSpec.format) 
                                        / 8
                                        * audioSpec.channels;
        
        if (obtainedRingBufferSize != this->ringBufferSize)
        {
            this->ringBufferSize = obtainedRingBufferSize;
            this->ringBuffer.resize(ringBufferSize);

            // set initial ring buffer to zero's, so I don't need to
            // implement states in getAudioData
            memset(this->ringBuffer.data(), 0, ringBufferSize);
        }
        else
        {
            // no need to resize ring buffer
        }

    }
}


void microphoneAudioCallback(void* userdata, Uint8* stream, 
                             int callbackBufferSize)
{
    Microphone* mic = static_cast<Microphone*>(userdata);

    std::lock_guard<std::mutex> lock(mic->audioMutex);

    // finding the remaining buffer size
    // ---------------------------------
    if (mic->ringBufferSize > mic->audioBytePos)
    {
        mic->remainingRingBufferSize = mic->ringBufferSize - mic->audioBytePos;
    }
    else
    {
        // buffer has reached the end, time to loop back
        // no modulo operation needed, in theory
        mic->remainingRingBufferSize = 0;
    }

    // see what to do with the ring buffer
    // -----------------------------------
    if (static_cast<Uint32>(callbackBufferSize) > mic->remainingRingBufferSize)
    {
        // need to loop back to the begining
        // split the callback buffer into two, store at seperate ends
        // whatever can fit at the end 
        SDL_memcpy(mic->ringBufferPtr + mic->audioBytePos,
                   stream,
                   mic->remainingRingBufferSize);
        
        // the remaining can go to the front
        SDL_memcpy(mic->ringBufferPtr,
                   stream + mic->remainingRingBufferSize,
                   callbackBufferSize - mic->remainingRingBufferSize);
        
        mic->audioBytePos = (callbackBufferSize - mic->remainingRingBufferSize);
    }
    else
    {
        // no need to loop back, proceed as normal
        SDL_memcpy(mic->ringBufferPtr + mic->audioBytePos,
                   stream,
                   callbackBufferSize);
                   
        mic->audioBytePos += callbackBufferSize;
    }

    // be pedantic and make doubly sure it will wrap around
    mic->audioBytePos %= mic->ringBufferSize;
}
