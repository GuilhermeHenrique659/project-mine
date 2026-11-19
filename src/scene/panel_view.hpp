#pragma once

#include <functional>
#include <memory>

#include "input/input_handler.hpp"
#include "input/input_map.hpp"
#include "input/keyboard.hpp"
#include "input/pointer.hpp"
#include "scene/scene.hpp"
#include "ui/box.hpp"
#include "ui/canvas.hpp"
#include "ui/focus_group.hpp"

namespace scene {

// A menu screen: one widget tree (usually a ui::card()), centred horizontally CardTop
// pixels below the top of the screen, plus the keyboard focus of its controls. A subclass
// builds the tree in its constructor and hands it to set_root(). Clicks and the wheel reach
// the tree through handle_pointer() / handle_scroll() (see PanelController).
class PanelView : public View {
public:
    static constexpr float CardTop = 80.0f;

    // Places the tree on a screen of the given width.
    void layout(const ui::Canvas& canvas, float screen_width) const;
    void draw(ui::Canvas& canvas, ui::Size screen) const;
    // Draws on the raylib window.
    void draw() const override;

    // Pointer position in screen space; positions come from the last layout.
    virtual void handle_pointer(ui::Point position, bool released);
    // Mouse wheel notches; true when a scroll view under the pointer took them.
    bool handle_scroll(ui::Point position, float notches);

    // Keyboard navigation over the screen's controls.
    ui::FocusGroup& focus_group() { return focus_group_; }

    const ui::Box& root() const { return *root_; }

protected:
    void set_root(std::unique_ptr<ui::Box> root) { root_ = std::move(root); }

    ui::FocusGroup focus_group_;

private:
    std::unique_ptr<ui::Box> root_;
};

// Drives a PanelView: each frame it forwards the injected pointer (click and wheel) to the
// view, then polls the keyboard through an InputHandler. Up/Down move the focus, Confirm
// activates the focused control and Cancel runs on_cancel(). Screens add their own actions
// with input().on(...). The view must outlive it.
class PanelController : public Controller {
public:
    PanelController(PanelView& view, std::unique_ptr<input::Keyboard> keyboard, input::InputMap map,
                    std::shared_ptr<const input::Pointer> pointer);

    void handle_input() override;

    void on_cancel(std::function<void()> handler);
    input::InputHandler& input() { return input_; }
    input::Keyboard& keyboard() { return *keyboard_; }

protected:
    // Forwards the pointer to the view.
    void handle_pointer();

private:
    PanelView& view_;
    std::unique_ptr<input::Keyboard> keyboard_; // declared before input_, which references it
    input::InputHandler input_;
    std::shared_ptr<const input::Pointer> pointer_;
};

} // namespace scene
