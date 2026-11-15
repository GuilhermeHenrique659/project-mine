#include <gtest/gtest.h>

#include <set>

#include "scene/world/core/construction/construction.hpp"

TEST(ConstructionCatalog, HasSpecialMinesFactoriesAndMilitaryInThatOrder) {
    // Given / When
    const auto& catalog = world::construction_catalog();

    // Then
    ASSERT_EQ(catalog.size(), 4u);
    EXPECT_EQ(catalog[0].category, world::ConstructionCategory::Special);
    EXPECT_EQ(catalog[1].category, world::ConstructionCategory::Mine);
    EXPECT_EQ(catalog[2].category, world::ConstructionCategory::Factory);
    EXPECT_EQ(catalog[3].category, world::ConstructionCategory::Military);
}

TEST(ConstructionCatalog, EveryCategoryOffersConstructionsWithTheirOwnIcons) {
    // Given
    const auto& catalog = world::construction_catalog();
    std::set<std::string> icons;
    std::size_t total = 0;

    // When
    for (const auto& group : catalog) {
        EXPECT_FALSE(group.constructions.empty()) << group.name;
        icons.insert(group.icon);
        ++total;
        for (const auto& construction : group.constructions) {
            icons.insert(construction.icon);
            ++total;
        }
    }

    // Then
    EXPECT_EQ(icons.size(), total);
}
