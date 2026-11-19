#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ui/geometry.hpp"

namespace graphics {

// A run of frames in one row of a sheet, played at a fixed rate.
struct SpriteClip {
    std::string name;
    int first = 0;   // first column
    int count = 1;   // frames, side by side from `first`
    float frames_per_second = 0.0f;
    bool loop = true; // otherwise it holds its last frame
};

// What a `.sprite` file describes: a sheet of equal frames laid out in `rows` x `columns`,
// the pixel of a frame that sits on the ground point, and its named clips. The caller picks
// the row (a facing, a variant) and a clip plays along that row.
struct SpriteDefinition {
    std::string texture; // file name, next to the .sprite file
    int frame_width = 0;
    int frame_height = 0;
    int columns = 1;
    int rows = 1;
    ui::Point anchor;
    std::vector<SpriteClip> clips;
};

// Reads a `.sprite` file: `key=value` lines, `#` starts a comment.
//
//   texture=headquarters_sheet.png     (required)
//   frame_width=336                    (required)
//   frame_height=352                   (required)
//   columns=8  rows=1                  (default 1)
//   anchor_x=168  anchor_y=264         (default 0)
//   clip.idle=0,8,4,loop               (first column, count, fps, loop|once)
//
// Empty, with `error` set to the reason, when a required key is missing, a key is unknown,
// a number is invalid, or a clip runs outside its row.
std::optional<SpriteDefinition> parse_sprite_definition(std::string_view text,
                                                        std::string* error = nullptr);

} // namespace graphics
