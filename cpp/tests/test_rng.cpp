#include <gtest/gtest.h>
#include "mms/rng.hpp"
#include <vector>

namespace mms {

class RNGTest : public ::testing::Test {
protected:
    void SetUp() override {
        rng.seed(42);
    }
    
    RNG rng;
};

TEST_F(RNGTest, SeededReproducibility) {
    RNG rng1(123);
    RNG rng2(123);
    
    // Generate same sequence with same seed
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.uniform_int(1, 100), rng2.uniform_int(1, 100));
        EXPECT_DOUBLE_EQ(rng1.uniform_real(), rng2.uniform_real());
    }
}

TEST_F(RNGTest, DifferentSeedsProduceDifferentResults) {
    RNG rng1(123);
    RNG rng2(456);
    
    // Different seeds should produce different results
    bool found_difference = false;
    for (int i = 0; i < 100; ++i) {
        if (rng1.uniform_int(1, 100) != rng2.uniform_int(1, 100)) {
            found_difference = true;
            break;
        }
    }
    EXPECT_TRUE(found_difference);
}

TEST_F(RNGTest, UniformIntRange) {
    const int min = 10;
    const int max = 50;
    
    for (int i = 0; i < 1000; ++i) {
        int value = rng.uniform_int(min, max);
        EXPECT_GE(value, min);
        EXPECT_LE(value, max);
    }
}

TEST_F(RNGTest, UniformRealRange) {
    for (int i = 0; i < 1000; ++i) {
        double value = rng.uniform_real();
        EXPECT_GE(value, 0.0);
        EXPECT_LT(value, 1.0);
    }
}

TEST_F(RNGTest, UniformRealRangeWithBounds) {
    const double min = 5.0;
    const double max = 15.0;
    
    for (int i = 0; i < 1000; ++i) {
        double value = rng.uniform_real(min, max);
        EXPECT_GE(value, min);
        EXPECT_LT(value, max);
    }
}

TEST_F(RNGTest, ExponentialDistribution) {
    const double lambda = 2.0;
    
    for (int i = 0; i < 1000; ++i) {
        double value = rng.exponential(lambda);
        EXPECT_GE(value, 0.0);
    }
}

TEST_F(RNGTest, NormalDistribution) {
    const double mean = 10.0;
    const double stddev = 2.0;
    
    double sum = 0.0;
    for (int i = 0; i < 1000; ++i) {
        double value = rng.normal(mean, stddev);
        sum += value;
    }
    
    double sample_mean = sum / 1000.0;
    // Should be close to the expected mean (within 3 standard errors)
    EXPECT_NEAR(sample_mean, mean, 3 * stddev / std::sqrt(1000.0));
}

TEST_F(RNGTest, PoissonDistribution) {
    const double lambda = 3.0;
    
    for (int i = 0; i < 1000; ++i) {
        int value = rng.poisson(lambda);
        EXPECT_GE(value, 0);
    }
}

TEST_F(RNGTest, BernoulliDistribution) {
    const double p = 0.7;
    int true_count = 0;
    
    for (int i = 0; i < 1000; ++i) {
        if (rng.bernoulli(p)) {
            true_count++;
        }
    }
    
    double sample_prob = static_cast<double>(true_count) / 1000.0;
    // Should be close to the expected probability
    EXPECT_NEAR(sample_prob, p, 0.1);
}

TEST_F(RNGTest, SeedReset) {
    RNG rng1(123);
    RNG rng2(456);
    
    // Generate different sequences
    int val1 = rng1.uniform_int(1, 100);
    int val2 = rng2.uniform_int(1, 100);
    EXPECT_NE(val1, val2);
    
    // Reset rng2 with same seed as rng1
    rng2.seed(123);
    
    // Now they should produce the same sequence
    EXPECT_EQ(rng1.uniform_int(1, 100), rng2.uniform_int(1, 100));
}

TEST_F(RNGTest, ContainerShuffle) {
    std::vector<int> original = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> shuffled = original;
    
    rng.shuffle(shuffled);
    
    // Should have same elements but potentially different order
    EXPECT_EQ(original.size(), shuffled.size());
    
    std::sort(original.begin(), original.end());
    std::sort(shuffled.begin(), shuffled.end());
    EXPECT_EQ(original, shuffled);
}

} // namespace mms
