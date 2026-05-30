/*
 * Minimal zero-dependency unit-test harness for SWIM.
 *
 * SWIM's main modules are tightly coupled to the OMNeT++ kernel, but a few
 * model classes (Configuration, Environment, Observations) are plain C++ that
 * only depend on the trivial pladapt mock. This harness lets those be tested
 * with nothing but a C++ compiler -- no OMNeT++, no Boost, no gtest -- so the
 * tests can run anywhere, including CI without the simulator toolchain.
 *
 * Define tests with TEST(name) { ... } and assert with CHECK / CHECK_EQ /
 * CHECK_NEAR. Tests self-register; main() just calls swimtest::runAll().
 */
#ifndef SWIM_TEST_FRAMEWORK_H
#define SWIM_TEST_FRAMEWORK_H

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace swimtest {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}
inline int& failures() {
    static int f = 0;
    return f;
}
inline int& checks() {
    static int c = 0;
    return c;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

inline void reportFailure(const std::string& expr, const char* file, int line) {
    ++failures();
    std::cerr << "    FAIL: " << expr << "  (" << file << ":" << line << ")\n";
}

inline int runAll() {
    int failedTests = 0;
    for (const auto& t : registry()) {
        const int before = failures();
        try {
            t.fn();
        } catch (const std::exception& e) {
            ++failures();
            std::cerr << "    EXCEPTION in " << t.name << ": " << e.what() << "\n";
        } catch (...) {
            ++failures();
            std::cerr << "    EXCEPTION in " << t.name << "\n";
        }
        const bool ok = failures() == before;
        std::cout << (ok ? "[ PASS ] " : "[ FAIL ] ") << t.name << "\n";
        if (!ok) ++failedTests;
    }
    std::cout << "\n"
              << registry().size() << " tests, " << checks() << " checks, "
              << failures() << " failure(s).\n";
    return failedTests == 0 ? 0 : 1;
}

}  // namespace swimtest

#define TEST(name)                                                       \
    static void name();                                                  \
    static swimtest::Registrar swim_registrar_##name(#name, name);       \
    static void name()

#define CHECK(cond)                                                      \
    do {                                                                 \
        ++swimtest::checks();                                            \
        if (!(cond)) swimtest::reportFailure(#cond, __FILE__, __LINE__); \
    } while (0)

#define CHECK_EQ(a, b)                                                          \
    do {                                                                        \
        ++swimtest::checks();                                                   \
        if (!((a) == (b)))                                                      \
            swimtest::reportFailure(#a " == " #b, __FILE__, __LINE__);          \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                   \
    do {                                                                        \
        ++swimtest::checks();                                                   \
        if (std::fabs((a) - (b)) > (eps))                                       \
            swimtest::reportFailure(#a " ~= " #b, __FILE__, __LINE__);          \
    } while (0)

#endif  // SWIM_TEST_FRAMEWORK_H
