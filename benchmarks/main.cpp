#include <BenchmarkHarness.hpp>
#include <Allocators/LinearBench.hpp>
#include <Allocators/PoolBench.hpp>
#include <Allocators/BuddyBench.hpp>
#include <Containers/SPSCBench.hpp>
#include <Containers/SmallVectorBench.hpp>
#include <Containers/RingBufferBench.hpp>

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    bool run_alloc = true;
    bool run_cont = true;
    std::string json_path = "benchmarks/site/results.json";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--allocators") {
            run_alloc = true;
            run_cont = false;
        } else if (arg == "--containers") {
            run_alloc = false;
            run_cont = true;
        } else if (arg == "--all") {
            run_alloc = true;
            run_cont = true;
        } else if (arg == "--json" && i + 1 < argc) {
            json_path = argv[++i];
        }
    }

    std::cout << "Starting Volt Benchmarks...\n";

    if (run_alloc) {
        std::cout << "[Running Allocator Benchmarks...]\n";
        volt::bench::run_linear_allocator_benchmarks();
        volt::bench::run_pool_allocator_benchmarks();
        volt::bench::run_buddy_allocator_benchmarks();
    }

    if (run_cont) {
        std::cout << "[Running Container Benchmarks...]\n";
        volt::bench::run_spsc_benchmarks();
        volt::bench::run_smallvector_benchmarks();
        volt::bench::run_ringbuffer_benchmarks();
    }

    // Print table report to stdout
    volt::bench::BenchmarkSuite::instance().print_table();

    // Save JSON and JS outputs for the interactive dashboard
    if (!json_path.empty()) {
        if (volt::bench::BenchmarkSuite::instance().save_json(json_path)) {
            std::cout << "Results exported successfully to: " << json_path << "\n";
        } else {
            std::cerr << "Warning: Failed to write JSON results to " << json_path << "\n";
        }

        std::string js_path = json_path;
        if (js_path.length() >= 5 && js_path.substr(js_path.length() - 5) == ".json") {
            js_path.replace(js_path.length() - 5, 5, ".js");
            if (volt::bench::BenchmarkSuite::instance().save_js(js_path)) {
                std::cout << "Dashboard data file created: " << js_path << "\n";
            }
        }
        std::cout << "Open benchmarks/site/index.html in your browser to view the interactive dashboard!\n";
    }

    return 0;
}
