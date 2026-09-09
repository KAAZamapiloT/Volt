#include<iostream>
#include<volt/types/uniqueptr.hpp>
int main()
{
	std::unique_ptr<std::string> str_ptr = std::make_unique<std::string>("Hello, World!");
	volt::unique_ptr<std::string> volt_str_ptr = volt::make_unique<std::string>("Hello, Volt!");


	std::cout << "std::unique_ptr: " << *str_ptr << std::endl;
	std::cout << "volt::unique_ptr: " << *volt_str_ptr << std::endl;
    return 0;
    
}
/*

cmake -S . -B out\build\x64-Debug -G Ninja
cmake --build out\build\x64-Debug

out\build\x64-Debug\volt_tests.exe

out\build\x64-Debug\volt.exe
*/