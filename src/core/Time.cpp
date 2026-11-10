#include "Time.h"
#include <algorithm>
namespace sky {
double Time::tick() { auto now=Clock::now(); double dt=std::chrono::duration<double>(now-previous_).count(); previous_=now; return accumulate(dt); }
double Time::accumulate(double elapsed) { double dt=std::clamp(elapsed,0.0,maxFrameDelta); accumulator_+=dt; return dt; }
bool Time::consumeStep() { if(accumulator_+1e-12<fixedDelta) return false; accumulator_=std::max(0.0,accumulator_-fixedDelta); return true; }
void Time::reset() { previous_=Clock::now(); accumulator_=0; }
}
