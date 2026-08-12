#include<iostream>

int main()
{
    std::cout<<"HELLO WORLD";
    return 0;
}
/*

cmake -S . -B out\build\x64-Debug -G Ninja
cmake --build out\build\x64-Debug

out\build\x64-Debug\volt_tests.exe

out\build\x64-Debug\volt.exe
*/