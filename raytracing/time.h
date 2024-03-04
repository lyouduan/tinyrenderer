#ifndef TIMER_H
#define TIMER_H

#include <chrono>

class timer {
public:
    timer() : m_start(std::chrono::high_resolution_clock::now()) {}

    void reset() {
        m_start = std::chrono::high_resolution_clock::now();
    }

    double duration() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<double>>(end - m_start).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

#endif // TIMER_H
