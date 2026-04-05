#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include "tools/cpp/runfiles/runfiles.h"
using bazel::tools::cpp::runfiles::Runfiles;

#include "src/base/include/PageManager.h"
#include "src/ui/include/SerialAssistantPage.h"

static std::string ResolveRunfilePath(const bazel::tools::cpp::runfiles::Runfiles* runfiles, const std::string& runfile_path)
{
    if (runfiles == nullptr) {
        return runfile_path;
    }
    const std::string resolved = runfiles->Rlocation(runfile_path);
    return resolved.empty() ? runfile_path : resolved;
}

static void ConfigureUIFont(ImGuiIO& io, const bazel::tools::cpp::runfiles::Runfiles* runfiles) {
    // 总字号
    constexpr float kUiFontSize = 18.0f;
    // 英文字体
    const std::string english_font_path = ResolveRunfilePath(
        runfiles,
        "gui/assets/fonts/consola_mono/ConsolaMono-Bold.ttf"
    );
    // 中文字体
    const std::string chinese_font_path = ResolveRunfilePath(
        runfiles,
        "gui/assets/fonts/Sarasa-TTC-1.0.36/Sarasa-Bold.ttc"
    );

    // 英文字体配置
    ImFont* english_font = nullptr;
    if (std::filesystem::exists(english_font_path)) {
        ImFontConfig en_cfg;
        en_cfg.OversampleH = 2;
        en_cfg.OversampleV = 2;
        en_cfg.PixelSnapH = false;
        english_font = io.Fonts->AddFontFromFileTTF(
            english_font_path.c_str(),
            kUiFontSize,
            &en_cfg,
            io.Fonts->GetGlyphRangesDefault()
        );
        if (english_font) {
            io.FontDefault = english_font;
            std::cerr << "[Font] Loaded English font: " << english_font_path << std::endl;
        } else {
            std::cerr << "[Font][WARN] English font exists but failed to load: " << english_font_path << std::endl;
        }
    } else {
        std::cerr << "[Font][WARN] English font file not found: " << english_font_path << std::endl;
    }

    if (english_font == nullptr) {
        ImFontConfig def_cfg;
        def_cfg.SizePixels = kUiFontSize;
        english_font = io.Fonts->AddFontDefault(&def_cfg);
        io.FontDefault = english_font;
        std::cerr << "[Font] Fallback to default font for Latin glyphs.\n";
    }

    // 中文字体配置
    if (std::filesystem::exists(chinese_font_path)) {
        ImFontConfig zh_cfg;
        zh_cfg.MergeMode = true;
        zh_cfg.OversampleH = 2;
        zh_cfg.OversampleV = 2;
        zh_cfg.PixelSnapH = false;
        ImFont* merged = io.Fonts->AddFontFromFileTTF(
            chinese_font_path.c_str(),
            kUiFontSize,
            &zh_cfg,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon()
        );
        if (merged) {
            std::cerr << "[Font] Merged Chinese font: " << chinese_font_path << std::endl;
        } else {
            std::cerr << "[Font][WARN] Chinese font exists but merge failed: " << chinese_font_path << std::endl;
        }
    } else {
        std::cerr << "[Font][WARN] Chinese font file not found, Chinese glyphs may fallback/miss: " << chinese_font_path << std::endl;
    }
}

// 空页面，用于占位
class DummyPage : public IPage {
public:
    const char* GetPageName() const override { return "None"; }
    const char* GetIcon() const override { return "•"; }

    void RenderSidePanel() override {}
    void RenderMainWorkspace() override {}
};

int main(int, char** argv) {
    // 1. 初始化 GLFW
    if (!glfwInit())
        return 1;

    // 决定 GL+GLSL 版本 (这里使用 GL 3.0 + GLSL 130)
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // 2. 创建窗口
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui OpenGL+GLFW App", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 开启垂直同步 (VSync)

    // 3. 初始化 Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 允许键盘控制
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // 允许停靠

    std::string runfiles_error;
    std::unique_ptr<Runfiles> runfiles(Runfiles::Create(argv[0], &runfiles_error));
    if (!runfiles) {
        std::cerr << "[Font][WARN] Failed to initialize Bazel runfiles: " << runfiles_error
                  << ". Falling back to plain relative paths.\n";
    }
    ConfigureUIFont(io, runfiles.get());

    PageManager page_manager;
    // page_manager.RegisterPage(std::make_shared<DummyPage>());
    page_manager.RegisterPage(std::make_shared<SerialAssistantPage>());
    page_manager.ApplyVSCodeLikeTheme();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 4. 主循环
    while (!glfwWindowShouldClose(window)) {
        // 轮询事件
        glfwPollEvents();

        // 开启新一帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        page_manager.Render();

        // 渲染准备
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.118f, 0.118f, 0.118f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 将 ImGui 渲染数据提交给 OpenGL
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 交换缓冲区
        glfwSwapBuffers(window);
    }

    // 5. 清理资源
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}