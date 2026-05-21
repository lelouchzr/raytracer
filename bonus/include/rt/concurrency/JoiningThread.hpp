#pragma once
#include <concepts>
#include <thread>
#include <type_traits>
#include <utility>

namespace rt {
// Join on every exit path, including partial worker-pool construction and exceptions.
// Cancellation belongs to the render operation rather than the thread's lifetime.
class JoiningThread {
    std::thread thread_;

  public:
    JoiningThread() = default;

    template <class Function, class... Args>
        requires std::invocable<std::decay_t<Function>, std::decay_t<Args>...>
    explicit JoiningThread(Function &&function, Args &&...args)
        : thread_(std::forward<Function>(function), std::forward<Args>(args)...) {}

    ~JoiningThread();
    JoiningThread(const JoiningThread &) = delete;
    JoiningThread &operator=(const JoiningThread &) = delete;
    JoiningThread(JoiningThread &&) noexcept;
    JoiningThread &operator=(JoiningThread &&) noexcept;
    bool joinable() const noexcept;
    void join();
};
} // namespace rt
