/*
 * @Author: Ninter6 mc525740@outlook.com
 * @Date: 2023-08-22 16:35:10
 * @LastEditors: Ninter6
 * @LastEditTime: 2024-01-14 12:11:15
 */
#pragma once

#include <chrono>

namespace st {

class Timer {
public:
    using time_point = std::chrono::high_resolution_clock::time_point;

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
    time_point startTime, lastTick, breakpoint;
};

class Countdown : public Timer {
public:
    using duration = std::chrono::duration<double, std::chrono::seconds::period>;

    Countdown() = default;
    Countdown(duration seconds) : seconds(seconds) {}

    bool IsTimeOut() {
        return Total() >= seconds;
    }

    void Reset(duration seconds) {
        this->seconds = seconds;
    }

    auto Remainder() {
        return duration(seconds - Total());
    }

private:
    duration seconds;
};

}