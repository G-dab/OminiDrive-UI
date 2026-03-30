#pragma once

#include "imgui.h"

// Extension guide:
// 1) Add a new page class under src/ui/include + src/ui/src (e.g. CameraPage.h/.cpp).
// 2) Inherit from IPage and implement all methods below.
// 3) Register the page instance in main.cpp via PageManager::RegisterPage().
// No changes are required in the framework shell (src/base).
class IPage {
public:
    virtual ~IPage() = default;

    virtual const char* GetPageName() const = 0;
    virtual const char* GetIcon() const = 0;

    virtual void RenderSidePanel() = 0;
    virtual void RenderMainWorkspace() = 0;
};

