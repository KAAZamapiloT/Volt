#pragma once

#include <cmath>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace testing {

class Test {
public:
    virtual ~Test() = default;
    virtual void TestBody() = 0;
};

struct TestInfo {
    std::string suite_name;
    std::string test_name;
    std::function<Test*()> factory;
};

class UnitTest {
public:
    static UnitTest& GetInstance() {
        static UnitTest instance;
        return instance;
    }

    void RegisterTest(TestInfo info) {
        tests_.push_back(std::move(info));
    }

    int RunAllTests() {
        std::cout << "[==========] Running " << tests_.size() << " tests.\n";
        std::cout << "[----------] Global test environment set-up.\n";

        int passed = 0;
        int failed = 0;
        const auto total_start = std::chrono::steady_clock::now();

        std::string current_suite = "";

        for (const auto& test_info : tests_) {
            if (test_info.suite_name != current_suite) {
                current_suite = test_info.suite_name;
                std::cout << "[----------] " << test_info.suite_name << "\n";
            }

            std::cout << "[ RUN      ] " << test_info.suite_name << "." << test_info.test_name << "\n";
            current_failed_assertions_ = 0;

            const auto start = std::chrono::steady_clock::now();
            std::unique_ptr<Test> test(test_info.factory());
            try {
                test->TestBody();
            } catch (const std::exception& e) {
                std::cout << "  [EXCEPTION] Unexpected exception: " << e.what() << "\n";
                ++current_failed_assertions_;
            } catch (...) {
                std::cout << "  [EXCEPTION] Unknown exception thrown\n";
                ++current_failed_assertions_;
            }
            const auto end = std::chrono::steady_clock::now();
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            if (current_failed_assertions_ == 0) {
                std::cout << "[       OK ] " << test_info.suite_name << "." << test_info.test_name 
                          << " (" << ms << " ms)\n";
                ++passed;
            } else {
                std::cout << "[  FAILED  ] " << test_info.suite_name << "." << test_info.test_name 
                          << " (" << ms << " ms)\n";
                ++failed;
            }
        }

        const auto total_end = std::chrono::steady_clock::now();
        const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count();

        std::cout << "[----------] Global test environment tear-down\n";
        std::cout << "[==========] " << tests_.size() << " tests ran. (" << total_ms << " ms total)\n";
        std::cout << "[  PASSED  ] " << passed << " tests.\n";
        if (failed > 0) {
            std::cout << "[  FAILED  ] " << failed << " tests.\n";
            return 1;
        }
        return 0;
    }

    void AddFailure(const std::string& file, int line, const std::string& msg) {
        std::cout << file << ":" << line << ": Failure\n" << msg << "\n";
        ++current_failed_assertions_;
    }

private:
    std::vector<TestInfo> tests_;
    int current_failed_assertions_ = 0;
};

template <typename T>
struct TestRegistrar {
    TestRegistrar(const char* suite, const char* name) {
        UnitTest::GetInstance().RegisterTest({
            suite,
            name,
            []() -> Test* { return new T(); }
        });
    }
};

} // namespace testing

// ----------------------------------------------------------------------------
// Assertion Macros
// ----------------------------------------------------------------------------

#define EXPECT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            testing::UnitTest::GetInstance().AddFailure(__FILE__, __LINE__, "Value of: " #cond "\n  Actual: false\nExpected: true"); \
        } \
    } while (false)

#define EXPECT_FALSE(cond) \
    do { \
        if (cond) { \
            testing::UnitTest::GetInstance().AddFailure(__FILE__, __LINE__, "Value of: " #cond "\n  Actual: true\nExpected: false"); \
        } \
    } while (false)

#define ASSERT_TRUE(cond) EXPECT_TRUE(cond)
#define ASSERT_FALSE(cond) EXPECT_FALSE(cond)

#define EXPECT_EQ(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 == _v2)) { \
            std::stringstream _ss; \
            _ss << "Expected equality of these values:\n  " #val1 "\n    Which is: " << _v1 \
                << "\n  " #val2 "\n    Which is: " << _v2; \
            testing::UnitTest::GetInstance().AddFailure(__FILE__, __LINE__, _ss.str()); \
        } \
    } while (false)

#define EXPECT_NE(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (_v1 == _v2) { \
            std::stringstream _ss; \
            _ss << "Expected inequality of these values:\n  " #val1 "\n    Which is: " << _v1 \
                << "\n  " #val2 "\n    Which is: " << _v2; \
            testing::UnitTest::GetInstance().AddFailure(__FILE__, __LINE__, _ss.str()); \
        } \
    } while (false)

#define ASSERT_EQ(val1, val2) EXPECT_EQ(val1, val2)
#define ASSERT_NE(val1, val2) EXPECT_NE(val1, val2)

#define EXPECT_FLOAT_EQ(val1, val2) \
    do { \
        float _v1 = (val1); \
        float _v2 = (val2); \
        if (std::abs(_v1 - _v2) > 1e-5f) { \
            std::stringstream _ss; \
            _ss << "Expected float equality of these values:\n  " #val1 "\n    Which is: " << _v1 \
                << "\n  " #val2 "\n    Which is: " << _v2; \
            testing::UnitTest::GetInstance().AddFailure(__FILE__, __LINE__, _ss.str()); \
        } \
    } while (false)

// ----------------------------------------------------------------------------
// TEST Macro definition
// ----------------------------------------------------------------------------

#define TEST(test_suite_name, test_name) \
    class test_suite_name##_##test_name##_Test : public testing::Test { \
    public: \
        void TestBody() override; \
    }; \
    static testing::TestRegistrar<test_suite_name##_##test_name##_Test> \
        test_suite_name##_##test_name##_Registrar(#test_suite_name, #test_name); \
    void test_suite_name##_##test_name##_Test::TestBody()
