#ifndef GUI_COLOR_HPP
#define GUI_COLOR_HPP

#include <cassert>

#include <glm/glm.hpp>

#include "imgui.h"

/// Struct containing some color from the Catppuccin Frappe color palette
/// PLEASE INITIATE THIS ONLY ONCE, NOT EVERY FRAME
///
struct guiColorPalette {
    ImVec4 crust =      ImColor(35, 38, 52, 255);
    ImVec4 mantle =     ImColor(41, 44, 60, 255);
    ImVec4 base =       ImColor(48, 52, 70, 255);
    ImVec4 surface0 =   ImColor(65, 69, 89, 255);
    ImVec4 surface1 =   ImColor(81, 87, 109, 255);
    ImVec4 surface2 =   ImColor(98, 104, 128, 255);
    ImVec4 overlay0 =   ImColor(115, 121, 148, 255);
    ImVec4 overlay1 =   ImColor(131, 139, 167, 255);
    ImVec4 overlay2 =   ImColor(148, 156, 187, 255);
    ImVec4 text =       ImColor(198, 208, 245, 255);
    ImVec4 subtext0 =   ImColor(165, 173, 206, 255);
    ImVec4 subtext1 =   ImColor(181, 191, 226, 255);
    ImVec4 blue =       ImColor(140, 170, 238, 255);
    ImVec4 green =      ImColor(166, 209, 137, 255);
    ImVec4 yellow =     ImColor(229, 200, 144, 255);
    ImVec4 red =        ImColor(231, 130, 132, 255);
    ImVec4 rosewater =  ImColor(242, 213, 207, 255);
    ImVec4 lavender =   ImColor(186, 187, 241, 255);
    ImVec4 teal =       ImColor(129, 200, 190, 255);
    ImVec4 peach =      ImColor(239, 159, 118, 255);
    ImVec4 mauve =      ImColor(202, 158, 230, 255);
};


// /// Change the alpha value (w) of an ImVec4, won't mutate input
// ///
// /// \param myColor      guiColorPalette.[color], or any ImColor
// ///
// /// \param alpha        floating-point alpha value between [0, 1]
// ///
// ImVec4 guiColorSwapAlpha(const ImVec4 myColor, float alpha)
// {
//     assert (alpha <= 1.0f && alpha >= 0.0f);
//     return ImColor(myColor.x, myColor.y, myColor.z, alpha);
// }


// glm::vec4 guiImColorToGlmVec4(const ImVec4 myVec4)
// {
//     return glm::vec4(myVec4.x, myVec4.y, myVec4.z, myVec4.w);
// }


#endif