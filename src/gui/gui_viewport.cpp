#include "gui_components.hpp"

#include "imgui.h"
#include "gui_color.hpp"

extern guiColorPalette g_color;

// viewport
// --------
static bool s_viewportHovered = false;


void guiViewport(Camera& camera, GLuint textureID)
{
    // std::cout << "viewport initiated" << std::endl;
    ImGui::Begin("Spectrogram");
    {
        ImGui::PushStyleColor(ImGuiCol_Text, g_color.base);
        if (ImGui::Button("Home"))
        {
            camera.returnButton();
        }

        ImGui::SameLine();
        if (ImGui::Button("Undo"))
        {
            camera.undoButton();
        }

        ImGui::SameLine();
        if (ImGui::Button("Redo"))
        {
            camera.redoButton();
        }       
        ImGui::PopStyleColor();

        ImGui::BeginChild("Viewport");

        ImGui::Image(
            textureID, // for ImGui master branch, type casting is required

            // // uncomment the below line, and comment the above line
            // // for the ImGui master branch
            // reinterpret_cast<void*>(static_cast<intptr_t>(textureID)),

            ImGui::GetContentRegionAvail(), 
            ImVec2(0, 1), 
            ImVec2(1, 0)
        );

        s_viewportHovered = ImGui::IsWindowHovered();
        // viewportWidth = ImGui::GetItemRectSize().x;
        // viewportHeight = ImGui::GetItemRectSize().y;
    
        ImGui::EndChild();
    }
    ImGui::End();
}


bool guiViewportGetHovered()
{
    return s_viewportHovered;
}
