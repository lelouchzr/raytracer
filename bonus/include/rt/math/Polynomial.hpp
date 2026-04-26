#pragma once
#include <vector>

namespace rt {
class Polynomial {
  public:
    static std::vector<double> roots(std::vector<double> coefficients);
};
} // namespace rt
