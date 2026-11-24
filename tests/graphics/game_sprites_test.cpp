#include <gtest/gtest.h>

#include <set>
#include <string>

#include "graphics/game_sprites.hpp"

using graphics::GameSprites;
using graphics::SpriteClip;
using graphics::SpriteDefinition;
using graphics::SpriteLibrary;
using world::BuildingType;
using world::CharacterState;
using world::Direction;
using world::TileType;

namespace {

// 8 frames of 336x352 in one row, the antenna turning at 4 fps.
SpriteDefinition headquarters() {
    return {"hq.png", 336, 352, 8, 1, {168.0f, 264.0f}, {SpriteClip{"idle", 0, 8, 4.0f, true}}};
}

// 8 rows (facings) of 7 columns: standing, then a 6-frame walk cycle.
SpriteDefinition character() {
    return {"character.png",
            32,
            48,
            7,
            8,
            {16.0f, 44.0f},
            {SpriteClip{"idle", 0, 1, 0.0f, true}, SpriteClip{"walk", 1, 6, 10.0f, true},
             SpriteClip{"run", 1, 6, 16.0f, true}}};
}

// 8 rows (facings) of 5 columns: parked, then a 4-frame drive cycle (moving treads).
SpriteDefinition battle_tank() {
    return {"battle_tank.png", 80, 64, 5, 8, {40.0f, 40.0f},
            {SpriteClip{"idle", 0, 1, 0.0f, true}, SpriteClip{"drive", 1, 4, 8.0f, true}}};
}

SpriteDefinition tile(int rows, int columns, float fps) {
    return {"tile.png", 64, 32, columns, rows, {}, {SpriteClip{"idle", 0, columns, fps, true}}};
}

// The sheet cell of a frame: {column, row}.
std::pair<int, int> cell_of(const graphics::SpriteFrame& frame) {
    return {static_cast<int>(frame.source.x / frame.size.width),
            static_cast<int>(frame.source.y / frame.size.height)};
}

} // namespace

TEST(GameSprites, ABuildingSpriteIsFoundByItsTypeInSnakeCase) {
    // Given
    SpriteLibrary library;
    library.add("buildings/electronic_factory", headquarters(), graphics::TextureId{});

    // When
    const GameSprites sprites(std::move(library));

    // Then
    EXPECT_EQ(graphics::building_sprite_name(BuildingType::ElectronicFactory),
              "buildings/electronic_factory");
    EXPECT_TRUE(sprites.building(BuildingType::ElectronicFactory, 0.0f));
    EXPECT_FALSE(sprites.building(BuildingType::Mine, 0.0f)); // keeps its box
}

TEST(GameSprites, TheHeadquartersAntennaTurnsAndStartsOverAfterAFullTurn) {
    // Given: a frame lasts 1/4 s, a turn is 8 frames
    SpriteLibrary library;
    library.add("buildings/headquarters", headquarters(), graphics::TextureId{});
    const GameSprites sprites(std::move(library));

    // When
    const auto third = sprites.building(BuildingType::Headquarters, 2.5f * 0.25f);
    const auto turned = sprites.building(BuildingType::Headquarters, 8.5f * 0.25f);

    // Then
    ASSERT_TRUE(third && turned);
    EXPECT_EQ(cell_of(*third).first, 2);
    EXPECT_EQ(cell_of(*turned).first, 0);
    EXPECT_FLOAT_EQ(third->anchor.y, 264.0f);
}

TEST(GameSprites, ACharacterUsesTheRowOfItsFacing) {
    // Given
    SpriteLibrary library;
    library.add(graphics::CharacterSpriteName, character(), graphics::TextureId{});
    const GameSprites sprites(std::move(library));

    // When
    const auto west = sprites.character(Direction::West, CharacterState::Idle, 3.0f);

    // Then: row 6, the standing column
    ASSERT_TRUE(west);
    EXPECT_EQ(cell_of(*west), std::make_pair(0, 6));
}

TEST(GameSprites, AWalkingCharacterPlaysTheWalkCycleAndARunningOneFaster) {
    // Given
    SpriteLibrary library;
    library.add(graphics::CharacterSpriteName, character(), graphics::TextureId{});
    const GameSprites sprites(std::move(library));

    // When: 0.25 s is 2.5 frames walking, 4 frames running
    const auto walking = sprites.character(Direction::North, CharacterState::Moving, 0.25f);
    const auto running = sprites.character(Direction::North, CharacterState::Running, 0.25f);

    // Then: the cycle starts at column 1
    ASSERT_TRUE(walking && running);
    EXPECT_EQ(cell_of(*walking).first, 1 + 2);
    EXPECT_EQ(cell_of(*running).first, 1 + 4);
}

TEST(GameSprites, ACharacterSheetWithoutARunClipWalksWhenRunning) {
    // Given: only idle and walk
    SpriteDefinition walker = character();
    walker.clips.pop_back();
    SpriteLibrary library;
    library.add(graphics::CharacterSpriteName, walker, graphics::TextureId{});
    const GameSprites sprites(std::move(library));

    // When
    const auto running = sprites.character(Direction::North, CharacterState::Running, 0.25f);

    // Then: shown at the walk rate
    ASSERT_TRUE(running);
    EXPECT_EQ(cell_of(*running).first, 1 + 2);
}

TEST(GameSprites, WithoutACharacterSpriteTheCharacterHasNone) {
    // When / Then
    EXPECT_FALSE(GameSprites().character(Direction::North, CharacterState::Idle, 0.0f));
}

TEST(GameSprites, AGrassTileAlwaysKeepsTheSameVariantAndAFieldUsesThemAll) {
    // Given: 4 variants, one per row
    SpriteLibrary library;
    library.add("environment/grass", tile(4, 1, 0.0f), graphics::TextureId{});
    const GameSprites sprites(std::move(library));

    // When
    std::set<int> used;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            used.insert(cell_of(*sprites.tile(TileType::Grass, {x, y}, 0.0f)).second);
        }
    }
    const auto first = sprites.tile(TileType::Grass, {7, -3}, 0.0f);
    const auto later = sprites.tile(TileType::Grass, {7, -3}, 5.3f);

    // Then
    EXPECT_EQ(used.size(), 4u);
    EXPECT_EQ(cell_of(*first), cell_of(*later));
}

TEST(GameSprites, WaterPlaysItsFramesInALoop) {
    // Given: 8 frames at 8 fps
    SpriteLibrary library;
    library.add("environment/water", tile(1, 8, 8.0f), graphics::TextureId{});
    const GameSprites sprites(std::move(library));

    // When
    const auto third = sprites.tile(TileType::Water, {2, 2}, 2.5f / 8.0f);
    const auto looped = sprites.tile(TileType::Water, {2, 2}, 9.5f / 8.0f);

    // Then
    ASSERT_TRUE(third && looped);
    EXPECT_EQ(cell_of(*third).first, 2);
    EXPECT_EQ(cell_of(*looped).first, 1);
}

TEST(GameSprites, EachTileTypeHasItsOwnSpriteName) {
    // Given
    SpriteLibrary library;
    library.add("environment/copper_ore", tile(1, 1, 0.0f), graphics::TextureId{7});
    const GameSprites sprites(std::move(library));

    // When / Then
    EXPECT_EQ(graphics::tile_sprite_name(TileType::IronOre), "environment/iron_ore");
    EXPECT_EQ(graphics::tile_sprite_name(TileType::Coal), "environment/coal");
    ASSERT_TRUE(sprites.tile(TileType::CopperOre, {0, 0}, 0.0f));
    EXPECT_EQ(sprites.tile(TileType::CopperOre, {0, 0}, 0.0f)->texture, graphics::TextureId{7});
    EXPECT_FALSE(sprites.tile(TileType::Coal, {0, 0}, 0.0f));
}

TEST(GameSprites, TheLastBuildingTypesFindTheirSpritesToo) {
    // Given
    SpriteLibrary library;
    library.add("buildings/engine_factory", headquarters(), graphics::TextureId{});
    library.add("buildings/tank_factory", headquarters(), graphics::TextureId{});

    // When
    const GameSprites sprites(std::move(library));

    // Then
    EXPECT_TRUE(sprites.building(BuildingType::EngineFactory, 0.0f));
    EXPECT_TRUE(sprites.building(BuildingType::TankFactory, 0.0f));
}

TEST(GameSprites, AParkedBattleTankUsesTheRowOfItsFacing) {
    // Given
    SpriteLibrary library;
    library.add(graphics::BattleTankSpriteName, battle_tank(), graphics::TextureId{});
    const GameSprites sprites(std::move(library));

    // When
    const auto east = sprites.battle_tank(Direction::East, CharacterState::Idle, 3.0f);

    // Then: row 2, the parked column
    ASSERT_TRUE(east);
    EXPECT_EQ(cell_of(*east), std::make_pair(0, 2));
}

TEST(GameSprites, AMovingBattleTankPlaysTheDriveCycle) {
    // Given
    SpriteLibrary library;
    library.add(graphics::BattleTankSpriteName, battle_tank(), graphics::TextureId{});
    const GameSprites sprites(std::move(library));

    // When: 0.25 s at 8 fps is the third drive frame
    const auto driving = sprites.battle_tank(Direction::South, CharacterState::Moving, 0.25f);

    // Then: the cycle starts at column 1
    ASSERT_TRUE(driving);
    EXPECT_EQ(cell_of(*driving), std::make_pair(1 + 2, 4));
}

TEST(GameSprites, NoBattleTankSheetKeepsItsBox) {
    // Given
    const GameSprites sprites{SpriteLibrary{}};

    // When
    const auto frame = sprites.battle_tank(Direction::North, CharacterState::Idle, 0.0f);

    // Then
    EXPECT_FALSE(frame);
}
