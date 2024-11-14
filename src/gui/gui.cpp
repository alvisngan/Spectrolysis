#include "gui.hpp"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h" // for docking layout (DockBuilder)
#include "implot.h"

#include "gui_components.hpp"
#include "gui_theme.hpp"

// This code is for ImGui docking branch
// for ImGui master branch - ImGui::Image(ImTextureID, ...)
// has a different definition

// font
// ----
static void s_setUpFonts();

// main menu
// ---------
void s_guiMainMenu(ImGuiDockNodeFlags& dockspace_flags, GuiInputs& inputs);
static void s_guiDockingOptMenu(ImGuiDockNodeFlags& dockspace_flags);

static void s_guiViewMenu();
static bool s_showFreqPlot = true;
static bool s_showSpectrogram = true;

static void s_guiGraphicsMenu();
static bool s_enableDepthTesting = true;   // default, otherwise change main
static bool s_enableFaceCulling = true;     // default, otherwise change main
static bool s_showFrameRate = false;

static void s_guiPlotMenu(Grid& grid);

void guiInit(SDL_Window *window, SDL_GLContext gl_context, const char* version)
{
    // setup Dear ImGui context
    // ------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // sertup ImGui IO
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.WantCaptureMouse = true;    

    // setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // setup platform/renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(version);

#ifndef __EMSCRIPTEN__
    // load fonts
    s_setUpFonts();
#endif

    // set style
    guiThemeApplyColor();
    guiThemeApplyStyling();
}


void guiCleanUp()
{
    guiAudioInterfaceCleanUp();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}




void guiNewFrame()
{
        // start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}


void guiRender()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// also includes main menu
void guiApp(GuiInputs& inputs)
{
    // adopted from imgui_demo.cpp ShowExampleAppDockSpace
    // ---------------------------------------------------

    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // ImGuiWindowFlags_NoDocking: parent window not dockable into
    // it would be confusing to have two docking targets within each others
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | 
                                    ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    window_flags |= ImGuiWindowFlags_NoTitleBar | 
                    ImGuiWindowFlags_NoCollapse | 
                    ImGuiWindowFlags_NoResize | 
                    ImGuiWindowFlags_NoMove;

    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | 
                    ImGuiWindowFlags_NoNavFocus;


    // when using ImGuiDockNodeFlags_PassthruCentralNode, 
    // DockSpace() will render our background and handle the pass-thru hole, 
    // so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // main dockspace
    // --------------
    ImGui::Begin("DockSpace", nullptr, window_flags);
    {
        ImGui::PopStyleVar(2);

        // submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

            // initialize DockBuilder from imgui_internal.h
            // --------------------------------------------
            static bool dockspace_initialized = false;
            if (!dockspace_initialized)
            {
                dockspace_initialized = true;

                // default docking layout
                // ----------------------
                
                // remove existing layout
                ImGui::DockBuilderRemoveNode(dockspace_id);
                
                ImGui::DockBuilderAddNode(dockspace_id, 
                                          ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

                // keep track of the central node
                ImGuiID dock_main_id = dockspace_id;

                // split the dockspace into regions
                ImGuiID dock_id_left, dock_id_bottom;
                ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, 
                                            &dock_id_left, &dock_main_id);
                // ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, 
                //                             &dock_id_right, &dock_main_id);
                ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.32f,
                                            &dock_id_bottom, &dock_main_id);

                // assigning widgets to dockspace
                ImGui::DockBuilderDockWindow("Frequency", dock_id_bottom);
                ImGui::DockBuilderDockWindow("Audio Interface", dock_id_left);
                ImGui::DockBuilderDockWindow("Spectrogram", dock_main_id);

                ImGui::DockBuilderFinish(dockspace_id);
            }
        }
        else
        {
        }

        // main menu, defined in the main dockspace
        // ----------------------------------------
        s_guiMainMenu(dockspace_flags, inputs);


    }
    ImGui::End();

    // components
    // ----------
    // using * we want to get to the objects stored by the ptr
    guiAudioInterface(*inputs.audioPlayerPtr, *inputs.micPtr);

    if (s_showSpectrogram)
    { 
        guiViewport(*inputs.cameraPtr, inputs.viewportTextureID);
    }
    
    if (s_showFreqPlot)
    {
        bool logScale = inputs.gridPtr->getLogScale();
        guiFrequencyPlot(inputs.freqPlotX, inputs.freqPlotY,
                         inputs.freqPlotLen, logScale);
    }

}


void s_guiMainMenu(ImGuiDockNodeFlags& dockspace_flags, GuiInputs& inputs)
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Option"))
        { 
            s_guiDockingOptMenu(dockspace_flags);
            

            ImGui::Separator();

            s_guiGraphicsMenu();

            ImGui::Separator();

            guiAudioInterfaceMenu();
            
            ImGui::EndMenu();
        }

        s_guiViewMenu();

        s_guiPlotMenu(*inputs.gridPtr);

        // framerate counter
        // ----------------
        if (s_showFrameRate)
        {
            // right justify the framerate counter on the menu bar
            float windowWidth = ImGui::GetWindowWidth();
            // using a dummy text to set max text size
            float itemWidth = ImGui::CalcTextSize("Framerate: 0000.0 FPS").x;
            ImGui::SetCursorPosX(windowWidth - itemWidth - ImGui::GetStyle().ItemSpacing.x);
            ImGui::Text("Framerate: %.1f FPS", ImGui::GetIO().Framerate);
        }

        ImGui::EndMenuBar();
    }
}


void s_guiViewMenu()
{
    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::MenuItem(
            "Frequency Plot",
            "",
            s_showFreqPlot
        ))
        {
            s_showFreqPlot = !s_showFreqPlot;
        }

        if (ImGui::MenuItem(
            "Spectrogram",
            "",
            s_showSpectrogram
        ))
        {
            s_showSpectrogram = !s_showSpectrogram;
        }

        ImGui::EndMenu();
    }
}


void s_guiGraphicsMenu()
{
    if (ImGui::BeginMenu("Graphics"))
    {
        if (ImGui::MenuItem(
            "Depth Testing",
            "",
            s_enableDepthTesting
        ))
        {
            // changing state
            s_enableDepthTesting = !s_enableDepthTesting;

            // implement logic here so we don't have to do it every frame
            // glad is loaded in the header
            if (s_enableDepthTesting)
            {
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
            }
            else
            {
                glDisable(GL_DEPTH_TEST);
            }
        }

        if (ImGui::MenuItem(
            "Face Culling",
            "",
            s_enableFaceCulling
        ))
        {
            s_enableFaceCulling = !s_enableFaceCulling;
            // implement logic here so we don't have to do it every frame
            // glad is loaded in the header
            if (s_enableFaceCulling)
            {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glFrontFace(GL_CCW);
            }
            else
            {
                glDisable(GL_CULL_FACE);
            }
        }

        if (ImGui::MenuItem(
            "Show Framerate",
            "",
            s_showFrameRate
        ))
        {
            s_showFrameRate = !s_showFrameRate;
        }

        ImGui::EndMenu();
    }
}


/// TODO: frequency plot log scale
void s_guiPlotMenu(Grid& grid)
{
    if (ImGui::BeginMenu("Plot"))
    {
        bool logScale = grid.getLogScale();

        if (ImGui::MenuItem(
            "Log Scale",
            "",
            logScale
        ))
        {
            grid.gridSwitchLogScale();
        }

        ImGui::EndMenu();
    }
}


void s_guiDockingOptMenu(ImGuiDockNodeFlags& dockspace_flags)
{
    // adopted from imgui_demo.cpp ShowExampleAppDockSpace

    if (ImGui::BeginMenu("Window"))
    {
        // ImGui::Separator();
        if (ImGui::MenuItem(
            "Docking Over Central Node", 
            "", 
            (dockspace_flags & ImGuiDockNodeFlags_NoDockingOverCentralNode) == 0
        )) 
        { 
            dockspace_flags ^= ImGuiDockNodeFlags_NoDockingOverCentralNode; 
        }

        if (ImGui::MenuItem(
            "Docking Split",         
            "", 
            (dockspace_flags & ImGuiDockNodeFlags_NoDockingSplit) == 0
        ))             
        { 
            dockspace_flags ^= ImGuiDockNodeFlags_NoDockingSplit; 
        }
        
        if (ImGui::MenuItem(
            "Undocking",            
            "", 
            (dockspace_flags & ImGuiDockNodeFlags_NoUndocking) == 0
        ))                
        { 
            dockspace_flags ^= ImGuiDockNodeFlags_NoUndocking; 
        }
        
        if (ImGui::MenuItem(
            "Resize",               
            "", 
            (dockspace_flags & ImGuiDockNodeFlags_NoResize) == 0
        ))                   
        { 
            dockspace_flags ^= ImGuiDockNodeFlags_NoResize; 
        }

        if (ImGui::MenuItem(
            "Auto Hide Tab Bar",         
            "", 
            (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0
        ))             
        { 
            dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; 
        }
        // ImGui::Separator();

        ImGui::EndMenu();
    }
}


void s_setUpFonts()
{
    // written by ChatGPT o1-preview, can't be bothered by god damned fonts
    ImGuiIO& io = ImGui::GetIO();

    // Build custom glyph ranges
    ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;

    // Add the glyph ranges you need
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());

    ImFont* font = io.Fonts->AddFontFromFileTTF(
        "../asset/fonts/NotoSans-Regular.ttf",
        20.0f, NULL, io.Fonts->GetGlyphRangesDefault()
    );

    // Configure font merging
    ImFontConfig config;
    config.MergeMode = true;

    builder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
    builder.AddRanges(io.Fonts->GetGlyphRangesJapanese()); 
    builder.AddRanges(io.Fonts->GetGlyphRangesKorean());
    builder.BuildRanges(&ranges);

    io.Fonts->AddFontFromFileTTF(
        "../asset/fonts/NotoSansCJK-VF.ttf.ttc", 
        20.0f, &config, ranges.Data
    );

    // Build the font atlas
    io.Fonts->Build();    
}
