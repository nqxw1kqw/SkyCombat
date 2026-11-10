#include "FlightController.h"
namespace sky {
void FlightController::update(Aircraft& a,const FlightInput& input,double dt) const {
    if(dt<=0) return;
    const double pitch=std::clamp(input.pitch,-1.0,1.0),turn=std::clamp(input.turn,-1.0,1.0);
    const double throttle=std::clamp(input.throttle,-1.0,1.0);
    a.speed=std::clamp(a.speed+throttle*(throttle>=0?FlightTuning::acceleration:FlightTuning::deceleration)*dt,
                       FlightTuning::minSpeed,FlightTuning::maxSpeed);
    // World-up yaw, then local-right pitch. W uses negative X rotation to raise +Z.
    a.transform.orientation=(Quat::axisAngle({0,1,0},turn*FlightTuning::yawRate*dt)*
        a.transform.orientation*Quat::axisAngle({1,0,0},-pitch*FlightTuning::pitchRate*dt)).normalized();
    a.visualBank+=(-turn*FlightTuning::maxBank-a.visualBank)*response(FlightTuning::levelResponse,dt);
    a.transform.position=a.transform.position+a.transform.forward()*(a.speed*dt);
}
}
