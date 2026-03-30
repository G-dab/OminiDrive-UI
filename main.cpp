// main.cpp
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "src/base/include/PageManager.h"

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

static void ConfigureUIFont(ImGuiIO& io) {
    // Font customization entry:
    // 1) Put real font files under assets/fonts/
    // 2) Replace placeholder file names below
    // 3) Adjust kUiFontSize for global text size
    constexpr float kUiFontSize = 18.0f;
    constexpr const char* kEnglishFontPath = "assets/fonts/ConsolaMono-Book.ttf";
    constexpr const char* kChineseFontPath = "assets/fonts/NotoSansCJK-Regular.ttf";

    ImFont* english_font = nullptr;
    if (std::filesystem::exists(kEnglishFontPath)) {
        ImFontConfig en_cfg;
        en_cfg.OversampleH = 2;
        en_cfg.OversampleV = 2;
        en_cfg.PixelSnapH = false;
        english_font = io.Fonts->AddFontFromFileTTF(
            kEnglishFontPath,
            kUiFontSize,
            &en_cfg,
            io.Fonts->GetGlyphRangesDefault()
        );
        if (english_font) {
            io.FontDefault = english_font;
            std::cerr << "[Font] Loaded English font: " << kEnglishFontPath << std::endl;
        } else {
            std::cerr << "[Font][WARN] English font exists but failed to load: " << kEnglishFontPath << std::endl;
        }
    } else {
        std::cerr << "[Font][WARN] English font file not found: " << kEnglishFontPath << std::endl;
    }

    if (english_font == nullptr) {
        ImFontConfig def_cfg;
        def_cfg.SizePixels = kUiFontSize;
        english_font = io.Fonts->AddFontDefault(&def_cfg);
        io.FontDefault = english_font;
        std::cerr << "[Font] Fallback to default font for Latin glyphs.\n";
    }

    if (std::filesystem::exists(kChineseFontPath)) {
        ImFontConfig zh_cfg;
        zh_cfg.MergeMode = true;
        zh_cfg.OversampleH = 2;
        zh_cfg.OversampleV = 2;
        zh_cfg.PixelSnapH = false;
        ImFont* merged = io.Fonts->AddFontFromFileTTF(
            kChineseFontPath,
            kUiFontSize,
            &zh_cfg,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon()
        );
        if (merged) {
            std::cerr << "[Font] Merged Chinese font: " << kChineseFontPath << std::endl;
        } else {
            std::cerr << "[Font][WARN] Chinese font exists but merge failed: " << kChineseFontPath << std::endl;
        }
    } else {
        std::cerr << "[Font][WARN] Chinese font file not found, Chinese glyphs may fallback/miss: " << kChineseFontPath << std::endl;
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

int main(int, char**) {
    // 1. 设置错误回调并初始化 GLFW
    glfwSetErrorCallback(glfw_error_callback);
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
    ConfigureUIFont(io);

    PageManager page_manager;
    page_manager.RegisterPage(std::make_shared<DummyPage>());
    page_manager.ApplyVSCodeLikeTheme();

    // 初始化后端
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