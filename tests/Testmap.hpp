#pragma once

 // Change path if your map header lives elsewhere.
#include<volt/Containers/map.hpp>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace volt::test {

    // ------------------------------------------------------------
    // Small helpers
    // ------------------------------------------------------------

    inline void require(bool condition, const char* message)
    {
        if (!condition) {
            std::cerr << "[FAIL] " << message << '\n';
            std::abort();
        }
    }

    inline void print_test(const char* name)
    {
        std::cout << "[TEST] " << name << " ... ";
    }

    inline void pass()
    {
        std::cout << "PASS\n";
    }

    // ------------------------------------------------------------
    // Basic insert/find test
    // ------------------------------------------------------------

    inline void test_insert_find()
    {
        print_test("insert/find");

        volt::map<int, int> m;

        require(m.empty(), "new map must be empty");
        require(m.size() == 0, "new map size must be zero");

        require(m.insert(10, 100), "insert 10");
        require(m.insert(5, 50), "insert 5");
        require(m.insert(20, 200), "insert 20");
        require(m.insert(1, 10), "insert 1");
        require(m.insert(7, 70), "insert 7");
        require(m.insert(15, 150), "insert 15");
        require(m.insert(30, 300), "insert 30");

        require(m.size() == 7, "size after insertion");

        auto* v10 = m.find(10);
        auto* v5 = m.find(5);
        auto* v30 = m.find(30);
        auto* v99 = m.find(99);

        require(v10 && *v10 == 100, "find 10");
        require(v5 && *v5 == 50, "find 5");
        require(v30 && *v30 == 300, "find 30");
        require(v99 == nullptr, "missing key must return nullptr");

        pass();
    }

    // ------------------------------------------------------------
    // Duplicate insertion
    // ------------------------------------------------------------

    inline void test_duplicate_insert()
    {
        print_test("duplicate insertion");

        volt::map<int, int> m;

        require(m.insert(10, 100), "first insertion");
        require(!m.insert(10, 999), "duplicate insertion must fail");

        require(m.size() == 1, "duplicate must not increase size");

        auto* value = m.find(10);

        require(value != nullptr, "key must still exist");
        require(*value == 100, "duplicate must not replace value");

        pass();
    }

    // ------------------------------------------------------------
    // Delete leaf nodes
    // ------------------------------------------------------------

    inline void test_remove_leaf()
    {
        print_test("remove leaf");

        volt::map<int, int> m;

        m.insert(10, 10);
        m.insert(5, 5);
        m.insert(15, 15);
        m.insert(2, 2);
        m.insert(7, 7);

        require(m.remove(2), "remove leaf 2");
        require(m.find(2) == nullptr, "2 must be gone");
        require(m.size() == 4, "size after removing leaf");

        require(!m.remove(2), "removing missing key must fail");
        require(m.size() == 4, "failed remove must not change size");

        pass();
    }

    // ------------------------------------------------------------
    // One-child deletion
    // ------------------------------------------------------------

    inline void test_remove_one_child()
    {
        print_test("remove one-child node");

        volt::map<int, int> m;

        m.insert(10, 10);
        m.insert(5, 5);
        m.insert(15, 15);
        m.insert(20, 20);

        require(m.remove(15), "remove node with one child");
        require(m.find(15) == nullptr, "15 must be gone");
        require(m.find(20) != nullptr, "child must survive");
        require(m.size() == 3, "size after one-child removal");

        pass();
    }

    // ------------------------------------------------------------
    // Two-child deletion
    // ------------------------------------------------------------

    inline void test_remove_two_children()
    {
        print_test("remove two-child node");

        volt::map<int, int> m;

        m.insert(20, 20);
        m.insert(10, 10);
        m.insert(30, 30);
        m.insert(5, 5);
        m.insert(15, 15);
        m.insert(25, 25);
        m.insert(40, 40);

        require(m.remove(20), "remove root with two children");

        require(m.find(20) == nullptr, "20 must be gone");

        require(m.find(5) != nullptr, "5 survives");
        require(m.find(10) != nullptr, "10 survives");
        require(m.find(15) != nullptr, "15 survives");
        require(m.find(25) != nullptr, "25 survives");
        require(m.find(30) != nullptr, "30 survives");
        require(m.find(40) != nullptr, "40 survives");

        require(m.size() == 6, "size after two-child removal");

        pass();
    }

    // ------------------------------------------------------------
    // Root deletion
    // ------------------------------------------------------------

    inline void test_remove_root()
    {
        print_test("remove root");

        volt::map<int, int> m;

        m.insert(10, 10);

        require(m.remove(10), "remove only root");
        require(m.empty(), "map should be empty");
        require(m.size() == 0, "size should be zero");
        require(m.find(10) == nullptr, "root must be gone");

        pass();
    }

    // ------------------------------------------------------------
    // Repeated root removal
    // ------------------------------------------------------------

    inline void test_repeated_root_removal()
    {
        print_test("repeated root removal");

        volt::map<int, int> m;

        for (int i = 0; i < 100; ++i) {
            require(m.insert(i, i * 10), "insert");

            require(m.remove(i), "remove immediately inserted key");

            require(m.empty(), "map should become empty");
            require(m.size() == 0, "size should return to zero");
        }

        pass();
    }

    // ------------------------------------------------------------
    // Sequential insert/delete
    // ------------------------------------------------------------

    inline void test_sequential_operations()
    {
        print_test("sequential operations");

        volt::map<int, int> m;

        for (int i = 0; i < 1000; ++i) {
            require(m.insert(i, i * 7), "sequential insert");
        }

        require(m.size() == 1000, "1000 nodes inserted");

        for (int i = 0; i < 1000; ++i) {
            auto* value = m.find(i);

            require(value != nullptr, "all inserted keys must exist");
            require(*value == i * 7, "stored value is wrong");
        }

        for (int i = 0; i < 1000; ++i) {
            require(m.remove(i), "sequential remove");
        }

        require(m.empty(), "map empty after all removals");
        require(m.size() == 0, "size zero after all removals");

        pass();
    }

    // ------------------------------------------------------------
    // Copy constructor
    // ------------------------------------------------------------

    inline void test_copy_constructor()
    {
        print_test("copy constructor");

        volt::map<int, int> a;

        for (int i = 0; i < 100; ++i) {
            a.insert(i, i * 2);
        }

        volt::map<int, int> b(a);

        require(a.size() == 100, "original size");
        require(b.size() == 100, "copy size");

        for (int i = 0; i < 100; ++i) {
            auto* av = a.find(i);
            auto* bv = b.find(i);

            require(av && bv, "copied key exists");
            require(*av == *bv, "copied value matches");
        }

        // Verify deep copy.
        a.remove(50);

        require(a.find(50) == nullptr, "original modified");
        require(b.find(50) != nullptr, "copy must be independent");

        pass();
    }

    // ------------------------------------------------------------
    // Copy assignment
    // ------------------------------------------------------------

    inline void test_copy_assignment()
    {
        print_test("copy assignment");

        volt::map<int, int> a;
        volt::map<int, int> b;

        for (int i = 0; i < 100; ++i) {
            a.insert(i, i * 3);
        }

        for (int i = 100; i < 200; ++i) {
            b.insert(i, i * 3);
        }

        b = a;

        require(b.size() == a.size(), "assigned size");

        for (int i = 0; i < 100; ++i) {
            auto* av = a.find(i);
            auto* bv = b.find(i);

            require(av && bv, "assigned key exists");
            require(*av == *bv, "assigned value matches");
        }

        a.remove(10);

        require(a.find(10) == nullptr, "original modified");
        require(b.find(10) != nullptr, "assigned map must be independent");

        pass();
    }

    // ------------------------------------------------------------
    // Move constructor
    // ------------------------------------------------------------

    inline void test_move_constructor()
    {
        print_test("move constructor");

        volt::map<int, int> a;

        for (int i = 0; i < 100; ++i) {
            a.insert(i, i);
        }

        volt::map<int, int> b(std::move(a));

        require(a.empty(), "moved-from map must be empty");
        require(a.size() == 0, "moved-from size must be zero");

        require(b.size() == 100, "moved map size");

        for (int i = 0; i < 100; ++i) {
            auto* value = b.find(i);

            require(value != nullptr, "moved key exists");
            require(*value == i, "moved value correct");
        }

        pass();
    }

    // ------------------------------------------------------------
    // Move assignment
    // ------------------------------------------------------------

    inline void test_move_assignment()
    {
        print_test("move assignment");

        volt::map<int, int> a;
        volt::map<int, int> b;

        for (int i = 0; i < 100; ++i) {
            a.insert(i, i);
        }

        for (int i = 100; i < 200; ++i) {
            b.insert(i, i);
        }

        b = std::move(a);

        require(a.empty(), "moved-from map must be empty");
        require(b.size() == 100, "move-assigned size");

        for (int i = 0; i < 100; ++i) {
            auto* value = b.find(i);

            require(value != nullptr, "move assigned key");
            require(*value == i, "move assigned value");
        }

        pass();
    }

    // ------------------------------------------------------------
    // Self assignment
    // ------------------------------------------------------------

    inline void test_self_assignment()
    {
        print_test("self assignment");

        volt::map<int, int> m;

        for (int i = 0; i < 100; ++i) {
            m.insert(i, i);
        }

        m = m;

        require(m.size() == 100, "self assignment size");

        for (int i = 0; i < 100; ++i) {
            auto* value = m.find(i);

            require(value != nullptr, "self assigned key");
            require(*value == i, "self assigned value");
        }

        pass();
    }

    // ------------------------------------------------------------
    // Differential test against std::map
    //
    // Random operations are performed on both containers.
    // If your RB-tree is wrong, this tends to find it.
    // ------------------------------------------------------------

    inline void test_randomized(
        std::uint32_t seed = 0xC0FFEE,
        int operations = 100000,
        int key_range = 5000)
    {
        print_test("randomized differential test");

        volt::map<int, int> actual;
        std::map<int, int> reference;

        std::mt19937 rng(seed);

        std::uniform_int_distribution<int> key_dist(0, key_range - 1);
        std::uniform_int_distribution<int> value_dist(-1000000, 1000000);
        std::uniform_int_distribution<int> operation_dist(0, 99);

        for (int i = 0; i < operations; ++i) {

            const int key = key_dist(rng);
            const int operation = operation_dist(rng);

            if (operation < 55)
            {
                // INSERT
                const int value = value_dist(rng);

                const bool expected =
                    reference.emplace(key, value).second;

                const bool got =
                    actual.insert(key, value);

                require(
                    got == expected,
                    "insert result differs from std::map"
                );
            }
            else
            {
                // REMOVE
                const bool expected =
                    reference.erase(key) != 0;

                const bool got =
                    actual.remove(key);

                require(
                    got == expected,
                    "remove result differs from std::map"
                );
            }

            require(
                actual.size() == reference.size(),
                "size differs from std::map"
            );

            // Verify some random keys after every operation.
            for (int j = 0; j < 3; ++j)
            {
                const int check_key = key_dist(rng);

                auto ref_it = reference.find(check_key);
                auto* actual_value = actual.find(check_key);

                if (ref_it == reference.end())
                {
                    require(
                        actual_value == nullptr,
                        "missing key exists in actual map"
                    );
                }
                else
                {
                    require(
                        actual_value != nullptr,
                        "existing key missing from actual map"
                    );

                    require(
                        *actual_value == ref_it->second,
                        "value differs from std::map"
                    );
                }
            }
        }

        // Final complete comparison.
        require(
            actual.size() == reference.size(),
            "final size differs"
        );

        for (const auto& [key, value] : reference)
        {
            auto* actual_value = actual.find(key);

            require(
                actual_value != nullptr,
                "final key missing"
            );

            require(
                *actual_value == value,
                "final value differs"
            );
        }

        pass();
    }

    // ------------------------------------------------------------
    // Stress: insert sorted keys.
    //
    // A normal BST would become O(N) deep here.
    // RB tree should remain logarithmic.
    // ------------------------------------------------------------

    inline void test_sorted_insertion(int count = 100000)
    {
        print_test("sorted insertion stress");

        volt::map<int, int> m;

        for (int i = 0; i < count; ++i) {
            require(
                m.insert(i, i),
                "sorted insertion failed"
            );
        }

        require(
            m.size() == static_cast<usize>(count),
            "sorted insertion size"
        );

        for (int i = 0; i < count; ++i) {
            auto* value = m.find(i);

            require(
                value != nullptr && *value == i,
                "sorted insertion lookup failed"
            );
        }

        pass();
    }

    // ------------------------------------------------------------
    // Benchmark
    // ------------------------------------------------------------

    struct BenchmarkResult
    {
        double insert_ms;
        double find_ms;
        double remove_ms;
    };

    inline BenchmarkResult benchmark(
        int count = 1'000'000,
        std::uint32_t seed = 0x12345678)
    {
        std::cout << "\n========== BENCHMARK ==========\n";
        std::cout << "Elements: " << count << '\n';

        std::vector<int> keys(count);

        for (int i = 0; i < count; ++i)
            keys[i] = i;

        std::mt19937 rng(seed);
        std::shuffle(keys.begin(), keys.end(), rng);

        volt::map<int, int> m;

        // --------------------------------------------------------
        // INSERT
        // --------------------------------------------------------

        const auto insert_start =
            std::chrono::steady_clock::now();

        for (int key : keys)
            m.insert(key, key);

        const auto insert_end =
            std::chrono::steady_clock::now();

        // Prevent optimizer from making assumptions.
        volatile std::size_t insert_size =
            static_cast<std::size_t>(m.size());

        // --------------------------------------------------------
        // FIND
        // --------------------------------------------------------

        volatile std::int64_t find_checksum = 0;

        const auto find_start =
            std::chrono::steady_clock::now();

        for (int key : keys)
        {
            auto* value = m.find(key);

            if (value)
                find_checksum += *value;
        }

        const auto find_end =
            std::chrono::steady_clock::now();

        // --------------------------------------------------------
        // REMOVE
        // --------------------------------------------------------

        const auto remove_start =
            std::chrono::steady_clock::now();

        for (int key : keys)
            m.remove(key);

        const auto remove_end =
            std::chrono::steady_clock::now();

        const double insert_ms =
            std::chrono::duration<double, std::milli>(
                insert_end - insert_start
            ).count();

        const double find_ms =
            std::chrono::duration<double, std::milli>(
                find_end - find_start
            ).count();

        const double remove_ms =
            std::chrono::duration<double, std::milli>(
                remove_end - remove_start
            ).count();

        std::cout << "Insert : " << insert_ms << " ms\n";
        std::cout << "Find   : " << find_ms << " ms\n";
        std::cout << "Remove : " << remove_ms << " ms\n";

        std::cout << "Final size: " << m.size() << '\n';
        std::cout << "Find checksum: "
            << find_checksum << '\n';

        // Silence unused-variable warnings.
        (void)insert_size;

        return {
            insert_ms,
            find_ms,
            remove_ms
        };
    }

    inline void benchmark_mixed(
        int initial_size = 100'000,
        int operations = 2'000'000,
        int key_range = 1'000'000,
        std::uint32_t seed = 0x12345678)
    {
        using Clock = std::chrono::steady_clock;

        volt::map<int, int> m;

        std::mt19937 rng(seed);

        std::uniform_int_distribution<int> key_dist(0, key_range - 1);
        std::uniform_int_distribution<int> op_dist(0, 99);

        // ------------------------------------------------------------
        // Warmup population
        // ------------------------------------------------------------

        for (int i = 0; i < initial_size; ++i)
        {
            int key = key_dist(rng);
            m.insert(key, key);
        }

        std::size_t insert_success = 0;
        std::size_t remove_success = 0;
        std::size_t find_success = 0;

        std::size_t insert_fail = 0;
        std::size_t remove_fail = 0;
        std::size_t find_fail = 0;

        std::int64_t checksum = 0;

        std::chrono::nanoseconds insert_time{ 0 };
        std::chrono::nanoseconds remove_time{ 0 };
        std::chrono::nanoseconds find_time{ 0 };

        // ------------------------------------------------------------
        // Mixed workload
        //
        // 40% insert
        // 30% find
        // 30% remove
        // ------------------------------------------------------------

        for (int i = 0; i < operations; ++i)
        {
            const int key = key_dist(rng);
            const int operation = op_dist(rng);

            if (operation < 40)
            {
                const auto start = Clock::now();

                const bool result = m.insert(key, key);

                const auto end = Clock::now();

                insert_time +=
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        end - start
                    );

                if (result)
                    ++insert_success;
                else
                    ++insert_fail;
            }
            else if (operation < 70)
            {
                const auto start = Clock::now();

                auto* value = m.find(key);

                const auto end = Clock::now();

                find_time +=
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        end - start
                    );

                if (value)
                {
                    ++find_success;
                    checksum += *value;
                }
                else
                {
                    ++find_fail;
                }
            }
            else
            {
                const auto start = Clock::now();

                const bool result = m.remove(key);

                const auto end = Clock::now();

                remove_time +=
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        end - start
                    );

                if (result)
                    ++remove_success;
                else
                    ++remove_fail;
            }
        }

        const auto insert_ns = insert_time.count();
        const auto remove_ns = remove_time.count();
        const auto find_ns = find_time.count();

        std::cout << "\n========== MIXED WORKLOAD ==========\n";
        std::cout << "Initial size : " << initial_size << '\n';
        std::cout << "Operations   : " << operations << '\n';
        std::cout << "Key range    : " << key_range << '\n';
        std::cout << "Final size   : " << m.size() << "\n\n";

        std::cout << "INSERT\n";
        std::cout << "  success    : " << insert_success << '\n';
        std::cout << "  failed     : " << insert_fail << '\n';
        std::cout << "  avg ns/op  : "
            << (insert_success + insert_fail
                ? static_cast<double>(insert_ns) /
                (insert_success + insert_fail)
                : 0.0)
            << "\n\n";

        std::cout << "FIND\n";
        std::cout << "  success    : " << find_success << '\n';
        std::cout << "  failed     : " << find_fail << '\n';
        std::cout << "  avg ns/op  : "
            << (find_success + find_fail
                ? static_cast<double>(find_ns) /
                (find_success + find_fail)
                : 0.0)
            << "\n\n";

        std::cout << "REMOVE\n";
        std::cout << "  success    : " << remove_success << '\n';
        std::cout << "  failed     : " << remove_fail << '\n';
        std::cout << "  avg ns/op  : "
            << (remove_success + remove_fail
                ? static_cast<double>(remove_ns) /
                (remove_success + remove_fail)
                : 0.0)
            << "\n\n";

        std::cout << "Checksum     : " << checksum << '\n';
    }
    // ------------------------------------------------------------
    // Run all correctness tests
    // ------------------------------------------------------------

    inline void run_correctness_tests()
    {
        std::cout << "\n========== RB MAP TESTS ==========\n";

        test_insert_find();
        test_duplicate_insert();

        test_remove_leaf();
        test_remove_one_child();
        test_remove_two_children();
        test_remove_root();
        test_repeated_root_removal();

        test_sequential_operations();

        test_copy_constructor();
        test_copy_assignment();
        test_move_constructor();
        test_move_assignment();
        test_self_assignment();

        test_sorted_insertion();

        test_randomized();

        std::cout << "\nALL TESTS PASSED\n";
    }

} // namespace volt::test