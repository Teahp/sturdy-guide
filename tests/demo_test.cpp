#include "sturdy_guide/demo.hpp"
#include "iostream"
#include "string"
int main(){
    auto str=sturdy_guide::user_greeting();
    std::cout<<str;
    return 0;
}