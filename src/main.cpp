#include <iostream>
#include <cstring>
#include <array>
#include <vector>

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"

#include "shader.hpp"
#include "array2d.hpp"
#include "grid.hpp"
#include "camera.hpp"
#include "frame_buffer.hpp"
#include "gui/gui.hpp"
#include "gui/gui_color.hpp" // global, used in gui_theme.hpp
#include "audio_player.hpp"
#include "microphone.hpp"
#include "fft.hpp"
#include "smoothing.hpp"


/// \param window   SDL2 window
///
/// \return         done using the window
///
bool processInput(SDL_Window* window, Camera& camera);


/// For debuging OpenGL error, will display OpenGL error message on concole
///
/// \param errorLocation    console output name for identifying the code 
///                         location, useful when using multiple of the same 
///                         function 
///
void checkRenderErrors(const char* errorLocation = "");


// settings
const int g_SCR_WIDTH = 1920;
const int g_SCR_HEIGHT = 1080;
guiColorPalette g_color; // used in gui_theme.cpp


// FFT len
constexpr int g_FFT_LEN = 8192; //std::pow(2, 13); // must be multiple of 2 and >32
constexpr int g_AUDIO_BUFFER_LEN = 2048; //std::pow(2,11);




int main()
{   
    // ------------------------------
    // SDL2: initialize and configure
    // ------------------------------
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        std::cout << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | 
                                                     SDL_WINDOW_RESIZABLE | 
                                                     SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Spectrolysis", 
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          g_SCR_WIDTH, 
                                          g_SCR_HEIGHT, 
                                          window_flags);
    if (window == NULL)
    {
        std::cout << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == NULL)
    {
        std::cout << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // enable V-Sync

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // -----------------------------------
    // Initiate my own objects and context
    // -----------------------------------

    // setup Dear ImGui context
    // ------------------------
    guiInit(window, gl_context);

    // build and compile our shader program
    // ------------------------------------
    Shader rectShader("../src/shader_programs/rect.vs",
                      "../src/shader_programs/rect.fs");

    // z-coordinates vector
    // --------------------
    int nRowsV = 200;
    // omit DC and the freq before Nyquist, for convolution smoothing
    // TODO: chnage this on runtime to filter out high frequency
    int nColsV = (g_FFT_LEN / 2) - 2;
    std::vector<float> z(nRowsV * nColsV, 0);

    // generate grid object
    // --------------------
    Grid xy(z.data(), 
            nRowsV, nColsV, 
            0, 
            false, 
            0.8f, -0.8f, 
            0.8f, -0.8f);
    
    // creating viewport
    // -----------------
    Camera camera;
    FrameBuffer sceneBuffer(g_SCR_WIDTH, g_SCR_HEIGHT);

    // Audio Interface
    // ----------------
    AudioPlayer audioPlayer;
    Microphone mic;

    // if it is paused in its repective audio interface mode
    bool audioInterfaceIsPaused; 
    bool audioInterfacePlayerMode;

    // FFT and smoothing
    // -----------------
    fftInit(g_FFT_LEN);
    // buffers has to be aligned memory
    alignas(64) float complexBuffer[g_FFT_LEN];
    alignas(64) float signalBuffer[g_FFT_LEN];
    alignas(64) float workBuffer[g_FFT_LEN];
    std::array<float, g_FFT_LEN / 2> magnitudeBuffer{};
    std::array<float, g_FFT_LEN / 2> freqArray{};

    // blurring
    constexpr int nConvRows = 10; // 10 seems good for freqPlot
    float colKernel[nConvRows];
    smoothingHalfGaussian(&colKernel[0], nConvRows);

    // work buffer for blurring rows
    alignas(64) float previousRows[(nConvRows - 1) * g_FFT_LEN/2];
    alignas(64) float workRow[g_FFT_LEN/2];
    alignas(64) float workFFTRow[g_FFT_LEN/2];
    alignas(64) float workConvRow[g_FFT_LEN/2];

    memset(&previousRows[0], 0, ((nConvRows - 1) * g_FFT_LEN/2) * sizeof(float));

    // Open GL settings
    //-----------------
    // // poly mode, for debug
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    // enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // enable face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // render loop
    // -----------
    bool done = false;
    while (!done)
    {
        // input
        // -----
        // done = processInput(window);
        done = processInput(window, camera);

        // render viewport to frame buffer texture
        // ---------------------------------------
        sceneBuffer.bind();

        // background for viewport only
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw 
        rectShader.use();
        rectShader.setMat4("rotationMat", camera.getPVMMat());


        // audioInterface
        // --------------
        audioInterfacePlayerMode = guiAudioInterfaceGetPlayerMode();

        if (audioInterfacePlayerMode)
        {
            if (!audioPlayer.getIsPaused())
            {
                audioPlayer.getAudioData(&signalBuffer[0], 
                                         g_AUDIO_BUFFER_LEN);

                fftFrequency(&freqArray[0], 
                             audioPlayer.getFreq(), 
                             g_FFT_LEN / 2);
                             
                audioInterfaceIsPaused = false;
            }
            else
            {
                audioInterfaceIsPaused = true;
            }
        }
        else
        {
            if (!mic.getIsPaused())
            {
                mic.getAudioData(&signalBuffer[0], 
                                 g_AUDIO_BUFFER_LEN);

                fftFrequency(&freqArray[0], 
                             mic.getFreq(), 
                             g_FFT_LEN / 2);

                audioInterfaceIsPaused = false;
            }
            else
            {
                audioInterfaceIsPaused = true;
            }
        }

        if (!audioInterfaceIsPaused)
        {
            // update the spectrogram
            // ----------------------
            // perfrom FFT
            memset(&signalBuffer[g_AUDIO_BUFFER_LEN], 
                   0, 
                   (g_FFT_LEN - g_AUDIO_BUFFER_LEN) * sizeof(float));

            fftForwardFFT(&signalBuffer[0], 
                          &complexBuffer[0], 
                          &workBuffer[0]);

            fftComplexToRealDB(&magnitudeBuffer[0], 
                               &complexBuffer[0], 
                               g_FFT_LEN / 2, 
                               true);

            // spectrogram stuff
            array2dMoveRowsUp(&z[0], nRowsV, nColsV, 1);

            smoothingBlurRow(&magnitudeBuffer[1],
                             &magnitudeBuffer[1],
                             &previousRows[0],
                             &workRow[0],
                             &workFFTRow[0],
                             &workConvRow[0],
                             &colKernel[0],
                             g_FFT_LEN,
                             nConvRows);

            //  omit DC and the freq before Nyquist
            memcpy(&z[array2dIdx(nRowsV - 1, 0, nColsV)], 
                   &magnitudeBuffer[1], 
                   (g_FFT_LEN / 2 - 2) * sizeof(float));

            // modify z array on GPU
            xy.zSubAllData(z.data());  
        }
        else
        {
            // nothing, the buffer will stay the same hence achieving pause
        }

        // draw and unbind viewport
        // ------------------------
        xy.draw();
        sceneBuffer.unbind();
        
        //--------------
        // window render
        // -------------
        // background color
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // gui render
        // ----------
        guiNewFrame();

        guiInputs inputs;

        inputs.freqPlotLen = g_FFT_LEN / 2 - 2;
        inputs.freqPlotX = &freqArray[1];
        inputs.freqPlotY = &magnitudeBuffer[1];
        inputs.viewportTextureID = sceneBuffer.getFrameTexture();
        inputs.cameraPtr = &camera;
        inputs.audioPlayerPtr = &audioPlayer;
        inputs.micPtr = &mic;
        inputs.gridPtr = &xy;

        guiApp(inputs);

        guiRender();

        SDL_GL_SwapWindow(window);
    }

    // clean up
    // --------
    guiCleanUp();
    fftCleanUp();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}


bool processInput(SDL_Window* window, Camera& camera)
{
    bool done = false;

    // event handling
    // --------------
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        // bind ImGui io to SDL
        // --------------------
        ImGui_ImplSDL2_ProcessEvent(&event);

        // handle quit
        // -----------
        if (event.type == SDL_QUIT)
        {
            done = true;
        }
        else if (event.type == SDL_WINDOWEVENT 
            && event.window.event == SDL_WINDOWEVENT_CLOSE 
            && event.window.windowID == SDL_GetWindowID(window)
        )
        {
            done = true;
        }
        else
        {
            done = false;
        }

        // handle SDL mouse wheel event
        // ----------------------------
        if (event.type == SDL_MOUSEWHEEL)
        {
            // get the hovered state from ImGui
            if (guiViewportGetHovered())
            {
                camera.scroll2Zoom(static_cast<double>(event.wheel.y));
            }
        }
        else
        {
            camera.isNotScrolling();
        }
    }

    // drag and rotate
    // ---------------
    bool clicking = (
        guiViewportGetHovered() 
        && ImGui::IsMouseDown(ImGuiMouseButton_Left)
    );
    double x, y;
    x = ImGui::GetMousePos().x;
    y = ImGui::GetMousePos().y;
    camera.drag2Rotate(x, y, clicking);

    // right drag and rotate
    // ---------------
    bool rightClicking = (
        guiViewportGetHovered() 
        && ImGui::IsMouseDown(ImGuiMouseButton_Right)
    );
    camera.rightDrag2Move(x, y, rightClicking);

    // return window state
    // -------------------
    return done;
}

void checkRenderErrors(const char* errorLocation)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "Render error at " << errorLocation << ": " << err << std::endl;
    }    
}

