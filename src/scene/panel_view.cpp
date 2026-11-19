#include "scene/panel_view.hpp"

#include "ui/raylib_canvas.hpp"

namespace scene {

void PanelView::layout(const ui::Canvas& canvas, float screen_width) const {
    const ui::Size size = root_->preferred_size(canvas);
    root_->layout({(screen_width - size.width) / 2.0f, CardTop, size.width, size.height}, canvas);
}

void PanelView::draw(ui::Canvas& canvas, ui::Size screen) const {
    layout(canvas, screen.width);
    root_->draw(canvas);
}

void PanelView::draw() const {
    ui::RaylibCanvas canvas;
    draw(canvas, ui::RaylibCanvas::screen_size());
}

void PanelView::handle_pointer(ui::Point position, bool released) {
    root_->handle_pointer(position, released);
}

bool PanelView::handle_scroll(ui::Point position, float notches) {
    return root_->handle_scroll(position, notches);
}

PanelController::PanelController(PanelView& view, std::unique_ptr<input::Keyboard> keyboard,
                                 input::InputMap map,
                                 std::shared_ptr<const input::Pointer> pointer)
    : view_(view), keyboard_(std::move(keyboard)), input_(*keyboard_, std::move(map)),
      pointer_(std::move(pointer)) {
    ui::FocusGroup& focus = view_.focus_group();
    input_.on(input::Action::NavigateUp, [&focus] { focus.focus_previous(); });
    input_.on(input::Action::NavigateDown, [&focus] { focus.focus_next(); });
    input_.on(input::Action::Confirm, [&focus] { focus.activate_focused(); });
}

void PanelController::on_cancel(std::function<void()> handler) {
    input_.on(input::Action::Cancel, std::move(handler));
}

void PanelController::handle_input() {
    handle_pointer();
    input_.update();
}

void PanelController::handle_pointer() {
    const input::PointerPosition at = pointer_->position();
    view_.handle_pointer({at.x, at.y}, pointer_->released());
    view_.handle_scroll({at.x, at.y}, pointer_->scroll());
}

} // namespace scene
