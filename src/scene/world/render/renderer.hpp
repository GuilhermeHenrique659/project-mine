#pragma once

#include "ui/ui.hpp"

namespace scene::world {

// One thing the world screen shows. Like a model system, a renderer gets what it
// depends on (a registry, the event bus, an input source) in its constructor, and
// WorldView keeps all of them in one list: update() polls input for the interactive
// ones, draw() paints, in list order (first = bottom).
class Renderer {
public:
    virtual ~Renderer() = default;

    // A hidden renderer is neither updated nor drawn.
    virtual bool visible() const { return true; }

    // Whether the renderer takes the pointer at `position` (from the last layout): the
    // renderers below it then see no click, held button or wheel this frame.
    virtual bool captures_pointer(ui::Point) const { return false; }

    // Polls input; most renderers have none.
    virtual void update() {}

    virtual void draw(ui::Canvas& canvas, ui::Size screen) const = 0;
};

} // namespace scene::world
