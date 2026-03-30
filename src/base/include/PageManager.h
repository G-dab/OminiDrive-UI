#pragma once

#include <memory>
#include <vector>

#include "../../template/include/IPage.h"

class PageManager {
public:
    void RegisterPage(const std::shared_ptr<IPage>& page);
    void Render();

    void ApplyVSCodeLikeTheme() const;

private:
    std::vector<std::shared_ptr<IPage>> pages_;
    int selected_page_index_ = 0;
};

