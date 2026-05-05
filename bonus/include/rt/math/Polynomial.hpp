#pragma once
#include <array>
#include <cstddef>
#include <vector>

namespace rt {
class Polynomial {
  public:
    // Tolerance can retain more candidates than the degree. For degree <= 4,
    // R(1)=1, R(n)<=2*R(n-1)+3 bounds the pre-dedup candidate count by 29.
    using RootBuffer = std::array<double, 32>;
    static std::vector<double> roots(std::vector<double> coefficients);
    static std::size_t roots(const std::array<double, 5> &, RootBuffer &);

  private:
    static std::size_t solveSmall(const std::array<double, 5> &, std::size_t, RootBuffer &);
};
} // namespace rt
