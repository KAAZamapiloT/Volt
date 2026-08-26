#include<Testmap.hpp>

int main()
{
    volt::test::run_correctness_tests();

    volt::test::benchmark_mixed(
        100'000,
        2'000'000,
        1'000'000
    );
}