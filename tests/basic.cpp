#include "testvector.hpp"

int main()
{
    volt::tests::run_all<64>(1'000'000);
    volt::tests::print_result(
        volt::tests::benchmark_inline_push<64>(1'000'000)
    );
    return 0;
}