#pragma once

#include "input/pointer.hpp"

namespace input {

class RaylibPointer : public Pointer {
public:
    PointerPosition position() const override;
    bool released() const override;
};

} // namespace input
