#include <chrono>

namespace st {

class Timer {
public:
    Timer() = default;

    auto Start() {
        startTime = lastTick = std::chrono::high_resolution_clock::now();
        return startTime;
    }

    auto Tick() {
        auto past = std::chrono::high_resolution_clock::now() - lastTick;
        lastTick = std::chrono::high_resolution_clock::now();
        return past;
    }

    auto Total() {
        return std::chrono::high_resolution_clock::now() - startTime;
    }

    void Stop() {
        breakpoint = std::chrono::high_resolution_clock::now();
    }

    void Continue() {
        startTime = lastTick += std::chrono::high_resolution_clock::now() - breakpoint;
    }

private:
    std::chrono::high_resolution_clock::time_point startTime, lastTick, breakpoint;
};

class Countdown : public Timer {
public:
    Countdown() = default;
    Countdown(double seconds) : seconds(seconds) {}

    bool IsTimeOut() {
        return Total() >= seconds;
    }

    void Reset(double seconds) {
        this->seconds = std::chrono::duration<double, std::chrono::seconds::period>(seconds);
    }

    auto Remainder() {
        return std::chrono::duration<double, std::chrono::seconds::period>(seconds - Total());
    }

private:
    std::chrono::duration<double, std::chrono::seconds::period> seconds;
};

}