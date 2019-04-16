
#include "gtest/gtest.h"
#include "services/golomb_coding.h"
#include <bitset>
#include <iostream>

namespace GolombCodingAddBitTest
{

class GolombCodingAddBitTest : public ::testing::TestWithParam<tuple<uint64_t, uint64_t, uint64_t, uint64_t, const uint8_t *>>
{
protected:
  GolombCoding<uint64_t> g;
};

TEST_F(GolombCodingAddBitTest, DoesNothingWhenBitAlreadyAdded)
{
  g.init(693147, 1000000);

  const uint8_t expected_bytes[6] = {0x4A, 0xEA, 0x48, 0x00, 0x00, 0x00};
  g.add_bit(306853);
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(expected_bytes[i], g.bytes[i]);
  }

  g.add_bit(306853);
  g.add_bit(306853);
  g.add_bit(306853);

  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(expected_bytes[i], g.bytes[i]);
  }
}

TEST_F(GolombCodingAddBitTest, CanAddBitRightAfterASetBit)
{
  g.init(693147, 1000000);

  const uint8_t initial_bytes[6] = {0x4A, 0xEA, 0x48, 0x00, 0x00, 0x00};
  g.add_bit(306853);
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(initial_bytes[i], g.bytes[i]);
  }

  const uint8_t expected_bytes[9] = {0x4A, 0xEA, 0x40, 0x00, 0x00, 0x7F, 0xFF, 0xF8};
  g.add_bit(306854);
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(expected_bytes[i], g.bytes[i]);
  }
  EXPECT_EQ(61, g.n_bits);
}

TEST_F(GolombCodingAddBitTest, CanAddBitRightBeforeASetBit)
{
  g.init(693147, 1000000);

  const uint8_t initial_bytes[6] = {0x4A, 0xEA, 0x48, 0x00, 0x00, 0x00};
  g.add_bit(306853);
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(initial_bytes[i], g.bytes[i]);
  }

  g.add_bit(306852);
  const uint8_t expected_bytes[8] = {0x4A, 0xEA, 0x30, 0x00, 0x00, 0x80, 0x00, 0x00};
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(expected_bytes[i], g.bytes[i]);
  }
  EXPECT_EQ(61, g.n_bits);
}

TEST_F(GolombCodingAddBitTest, CanAddBitRightBeforeTwoBitsAreSet)
{
  g.init(693147, 1000000);

  const uint8_t initial_bytes[6] = {0x4A, 0xEA, 0x48, 0x00, 0x00, 0x00};
  g.add_bit(306853);
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(initial_bytes[i], g.bytes[i]);
  }

  g.add_bit(306852);
  const uint8_t bytes_after_second_bit[8] = {0x4A, 0xEA, 0x30, 0x00, 0x00, 0x80, 0x00, 0x00};
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(bytes_after_second_bit[i], g.bytes[i]);
  }

  g.add_bit(306851);
  const uint8_t expected_bytes[11] = {0x4A, 0xEA, 0x20, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00};
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(expected_bytes[i], g.bytes[i]);
  }
  EXPECT_EQ(81, g.n_bits);
}

TEST_F(GolombCodingAddBitTest, CanAddBitRightBeforeTwoBitsAreSetAtFrontOfSet)
{
  g.init(693147, 1000000);

  const uint8_t initial_bytes[6] = {0x00, 0x00, 0x2A, 0x57, 0x51, 0x00};
  g.add_bit(3);
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(initial_bytes[i], g.bytes[i]);
  }

  g.add_bit(1);
  const uint8_t bytes_after_second_bit[8] = {0x00, 0x00, 0x00, 0x00, 0x01, 0xA5, 0x75, 0x10};
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(bytes_after_second_bit[i], g.bytes[i]);
  }

  g.add_bit(2);
  const uint8_t expected_bytes[11] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x57, 0x51, 0x00};
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(expected_bytes[i], g.bytes[i]);
  }
  EXPECT_EQ(81, g.n_bits);
}

TEST_F(GolombCodingAddBitTest, CanHandleEdgeCase)
{
  g.init(20, 4096);
  const uint8_t initial_bytes[27] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x00};

  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(initial_bytes[i], g.bytes[i]);
  }
  EXPECT_EQ(210, g.n_bits);

  g.add_bit(2962);
  const uint8_t bytes_after_first_bit[27] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB4};
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(bytes_after_first_bit[i], g.bytes[i]);
  }
  EXPECT_EQ(215, g.n_bits);

  g.add_bit(3224);
  const uint8_t bytes_after_second_bit[28] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFC, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0x80};
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(bytes_after_second_bit[i], g.bytes[i]);
  }
  EXPECT_EQ(220, g.n_bits);

  g.add_bit(2549);
  const uint8_t bytes_after_third_bit[29] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x8F, 0xFF, 0xFF, 0x63, 0xFF, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEC, 0x00};
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(bytes_after_third_bit[i], g.bytes[i]);
  }
  EXPECT_EQ(225, g.n_bits);

  g.add_bit(1322);
  const uint8_t bytes_final[29] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0x7F, 0xFF, 0xFB, 0x1F, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x60};
  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(bytes_final[i], g.bytes[i]);
  }
  EXPECT_EQ(230, g.n_bits);
}

TEST_P(GolombCodingAddBitTest, AddsBitCodedCorrectly)
{

  tuple<uint64_t, uint64_t, uint64_t, uint64_t, const uint8_t *> params = GetParam();
  uint64_t m = get<0>(params);
  uint64_t size = get<1>(params);
  uint64_t bit_to_add = get<2>(params);
  uint64_t expected_bits = get<3>(params);
  const uint8_t *expected_bytes = get<4>(params);
  g.init(m, size);
  g.add_bit(bit_to_add);

  EXPECT_EQ(expected_bits, g.n_bits);

  for (uint8_t i = 0; i < g.n_bits / 8 + (g.n_bits % 8 > 0); i++)
  {
    EXPECT_EQ(expected_bytes[i], g.bytes[i]);
  }
}

tuple<uint64_t, uint64_t, uint64_t, uint64_t, const uint8_t *> const AddBitParams[] = {
    // 693147 is when fales positive is 1 in 1M

    // tests that you cant add a bit outside the range
    make_tuple(693147, 1000000, 0, 21, (const uint8_t[3]){0xA5, 0x75, 0x28}),
    make_tuple(693147, 1000000, 1000001, 21, (const uint8_t[3]){0xA5, 0x75, 0x28}),

    // test for adding to bit 1
    make_tuple(693147, 1000000, 1, 41, (const uint8_t[6]){0x00, 0x00, 0x0A, 0x57, 0x52, 0x00}),

    // test for adding to last bit
    make_tuple(693147, 1000000, 1000000, 41, (const uint8_t[6]){0xA5, 0x75, 0x20, 0x00, 0x00, 0x00}),

};

INSTANTIATE_TEST_CASE_P(AddsBitCodedCorrectly, GolombCodingAddBitTest, ::testing::ValuesIn(AddBitParams));

} // namespace GolombCodingAddBitTest