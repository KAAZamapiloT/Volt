
#include "ring_buffer.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace volt::tests {

    namespace {

        struct Payload {
            std::uint64_t value;

            Payload(std::uint64_t v = 0)
                : value(v) {
            }

            ~Payload() = default;
        };

        template<typename Buffer>
        double run_benchmark(Buffer& buffer, std::size_t operations)
        {
            std::uint64_t checksum = 0;

            const auto start = std::chrono::steady_clock::now();

            for (std::size_t i = 0; i < operations; ++i) {

                if (buffer.full()) {
                    checksum += buffer.front().value;
                    buffer.pop();
                }

                buffer.push(Payload{ i });
            }

            while (!buffer.empty()) {
                checksum += buffer.front().value;
                buffer.pop();
            }

            const auto end = std::chrono::steady_clock::now();

            // Prevent optimizer from removing useful work.
            std::cout << "Checksum: " << checksum << '\n';

            return std::chrono::duration<double, std::milli>(
                end - start
            ).count();
        }

    } // anonymous namespace


    void benchmark_buffers()
    {
        constexpr std::size_t Capacity = 4096;
        constexpr std::size_t Operations = 10'000'000;

        std::cout << "\n=== Ring Buffer Benchmark ===\n";
        std::cout << "Capacity:   " << Capacity << '\n';
        std::cout << "Operations: " << Operations << "\n\n";


        // ------------------------------------------------------------
        // Immediate destruction
        // ------------------------------------------------------------

        {
            RingBuffer<Payload, Capacity> buffer;

            const double time =
                run_benchmark(buffer, Operations);

            std::cout << std::fixed
                << std::setprecision(3);

            std::cout
                << "Immediate destruction: "
                << time
                << " ms\n";
        }


        // ------------------------------------------------------------
        // Logical/deferred destruction
        // ------------------------------------------------------------

        {
            RingBufferLogicalPop<Payload, Capacity> buffer;

            const double time =
                run_benchmark(buffer, Operations);

            std::cout
                << "Deferred destruction:  "
                << time
                << " ms\n";
        }
    }

} // namespace volt::tests