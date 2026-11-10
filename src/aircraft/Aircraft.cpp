#include "Aircraft.h"
namespace sky {
void Aircraft::reset() { transform={{0,120,0},{}}; speed=70; health=100; visualBank=0; }
Aircraft interpolate(const Aircraft& a,const Aircraft& b,double t) {
    Aircraft result=b; result.transform=interpolate(a.transform,b.transform,t);
    result.speed=a.speed+(b.speed-a.speed)*t;
    result.visualBank=a.visualBank+(b.visualBank-a.visualBank)*t;
    return result;
}
}
