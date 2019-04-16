
#include "gtest/gtest.h"
#include "services/golomb_coding.h"
#include <bitset>
#include <iostream>

namespace GolombCodingInitTest
{

class GolombCodingInitTest : public ::testing::TestWithParam<tuple<uint64_t, uint64_t, uint64_t, const uint8_t *>>
{
  protected:
    GolombCoding<uint64_t> g;
};

TEST_P(GolombCodingInitTest, ItInitializesWithCorrectBytes)
{

    tuple<uint64_t, uint64_t, uint64_t, const uint8_t *> params = GetParam();
    uint64_t m = get<0>(params);
    uint64_t size = get<1>(params);
    uint64_t expected_bits = get<2>(params);
    const uint8_t *expected_bytes = get<3>(params);
    g.init(m, size);

    EXPECT_EQ(expected_bits, g.n_bits);

    for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
    {
        EXPECT_EQ(expected_bytes[i], g.bytes[i]);
    }
}

// these indirectly test encode_remainder since it is a private method and cant be tested directly
tuple<uint64_t, uint64_t, uint64_t, const uint8_t *> const InitializationParams[] = {
    // 693147 is when fales positive is 1 in 1M

    // tests when remainder_starting_bit > 0 and remainder_bits > 0
    make_tuple(693147, 1000000, 21, (const uint8_t[3]){0xA5, 0x75, 0x28}),

    // tests when remainder_starting_bit % 8 = 0 and remainder_bits > 0
    // 4852029 is 7x 693147, which will result in 8 bits for unary and 0 starting bit for remainder, we add some bits to get a remainder
    make_tuple(693147, 4852029 + 500, 27, (const uint8_t[4]){0xFE, 0x00, 0x3E, 0x80}),

    // tests when remainder_bits % 8 = 0, 50000 results in b = 16, cutoff of 15536 so remainder_bits will be 16
    make_tuple(50000, 50000 + 15536, 18, (const uint8_t[3]){0x9E, 0x58, 0x00}),

    // tests when starting_bit > 0, remainder_bits > 0 and starting_bit + remainder_bit > 8
    make_tuple(693147, 4000000, 26, (const uint8_t[4]){0xFB, 0x64, 0xD7, 0x80}),

    // tests when num_remainder_bytes = 0
    make_tuple(20, 4096, 210, (const uint8_t[27]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x00}),
};

INSTANTIATE_TEST_CASE_P(ItInitializesWithCorrectBytes, GolombCodingInitTest, ::testing::ValuesIn(InitializationParams));

class GolombCodingInitWithDifferences : public ::testing::TestWithParam<tuple<uint64_t, uint64_t, uint64_t, vector<pair<uint64_t, bool>>, const uint8_t *>>
{
  protected:
    GolombCoding<uint64_t> g;
};

TEST_P(GolombCodingInitWithDifferences, EncodesBytesCorrectly)
{

    tuple<uint64_t, uint64_t, uint64_t, vector<pair<uint64_t, bool>>, const uint8_t *> params = GetParam();
    uint64_t m = get<0>(params);
    uint64_t size = get<1>(params);
    uint64_t expected_bits = get<2>(params);
    vector<pair<uint64_t, bool>> differences = get<3>(params);
    const uint8_t *expected_bytes = get<4>(params);

    g.init(m, size, differences);

    EXPECT_EQ(expected_bits, g.n_bits);

    for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
    {
        EXPECT_EQ(expected_bytes[i], g.bytes[i]);
    }
}

tuple<uint64_t, uint64_t, uint64_t, vector<pair<uint64_t, bool>>, const uint8_t *> const encodeDifferencesParams[] = {

    // m = 69, b = 7, cutoff = 59
    make_tuple(69, 100, 22, vector<pair<uint64_t, bool>>{pair<uint64_t, bool>(7, false), pair<uint64_t, bool>(1, true), pair<uint64_t, bool>(90, false), pair<uint64_t, bool>(1, true), pair<uint64_t, bool>(1, false)},
               (const uint8_t[3]){0x0F, 0x2A, 0x04}),

    // m = 69, b = 7, cutoff = 59
    // when num_ones that are equal to 1 are removed
    make_tuple(69, 100, 22, vector<pair<uint64_t, bool>>{pair<uint64_t, bool>(7, false), pair<uint64_t, bool>(90, false), pair<uint64_t, bool>(1, false)},
               (const uint8_t[3]){0x0F, 0x2A, 0x04}),

    // tests when there are bits at front
    make_tuple(69, 100, 15, vector<pair<uint64_t, bool>>{pair<uint64_t, bool>(1, true), pair<uint64_t, bool>(99, false)}, (const uint8_t[2]){0x01, 0x3C}),

    // tests when there are one bits at end
    make_tuple(69, 100, 22, vector<pair<uint64_t, bool>>{pair<uint64_t, bool>(98, false), pair<uint64_t, bool>(2, true)},
               (const uint8_t[3]){0x9D, 0x00, 0x00}),

    // tests when there are multiple one bits
    make_tuple(69, 100, 22, vector<pair<uint64_t, bool>>{pair<uint64_t, bool>(69, false), pair<uint64_t, bool>(2, true), pair<uint64_t, bool>(29, false)},
               (const uint8_t[3]){0x80, 0x00, 0x74}),
};

INSTANTIATE_TEST_CASE_P(EncodesBytesCorrectly, GolombCodingInitWithDifferences, ::testing::ValuesIn(encodeDifferencesParams));

} // namespace GolombCodingInitTest