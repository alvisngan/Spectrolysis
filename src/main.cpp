#include <iostream>
#include <cstring>
#include <array>
#include <vector>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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

#ifdef __EMSCRIPTEN__
#include "shader_programs/shader_programs.h" 
#endif


/// Passing the input references to the mainloop, a lot of members because
/// we don't want to initializen objects and arrays every single frame
///
/// TODO: some of the buffers could be aliased
///
/// TODO: parameter documentations
///
typedef struct {
    // grid information
    int nRowsV;
    int nColsV;
    float* z;

    // objects
    SDL_Window* window;
    Grid* gridPtr;
    Shader* spectroShaderPtr;
    Camera* cameraPtr;
    FrameBuffer* sceneBufferPtr;
    AudioPlayer* audioPlayerPtr;
    Microphone* micPtr;

    // FFT
    float* complexBuffer;           // len = g_FFT_LEN
    float* signalBuffer;            // len = g_FFT_LEN
    float* workBuffer;              // len = g_FFT_LEN
    float* magnitudeBuffer;         // len = g_FFT_LEN / 2
    float* freqArray;               // len = g_FFT_LEN / 2

    // bluring rows
    int nConvRows;
    float* colKernel;               // len = nConvRows
    float* previousRows;            // len = (nConvRows - 1) * g_FFT_LEN/2
    float* workRow;                 // len = g_FFT_LEN / 2
    float* workFFTRow;              // len = g_FFT_LEN / 2
    float* workConvRow;             // len = g_FFT_LEN / 2

} MainloopInputs;


void mainloop(MainloopInputs inputs);


void mainloopWrapper(void* arg)
{
    MainloopInputs* inputs = (MainloopInputs*)arg;

    mainloop(*inputs);
}


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

bool g_done = false;

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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

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
    // SDL_GL_SetSwapInterval(1); // enable V-Sync

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
#ifdef __EMSCRIPTEN__ 
    // use c-string shaders to aviod preloading mess
    Shader spectroShader(vertexShader,
                         fragmentShader,
                         SHADER_SOURCE_CSTR);
#else 
    // use shader files for easier changes
    Shader spectroShader("../src/shader_programs/rect.vs",
                         "../src/shader_programs/rect.fs",
                         SHADER_SOURCE_PATH);
#endif

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

    // FFT and smoothing
    // -----------------
    fftInit(g_FFT_LEN);

    // number of rows for the bluring kernel
    constexpr int nConvRows = 10; // 10 seems good for freqPlot
    
    // buffers has to be aligned memory
#ifdef __EMSCRIPTEN__ 
    // .wasm is very stingy when it comes to stack memory
    // allocate aligned memory to the heap
    // free after the mainloop (mainloopInputs will borrow these pointers!)
    float* complexBuffer = static_cast<float*>(std::aligned_alloc(64, g_FFT_LEN * sizeof(float)));
    float* signalBuffer = static_cast<float*>(std::aligned_alloc(64, g_FFT_LEN * sizeof(float)));
    float* workBuffer = static_cast<float*>(std::aligned_alloc(64, g_FFT_LEN * sizeof(float)));
    float* magnitudeBuffer = static_cast<float*>(std::aligned_alloc(64, g_FFT_LEN/2 * sizeof(float)));
    float* freqArray = static_cast<float*>(std::aligned_alloc(64, g_FFT_LEN/2 * sizeof(float)));

    // bluring
    std::vector<float> colKernel(nConvRows);

    // work buffers for bluring rows
    float* previousRows = static_cast<float*>(std::aligned_alloc(64, (nConvRows - 1) * g_FFT_LEN/2 * sizeof(float)));
    float* workRow = static_cast<float*>(std::aligned_alloc(64, g_FFT_LEN/2 * sizeof(float)));
    float* workFFTRow = static_cast<float*>(std::aligned_alloc(64, g_FFT_LEN/2 * sizeof(float)));
    float* workConvRow = static_cast<float*>(std::aligned_alloc(64, g_FFT_LEN/2 * sizeof(float)));
#else
    // statically allocate aligned memory
    alignas(64) float complexBuffer[g_FFT_LEN];
    alignas(64) float signalBuffer[g_FFT_LEN];
    alignas(64) float workBuffer[g_FFT_LEN];
    std::array<float, g_FFT_LEN / 2> magnitudeBuffer{};
    std::array<float, g_FFT_LEN / 2> freqArray{};

    // blurring
    float colKernel[nConvRows];

    // work buffers for blurring rows
    alignas(64) float previousRows[(nConvRows - 1) * g_FFT_LEN/2];
    alignas(64) float workRow[g_FFT_LEN/2];
    alignas(64) float workFFTRow[g_FFT_LEN/2];
    alignas(64) float workConvRow[g_FFT_LEN/2];
#endif

    // bluring: create kernel and set previousRows to zero's
    smoothingHalfGaussian(&colKernel[0], nConvRows);
    memset(&previousRows[0], 0, ((nConvRows - 1) * g_FFT_LEN/2) * sizeof(float));

    // OpenGL settings
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

    // passing inputs into the MainloopInputs struct
    // ---------------------------------------------
    MainloopInputs inputs;

    inputs.nRowsV = nRowsV;
    inputs.nColsV = nColsV;

    inputs.z = z.data();

    inputs.window = window;
    inputs.gridPtr = &xy;
    inputs.spectroShaderPtr = &spectroShader;
    inputs.cameraPtr = &camera;
    inputs.sceneBufferPtr = &sceneBuffer;
    inputs.audioPlayerPtr = &audioPlayer;
    inputs.micPtr = &mic;

    // FFT
    inputs.complexBuffer = &complexBuffer[0];          
    inputs.signalBuffer = &signalBuffer[0];
    inputs.workBuffer = &workBuffer[0];
    inputs.magnitudeBuffer = &magnitudeBuffer[0]; 
    inputs.freqArray = &freqArray[0];

    // bluring rows
    inputs.nConvRows = nConvRows;
    inputs.colKernel = &colKernel[0];    
    inputs.previousRows = &previousRows[0];    
    inputs.workRow = &workRow[0];
    inputs.workFFTRow = &workFFTRow[0];        
    inputs.workConvRow = &workConvRow[0];  


    // render loop
    // -----------
#ifdef __EMSCRIPTEN__ 
    ImGui::GetIO().IniFilename = nullptr; // disable IniFilename for emscripten
    emscripten_set_main_loop_arg(mainloopWrapper, &inputs, 0, true);
    SDL_GL_SetSwapInterval(1); // enable V-Sync

#else
    SDL_GL_SetSwapInterval(1); // enable V-Sync
    while (!g_done)
    {
        mainloop(inputs);
    }
#endif

    // clean up
    // --------
    guiCleanUp();
    fftCleanUp();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

#ifdef __EMSCRIPTEN__ 
    std::free(complexBuffer);
    std::free(signalBuffer);
    std::free(workBuffer);
    std::free(magnitudeBuffer);
    std::free(freqArray);

    std::free(previousRows);
    std::free(workRow);
    std::free(workFFTRow);
    std::free(workConvRow);
#endif
    
    return 0;
}


void mainloop(MainloopInputs inputs)
{
    // input
    // -----
    g_done = processInput(inputs.window, *inputs.cameraPtr);

    // render viewport to frame buffer texture
    // ---------------------------------------
    inputs.sceneBufferPtr->bind();

    // background for viewport only
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw 
    inputs.spectroShaderPtr->use();
    inputs.spectroShaderPtr->setMat4("rotationMat", 
                                     inputs.cameraPtr->getPVMMat());


    // audioInterface
    // --------------
    // if it is paused in its repective audio interface mode
    bool audioInterfaceIsPaused; 
    bool audioInterfacePlayerMode;

    audioInterfacePlayerMode = guiAudioInterfaceGetPlayerMode();

    if (audioInterfacePlayerMode)
    {
        if (!inputs.audioPlayerPtr->getIsPaused())
        {
            inputs.audioPlayerPtr->getAudioData(&inputs.signalBuffer[0], 
                                                g_AUDIO_BUFFER_LEN);

            fftFrequency(inputs.freqArray, 
                         inputs.audioPlayerPtr->getFreq(), 
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
        if (!inputs.micPtr->getIsPaused())
        {
            inputs.micPtr->getAudioData(inputs.signalBuffer, 
                                        g_AUDIO_BUFFER_LEN);

            fftFrequency(inputs.freqArray, 
                         inputs.micPtr->getFreq(), 
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
        memset(&inputs.signalBuffer[g_AUDIO_BUFFER_LEN], 
                0, 
                (g_FFT_LEN - g_AUDIO_BUFFER_LEN) * sizeof(float));

        fftForwardFFT(inputs.signalBuffer, 
                      inputs.complexBuffer, 
                      inputs.workBuffer);

        fftComplexToRealDB(inputs.magnitudeBuffer, 
                            inputs.complexBuffer, 
                            g_FFT_LEN / 2, 
                            true);

        // spectrogram stuff
        array2dMoveRowsUp(inputs.z, inputs.nRowsV, inputs.nColsV, 1);

        smoothingBlurRow(&inputs.magnitudeBuffer[1],
                         &inputs.magnitudeBuffer[1],
                            inputs.previousRows,
                            inputs.workRow,
                            inputs.workFFTRow,
                            inputs.workConvRow,
                            inputs.colKernel,
                            g_FFT_LEN,
                            inputs.nConvRows);

        //  omit DC and the freq before Nyquist
        memcpy(&inputs.z[array2dIdx(inputs.nRowsV - 1, 0, inputs.nColsV)], 
                &inputs.magnitudeBuffer[1], 
                (g_FFT_LEN / 2 - 2) * sizeof(float));

        // modify z array on GPU
        inputs.gridPtr->zSubAllData(inputs.z);  
    }
    else
    {
        // nothing, the buffer will stay the same hence achieving pause
    }

    // draw and unbind viewport
    // ------------------------
    inputs.gridPtr->draw();
    inputs.sceneBufferPtr->unbind();
    
    //--------------
    // window render
    // -------------
    // background color
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // gui render
    // ----------
    guiNewFrame();

    GuiInputs guiInputs;

    guiInputs.freqPlotLen = g_FFT_LEN / 2 - 2;
    guiInputs.freqPlotX = &inputs.freqArray[1];
    guiInputs.freqPlotY = &inputs.magnitudeBuffer[1];
    guiInputs.viewportTextureID = inputs.sceneBufferPtr->getFrameTexture();
    guiInputs.cameraPtr = inputs.cameraPtr;
    guiInputs.audioPlayerPtr = inputs.audioPlayerPtr;
    guiInputs.micPtr = inputs.micPtr;
    guiInputs.gridPtr = inputs.gridPtr;

    guiApp(guiInputs);

    guiRender();

    SDL_GL_SwapWindow(inputs.window);
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

