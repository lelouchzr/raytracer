#include "rt/concurrency/JoiningThread.hpp"
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main() {
    try {
        std::atomic<int> completed{0};
        {
            std::vector<rt::JoiningThread> workers;
            for (int i = 0; i < 32; ++i)
                workers.emplace_back([&completed](int value) { completed += value; }, 1);
        }
        check(completed == 32, "pool destruction did not join every worker");
        {
            rt::JoiningThread previous([&] { ++completed; });
            previous = rt::JoiningThread([&] { ++completed; });
        }
        check(completed == 34, "move assignment lost an outstanding worker");
        try {
            rt::JoiningThread worker([&] { ++completed; });
            throw std::runtime_error("unwind");
        } catch (const std::runtime_error &) {
        }
        check(completed == 35, "exception unwinding did not join the worker");
        rt::JoiningThread empty;
        check(!empty.joinable(), "default thread is unexpectedly active");
        rt::JoiningThread source([&] { ++completed; });
        rt::JoiningThread moved(std::move(source));
        check(!source.joinable() && moved.joinable(), "move did not transfer ownership");
        moved.join();
        check(!moved.joinable() && completed == 36, "explicit join did not complete");
        std::cout << "Worker ownership, pool growth, move assignment and unwinding passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
