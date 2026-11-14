#pragma once

#include <string>
#include <vector>

#include "ui/canvas.hpp"

struct FilledRect {
    ui::Rect rect;
    ui::Color color;
};

struct DrawnText {
    std::string text;
    ui::Point position;
    int font_size;
    ui::Color color;
};

// Records draw calls. Text is measured as font_size / 2 wide per character.
struct FakeCanvas : ui::Canvas {
    void fill_rect(ui::Rect rect, ui::Color color) override { fills.push_back({rect, color}); }
    void stroke_rect(ui::Rect, ui::Color, float) override {}
    void draw_text(const std::string& text, ui::Point position, int font_size,
                   ui::Color color) override {
        texts.push_back({text, position, font_size, color});
    }
    ui::Size measure_text(const std::string& text, int font_size) const override {
        return {static_cast<float>(text.size()) * static_cast<float>(font_size) / 2.0f,
                static_cast<float>(font_size)};
    }

    std::vector<FilledRect> fills;
    std::vector<DrawnText> texts;
};
