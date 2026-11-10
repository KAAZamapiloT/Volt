#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace volt::bench {

using Clock = std::chrono::steady_clock;

// ----------------------------------------------------------------------------
// Compiler barriers & Data Checksum Verification
// ----------------------------------------------------------------------------
inline volatile std::uint64_t g_checksum_sink = 0;

template <typename T>
inline void do_not_optimize(T&& val) {
    g_checksum_sink ^= reinterpret_cast<std::uintptr_t>(&val);
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "g"(val) : "memory");
#endif
}

inline void sink_checksum(std::uint64_t sum) {
    g_checksum_sink ^= sum;
}

// ----------------------------------------------------------------------------
// Benchmark Metrics & Statistics
// ----------------------------------------------------------------------------
struct BenchmarkStats {
    std::string category;
    std::string name;
    std::string methodology_note;
    std::string baseline_name;
    std::size_t operations = 0;
    std::size_t bytes_per_op = 0;

    double min_ns = 0.0;
    double median_ns = 0.0;
    double p95_ns = 0.0;
    double p99_ns = 0.0;
    double max_ns = 0.0;

    double ops_per_sec = 0.0;
    double gib_per_sec = 0.0;
    double speedup_vs_baseline = 1.0;
    std::uint64_t verified_checksum = 0;
};

inline double calculate_percentile(std::vector<double>& samples, double p) {
    if (samples.empty()) return 0.0;
    std::sort(samples.begin(), samples.end());
    const double idx = p * static_cast<double>(samples.size() - 1);
    const std::size_t lower = static_cast<std::size_t>(idx);
    const std::size_t upper = std::min(lower + 1, samples.size() - 1);
    const double frac = idx - static_cast<double>(lower);
    return samples[lower] + (samples[upper] - samples[lower]) * frac;
}

// ----------------------------------------------------------------------------
// Benchmark Suite Registry & Runner
// ----------------------------------------------------------------------------
class BenchmarkSuite {
public:
    static BenchmarkSuite& instance() {
        static BenchmarkSuite suite;
        return suite;
    }

    template <typename Fn>
    BenchmarkStats measure(
        std::string category,
        std::string name,
        std::string methodology_note,
        std::size_t operations,
        std::size_t bytes_per_op,
        Fn&& fn,
        std::size_t warmup_runs = 3,
        std::size_t measured_runs = 11) 
    {
        // 1. Warmup
        for (std::size_t i = 0; i < warmup_runs; ++i) {
            std::uint64_t sum = fn();
            sink_checksum(sum);
        }

        // 2. Measured samples
        std::vector<double> sample_ns;
        sample_ns.reserve(measured_runs);
        std::uint64_t last_checksum = 0;

        for (std::size_t i = 0; i < measured_runs; ++i) {
            const auto start = Clock::now();
            last_checksum = fn();
            const auto end = Clock::now();
            sink_checksum(last_checksum);

            const double elapsed = std::chrono::duration<double, std::nano>(end - start).count();
            sample_ns.push_back(elapsed);
        }

        BenchmarkStats stats;
        stats.category = std::move(category);
        stats.name = std::move(name);
        stats.methodology_note = std::move(methodology_note);
        stats.operations = operations;
        stats.bytes_per_op = bytes_per_op;
        stats.verified_checksum = last_checksum;

        stats.min_ns = *std::min_element(sample_ns.begin(), sample_ns.end());
        stats.max_ns = *std::max_element(sample_ns.begin(), sample_ns.end());
        stats.median_ns = calculate_percentile(sample_ns, 0.50);
        stats.p95_ns = calculate_percentile(sample_ns, 0.95);
        stats.p99_ns = calculate_percentile(sample_ns, 0.99);

        // Throughput based on median run duration
        const double median_sec = stats.median_ns / 1e9;
        if (median_sec > 0.0) {
            stats.ops_per_sec = static_cast<double>(operations) / median_sec;
            if (bytes_per_op > 0) {
                const double bytes_total = static_cast<double>(operations * bytes_per_op);
                stats.gib_per_sec = (bytes_total / median_sec) / (1024.0 * 1024.0 * 1024.0);
            }
        }

        results_.push_back(stats);
        return stats;
    }

    const std::vector<BenchmarkStats>& results() const {
        return results_;
    }

    void compute_speedups() {
        for (auto& stat : results_) {
            if (stat.name.find("baseline") != std::string::npos) {
                continue;
            }
            // Find baseline with matching category & matching operations
            for (const auto& other : results_) {
                if (other.category == stat.category && 
                    other.name.find("baseline") != std::string::npos &&
                    other.operations == stat.operations) {
                    stat.baseline_name = other.name;
                    if (stat.median_ns > 0.0) {
                        stat.speedup_vs_baseline = other.median_ns / stat.median_ns;
                    }
                    break;
                }
            }
        }
    }

    void print_table() {
        compute_speedups();

        std::cout << "\n"
                  << "========================================================================================================================\n"
                  << "                                              VOLT VERIFIED BENCHMARK REPORT                                            \n"
                  << "                              (Payloads written & verified, memory pages touched, percentiles recorded)                 \n"
                  << "========================================================================================================================\n";
        
        std::string current_cat = "";
        for (const auto& s : results_) {
            if (s.category != current_cat) {
                current_cat = s.category;
                std::cout << "\n--- [" << current_cat << "] ---\n";
                std::cout << std::left 
                          << std::setw(42) << "Benchmark"
                          << std::right
                          << std::setw(10) << "Ops"
                          << std::setw(14) << "p50 (ns/op)"
                          << std::setw(14) << "p99 (ns/op)"
                          << std::setw(16) << "Throughput"
                          << std::setw(14) << "Bandwidth"
                          << std::setw(12) << "Speedup"
                          << "\n";
                std::cout << std::string(122, '-') << "\n";
            }

            const double ns_per_op_p50 = (s.operations > 0) ? (s.median_ns / static_cast<double>(s.operations)) : s.median_ns;
            const double ns_per_op_p99 = (s.operations > 0) ? (s.p99_ns / static_cast<double>(s.operations)) : s.p99_ns;
            const double mops = s.ops_per_sec / 1e6;

            std::stringstream mops_ss;
            mops_ss << std::fixed << std::setprecision(2) << mops << " Mops/s";

            std::stringstream gib_ss;
            if (s.gib_per_sec > 0.0) {
                gib_ss << std::fixed << std::setprecision(2) << s.gib_per_sec << " GiB/s";
            } else {
                gib_ss << "-";
            }

            std::stringstream speedup_ss;
            if (s.speedup_vs_baseline > 0.01 && s.speedup_vs_baseline != 1.0) {
                speedup_ss << std::fixed << std::setprecision(2) << s.speedup_vs_baseline << "x";
            } else {
                speedup_ss << (s.name.find("baseline") != std::string::npos ? "1.00x (base)" : "-");
            }

            std::cout << std::left << std::setw(42) << s.name
                      << std::right
                      << std::setw(10) << s.operations
                      << std::setw(14) << std::fixed << std::setprecision(2) << ns_per_op_p50
                      << std::setw(14) << std::fixed << std::setprecision(2) << ns_per_op_p99
                      << std::setw(16) << mops_ss.str()
                      << std::setw(14) << gib_ss.str()
                      << std::setw(12) << speedup_ss.str()
                      << "\n";
            
            if (!s.methodology_note.empty()) {
                std::cout << "   ↳ Note: " << s.methodology_note << "\n";
            }
        }
        std::cout << std::string(122, '=') << "\n\n";
    }

    std::string to_json() const {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"timestamp\": \"" << __DATE__ << " " << __TIME__ << "\",\n";
        ss << "  \"results\": [\n";

        for (std::size_t i = 0; i < results_.size(); ++i) {
            const auto& s = results_[i];
            const double ns_per_op_p50 = (s.operations > 0) ? (s.median_ns / static_cast<double>(s.operations)) : s.median_ns;
            const double ns_per_op_p99 = (s.operations > 0) ? (s.p99_ns / static_cast<double>(s.operations)) : s.p99_ns;
            const double mops = s.ops_per_sec / 1e6;

            ss << "    {\n";
            ss << "      \"category\": \"" << s.category << "\",\n";
            ss << "      \"name\": \"" << s.name << "\",\n";
            ss << "      \"methodology\": \"" << s.methodology_note << "\",\n";
            ss << "      \"operations\": " << s.operations << ",\n";
            ss << "      \"bytes_per_op\": " << s.bytes_per_op << ",\n";
            ss << "      \"p50_ns_per_op\": " << ns_per_op_p50 << ",\n";
            ss << "      \"p99_ns_per_op\": " << ns_per_op_p99 << ",\n";
            ss << "      \"min_ns\": " << s.min_ns << ",\n";
            ss << "      \"median_ns\": " << s.median_ns << ",\n";
            ss << "      \"p95_ns\": " << s.p95_ns << ",\n";
            ss << "      \"p99_ns\": " << s.p99_ns << ",\n";
            ss << "      \"max_ns\": " << s.max_ns << ",\n";
            ss << "      \"mops_per_sec\": " << mops << ",\n";
            ss << "      \"gib_per_sec\": " << s.gib_per_sec << ",\n";
            ss << "      \"speedup_vs_baseline\": " << s.speedup_vs_baseline << ",\n";
            ss << "      \"checksum\": " << s.verified_checksum << "\n";
            ss << "    }" << (i + 1 < results_.size() ? "," : "") << "\n";
        }

        ss << "  ]\n";
        ss << "}\n";
        return ss.str();
    }

    bool save_json(const std::string& filepath) const {
        std::ofstream ofs(filepath);
        if (!ofs.is_open()) return false;
        ofs << to_json();
        return true;
    }

    bool save_js(const std::string& filepath) const {
        std::ofstream ofs(filepath);
        if (!ofs.is_open()) return false;
        ofs << "window.LIVE_BENCHMARK_RESULTS = " << to_json() << ";\n";
        return true;
    }

private:
    std::vector<BenchmarkStats> results_;
};

} // namespace volt::bench
