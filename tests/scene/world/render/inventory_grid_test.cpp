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

namespace {

// A 120-slot inventory (12 rows) shown 4 rows at a time, laid out at (0, 0).
struct ScrollGrid {
    ScrollGrid() : id(registry.create("Storage")) {
        registry.add(id, world::Inventory{120});
        grid = std::make_unique<scene::world::InventoryGrid>(registry, id, 4);
    }

    void draw() {
        canvas = FakeCanvas{};
        grid->layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);
        grid->draw(canvas);
    }

    bool drawn_text(const std::string& text) const {
        for (const DrawnText& drawn : canvas.texts) {
            if (drawn.text == text) return true;
        }
        return false;
    }

    core::Registry registry;
    core::EntityId id;
    std::unique_ptr<scene::world::InventoryGrid> grid;
    FakeCanvas canvas;
};

} // namespace

TEST(InventoryGrid, WithLimitedRowsOnlyTheFirstRowsGetBounds) {
    // Given
    ScrollGrid g;

    // When
    g.draw();

    // Then
    const auto& slots = g.grid->slot_bounds();
    ASSERT_EQ(slots.size(), 120u);
    EXPECT_GT(slots[39].width, 0.0f);
    EXPECT_FLOAT_EQ(slots[40].width, 0.0f);
    EXPECT_FLOAT_EQ(slots[119].width, 0.0f);
    EXPECT_LT(g.grid->bounds().height, 5.0f * scene::world::InventoryGrid::SlotSize);
}

TEST(InventoryGrid, ScrollingDownOneNotchShowsTheNextRowAtTheTop) {
    // Given
    ScrollGrid g;
    g.registry.get<world::Inventory>(g.id)->set_slot(0, world::ItemStack{world::items::Coal, 7});
    g.draw();
    const ui::Rect top_left = g.grid->slot_bounds()[0];

    // When
    g.grid->scroll(-1.0f);
    g.draw();

    // Then
    const auto& slots = g.grid->slot_bounds();
    EXPECT_FLOAT_EQ(slots[0].width, 0.0f);
    EXPECT_FLOAT_EQ(slots[10].x, top_left.x);
    EXPECT_FLOAT_EQ(slots[10].y, top_left.y);
    EXPECT_GT(slots[49].width, 0.0f);
    EXPECT_FALSE(g.drawn_text("7"));
}

TEST(InventoryGrid, ScrollingStopsAtTheTopAndAtTheLastRow) {
    // Given
    ScrollGrid g;

    // When
    g.grid->scroll(3.0f);
    const std::size_t at_top = g.grid->first_row();
    g.grid->scroll(-50.0f);

    // Then
    EXPECT_EQ(at_top, 0u);
    EXPECT_EQ(g.grid->first_row(), 8u); // rows 8..11 are the last four
}

TEST(InventoryGrid, AGridThatFitsDoesNotScroll) {
    // Given
    Grid g; // 40 slots, no row limit

    // When
    g.grid->scroll(-1.0f);

    // Then
    EXPECT_EQ(g.grid->first_row(), 0u);
}

TEST(InventoryGrid, AScrollbarIsDrawnOnlyWhenRowsAreHidden) {
    // Given
    ScrollGrid scrolled;
    Grid fits;

    // When
    scrolled.draw();
    fits.draw();

    // Then: something is painted right of the last column
    using scene::world::InventoryGrid;
    const float right = 10.0f * InventoryGrid::SlotSize + 9.0f * InventoryGrid::SlotGap;
    const auto painted_right = [right](const FakeCanvas& canvas) {
        for (const FilledRect& fill : canvas.fills) {
            if (fill.rect.x > right) return true;
        }
        return false;
    };
    EXPECT_TRUE(painted_right(scrolled.canvas));
    EXPECT_FALSE(painted_right(fits.canvas));
}
