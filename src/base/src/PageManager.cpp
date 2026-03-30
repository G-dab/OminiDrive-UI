#include "../include/PageManager.h"

#include <ctime>

namespace {
constexpr float kTopMenuHeight = 28.0f;
constexpr float kActivityBarWidth = 56.0f;
constexpr float kSideBarWidth = 300.0f;
constexpr float kStatusBarHeight = 26.0f;
const ImU32 kSeparatorColor = IM_COL32(70, 76, 84, 255);
} // namespace

void PageManager::RegisterPage(const std::shared_ptr<IPage>& page) {
    if (page) {
        pages_.push_back(page);
    }
}

void PageManager::ApplyVSCodeLikeTheme() const {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.WindowBorderSize = 0.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);      // #1E1E1E
    colors[ImGuiCol_ChildBg] = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.098f, 0.098f, 0.098f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.157f, 0.157f, 0.157f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.176f, 0.176f, 0.176f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.220f, 0.388f, 0.690f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.180f, 0.447f, 0.820f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.176f, 0.176f, 0.176f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.220f, 0.388f, 0.690f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.180f, 0.447f, 0.820f, 1.0f);
}

void PageManager::Render() {
    if (pages_.empty()) return;
    if (selected_page_index_ < 0 || selected_page_index_ >= static_cast<int>(pages_.size())) {
        selected_page_index_ = 0;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 vp_pos = viewport->WorkPos;
    const ImVec2 vp_size = viewport->WorkSize;

    ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                  ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
    ImGui::SetNextWindowPos(vp_pos);
    ImGui::SetNextWindowSize(vp_size);
    ImGui::Begin("MainDockHost", nullptr, host_flags);
    ImGuiID dockspace_id = ImGui::GetID("RootDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    const float content_top = vp_pos.y + kTopMenuHeight;
    const float content_height = vp_size.y - kTopMenuHeight - kStatusBarHeight;

    ImGui::SetNextWindowPos(vp_pos);
    ImGui::SetNextWindowSize(ImVec2(vp_size.x, kTopMenuHeight));
    ImGui::Begin("TopMenuBar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New", nullptr, false, false);
            ImGui::MenuItem("Open...", nullptr, false, false);
            ImGui::MenuItem("Save", nullptr, false, false);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo", nullptr, false, false);
            ImGui::MenuItem("Redo", nullptr, false, false);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Command Palette", nullptr, false, false);
            ImGui::MenuItem("Zoom In", nullptr, false, false);
            ImGui::MenuItem("Zoom Out", nullptr, false, false);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("Settings", nullptr, false, false);
            ImGui::MenuItem("Extensions", nullptr, false, false);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("Documentation", nullptr, false, false);
            ImGui::MenuItem("About", nullptr, false, false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(vp_pos.x, content_top));
    ImGui::SetNextWindowSize(ImVec2(kActivityBarWidth, content_height));
    ImGui::Begin("ActivityBar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    for (int i = 0; i < static_cast<int>(pages_.size()); ++i) {
        const bool selected = (i == selected_page_index_);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.180f, 0.447f, 0.820f, 1.0f));
        if (ImGui::Button(pages_[i]->GetIcon(), ImVec2(40, 40))) selected_page_index_ = i;
        if (selected) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pages_[i]->GetPageName());
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(vp_pos.x + kActivityBarWidth, content_top));
    ImGui::SetNextWindowSize(ImVec2(kSideBarWidth, content_height));
    ImGui::Begin("SideBar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    pages_[selected_page_index_]->RenderSidePanel();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(vp_pos.x + kActivityBarWidth + kSideBarWidth, content_top));
    ImGui::SetNextWindowSize(ImVec2(vp_size.x - kActivityBarWidth - kSideBarWidth, content_height));
    ImGui::Begin("MainArea", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    pages_[selected_page_index_]->RenderMainWorkspace();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(vp_pos.x, content_top + content_height));
    ImGui::SetNextWindowSize(ImVec2(vp_size.x, kStatusBarHeight));
    ImGui::Begin("StatusBar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.478f, 0.800f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.478f, 0.800f, 1.0f));
    ImGui::Text("Ready | Active: %s", pages_[selected_page_index_]->GetPageName());
    ImGui::SameLine();
    ImGui::TextUnformatted("|");
    ImGui::SameLine();

    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    tm_buf = *std::localtime(&t);
#endif
    char time_buf[64];
    std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tm_buf);
    ImGui::Text("Time: %s", time_buf);
    ImGui::PopStyleColor(2);
    ImGui::End();

    ImDrawList* fg = ImGui::GetForegroundDrawList(viewport);
    const float x1 = vp_pos.x + kActivityBarWidth;
    const float x2 = vp_pos.x + kActivityBarWidth + kSideBarWidth;
    const float yMenuBottom = vp_pos.y + kTopMenuHeight;
    const float yStatusTop = content_top + content_height;
    fg->AddLine(ImVec2(vp_pos.x, yMenuBottom), ImVec2(vp_pos.x + vp_size.x, yMenuBottom), kSeparatorColor, 1.0f);
    fg->AddLine(ImVec2(x1, content_top), ImVec2(x1, yStatusTop), kSeparatorColor, 1.0f);
    fg->AddLine(ImVec2(x2, content_top), ImVec2(x2, yStatusTop), kSeparatorColor, 1.0f);
    fg->AddLine(ImVec2(vp_pos.x, yStatusTop), ImVec2(vp_pos.x + vp_size.x, yStatusTop), kSeparatorColor, 1.0f);
}

