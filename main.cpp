#include<iostream>
#include"tests/poolallocator_test.hpp"
int main()
{
    volt::tests::run_pool_allocator_suite();
    return 0;
    
}
/*

cmake -S . -B out\build\x64-Debug -G Ninja
cmake --build out\build\x64-Debug

out\build\x64-Debug\volt_tests.exe

out\build\x64-Debug\volt.exe
*/