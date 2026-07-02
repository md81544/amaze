#include "imgui.h"
#include "igraphicsadapter.h"
#include "log.h"

#include <imgui-SFML.h>
#include <imgui.h>

Imgui::Imgui(sf::RenderWindow& window, const std::string& dataDir)
    : m_window(window)
{
    if (!ImGui::SFML::Init(window)) {
        mgo::Log::error("Could not initialise Imgui-Sfml");
    }
    // Load a custom font BEFORE the first UpdateFontTexture call.
    // If you skip this, the default ImGui font is used automatically.
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF((dataDir + "/Oxanium-SemiBold.ttf").c_str(), 18.0f);
    auto _ = ImGui::SFML::UpdateFontTexture();

    // Disable creation of imgui.ini (enable by removing this line if you want
    // imgui windows to store their positions between runs)
    io.IniFilename = nullptr;

    // Enable gamepad input (need to call io.AddKeyEvent() with gamepad-mapped ImGuiKey_Gamepad*
    // values - it doesn't work automatically)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    // Colours
    // Easiest approach: pick a built-in style, then tweak individual colours.
    ImGui::StyleColorsDark(); // or StyleColorsLight(), StyleColorsClassic()

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.f, 0.2f, 0.0f, 0.25f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.f, 0.5f, 0.f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.f, 0.5f, 0.f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.f, 0.9f, 0.0f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.f, 0.45f, 0.f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.65f, 0.0f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.f, 0.65f, 0.f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.f, 0.65f, 0.f, 1.00f);
    // The following affects all widgets, e.g. text input and checkboxes:
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.f, 0.25f, 0.f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.f, 0.45f, 0.f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.f, 0.45f, 0.f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.f, 0.8f, 0.f, 0.5f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.f, 0.9f, 0.f, 0.5f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.f, 0.65f, 0.f, 1.00f);
    style.WindowRounding = 8.0f;
}

Imgui::~Imgui() noexcept
{
    ImGui::SFML::Shutdown();
}

void Imgui::processEvents()
{
    while (const std::optional event = m_window.pollEvent()) {
        ImGui::SFML::ProcessEvent(m_window, *event);
    }
}

void Imgui::build(marengo::amaze::MenuType _)
{
    ImGui::SFML::Update(m_window, m_deltaClock.restart());
    ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_Always);
    ImGui::Begin(
        "Amaze",
        nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_Modal
            | ImGuiWindowFlags_NoMove);

    ImGui::Text("Main Menu");
    ImGui::Separator();

    ImGui::Checkbox("Enable thing", &m_enableThing);
    ImGui::Checkbox("Enable other thing", &m_enableOtherThing);

    ImGui::InputText("Name", m_nameBuffer, sizeof(m_nameBuffer));

    ImGui::SliderFloat("Stick dead zone %", &m_deadZone, 0.0f, 0.25f);

    if (ImGui::Button("Log state")) {
        mgo::Log::debug(
            std::format(
                "thing={} other={} name={} gain={}",
                m_enableThing,
                m_enableOtherThing,
                m_nameBuffer,
                m_deadZone));
    }

    ImGui::SetCursorPosY(360.f);
    ImGui::Separator();

    // Note we're explicitly setting the button's width here.
    // The height of zero means "use default".
    if (ImGui::Button("OK", ImVec2(80, 0))) {
        m_menuState = marengo::amaze::MenuType::Exit;
    }

    ImGui::SameLine(); // makes the next widget stack on the same line

    if (ImGui::Button("Cancel", ImVec2(80, 0))) {
        m_menuState = marengo::amaze::MenuType::Exit;
    }

    // Keyboard shortcuts
    // Note! Use IsKeyReleased() for keypresses to avoid repeats
    if (ImGui::IsKeyReleased(ImGuiKey_Escape)) {
        m_menuState = marengo::amaze::MenuType::Exit;
    }

    ImGui::SameLine();

    if (ImGui::Button("Exit Game", ImVec2(120, 0))) {
        m_quitGame = true;
    }

    ImGui::End();
}

marengo::amaze::MenuType Imgui::render()
{
    ImGui::SFML::Render(m_window);
    if (m_menuState == marengo::amaze::MenuType::Exit) {
        m_menuState = marengo::amaze::MenuType::None;
        m_window.setMouseCursorVisible(false);
        return marengo::amaze::MenuType::Exit;
    }
    return m_menuState;
}