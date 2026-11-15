#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "core/registry.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/render/inventory_grid.hpp"
#include "scene/world/render/terrain_renderer.hpp"
#include "support/fake_canvas.hpp"
#include "ui/theme.hpp"

namespace {

bool inside(const ui::Rect& inner, const ui::Rect& outer) {
    return inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}

// An entity with a 40-slot inventory and a grid showing it, laid out at (0, 0).
struct Grid {
    Grid() : id(registry.create("Player")) {
        registry.add(id, world::Inventory{40});
        grid = std::make_unique<scene::world::InventoryGrid>(registry, id);
    }

    world::Inventory& inventory() { return *registry.get<world::Inventory>(id); }

    void draw() {
        grid->layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);
        grid->draw(canvas);
    }

    core::Registry registry;
    core::EntityId id;
    std::unique_ptr<scene::world::InventoryGrid> grid;
    FakeCanvas canvas;
};

std::optional<ui::Color> face_of(const FakeCanvas& canvas, const ui::Rect& rect) {
    for (const FilledRect& fill : canvas.fills) {
        if (fill.rect.x == rect.x && fill.rect.y == rect.y && fill.rect.width == rect.width &&
            fill.rect.height == rect.height) {
            return fill.color;
        }
    }
    return std::nullopt;
}

} // namespace

TEST(InventoryGrid, FortySlotsAreLaidOutInFourRowsOfTen) {
    // Given
    Grid g;

    // When
    g.draw();

    // Then
    ASSERT_EQ(g.grid->slot_bounds().size(), 40u);
    const auto& slots = g.grid->slot_bounds();
    EXPECT_FLOAT_EQ(slots[9].y, slots[0].y);
    EXPECT_GT(slots[9].x, slots[8].x);
    EXPECT_FLOAT_EQ(slots[10].x, slots[0].x);
    EXPECT_GT(slots[10].y, slots[0].y);
    EXPECT_FLOAT_EQ(slots[39].x, slots[9].x);
    EXPECT_FLOAT_EQ(slots[39].y, slots[30].y);
}

TEST(InventoryGrid, AFilledSlotShowsTheItemColourAndItsCount) {
    // Given
    Grid g;
    g.inventory().set_slot(11, world::ItemStack{world::items::CopperOre, 37});

    // When
    g.draw();

    // Then
    const ui::Rect slot = g.grid->slot_bounds()[11];
    bool coloured = false;
    for (const FilledRect& fill : g.canvas.fills) {
        if (fill.color == scene::world::CopperOreColor && inside(fill.rect, slot)) coloured = true;
    }
    EXPECT_TRUE(coloured);
    bool counted = false;
    for (const DrawnText& text : g.canvas.texts) {
        if (text.text == "37" && slot.contains(text.position)) counted = true;
    }
    EXPECT_TRUE(counted);
}

TEST(InventoryGrid, AnEmptyInventoryShowsNoItemsOrCounts) {
    // Given
    Grid g;

    // When
    g.draw();

    // Then
    EXPECT_TRUE(g.canvas.texts.empty());
    for (const FilledRect& fill : g.canvas.fills) {
        EXPECT_NE(fill.color, scene::world::IronOreColor);
        EXPECT_NE(fill.color, scene::world::CopperOreColor);
        EXPECT_NE(fill.color, scene::world::CoalColor);
    }
}

TEST(InventoryGrid, AnEntityWithoutAnInventoryDrawsNoSlots) {
    // Given
    core::Registry registry;
    const core::EntityId id = registry.create("Rock");
    scene::world::InventoryGrid grid(registry, id);
    FakeCanvas canvas;

    // When
    grid.layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);
    grid.draw(canvas);

    // Then
    EXPECT_TRUE(grid.slot_bounds().empty());
    EXPECT_TRUE(canvas.fills.empty());
}

TEST(InventoryGrid, SlotsLookLikeButtonsAndTheHoveredOneChangesColour) {
    // Given
    Grid g;
    g.grid->layout({0.0f, 0.0f, 0.0f, 0.0f}, g.canvas);
    const ui::Rect hovered = g.grid->slot_bounds()[5];

    // When
    g.grid->handle_pointer({hovered.x + 1.0f, hovered.y + 1.0f}, false);
    g.grid->draw(g.canvas);

    // Then
    EXPECT_EQ(face_of(g.canvas, hovered), ui::theme::ButtonFaceHover);
    EXPECT_EQ(face_of(g.canvas, g.grid->slot_bounds()[4]), ui::theme::ButtonFace);
}

TEST(InventoryGrid, MovingThePointerAwayRestoresTheSlotColour) {
    // Given
    Grid g;
    g.grid->layout({0.0f, 0.0f, 0.0f, 0.0f}, g.canvas);
    const ui::Rect slot = g.grid->slot_bounds()[5];
    g.grid->handle_pointer({slot.x + 1.0f, slot.y + 1.0f}, false);

    // When
    g.grid->handle_pointer({-100.0f, -100.0f}, false);
    g.grid->draw(g.canvas);

    // Then
    EXPECT_EQ(face_of(g.canvas, slot), ui::theme::ButtonFace);
}
