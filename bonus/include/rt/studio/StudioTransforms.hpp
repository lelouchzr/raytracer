#pragma once
#include "rt/json.hpp"

namespace rt {
class StudioTransforms {
  public:
    // Make every visible transform explicit without changing its effective value.
    static void normalize(Json &object);
};
} // namespace rt
