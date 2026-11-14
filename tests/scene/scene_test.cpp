#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "scene/scene.hpp"

namespace {

using Calls = std::vector<std::string>;

struct RecordingModel : scene::Model {
    explicit RecordingModel(Calls& calls) : calls(calls) {}
    void update(float) override { calls.push_back("model"); }
    Calls& calls;
};

struct RecordingView : scene::View {
    explicit RecordingView(Calls& calls) : calls(calls) {}
    void update() override { calls.push_back("view"); }
    void draw() const override {}
    Calls& calls;
};

struct RecordingController : scene::Controller {
    explicit RecordingController(Calls& calls) : calls(calls) {}
    void handle_input() override { calls.push_back("controller"); }
    Calls& calls;
};

} // namespace

TEST(Scene, UpdateRunsTheControllerThenTheModelThenTheView) {
    // Given
    Calls calls;
    scene::Scene scene(std::make_unique<RecordingModel>(calls),
                       std::make_unique<RecordingView>(calls),
                       std::make_unique<RecordingController>(calls));

    // When
    scene.update(0.016f);

    // Then
    EXPECT_EQ(calls, (Calls{"controller", "model", "view"}));
}
