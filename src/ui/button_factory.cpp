#include "ui/button_factory.hpp"

#include <utility>

namespace ui {

std::unique_ptr<Button> ButtonFactory::create(std::string label,
                                              Widget::EventHandler on_click) const {
    auto button = ui::button(std::move(label)).on_click(std::move(on_click)).build();
    Button* raw = button.get();
    FocusGroup& group = focus_group_;
    button->set_on_hover([&group, raw] { group.focus(*raw); });
    group.add(*raw);
    return button;
}

} // namespace ui
