#pragma once
#include <chrono>
namespace sky {
class Time {
public:
    static constexpr double fixedDelta=1.0/60.0;
    static constexpr double maxFrameDelta=0.25;
    double tick();
    double accumulate(double elapsed);
    bool consumeStep();
    double alpha() const { return accumulator_/fixedDelta; }
    void reset();
private:
    using Clock=std::chrono::steady_clock;
    Clock::time_point previous_=Clock::now();
    double accumulator_{};
};
}
