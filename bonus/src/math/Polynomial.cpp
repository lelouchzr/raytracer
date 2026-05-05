#include "rt/math/Polynomial.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace rt {
std::vector<double> Polynomial::roots(std::vector<double> c) {
    while (c.size() > 1 && std::abs(c.back()) < 1e-18)
        c.pop_back();
    if (c.size() < 2)
        return {};
    if (c.size() == 2)
        return {-c[0] / c[1]};
    std::vector<double> derivative;
    for (std::size_t i = 1; i < c.size(); ++i)
        derivative.push_back(c[i] * static_cast<double>(i));
    auto critical = roots(std::move(derivative));
    double bound = 1;
    for (std::size_t i = 0; i + 1 < c.size(); ++i)
        bound = std::max(bound, 1 + std::abs(c[i] / c.back()));
    critical.erase(std::remove_if(critical.begin(), critical.end(),
                                  [&](double x) { return x <= -bound || x >= bound; }),
                   critical.end());
    critical.insert(critical.begin(), -bound);
    critical.push_back(bound);
    std::vector<double> result;
    auto evaluate = [&](double x) {
        double value = 0;
        for (auto i = c.rbegin(); i != c.rend(); ++i)
            value = value * x + *i;
        return value;
    };
    auto tolerance = [&](double x) {
        double value = 0;
        for (auto i = c.rbegin(); i != c.rend(); ++i)
            value = value * std::abs(x) + std::abs(*i);
        return 1e-12 * std::max(1., value);
    };
    for (double x : critical)
        if (std::abs(evaluate(x)) <= tolerance(x))
            result.push_back(x);
    for (std::size_t i = 1; i < critical.size(); ++i) {
        double a = critical[i - 1], b = critical[i], fa = evaluate(a), fb = evaluate(b);
        if (fa * fb >= 0)
            continue;
        for (int step = 0; step < 100; ++step) {
            double mid = (a + b) * .5, fm = evaluate(mid);
            if ((fa < 0) == (fm < 0)) {
                a = mid;
                fa = fm;
            } else
                b = mid;
        }
        result.push_back((a + b) * .5);
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end(),
                             [](double a, double b) {
                                 return std::abs(a - b) <
                                        1e-8 * std::max({1., std::abs(a), std::abs(b)});
                             }),
                 result.end());
    return result;
}

std::size_t Polynomial::roots(const std::array<double, 5> &coefficients, RootBuffer &output) {
    return solveSmall(coefficients, coefficients.size(), output);
}

std::size_t Polynomial::solveSmall(const std::array<double, 5> &c, std::size_t count,
                                   RootBuffer &output) {
    while (count > 1 && std::abs(c[count - 1]) < 1e-18)
        --count;
    if (count < 2)
        return 0;
    if (count == 2) {
        output[0] = -c[0] / c[1];
        return 1;
    }
    std::array<double, 5> derivative{};
    for (std::size_t i = 1; i < count; ++i)
        derivative[i - 1] = c[i] * static_cast<double>(i);
    RootBuffer critical{};
    std::size_t criticalCount = solveSmall(derivative, count - 1, critical);
    double bound = 1;
    for (std::size_t i = 0; i + 1 < count; ++i)
        bound = std::max(bound, 1 + std::abs(c[i] / c[count - 1]));
    auto criticalEnd = std::remove_if(critical.begin(),
                                      critical.begin() + static_cast<std::ptrdiff_t>(criticalCount),
                                      [&](double x) { return x <= -bound || x >= bound; });
    criticalCount = static_cast<std::size_t>(criticalEnd - critical.begin());
    std::move_backward(critical.begin(), criticalEnd, criticalEnd + 1);
    critical[0] = -bound;
    ++criticalCount;
    critical[criticalCount++] = bound;
    std::size_t resultCount = 0;
    auto append = [&](double value) {
        if (resultCount >= output.size())
            throw std::logic_error("quartic workspace capacity exceeded");
        output[resultCount++] = value;
    };
    auto evaluate = [&](double x) {
        double value = 0;
        for (std::size_t i = count; i-- > 0;)
            value = value * x + c[i];
        return value;
    };
    auto tolerance = [&](double x) {
        double value = 0;
        for (std::size_t i = count; i-- > 0;)
            value = value * std::abs(x) + std::abs(c[i]);
        return 1e-12 * std::max(1., value);
    };
    for (std::size_t i = 0; i < criticalCount; ++i) {
        double x = critical[i];
        if (std::abs(evaluate(x)) <= tolerance(x))
            append(x);
    }
    for (std::size_t i = 1; i < criticalCount; ++i) {
        double a = critical[i - 1], b = critical[i], fa = evaluate(a), fb = evaluate(b);
        if (fa * fb >= 0)
            continue;
        for (int step = 0; step < 100; ++step) {
            double mid = (a + b) * .5, fm = evaluate(mid);
            if ((fa < 0) == (fm < 0)) {
                a = mid;
                fa = fm;
            } else
                b = mid;
        }
        append((a + b) * .5);
    }
    auto end = output.begin() + static_cast<std::ptrdiff_t>(resultCount);
    std::sort(output.begin(), end);
    end = std::unique(output.begin(), end, [](double a, double b) {
        return std::abs(a - b) < 1e-8 * std::max({1., std::abs(a), std::abs(b)});
    });
    return static_cast<std::size_t>(end - output.begin());
}
} // namespace rt
