#pragma once

#include <string>
#include <vector>

#include "ui/canvas.hpp"

struct FilledRect {
    ui::Rect rect;
    ui::Color color;
};

struct FilledTriangle {
    ui::Point a;
    ui::Point b;
    ui::Point c;
    ui::Color color;
};

struct DrawnText {
    std::string text;
    ui::Point position;
    int font_size;
    ui::Color color;
};

struct DrawnTexture {
    graphics::Texture texture;
    ui::Rect dest;
    ui::TextureDraw options;
};

// Records draw calls. Text is measured as font_size / 2 wide per character.
struct FakeCanvas : ui::Canvas {
    void fill_rect(ui::Rect rect, ui::Color color) override { fills.push_back({rect, color}); }
    void fill_triangle(ui::Point a, ui::Point b, ui::Point c, ui::Color color) override {
        triangles.push_back({a, b, c, color});
    }
    void stroke_rect(ui::Rect, ui::Color, float) override {}
    void draw_text(const std::string& text, ui::Point position, int font_size,
                   ui::Color color) override {
        texts.push_back({text, position, font_size, color});
    }
    using ui::Canvas::draw_texture;
    void draw_texture(const graphics::Texture& texture, ui::Rect dest,
                      const ui::TextureDraw& options) override {
        textures.push_back({texture, dest, options});
    }
    ui::Size measure_text(const std::string& text, int font_size) const override {
        return {static_cast<float>(text.size()) * static_cast<float>(font_size) / 2.0f,
                static_cast<float>(font_size)};
    }

    std::vector<FilledRect> fills;
    std::vector<FilledTriangle> triangles;
    std::vector<DrawnText> texts;
    std::vector<DrawnTexture> textures;
};
