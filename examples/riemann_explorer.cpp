// Interactive Hardy-Z zero explorer. This is a visualization aid, not an RH
// proof: the scan uses ordinary double precision and sign changes only.

#include "core/platform/Platform.h"
#include "iqgpu/IQGPURuntime.h"
#include "iqgpu/ZetaCompute.h"
#include "math/RiemannZeta.h"
#include "math/ZetaSpectral.h"
#include "rendering/Renderer.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

constexpr float kPlotMinT = 14.0f;
constexpr int kPlotHeight = 360;

struct PlotData {
    std::vector<float> hardyZ;
    std::vector<double> zeros;
    float maxT = 80.0f;
    float scanStep = 0.01f;
    int sampleCount = 0;
};

void Recompute(PlotData& data, float maxT, int samples, float scanStep) {
    data.maxT = maxT;
    data.scanStep = scanStep;
    data.sampleCount = samples;
    data.hardyZ.resize(static_cast<std::size_t>(samples));
    const double span = static_cast<double>(maxT - kPlotMinT);
    for (int i = 0; i < samples; ++i) {
        const double t = kPlotMinT + span * i / (samples - 1);
        data.hardyZ[static_cast<std::size_t>(i)] =
            static_cast<float>(StonePotatoes::Math::Zeta::HardyZ(t));
    }
    const auto spectrum = StonePotatoes::Math::Zeta::ZeroSpectrum::Compute(
        kPlotMinT, maxT, scanStep);
    data.zeros = spectrum.Gammas();
}

void DrawPlot(const PlotData& data) {
    ImGui::BeginChild("Hardy Z plot", ImVec2(0.0f, kPlotHeight), true);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float width = std::max(available.x, 1.0f);
    const float height = std::max(available.y, 1.0f);
    const float left = origin.x + 48.0f;
    const float right = origin.x + width - 12.0f;
    const float top = origin.y + 12.0f;
    const float bottom = origin.y + height - 28.0f;
    const float plotWidth = std::max(right - left, 1.0f);
    const float plotHeight = std::max(bottom - top, 1.0f);
    const ImU32 grid = IM_COL32(75, 88, 103, 255);
    const ImU32 curve = IM_COL32(94, 190, 255, 255);
    const ImU32 zeroColor = IM_COL32(255, 190, 72, 255);
    ImDrawList* draw = ImGui::GetWindowDrawList();

    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height),
                         IM_COL32(18, 25, 35, 255));
    for (int i = 0; i <= 4; ++i) {
        const float y = top + plotHeight * i / 4.0f;
        draw->AddLine(ImVec2(left, y), ImVec2(right, y), grid);
    }
    draw->AddLine(ImVec2(left, top), ImVec2(left, bottom), grid);
    draw->AddLine(ImVec2(right, top), ImVec2(right, bottom), grid);
    draw->AddText(ImVec2(origin.x + 4.0f, top), IM_COL32_WHITE,
                  "Hardy Z(t)");
    draw->AddText(ImVec2(left, bottom + 6.0f), IM_COL32_WHITE, "14");
    char maxLabel[32]{};
    std::snprintf(maxLabel, sizeof(maxLabel), "%.0f", data.maxT);
    draw->AddText(ImVec2(right - 22.0f, bottom + 6.0f), IM_COL32_WHITE,
                  maxLabel);

    float magnitude = 1.0f;
    for (float value : data.hardyZ) {
        magnitude = std::max(magnitude, std::abs(value));
    }
    const float midY = top + plotHeight * 0.5f;
    draw->AddLine(ImVec2(left, midY), ImVec2(right, midY),
                  IM_COL32(130, 140, 150, 255));
    if (data.hardyZ.size() > 1) {
        for (std::size_t i = 1; i < data.hardyZ.size(); ++i) {
            const float x0 = left + plotWidth *
                static_cast<float>(i - 1) / (data.hardyZ.size() - 1);
            const float x1 = left + plotWidth *
                static_cast<float>(i) / (data.hardyZ.size() - 1);
            const float y0 = midY - data.hardyZ[i - 1] /
                magnitude * plotHeight * 0.45f;
            const float y1 = midY - data.hardyZ[i] /
                magnitude * plotHeight * 0.45f;
            draw->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), curve, 1.5f);
        }
    }

    const double tSpan = static_cast<double>(data.maxT - kPlotMinT);
    for (double gamma : data.zeros) {
        const float x = left + plotWidth * static_cast<float>(
            (gamma - kPlotMinT) / tSpan);
        draw->AddLine(ImVec2(x, top), ImVec2(x, bottom), zeroColor, 1.0f);
    }
    draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height),
                  IM_COL32(80, 90, 105, 255));
    ImGui::Dummy(ImVec2(width, height));
    ImGui::EndChild();
}

} // namespace

int main() {
    using namespace StonePotatoes;
    if (!Platform::Initialize()) {
        std::fprintf(stderr, "Failed to initialize platform\n");
        return 1;
    }

    Rendering::Renderer renderer;
    if (!renderer.Initialize({1100, 720, "StonePotatoes | Riemann Explorer"})) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        Platform::Shutdown();
        return 1;
    }

    IQGPU::IQGPURuntime runtime;
    runtime.Initialize();
    IQGPU::ZetaCompute zeta(runtime);

    float maxT = 80.0f;
    float scanStep = 0.01f;
    int sampleCount = 1800;
    PlotData plot;
    IQGPU::ZetaCompute::GramReport verification;
    bool hasVerification = false;
    Recompute(plot, maxT, sampleCount, scanStep);

    while (!renderer.ShouldClose()) {
        renderer.BeginFrame();
        renderer.BeginImGui();
        ImGui::SetNextWindowPos(ImVec2(14.0f, 14.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(1060.0f, 680.0f),
                                 ImGuiCond_FirstUseEver);
        ImGui::Begin("Riemann Hypothesis Explorer");
        ImGui::TextUnformatted(
            "Critical-line exploration: Hardy Z(t), 14 <= t <= T");
        ImGui::SliderFloat("Upper height T", &maxT, 30.0f, 200.0f, "%.1f");
        ImGui::SliderFloat("Sign-scan step", &scanStep, 0.005f, 0.05f,
                           "%.3f");
        ImGui::SliderInt("Plot samples", &sampleCount, 500, 5000);
        if (ImGui::Button("Recompute samples and sign-change zeros")) {
            Recompute(plot, maxT, sampleCount, scanStep);
            hasVerification = false;
        }
        ImGui::SameLine();
        ImGui::Text("Sign-change candidates: %zu", plot.zeros.size());
        ImGui::Text("Showing T=%.1f, step=%.3f, samples=%d",
                    plot.maxT, plot.scanStep, plot.sampleCount);
        if (ImGui::Button("Run finite Gram/Turing count check")) {
            int64_t gramLast = -1;
            while (gramLast < 100000 &&
                   Math::Zeta::GramPoint(gramLast + 1) <= maxT) {
                ++gramLast;
            }
            verification = zeta.VerifyCriticalLine(-1, gramLast, 8);
            hasVerification = true;
        }
        if (hasVerification) {
            ImGui::Text("Finite count: found %llu / expected %llu; "
                        "uncertain signs: %llu",
                static_cast<unsigned long long>(verification.zerosFound),
                static_cast<unsigned long long>(verification.zerosExpected),
                static_cast<unsigned long long>(verification.uncertainSigns));
            ImGui::Text("Internal checks: verified=%s, Turing-count=%s, "
                        "height=%.3f (g_%lld)",
                verification.verified ? "yes" : "no",
                verification.turingCertified ? "passed" : "not certified",
                verification.certifiedHeight,
                static_cast<long long>(verification.certifiedGram));
        }
        DrawPlot(plot);
        ImGui::TextWrapped(
            "Exploratory only: the plot uses ordinary double precision and "
            "sign-change search can miss zeros. The finite Gram/Turing "
            "status is the engine's numerical report, not an interval-"
            "arithmetic machine-checked proof or a proof of RH.");
        ImGui::End();
        renderer.EndImGui();
        renderer.EndFrame();
    }

    runtime.Shutdown();
    renderer.Shutdown();
    Platform::Shutdown();
    return 0;
}
