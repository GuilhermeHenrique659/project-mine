#include <gtest/gtest.h>

#include <memory>

#include "support/fake_canvas.hpp"
#include "ui/composite.hpp"
#include "ui/label.hpp"

namespace {

// A composite showing one label; counts how often it brings its tree up to date.
class Counting : public ui::Composite {
public:
    Counting() : ui::Composite(ui::Style{}) {}

    mutable int syncs = 0;
    std::string text = "a";

private:
    void sync(const ui::Canvas&) const override {
        ++syncs;
        if (!label_) {
            auto label = ui::label("").build();
            label_ = label.get();
            set_tree(ui::column().child(std::move(label)).build());
        }
        label_->set_text(text);
    }

    mutable ui::Label* label_ = nullptr;
};

} // namespace

TEST(Composite, SyncsItsTreeOncePerLayoutEvenWhenMeasuredToo) {
    // Given: inside a box, so it is measured and then laid out in one pass
    FakeCanvas canvas;
    Counting composite;
    auto box = ui::column().child_ref(composite).build();

    // When
    box->layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);

    // Then
    EXPECT_EQ(composite.syncs, 1);
}

TEST(Composite, ShowsWhatChangedOnTheNextLayout) {
    // Given
    FakeCanvas canvas;
    Counting composite;
    composite.layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);

    // When
    composite.text = "changed";
    composite.layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);
    composite.draw(canvas);

    // Then
    ASSERT_EQ(canvas.texts.size(), 1u);
    EXPECT_EQ(canvas.texts[0].text, "changed");
}
