#include "Test.h"
#include "aircraft/FlightController.h"
#include "camera/ChaseCamera.h"
#include "core/Time.h"
using namespace sky;
Aircraft simulate(int fps) {
    Aircraft a; a.reset(); FlightController c; Time clock;
    for(int frame=0;frame<fps*6;++frame) {
        clock.accumulate(1.0/fps);
        while(clock.consumeStep()) c.update(a,{0.15,0.3,0.2,false},Time::fixedDelta);
    }
    return a;
}
int main() {
    Tests t; Aircraft a; a.reset(); FlightController c;
    t.check(near(a.speed,70)&&near(a.health,100)&&near(a.transform.position.y,120),"spawn state");
    c.update(a,{},1); t.check(near(a.transform.position.z,70),"forward translation");
    a.reset(); c.update(a,{1,0,0,false},1); t.check(near(a.transform.forward().y,std::sin(radians(65))),"pitch rate");
    a.reset(); c.update(a,{0,1,0,false},1); t.check(near(a.transform.forward().x,std::sin(radians(55)))&&a.visualBank<0,"right yaw and bank sign");
    a.reset(); c.update(a,{0,-1,0,false},1); t.check(a.transform.forward().x<0&&a.visualBank>0,"left yaw and bank sign");
    a.reset(); c.update(a,{0,0,1,true},20); t.check(near(a.speed,140),"maximum speed");
    c.update(a,{0,0,-1,false},20); t.check(near(a.speed,25),"minimum speed");
    c.update(a,{},1); t.check(near(a.speed,25),"released throttle holds speed");
    a.reset(); a.visualBank=0.7; c.update(a,{},1);
    t.check(near(a.visualBank,0.7*std::exp(-3.5)),"auto level response");
    a.reset(); c.update(a,{1,0,0,false},0.3); auto forward=a.transform.forward(); c.update(a,{},0.3);
    t.check(length(a.transform.forward()-forward)<1e-8,"no pitch auto level");
    auto p30=simulate(30),p60=simulate(60),p144=simulate(144);
    t.check(length(p30.transform.position-p60.transform.position)<1e-8&&length(p144.transform.position-p60.transform.position)<1e-8,"30 60 144 FPS fixed-step equality");
    Time clock; t.check(near(clock.accumulate(5),0.25),"frame clamp"); int steps=0; while(clock.consumeStep()) ++steps;
    t.check(steps==15&&clock.alpha()<1,"bounded catch-up and interpolation");
    clock.accumulate(1.0/120); t.check(near(clock.alpha(),0.5),"half-step alpha");
    Input input; KeyboardState keys; keys.w=keys.s=keys.a=keys.d=keys.shift=keys.ctrl=true;
    auto mapped=input.update(keys); t.check(near(mapped.flight.pitch,0)&&near(mapped.flight.turn,0)&&near(mapped.flight.throttle,0)&&!mapped.flight.boost,"opposing input cancels");
    keys={}; keys.reset=true; t.check(input.update(keys).reset&&!input.update(keys).reset,"reset edge");
    keys={}; input.update(keys); keys.reset=keys.escape=keys.w=keys.d=keys.shift=true; mapped=input.update(keys);
    t.check(mapped.reset&&mapped.quit&&mapped.flight.pitch==1&&mapped.flight.turn==1&&mapped.flight.throttle==1&&mapped.flight.boost,"key mapping");
    a.reset(); ChaseCamera camera; camera.reset(a); auto pose=camera.pose();
    t.check(pose.position.z<a.transform.position.z&&pose.position.y>a.transform.position.y&&pose.target.z>0,"camera behind above and looking ahead");
    a.speed=140; camera.reset(a); t.check(near(camera.pose().fovDegrees,80),"maximum FOV");
    a.speed=25; camera.reset(a); t.check(near(camera.pose().fovDegrees,65),"minimum FOV");
    ChaseCamera cameras[3]; int rates[]={30,60,144};
    for(int j=0;j<3;++j) { a.reset(); cameras[j].reset(a); a.transform.position={200,200,400}; a.speed=140;
        for(int i=0;i<rates[j]*2;++i) cameras[j].update(a,1.0/rates[j]); }
    t.check(length(cameras[0].pose().position-cameras[2].pose().position)<1e-8&&near(cameras[0].pose().fovDegrees,cameras[1].pose().fovDegrees),"camera smoothing FPS independence for fixed target");
    a.reset(); camera.reset(a); t.check(length(camera.pose().position-pose.position)<1e-8,"camera reset snaps");
    for(int i=0;i<1440;++i) { c.update(a,{1,0.2,0,false},1.0/60); camera.update(a,1.0/60); }
    t.check(std::isfinite(camera.pose().position.y)&&near(length(camera.pose().up),1),"looping camera remains finite");
    a.reset(); t.check(near(a.visualBank,0)&&near(a.transform.forward().z,1)&&near(a.speed,70),"aircraft reset restores flight");
    return t.finish();
}
