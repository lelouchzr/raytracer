#include "rt/concurrency/JoiningThread.hpp"

namespace rt {
JoiningThread::~JoiningThread() {
    if (thread_.joinable())
        thread_.join();
}

JoiningThread::JoiningThread(JoiningThread &&other) noexcept : thread_(std::move(other.thread_)) {}

JoiningThread &JoiningThread::operator=(JoiningThread &&other) noexcept {
    if (this != &other) {
        if (thread_.joinable())
            thread_.join();
        thread_ = std::move(other.thread_);
    }
    return *this;
}

bool JoiningThread::joinable() const noexcept {
    return thread_.joinable();
}

void JoiningThread::join() {
    thread_.join();
}
} // namespace rt
