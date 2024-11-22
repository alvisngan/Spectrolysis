#include "file_dialog.hpp"

/// TODO:   phase out tinyfiledialogs, as it is
///         buggy with folder name with space
///         and filter patterns (Debian 12 KDE)
///
///         I am planning to use SDL3, that would require
///         more code, hence a standalone file
///
#ifdef __EMSCRIPTEN__ 
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#include "tinyfiledialogs.h"
#endif

#ifdef __EMSCRIPTEN__ 
/// BUG:    loading a file right after another can cause index out of bound 
///         exception in Firefox, due to async problem
///
/// BUG:    exiting file dialog without loading any file causes black screen
///         due to this function not properly returning null pointer
///
EM_JS(void, openFileDialog, (char* buffer, int bufferSize), {
    // JavaScript code:
    Asyncify.handleSleep(function(wakeUp) {
        var input = document.createElement('input');
        input.type = 'file';
        input.accept = '.mp3, .wav, .flac, .MP3, .WAV, .FLAC';

        input.onchange = e => {
            // Get only the first file
            var file = e.target.files[0];

            // No file selected
            if (!file) {
                buffer[0] = 0; // null-terminate buffer
                wakeUp();
                return;
            }

            var reader = new FileReader();
            reader.onload = function(event) {
                var data = new Uint8Array(event.target.result);
                var filename = '/' + file.name;
                FS.writeFile(filename, data);

                // includes the null terminator at the end
                var lengthBytes = lengthBytesUTF8(filename) + 1;
                if (lengthBytes > bufferSize) {
                    console.error('Filename too long for buffer');
                    buffer[0] = 0; // Indicate error
                    wakeUp();
                    return;
                }

                stringToUTF8(filename, buffer, bufferSize);
                wakeUp();
            };
            reader.readAsArrayBuffer(file);
        };
        input.click();
    });
});

static char filePathBuffer[1024]; // statically allocate memory
const char* fileDialogGetAudioPath()
{
    openFileDialog(filePathBuffer,1024);
    return filePathBuffer;
}

#else
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
#endif
