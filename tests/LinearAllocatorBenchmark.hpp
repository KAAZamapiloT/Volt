#include <volt/memory/LinearAllocator.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory_resource>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================
// Volt LinearAllocator Benchmark
//
// Compares:
//
//   1. volt::LinearAllocator
//   2. ::operator new / ::operator delete
//   3. std::pmr::monotonic_buffer_resource
//
// Benchmarks:
//
//   - allocation-only latency
//   - allocation throughput
//   - size sweep
//   - alignment sweep
//   - full lifecycle:
//         Linear  = allocate + reset
//         new      = allocate + delete
//         PMR      = allocate + release
//   - raw reset cost
//   - allocator memory overhead caused by alignment/padding
//
// Build:
//   C++23 recommended
//
// Release example:
//   clang++ -std=c++23 -O3 -DNDEBUG -I/path/to/volt/include \
//       LinearAllocatorBenchmark.cpp -o linear_bench
//
// ============================================================

namespace benchmark {

    // ------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------

    constexpr std::size_t ARENA_SIZE =
        64ull * 1024ull * 1024ull; // 64 MiB

    constexpr std::size_t MAX_ALLOCATIONS =
        1'000'000;

    constexpr std::size_t WARMUP_RUNS =
        3;

    constexpr std::size_t MEASURED_RUNS =
        21;

    // The benchmark automatically chooses an allocation count
    // so that the requested workload fits into the arena.
    //
    // Smaller objects -> up to 1,000,000 allocations
    // Larger objects  -> fewer allocations
    constexpr std::size_t MIN_ALLOCATIONS =
        10'000;

    // Prevent the compiler from considering benchmark results
    // completely unused.
    volatile std::uintptr_t g_sink = 0;

    // ------------------------------------------------------------
    // Clock
    // ------------------------------------------------------------

    using Clock = std::chrono::steady_clock;

    // ------------------------------------------------------------
    // Aligned raw storage
    // ------------------------------------------------------------
    //
    // We need properly aligned storage for the LinearAllocator
    // and PMR benchmarks.
    //
    // The buffer itself is allocated at 64-byte alignment.
    // Individual allocations inside it may request less/equal
    // alignment.
    //
    // ------------------------------------------------------------

    class AlignedBuffer {
    public:
        AlignedBuffer(std::size_t size, std::size_t alignment)
            : m_size(size),
            m_alignment(alignment),
            m_ptr(::operator new(
                size,
                std::align_val_t(alignment))) {
        }

        ~AlignedBuffer() {
            if (m_ptr != nullptr) {
                ::operator delete(
                    m_ptr,
                    std::align_val_t(m_alignment));
            }
        }

        AlignedBuffer(const AlignedBuffer&) = delete;
        AlignedBuffer& operator=(const AlignedBuffer&) = delete;

        AlignedBuffer(AlignedBuffer&&) = delete;
        AlignedBuffer& operator=(AlignedBuffer&&) = delete;

        void* data() noexcept {
            return m_ptr;
        }

        std::size_t size() const noexcept {
            return m_size;
        }

    private:
        std::size_t m_size;
        std::size_t m_alignment;
        void* m_ptr;
    };

    // ------------------------------------------------------------
    // Statistics
    // ------------------------------------------------------------

    struct Stats {
        double min_ns = 0.0;
        double median_ns = 0.0;
        double p95_ns = 0.0;
        double p99_ns = 0.0;
        double max_ns = 0.0;
    };

    double percentile(
        std::vector<double>& values,
        double p)
    {
        if (values.empty())
            return 0.0;

        std::sort(values.begin(), values.end());

        const double index =
            p * static_cast<double>(values.size() - 1);

        const std::size_t lower =
            static_cast<std::size_t>(index);

        const std::size_t upper =
            std::min(
                lower + 1,
                values.size() - 1);

        const double fraction =
            index - static_cast<double>(lower);

        return values[lower] +
            (values[upper] - values[lower]) *
            fraction;
    }

    template <typename F>
    Stats measure(F&& benchmark)
    {
        // Warmup.
        for (std::size_t i = 0; i < WARMUP_RUNS; ++i) {
            benchmark();
        }

        std::vector<double> samples;
        samples.reserve(MEASURED_RUNS);

        for (std::size_t i = 0; i < MEASURED_RUNS; ++i) {
            samples.push_back(benchmark());
        }

        Stats result;

        result.min_ns =
            *std::min_element(
                samples.begin(),
                samples.end());

        result.max_ns =
            *std::max_element(
                samples.begin(),
                samples.end());

        // percentile() sorts the vector.
        result.median_ns =
            percentile(samples, 0.50);

        result.p95_ns =
            percentile(samples, 0.95);

        result.p99_ns =
            percentile(samples, 0.99);

        return result;
    }

    // ------------------------------------------------------------
    // Allocation count
    // ------------------------------------------------------------

    std::size_t allocation_count_for(
        std::size_t allocation_size,
        std::size_t alignment)
    {
        // Worst-case padding per allocation is approximately
        // alignment - 1.
        //
        // Use a conservative estimate so the linear allocator
        // does not exhaust its arena.
        const std::size_t bytes_per_allocation =
            allocation_size + alignment - 1;

        const std::size_t maximum_that_fits =
            ARENA_SIZE / bytes_per_allocation;

        const std::size_t count =
            std::min(
                MAX_ALLOCATIONS,
                maximum_that_fits);

        return std::max(
            MIN_ALLOCATIONS,
            count);
    }

    // ------------------------------------------------------------
    // Formatting
    // ------------------------------------------------------------

    double mops_from_ns(double ns_per_alloc)
    {
        if (ns_per_alloc <= 0.0)
            return 0.0;

        // 1 second = 1,000,000,000 ns
        // 1 M allocations = 1,000,000 allocations
        //
        // MOPS = 1000 / ns-per-allocation
        return 1000.0 / ns_per_alloc;
    }

    void print_stats(
        const char* name,
        const Stats& stats)
    {
        std::cout
            << std::left
            << std::setw(18)
            << name
            << std::right
            << std::setw(12)
            << std::fixed
            << std::setprecision(2)
            << stats.median_ns
            << std::setw(12)
            << mops_from_ns(stats.median_ns)
            << std::setw(12)
            << stats.p95_ns
            << std::setw(12)
            << stats.p99_ns
            << '\n';
    }

    // ------------------------------------------------------------
    // Volt LinearAllocator
    // ------------------------------------------------------------

    Stats benchmark_linear_allocate_only(
        std::size_t allocation_size,
        std::size_t alignment,
        std::size_t count)
    {
        AlignedBuffer storage(
            ARENA_SIZE,
            64);

        volt::LinearAllocator arena(
            storage.data(),
            storage.size());

        std::uintptr_t checksum = 0;

        auto run = [&]() -> double {

            arena.reset();

            checksum = 0;

            const auto start = Clock::now();

            for (std::size_t i = 0; i < count; ++i) {

                void* ptr =
                    arena.allocate(
                        allocation_size,
                        alignment);

                if (ptr == nullptr) {
                    throw std::runtime_error(
                        "LinearAllocator exhausted during benchmark");
                }

                checksum ^=
                    reinterpret_cast<std::uintptr_t>(ptr);
            }

            const auto end = Clock::now();

            // Make the pointer values observable.
            g_sink = checksum;

            // Reset outside the timed region.
            arena.reset();

            return std::chrono::duration<double, std::nano>(
                end - start)
                .count() /
                static_cast<double>(count);
            };

        return measure(run);
    }

    // ------------------------------------------------------------
    // operator new
    // ------------------------------------------------------------

    Stats benchmark_new_allocate_only(
        std::size_t allocation_size,
        std::size_t alignment,
        std::size_t count)
    {
        std::vector<void*> pointers(count);

        std::uintptr_t checksum = 0;

        auto run = [&]() -> double {

            checksum = 0;

            const auto start = Clock::now();

            for (std::size_t i = 0; i < count; ++i) {

                void* ptr =
                    ::operator new(
                        allocation_size,
                        std::align_val_t(alignment));

                pointers[i] = ptr;

                checksum ^=
                    reinterpret_cast<std::uintptr_t>(ptr);
            }

            const auto end = Clock::now();

            g_sink = checksum;

            // Deliberately OUTSIDE timed region.
            //
            // This benchmark measures allocation only.
            for (void* ptr : pointers) {
                ::operator delete(
                    ptr,
                    std::align_val_t(alignment));
            }

            return std::chrono::duration<double, std::nano>(
                end - start)
                .count() /
                static_cast<double>(count);
            };

        return measure(run);
    }

    // ------------------------------------------------------------
    // std::pmr::monotonic_buffer_resource
    // ------------------------------------------------------------

    Stats benchmark_pmr_allocate_only(
        std::size_t allocation_size,
        std::size_t alignment,
        std::size_t count)
    {
        AlignedBuffer storage(
            ARENA_SIZE,
            64);

        std::pmr::monotonic_buffer_resource resource(
            storage.data(),
            storage.size(),
            std::pmr::null_memory_resource());

        std::uintptr_t checksum = 0;

        auto run = [&]() -> double {

            resource.release();

            checksum = 0;

            const auto start = Clock::now();

            for (std::size_t i = 0; i < count; ++i) {

                void* ptr =
                    resource.allocate(
                        allocation_size,
                        alignment);

                checksum ^=
                    reinterpret_cast<std::uintptr_t>(ptr);
            }

            const auto end = Clock::now();

            g_sink = checksum;

            // Outside timed region.
            resource.release();

            return std::chrono::duration<double, std::nano>(
                end - start)
                .count() /
                static_cast<double>(count);
            };

        return measure(run);
    }

    // ------------------------------------------------------------
    // Full lifecycle:
    //
    // Linear = allocate + reset
    // new    = allocate + delete
    // PMR    = allocate + release
    // ------------------------------------------------------------

    Stats benchmark_linear_lifecycle(
        std::size_t allocation_size,
        std::size_t alignment,
        std::size_t count)
    {
        AlignedBuffer storage(
            ARENA_SIZE,
            64);

        volt::LinearAllocator arena(
            storage.data(),
            storage.size());

        std::uintptr_t checksum = 0;

        auto run = [&]() -> double {

            arena.reset();

            checksum = 0;

            const auto start = Clock::now();

            for (std::size_t i = 0; i < count; ++i) {

                void* ptr =
                    arena.allocate(
                        allocation_size,
                        alignment);

                if (ptr == nullptr) {
                    throw std::runtime_error(
                        "LinearAllocator exhausted");
                }

                checksum ^=
                    reinterpret_cast<std::uintptr_t>(ptr);
            }

            // Included intentionally.
            arena.reset();

            const auto end = Clock::now();

            g_sink = checksum;

            return std::chrono::duration<double, std::nano>(
                end - start)
                .count() /
                static_cast<double>(count);
            };

        return measure(run);
    }

    Stats benchmark_new_lifecycle(
        std::size_t allocation_size,
        std::size_t alignment,
        std::size_t count)
    {
        std::vector<void*> pointers(count);

        std::uintptr_t checksum = 0;

        auto run = [&]() -> double {

            checksum = 0;

            const auto start = Clock::now();

            for (std::size_t i = 0; i < count; ++i) {

                void* ptr =
                    ::operator new(
                        allocation_size,
                        std::align_val_t(alignment));

                pointers[i] = ptr;

                checksum ^=
                    reinterpret_cast<std::uintptr_t>(ptr);
            }

            for (void* ptr : pointers) {
                ::operator delete(
                    ptr,
                    std::align_val_t(alignment));
            }

            const auto end = Clock::now();

            g_sink = checksum;

            return std::chrono::duration<double, std::nano>(
                end - start)
                .count() /
                static_cast<double>(count);
            };

        return measure(run);
    }

    Stats benchmark_pmr_lifecycle(
        std::size_t allocation_size,
        std::size_t alignment,
        std::size_t count)
    {
        AlignedBuffer storage(
            ARENA_SIZE,
            64);

        std::pmr::monotonic_buffer_resource resource(
            storage.data(),
            storage.size(),
            std::pmr::null_memory_resource());

        std::uintptr_t checksum = 0;

        auto run = [&]() -> double {

            resource.release();

            checksum = 0;

            const auto start = Clock::now();

            for (std::size_t i = 0; i < count; ++i) {

                void* ptr =
                    resource.allocate(
                        allocation_size,
                        alignment);

                checksum ^=
                    reinterpret_cast<std::uintptr_t>(ptr);
            }

            // Equivalent bulk reclamation operation.
            resource.release();

            const auto end = Clock::now();

            g_sink = checksum;

            return std::chrono::duration<double, std::nano>(
                end - start)
                .count() /
                static_cast<double>(count);
            };

        return measure(run);
    }

    // ------------------------------------------------------------
    // Memory overhead
    // ------------------------------------------------------------

    void benchmark_linear_overhead(
        std::size_t allocation_size,
        std::size_t alignment,
        std::size_t count)
    {
        AlignedBuffer storage(
            ARENA_SIZE,
            64);

        volt::LinearAllocator arena(
            storage.data(),
            storage.size());

        arena.reset();

        for (std::size_t i = 0; i < count; ++i) {

            void* ptr =
                arena.allocate(
                    allocation_size,
                    alignment);

            if (ptr == nullptr) {
                throw std::runtime_error(
                    "LinearAllocator exhausted");
            }
        }

        const std::size_t requested =
            allocation_size * count;

        const std::size_t used =
            arena.used();

        const std::size_t overhead =
            used > requested
            ? used - requested
            : 0;

        const double overhead_percent =
            requested == 0
            ? 0.0
            : 100.0 *
            static_cast<double>(overhead) /
            static_cast<double>(requested);

        std::cout
            << "Requested: "
            << requested / 1024.0 / 1024.0
            << " MiB | Used: "
            << used / 1024.0 / 1024.0
            << " MiB | Overhead: "
            << overhead / 1024.0 / 1024.0
            << " MiB ("
            << overhead_percent
            << "%)\n";
    }

    // ------------------------------------------------------------
    // Raw reset benchmark
    //
    // Diagnostic only.
    //
    // Reset is normally extremely cheap for a linear allocator.
    // This benchmark exists to quantify it.
    // ------------------------------------------------------------

    Stats benchmark_linear_reset_only()
    {
        AlignedBuffer storage(
            ARENA_SIZE,
            64);

        volt::LinearAllocator arena(
            storage.data(),
            storage.size());

        constexpr std::size_t RESET_COUNT =
            1'000'000;

        std::uintptr_t checksum = 0;

        auto run = [&]() -> double {

            arena.allocate(64, 8);

            checksum = 0;

            const auto start = Clock::now();

            for (std::size_t i = 0;
                i < RESET_COUNT;
                ++i)
            {
                arena.reset();

                // Force observable interaction with the object.
                checksum ^= arena.used();
            }

            const auto end = Clock::now();

            g_sink = checksum;

            return std::chrono::duration<double, std::nano>(
                end - start)
                .count() /
                static_cast<double>(RESET_COUNT);
            };

        return measure(run);
    }

    // ------------------------------------------------------------
    // Header printing
    // ------------------------------------------------------------

    void print_header(
        const char* title,
        std::size_t allocation_size,
        std::size_t alignment,
        std::size_t count)
    {
        std::cout
            << "\n============================================================\n"
            << title << '\n'
            << "============================================================\n";

        std::cout
            << "Allocation size : "
            << allocation_size
            << " bytes\n"

            << "Alignment        : "
            << alignment
            << " bytes\n"

            << "Allocations      : "
            << count
            << '\n'

            << "Arena             : "
            << ARENA_SIZE / 1024 / 1024
            << " MiB\n\n";

        std::cout
            << std::left
            << std::setw(18)
            << "Allocator"
            << std::right
            << std::setw(12)
            << "ns/alloc"
            << std::setw(12)
            << "M ops/s"
            << std::setw(12)
            << "P95 ns"
            << std::setw(12)
            << "P99 ns"
            << '\n';

        std::cout
            << std::string(66, '-')
            << '\n';
    }

    // ------------------------------------------------------------
    // Allocation benchmark
    // ------------------------------------------------------------

    void run_allocation_benchmark(
        std::size_t allocation_size,
        std::size_t alignment)
    {
        const std::size_t count =
            allocation_count_for(
                allocation_size,
                alignment);

        print_header(
            "RAW ALLOCATION",
            allocation_size,
            alignment,
            count);

        const Stats linear =
            benchmark_linear_allocate_only(
                allocation_size,
                alignment,
                count);

        const Stats normal_new =
            benchmark_new_allocate_only(
                allocation_size,
                alignment,
                count);

        const Stats pmr =
            benchmark_pmr_allocate_only(
                allocation_size,
                alignment,
                count);

        print_stats("Volt LinearAllocator", linear);
        print_stats("operator new", normal_new);
        print_stats("PMR monotonic", pmr);

        std::cout
            << '\n'
            << "new / Linear speedup : "
            << normal_new.median_ns /
            linear.median_ns
            << "x\n"

            << "PMR / Linear speedup : "
            << pmr.median_ns /
            linear.median_ns
            << "x\n";

        std::cout
            << "\nMemory usage:\n";

        benchmark_linear_overhead(
            allocation_size,
            alignment,
            count);
    }

    // ------------------------------------------------------------
    // Lifecycle benchmark
    // ------------------------------------------------------------

    void run_lifecycle_benchmark(
        std::size_t allocation_size,
        std::size_t alignment)
    {
        const std::size_t count =
            allocation_count_for(
                allocation_size,
                alignment);

        print_header(
            "FULL ALLOCATION LIFECYCLE",
            allocation_size,
            alignment,
            count);

        const Stats linear =
            benchmark_linear_lifecycle(
                allocation_size,
                alignment,
                count);

        const Stats normal_new =
            benchmark_new_lifecycle(
                allocation_size,
                alignment,
                count);

        const Stats pmr =
            benchmark_pmr_lifecycle(
                allocation_size,
                alignment,
                count);

        print_stats("Volt LinearAllocator", linear);
        print_stats("operator new/delete", normal_new);
        print_stats("PMR monotonic", pmr);

        std::cout
            << '\n'
            << "new/delete / Linear : "
            << normal_new.median_ns /
            linear.median_ns
            << "x\n"

            << "PMR / Linear        : "
            << pmr.median_ns /
            linear.median_ns
            << "x\n";
    }

    // ------------------------------------------------------------
    // Size sweep
    // ------------------------------------------------------------

    void run_size_sweep()
    {
        constexpr std::array<std::size_t, 8> sizes = {
            8,
            16,
            32,
            64,
            128,
            256,
            512,
            1024
        };

        constexpr std::size_t alignment = 16;

        std::cout
            << "\n============================================================\n"
            << "SIZE SWEEP\n"
            << "============================================================\n";

        std::cout
            << std::left
            << std::setw(10)
            << "Size"
            << std::right
            << std::setw(12)
            << "Count"
            << std::setw(16)
            << "Linear ns"
            << std::setw(16)
            << "new ns"
            << std::setw(16)
            << "PMR ns"
            << std::setw(14)
            << "new/Linear"
            << '\n';

        std::cout
            << std::string(84, '-')
            << '\n';

        for (const std::size_t size : sizes) {

            const std::size_t count =
                allocation_count_for(
                    size,
                    alignment);

            const Stats linear =
                benchmark_linear_allocate_only(
                    size,
                    alignment,
                    count);

            const Stats normal_new =
                benchmark_new_allocate_only(
                    size,
                    alignment,
                    count);

            const Stats pmr =
                benchmark_pmr_allocate_only(
                    size,
                    alignment,
                    count);

            std::cout
                << std::left
                << std::setw(10)
                << size

                << std::right
                << std::setw(12)
                << count

                << std::setw(16)
                << std::fixed
                << std::setprecision(2)
                << linear.median_ns

                << std::setw(16)
                << normal_new.median_ns

                << std::setw(16)
                << pmr.median_ns

                << std::setw(14)
                << normal_new.median_ns /
                linear.median_ns

                << '\n';
        }
    }

    // ------------------------------------------------------------
    // Alignment sweep
    // ------------------------------------------------------------

    void run_alignment_sweep()
    {
        constexpr std::size_t allocation_size = 64;

        constexpr std::array<std::size_t, 4> alignments = {
            8,
            16,
            32,
            64
        };

        std::cout
            << "\n============================================================\n"
            << "ALIGNMENT SWEEP\n"
            << "============================================================\n";

        std::cout
            << std::left
            << std::setw(12)
            << "Alignment"
            << std::right
            << std::setw(12)
            << "Count"
            << std::setw(16)
            << "Linear ns"
            << std::setw(16)
            << "new ns"
            << std::setw(16)
            << "PMR ns"
            << std::setw(14)
            << "new/Linear"
            << '\n';

        std::cout
            << std::string(86, '-')
            << '\n';

        for (const std::size_t alignment : alignments) {

            const std::size_t count =
                allocation_count_for(
                    allocation_size,
                    alignment);

            const Stats linear =
                benchmark_linear_allocate_only(
                    allocation_size,
                    alignment,
                    count);

            const Stats normal_new =
                benchmark_new_allocate_only(
                    allocation_size,
                    alignment,
                    count);

            const Stats pmr =
                benchmark_pmr_allocate_only(
                    allocation_size,
                    alignment,
                    count);

            std::cout
                << std::left
                << std::setw(12)
                << alignment

                << std::right
                << std::setw(12)
                << count

                << std::setw(16)
                << std::fixed
                << std::setprecision(2)
                << linear.median_ns

                << std::setw(16)
                << normal_new.median_ns

                << std::setw(16)
                << pmr.median_ns

                << std::setw(14)
                << normal_new.median_ns /
                linear.median_ns

                << '\n';
        }
    }

    // ------------------------------------------------------------
    // Reset benchmark
    // ------------------------------------------------------------

    void run_reset_benchmark()
    {
        std::cout
            << "\n============================================================\n"
            << "RAW RESET COST\n"
            << "============================================================\n";

        const Stats stats =
            benchmark_linear_reset_only();

        std::cout
            << "Median reset cost : "
            << std::fixed
            << std::setprecision(2)
            << stats.median_ns
            << " ns\n"

            << "P95 reset cost    : "
            << stats.p95_ns
            << " ns\n"

            << "P99 reset cost    : "
            << stats.p99_ns
            << " ns\n";
    }

    // ------------------------------------------------------------
    // Full benchmark suite
    // ------------------------------------------------------------

    void run_suite()
    {
        std::cout
            << "############################################################\n"
            << "# Volt LinearAllocator Benchmark\n"
            << "############################################################\n\n"

            << "Arena size       : "
            << ARENA_SIZE / 1024 / 1024
            << " MiB\n"

            << "Warmup runs      : "
            << WARMUP_RUNS
            << '\n'

            << "Measured runs    : "
            << MEASURED_RUNS
            << '\n'

            << "Max allocations : "
            << MAX_ALLOCATIONS
            << '\n';

        // --------------------------------------------------------
        // 1. Broad size sweep
        // --------------------------------------------------------

        run_size_sweep();

        // --------------------------------------------------------
        // 2. Alignment sweep
        // --------------------------------------------------------

        run_alignment_sweep();

        // --------------------------------------------------------
        // 3. Detailed 64-byte case
        // --------------------------------------------------------

        run_allocation_benchmark(
            64,
            16);

        // --------------------------------------------------------
        // 4. Full lifecycle
        //
        // This is the most relevant benchmark for temporary
        // game-engine allocations.
        // --------------------------------------------------------

        run_lifecycle_benchmark(
            64,
            16);

        // --------------------------------------------------------
        // 5. Another realistic object size
        // --------------------------------------------------------

        run_lifecycle_benchmark(
            256,
            16);

        // --------------------------------------------------------
        // 6. Reset cost
        // --------------------------------------------------------

        run_reset_benchmark();

        std::cout
            << "\n============================================================\n"
            << "BENCHMARK COMPLETE\n"
            << "============================================================\n";

        std::cout
            << "Global sink: "
            << g_sink
            << '\n';
    }

} // namespace benchmark

// ============================================================
// MAIN
// ============================================================

