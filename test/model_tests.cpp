/*
 * Unit tests for SWIM's OMNeT++-independent model classes.
 *
 * These pin down behavior that is easy to break during a framework port:
 *  - Configuration: the booting-server accounting in getServers() vs
 *    getActiveServers(), and the bootRemain reset side effect.
 *  - Environment: the asDouble() projection used as the environment key.
 *  - Observations: documented default-construction state.
 */
#include <sstream>

#include "model/Configuration.h"
#include "model/Environment.h"
#include "model/Observations.h"
#include "test_framework.h"

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

TEST(Configuration_defaultsAreZero) {
    Configuration c;
    CHECK_EQ(c.getActiveServers(), 0);
    CHECK_EQ(c.getServers(), 0);
    CHECK_EQ(c.getBootRemain(), 0);
    CHECK_EQ(c.getBrownOutLevel(), 0);
    CHECK(!c.isColdCache());
}

TEST(Configuration_parameterizedCtorStoresFields) {
    Configuration c(2, 1, 3, true);
    CHECK_EQ(c.getActiveServers(), 2);
    CHECK_EQ(c.getBootRemain(), 1);
    CHECK_EQ(c.getBrownOutLevel(), 3);
    CHECK(c.isColdCache());
}

// getServers() counts the active servers plus one more if a server is booting.
TEST(Configuration_getServersCountsBootingServer) {
    Configuration booting(2, 1, 0, false);
    CHECK_EQ(booting.getActiveServers(), 2);
    CHECK_EQ(booting.getServers(), 3);  // 2 active + 1 booting

    Configuration steady(2, 0, 0, false);
    CHECK_EQ(steady.getServers(), 2);  // nothing booting

    // bootRemain > 1 still only adds a single pending server.
    Configuration longBoot(4, 5, 0, false);
    CHECK_EQ(longBoot.getServers(), 5);
}

// setActiveServers() also clears any in-progress boot.
TEST(Configuration_setActiveServersResetsBootRemain) {
    Configuration c(2, 3, 0, false);
    CHECK_EQ(c.getServers(), 3);  // booting before
    c.setActiveServers(5);
    CHECK_EQ(c.getActiveServers(), 5);
    CHECK_EQ(c.getBootRemain(), 0);
    CHECK_EQ(c.getServers(), 5);  // no longer booting
}

TEST(Configuration_settersRoundTrip) {
    Configuration c;
    c.setBootRemain(4);
    c.setBrownOutLevel(2);
    c.setColdCache(true);
    CHECK_EQ(c.getBootRemain(), 4);
    CHECK_EQ(c.getBrownOutLevel(), 2);
    CHECK(c.isColdCache());
}

// equals() is protected; expose it through a tiny test-only subclass so the
// value-equality semantics can be exercised.
namespace {
struct ConfigProbe : public Configuration {
    using Configuration::Configuration;
    bool eq(const Configuration& other) const { return equals(other); }
};
}  // namespace

TEST(Configuration_equalsComparesAllFields) {
    ConfigProbe base(2, 1, 3, true);
    CHECK(base.eq(Configuration(2, 1, 3, true)));
    CHECK(!base.eq(Configuration(3, 1, 3, true)));  // servers differ
    CHECK(!base.eq(Configuration(2, 0, 3, true)));  // bootRemain differs
    CHECK(!base.eq(Configuration(2, 1, 4, true)));  // brownoutLevel differs
    CHECK(!base.eq(Configuration(2, 1, 3, false)));  // coldCache differs
}

TEST(Configuration_printOnIncludesState) {
    Configuration c(2, 1, 3, true);
    std::ostringstream os;
    c.printOn(os);
    const std::string s = os.str();
    CHECK(s.find("servers=2") != std::string::npos);
    CHECK(s.find("bootRemain=1") != std::string::npos);
    CHECK(s.find("brownoutLevel=3") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Environment
// ---------------------------------------------------------------------------

TEST(Environment_defaultsAreZero) {
    Environment e;
    CHECK_NEAR(e.getArrivalMean(), 0.0, 1e-12);
    CHECK_NEAR(e.getArrivalVariance(), 0.0, 1e-12);
    CHECK_NEAR(e.asDouble(), 0.0, 1e-12);
}

TEST(Environment_parameterizedCtorStoresFields) {
    Environment e(1.5, 0.25);
    CHECK_NEAR(e.getArrivalMean(), 1.5, 1e-12);
    CHECK_NEAR(e.getArrivalVariance(), 0.25, 1e-12);
}

// asDouble() projects the environment onto its arrival mean (the key used to
// index environment state in the model).
TEST(Environment_asDoubleReturnsArrivalMean) {
    Environment e(2.75, 9.0);
    CHECK_NEAR(e.asDouble(), 2.75, 1e-12);
    CHECK_NEAR(e.asDouble(), e.getArrivalMean(), 1e-12);
}

TEST(Environment_settersRoundTrip) {
    Environment e;
    e.setArrivalMean(3.0);
    e.setArrivalVariance(0.5);
    CHECK_NEAR(e.getArrivalMean(), 3.0, 1e-12);
    CHECK_NEAR(e.getArrivalVariance(), 0.5, 1e-12);
    CHECK_NEAR(e.asDouble(), 3.0, 1e-12);
}

// ---------------------------------------------------------------------------
// Observations
// ---------------------------------------------------------------------------

// NOTE: the Observations() constructor only initializes avgResponseTime and
// utilization. The basic/opt response-time and throughput members are left
// uninitialized, so we deliberately do not assert on their default values
// here. (Flagged as a latent bug worth fixing separately.)
TEST(Observations_documentedDefaultsAreZero) {
    Observations o;
    CHECK_NEAR(o.avgResponseTime, 0.0, 1e-12);
    CHECK_NEAR(o.utilization, 0.0, 1e-12);
}

TEST(Observations_fieldsAreAssignable) {
    Observations o;
    o.basicResponseTime = 0.1;
    o.optResponseTime = 0.2;
    o.basicThroughput = 10.0;
    o.optThroughput = 20.0;
    o.avgResponseTime = 0.15;
    o.utilization = 0.8;
    CHECK_NEAR(o.basicResponseTime, 0.1, 1e-12);
    CHECK_NEAR(o.optResponseTime, 0.2, 1e-12);
    CHECK_NEAR(o.basicThroughput, 10.0, 1e-12);
    CHECK_NEAR(o.optThroughput, 20.0, 1e-12);
    CHECK_NEAR(o.avgResponseTime, 0.15, 1e-12);
    CHECK_NEAR(o.utilization, 0.8, 1e-12);
}
