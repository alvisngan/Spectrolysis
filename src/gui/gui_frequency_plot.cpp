#include "gui_components.hpp"

#include "imgui.h"
#include "implot.h"
#include "gui_color.hpp"

extern guiColorPalette g_color;


/// TODO:   change tick label based on floor dB
void guiFrequencyPlot(float* x, float* y, int len, bool logScale)
{
    ImGui::Begin("Frequency");
    {
        if (ImPlot::BeginPlot("Frequency Plot"))
        {
            ImPlot::SetupAxes("Frequency (Hz)", "");
            
            if (logScale)
            {
                ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
            }

            ImPlot::SetupAxisLimits(ImAxis_X1, 5.0, 22000.0);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 1.0);

            ImPlot::SetupAxisZoomConstraints(ImAxis_X1, 5.0, 22000.0);
            ImPlot::SetupAxisZoomConstraints(ImAxis_Y1, 0.0, 1.0);

            const char* const xTickLabels[] = {
                "-120dB",
                "-90dB",
                "-60dB",
                "-30dB",
                "0dB"
            };
            
            ImPlot::SetupAxisTicks(ImAxis_Y1, 0.0, 1.0, 5, xTickLabels);

            ImPlot::SetNextLineStyle(g_color.teal);

            ImPlot::PlotLine("signal", x, y, len);
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}