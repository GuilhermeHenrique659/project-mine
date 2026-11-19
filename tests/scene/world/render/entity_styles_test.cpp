#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/render/entity_renderer.hpp"
#include "scene/world/render/entity_styles.hpp"
#include "support/fake_canvas.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

// A kind of entity the game does not have: a crate.
struct Crate {};

// Crates show as a small cube labelled "<label> [crate]"; hidden ones are not drawn.
class CrateStyle : public scene::world::EntityStyle {
public:
    explicit CrateStyle(std::string tag) : tag_(std::move(tag)) {}

    bool applies_to(const core::Registry& registry, core::EntityId id) const override {
        return registry.has<Crate>(id);
    }
    scene::world::EntityLook look(const core::Registry&, core::EntityId,
                                  const scene::world::LookContext&) const override {
        return {scene::world::Cube(10.0f), std::nullopt, std::nullopt};
    }
    std::string label(const core::Registry&, const core::Entity& entity) const override {
        return entity.label + " [" + tag_ + "]";
    }

private:
    std::string tag_;
};

scene::world::EntityStyles styles_of(std::vector<std::string> tags) {
    std::vector<std::unique_ptr<scene::world::EntityStyle>> styles;
    for (std::string& tag : tags) {
        styles.push_back(std::make_unique<CrateStyle>(std::move(tag)));
    }
    return scene::world::EntityStyles(std::move(styles));
}

} // namespace

TEST(EntityStyles, ANewKindOfEntityIsDrawnByTheStyleAddedForIt) {
    // Given
    core::Registry registry;
    const core::EntityId crate = registry.create("Box");
    registry.add(crate, Crate{});
    registry.add(crate, world::Position{{0.0f, 0.0f}});
    const scene::world::EntityStyles styles = styles_of({"crate"});
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera, graphics::GameSprites::none(),
                                                styles);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    ASSERT_EQ(canvas.texts.size(), 1u);
    EXPECT_EQ(canvas.texts.front().text, "Box [crate]");
}

TEST(EntityStyles, TheFirstStyleThatAppliesWins) {
    // Given: two styles apply to crates
    core::Registry registry;
    const core::EntityId crate = registry.create("Box");
    registry.add(crate, Crate{});
    const scene::world::EntityStyles styles = styles_of({"first", "second"});

    // When
    const core::Entity& entity = *registry.entities().begin();
    const std::string label = styles.style_for(registry, crate).label(registry, entity);

    // Then
    EXPECT_EQ(label, "Box [first]");
}

TEST(EntityStyles, AnEntityNoStyleAppliesToIsAPlainCubeWithItsLabel) {
    // Given: a plain entity and a factory that only knows crates
    core::Registry registry;
    const core::EntityId rock = registry.create("Rock");
    registry.add(rock, world::Position{{0.0f, 0.0f}});
    const scene::world::EntityStyles styles = styles_of({"crate"});
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera, graphics::GameSprites::none(),
                                                styles);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then: one cube (6 triangles) and the bare label
    EXPECT_EQ(canvas.triangles.size(), 6u);
    ASSERT_EQ(canvas.texts.size(), 1u);
    EXPECT_EQ(canvas.texts.front().text, "Rock");
}
