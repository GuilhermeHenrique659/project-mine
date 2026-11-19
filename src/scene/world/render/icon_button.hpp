#pragma once

#include <string>
#include <utility>

#include "ui/widget.hpp"

namespace scene::world {

// A button of a fixed size showing a texture's icon (inset on its left) and an optional
// title right of it, painted like an inventory slot (theme::slot_style()): the gray face,
// darker on hover, green while active (an open tab), dark with a muted title while disabled.
// A disabled button ignores the pointer. The build bar's tabs and list entries are these.
class IconButton : public ui::Widget {
public:
    IconButton(std::string icon, std::string title, ui::Size size);

    ui::Size preferred_size(const ui::Canvas& canvas) const override;
    void draw(ui::Canvas& canvas) const override;
    void handle_pointer(ui::Point position, bool released) override;

    void set_active(bool active) { active_ = active; }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool enabled() const { return enabled_; }

private:
    std::string icon_;
    std::string title_;
    ui::Size size_;
    bool active_ = false;
    bool enabled_ = true;
};

} // namespace scene::world
