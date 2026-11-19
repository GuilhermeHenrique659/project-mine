#pragma once

#include "ui/box.hpp"
#include "ui/theme.hpp"

namespace ui {

// A card is a Box: a column with the card look (theme::card_style(), bevel included) whose
// children stretch to its inner width. `.style()` replaces the look and keeps the stretch.
using Card = Box;
using CardBuilder = BoxBuilder;

inline CardBuilder card() {
    CardBuilder builder = column();
    builder.style(theme::card_style()).align(Align::Stretch);
    return builder;
}

} // namespace ui
