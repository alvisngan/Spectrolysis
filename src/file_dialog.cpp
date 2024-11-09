#include "file_dialog.hpp"

/// TODO:   phase out tinyfiledialogs, as it is
///         buggy with folder name with space
///         and filter patterns (Debian 12 KDE)
///
///         I am planning to use SDL3, that would require
///         more code, hence a standalone file
///
#include "tinyfiledialogs.h"

static constexpr int s_numFilterPatterns = 6;
static const char* s_filterPatterns[s_numFilterPatterns] = {
    "*.wav", 
    "*.WAV",
    "*.flac",
    "*.FLAC",
    "*.mp3",
    "*.MP3"
};

const char* fileDialogGetAudioPath()
{
    return tinyfd_openFileDialog(nullptr,
                                 nullptr,
                                 s_numFilterPatterns,
                                 s_filterPatterns,
                                 nullptr,
                                 0);
}
