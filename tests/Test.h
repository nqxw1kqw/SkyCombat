#pragma once
#include <cmath>
#include <iostream>
#include <string_view>
struct Tests {
    int passed{},failed{};
    void check(bool ok,std::string_view name) { if(ok) ++passed; else { ++failed; std::cerr<<"FAIL: "<<name<<'\n'; } }
    int finish() const { std::cout<<"PASS "<<passed<<" / FAIL "<<failed<<'\n'; return failed?1:0; }
};
inline bool near(double a,double b,double epsilon=1e-8) { return std::abs(a-b)<epsilon; }
