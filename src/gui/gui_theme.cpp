#include "gui_theme.hpp"

#include "imgui.h"
#include "implot.h"

#include "gui_color.hpp"

extern guiColorPalette g_color;

void guiThemeApplyColor()
{
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, g_color.crust);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g_color.mantle); // menu
    ImGui::PushStyleColor(ImGuiCol_Border, g_color.surface0);

    ImGui::PushStyleColor(ImGuiCol_Tab, g_color.mantle);
    ImGui::PushStyleColor(ImGuiCol_TabHovered, g_color.mantle);
    ImGui::PushStyleColor(ImGuiCol_TabSelected, g_color.mantle);
    ImGui::PushStyleColor(ImGuiCol_TabSelectedOverline, g_color.peach);
    ImGui::PushStyleColor(ImGuiCol_TabDimmed, g_color.mantle);
    ImGui::PushStyleColor(ImGuiCol_TabDimmedSelected, g_color.mantle);
    ImGui::PushStyleColor(ImGuiCol_TabDimmedSelectedOverline, g_color.mantle);

    // menu items
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, g_color.base);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, g_color.surface0);

    ImGui::PushStyleColor(ImGuiCol_DockingPreview, g_color.lavender);
    ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, g_color.mantle);

    ImGui::PushStyleColor(ImGuiCol_TitleBg, g_color.mantle);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, g_color.mantle);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, g_color.mantle);

    ImGui::PushStyleColor(ImGuiCol_Text, g_color.text);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, g_color.base);

    ImGui::PushStyleColor(ImGuiCol_Button, g_color.blue);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_color.subtext1);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, g_color.text);

    ImGui::PushStyleColor(ImGuiCol_SliderGrab, g_color.peach);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, g_color.rosewater);

    // plot, dropdown, slider color
    ImGui::PushStyleColor(ImGuiCol_FrameBg, g_color.surface1);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, g_color.surface2);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, g_color.surface2);

    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, g_color.surface0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, g_color.surface1);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, g_color.surface2);

    // ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, g_color.surface0);

    // doesn't work
    // ImGui::PushStyleColor(ImPlotCol_Line, g_color.text);
    // ImGui::PushStyleColor(ImPlotCol_FrameBg, g_color.surface0);
}


void guiThemeApplyStyling()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
}
