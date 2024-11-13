#include "gui_components.hpp"

#include <string>
#include <cstring>
#include <vector>
#include <chrono>

#include "imgui.h"

#include "file_dialog.hpp"
#include "gui_color.hpp"

extern guiColorPalette g_color;

// audio player
// ------------
// for dropdown device menu
static const char* s_audioPlayerSelectedDevice = nullptr; 
static std::vector<std::string> s_audioPlayerDevices;

// for file dialog
static char* s_audioFilePath = nullptr;
static const char* s_getFileNameFromPath(const char* filePath);

// audio playback slider
void s_formatTimestamp(char *buffer, size_t size, 
                     float currentTime, float totalTime);
static bool s_audioPlayerSliderRecentlyClicked = false;
static std::chrono::time_point<std::chrono::high_resolution_clock> s_audioPlayerLastSlideTime;
static int s_timeout = 100;



// microphone
// ----------
// for dropdown device menu
static const char* s_micSelectedDevice = nullptr; 
static std::vector<std::string> s_micDevices;


// audio interface
// ---------------
static void s_guiAudioPlayer(AudioPlayer& audioPlayer);
static void s_guiMicrophone(Microphone& mic);
static bool s_audioInterfacePlayerMode = false; /// default mic
static bool s_audioInterfacePauseWhenSwitch = true;


void guiAudioInterface(AudioPlayer& audioPlayer, Microphone& mic)
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, g_color.base);

    ImGui::Begin("Audio Interface");
    {
        ImGui::PushStyleColor(ImGuiCol_Text, g_color.base);
        if (ImGui::Button("Switch Mode"))
        {
            if (s_audioInterfacePauseWhenSwitch)
            {
                if (s_audioInterfacePlayerMode)
                {
                    // switching from player to mic, pause stream
                    audioPlayer.pause();            
                }
                else
                {
                    // switching from mic to player, pause mic
                    mic.pause();
                }
            }

            // switch mode
            s_audioInterfacePlayerMode = ! s_audioInterfacePlayerMode;
        }
        ImGui::PopStyleColor();

        ImGui::NewLine();
        ImGui::Text(s_audioInterfacePlayerMode ? "Audio Player" : "Microphone");

        if (s_audioInterfacePlayerMode)
        {
            s_guiAudioPlayer(audioPlayer);
        }
        else
        {
            s_guiMicrophone(mic);
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
}


void guiAudioInterfaceCleanUp()
{
    if (s_audioFilePath != nullptr)
    {
        free(s_audioFilePath);
        s_audioFilePath = nullptr; 
    }

}


bool guiAudioInterfaceGetPlayerMode()
{
    return s_audioInterfacePlayerMode;
}


void guiAudioInterfaceMenu()
{
    if (ImGui::BeginMenu("Audio Interface"))
    {
        if (ImGui::MenuItem(
            "Pause when Switching",
            "",
            s_audioInterfacePauseWhenSwitch
        ))
        {
            s_audioInterfacePauseWhenSwitch = !s_audioInterfacePauseWhenSwitch;
        }

        ImGui::EndMenu();
    }
}


/// TODO: pause when press slider, replay (if isPaused was false) on release
void s_guiAudioPlayer(AudioPlayer& audioPlayer)
{
    // drop down device menu
    // ---------------------
    if (ImGui::BeginCombo(
        " ", 
        s_audioPlayerSelectedDevice ? s_audioPlayerSelectedDevice : "System Default")
    )
    {
        // option 1: "System Default"
        if (ImGui::Selectable("System Default", s_audioPlayerSelectedDevice == nullptr))
        {
            // pause audio player before switching
            audioPlayer.pause();
            s_audioPlayerSelectedDevice = nullptr;
            audioPlayer.setupDevice(s_audioPlayerSelectedDevice);
        }

        // option 2: s_audioPlayerDevices from the vector
        s_audioPlayerDevices = audioPlayer.getAvailableDevices();
        for (size_t i = 0; i < s_audioPlayerDevices.size(); ++i)
        {
            bool isSelected = (s_audioPlayerSelectedDevice == s_audioPlayerDevices[i].c_str());
            if (ImGui::Selectable(s_audioPlayerDevices[i].c_str(), isSelected))
            {
                // pause audio player before switching
                audioPlayer.pause();
                s_audioPlayerSelectedDevice = s_audioPlayerDevices[i].c_str(); 
                audioPlayer.setupDevice(s_audioPlayerSelectedDevice);
            }

            // set focus to the selected item when opening the combo
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
    float deviceMenuWidth = ImGui::GetItemRectSize().x;

    // file picker
    // -----------
    ImGui::Spacing(); 

    // left justify(ish) the button
    const char* filePickerLabel = "Choose File";

    float filePickerButtonStartX =  deviceMenuWidth 
                                  - ImGui::CalcTextSize(filePickerLabel).x
                                  - ImGui::GetStyle().FramePadding.x * 2;

    ImGui::PushTextWrapPos(filePickerButtonStartX);
    if (s_audioFilePath != nullptr)
    {
        ImGui::Text(s_getFileNameFromPath(s_audioFilePath));
    }
    else
    {
        ImGui::Text("No File Choosen");
    }
    ImGui::PopTextWrapPos();

    ImGui::SameLine();
    ImGui::SetCursorPosX(filePickerButtonStartX);
    ImGui::PushStyleColor(ImGuiCol_Text, g_color.base);
    if (ImGui::Button(filePickerLabel))
    {
        audioPlayer.pause();
        const char* newAudioFilePath = fileDialogGetAudioPath();

        // make sure opening the file dialog and cancelling it won't
        // return a nullptr if there was already a file selected
        if (newAudioFilePath == nullptr && s_audioFilePath != nullptr)
        {
            // nothing,we are keeping the last audio path
        
        }
        else if (newAudioFilePath != nullptr)
        {
             // free the data before copying, preventing memory leak
            if (s_audioFilePath != nullptr)
            {
                free(s_audioFilePath);
            }
            s_audioFilePath = strdup(newAudioFilePath);
            // free(newAudioFilePath); free by tinyfiledialog

            audioPlayer.skipBackward();
            audioPlayer.loadFile(s_audioFilePath);
        }
        else
        {
            // nothing
            // either:  no file were ever selected, return nullptr
            // or:      we are keeping the last audio path
        }
    }
    ImGui::PopStyleColor();

    // playback slider
    // ---------------
    ImGui::Spacing(); 
    ImGui::Spacing();

    float currentTime = audioPlayer.getCurrentTimeSec();
    float totalTime = audioPlayer.getTotalTimeSec();

    char timestamp[30];
    s_formatTimestamp(timestamp, sizeof(timestamp), currentTime, totalTime);
    

    if (ImGui::SliderFloat(timestamp, &currentTime, 0.0f, totalTime, ""))
    {
        if (!s_audioPlayerSliderRecentlyClicked)
        {
            // Update audio position based on slider
            audioPlayer.setAudioPosition(currentTime);
            s_audioPlayerSliderRecentlyClicked = true;
            s_audioPlayerLastSlideTime = std::chrono::high_resolution_clock::now();
        }
        else
        {
            if (std::chrono::high_resolution_clock::now() - s_audioPlayerLastSlideTime >
                std::chrono::milliseconds(s_timeout))
                s_audioPlayerSliderRecentlyClicked = false;
        }
        
    }

    // buttons
    // -------
    ImGui::PushStyleColor(ImGuiCol_Text, g_color.base);
    if (ImGui::Button("Play"))
    {
        if (s_audioFilePath != nullptr)
        {    
            audioPlayer.play();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        audioPlayer.pause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Skip Backward"))
    {
        audioPlayer.skipBackward();
    }
    ImGui::PopStyleColor();
}


void s_guiMicrophone(Microphone& mic)
{
    // drop down menu
    if (ImGui::BeginCombo(" ", s_micSelectedDevice ? s_micSelectedDevice : "System Default"))
    {
        // option 1: "System Default"
        if (ImGui::Selectable("System Default", s_micSelectedDevice == nullptr))
        {
            s_micSelectedDevice = nullptr;
            mic.setupDevice(s_micSelectedDevice);
        }

        // option 2: s_micDevices from the vector
        s_micDevices = mic.getAvailableDevices();
        for (size_t i = 0; i < s_micDevices.size(); ++i)
        {
            bool isSelected = (s_micSelectedDevice == s_micDevices[i].c_str());
            if (ImGui::Selectable(s_micDevices[i].c_str(), isSelected))
            {
                s_micSelectedDevice = s_micDevices[i].c_str(); 
                mic.setupDevice(s_micSelectedDevice);
            }

            // set focus to the selected item when opening the combo
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::Spacing(); 
    ImGui::PushStyleColor(ImGuiCol_Text, g_color.base);
    if (ImGui::Button("Record"))
    {
        mic.record();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        mic.pause();
    }
    ImGui::PopStyleColor();
}


const char* s_getFileNameFromPath(const char* filePath)
{
    const char* lastSlash = strrchr(filePath, '/');
    const char* lastBackslash = strrchr(filePath, '\\');

    // determine the position of the last path separator
    const char* lastSeparator = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;

    return lastSeparator ? lastSeparator + 1 : filePath;   
}


void s_formatTimestamp(char *buffer, size_t size, 
                     float currentTime, float totalTime) 
{
    // Calculate hours, minutes, and seconds for current time
    int currentHours = (int)(currentTime) / 3600;
    int currentMinutes = ((int)(currentTime) % 3600) / 60;
    int currentSeconds = (int)(currentTime) % 60;

    // Calculate hours, minutes, and seconds for total time
    int totalHours = (int)(totalTime) / 3600;
    int totalMinutes = ((int)(totalTime) % 3600) / 60;
    int totalSeconds = (int)(totalTime) % 60;

    if (totalHours > 0) 
    {
        // include hours in the format
        snprintf(buffer, size, "%d:%02d:%02d/%d:%02d:%02d", 
                                                             currentHours, 
                                                             currentMinutes, 
                                                             currentSeconds, 
                                                             totalHours, 
                                                             totalMinutes, 
                                                             totalSeconds);
    } 
    else 
    {
        snprintf(buffer, size, "%d:%02d/%d:%02d", 
                                                   currentMinutes, 
                                                   currentSeconds, 
                                                   totalMinutes, 
                                                   totalSeconds);
    }
}